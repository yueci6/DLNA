#include "dlna.h"

#define MAX_EPOLL_EVENT 30

typedef struct List_Node {
    struct list_head list;
    double time_interval;
    int cli_fd;
    int keep_live;
} List_Node;


char play_video_url[1024];
char play_video_url_next[1024];
int first_play = 0;
pthread_t player;
FILE *p = NULL;

void replaceAmp(char *str) {
    char *amp = "amp;";
    while (strstr(str, amp)) {
        char *pos = strstr(str, amp);
        memmove(pos, pos + strlen(amp), strlen(pos + strlen(amp)) + 1);
    }
}

// int http_reply_packet_process(char *recv_packet, char *reply_packet) {
//     if (recv_packet == NULL || reply_packet == NULL)
//         return -1;
//     char *p = NULL;
//     // Debug("%s\n",recv_packet);
//     memset(reply_packet, 0, MaxHttpReplyLen);
//     if (strstr(recv_packet, "GET / HTTP/1.1") != NULL || strstr(recv_packet, "GET http:") != NULL) {
//         // Debug("接收到http的GET请求");
//         sprintf(reply_packet, http_get_request_reply_head);
//     } else if (strstr(recv_packet, "GET /AVTransport") != NULL ) {
//         // Debug("接收到http get avtransport 请求");
//         sprintf(reply_packet, HTTP_AVTransport_REQUEST);
//     } else if (strstr(recv_packet, "GET /RenderingControl") != NULL) {
//         // Debug("接收到 RenderingControl请求");
//         sprintf(reply_packet, HTTP_RenderingControl_REQUEST);
//     } else if (strstr(recv_packet, "<CurrentURI>") != NULL) {
//         Debug("get paly video url");
//         return 1;
//     }else if((p = strstr(recv_packet, "SOAPACTION"))!= NULL){
//         Debug("接收到SOAP消息");
//         if(strstr(p, "SetAVTransportURI") != NULL){
//             Debug("recv message avtransporturl\n");
//             sprintf(reply_packet, HTTP_POST_SetAVTransportURI_REQUEST);
//         }else if(strstr(p, "GetTransportInfo")!= NULL){
//             Debug("recv message gettransportinfo\n");
//             sprintf(reply_packet, HTTP_POST_GetTransportInfoResponse);
//         }else if(strstr(p, "GetVolumeDBRange")!= NULL){
//             Debug("recv message getvolumedb\n");
//             sprintf(reply_packet ,HTTP_POST_GetVolumeDBRange);
//         }else if(strstr(p, "GetVolume")!= NULL){
//             Debug("recv message getvolume\n");
//             sprintf(reply_packet, HTTP_POST_GetVolume);
//         }
//     }else {
//         Debug("接收到未知消息:\n%s",recv_packet);
//     }
//     return 0;
// }

int http_reply_packet_process(char *recv_packet, int fd) 
{
    int ret;
    // Debug("%s\n",recv_packet);
    if (strstr(recv_packet, "GET / HTTP/1.1") != NULL || strstr(recv_packet, "GET http:") != NULL) {
        ret = send(fd, http_get_request_reply_head, strlen(http_get_request_reply_head), 0);
    } else if (strstr(recv_packet, "GET /AVTransport") != NULL ) {
        ret = send(fd, HTTP_GetAVTransport, strlen(HTTP_GetAVTransport), 0);
    } else if (strstr(recv_packet, "GET /RenderingControl") != NULL) {
        ret = send(fd, HTTP_GetRenderingControl, strlen(HTTP_GetRenderingControl), 0);
    } else if (strstr(recv_packet, "<CurrentURI>") != NULL) {
        Debug("get paly video url");
        return 1;
    }else if(strstr(recv_packet, "SOAPACTION")!= NULL){
        if (strstr(recv_packet, "SOAPACTION: \"urn:schemas-upnp-org:service:RenderingControl:1#GetVolumeDBRange\"") != NULL) {
            Debug("recv message getvolumedb\n");
            ret = send(fd, POST_GetVolumeDBRange, strlen(POST_GetVolumeDBRange), 0);
        } else if (strstr(recv_packet, "SOAPACTION: \"urn:schemas-upnp-org:service:AVTransport:1#SetAVTransportURI\"") != NULL) {
            Debug("recv message avtransporturi\n");
            ret = send(fd, POST_SetAVTransportURI, strlen(POST_SetAVTransportURI), 0);
        } else if (strstr(recv_packet, "GetTransportInfo") != NULL) {
            Debug("recv message gettransportinfo\n");
            ret = send(fd, POST_GetTransportInfoResponse, strlen(POST_GetTransportInfoResponse), 0);
        } else if (strstr(recv_packet, "SOAPACTION: \"urn:schemas-upnp-org:service:RenderingControl:1#GetVolume\"") != NULL) {
            Debug("recv message getvolume\n");
            ret = send(fd, POST_GetVolume, strlen(POST_GetVolume), 0);
        }else if(strstr(recv_packet, "SOAPACTION: \"urn:schemas-upnp-org:service:AVTransport:1#Stop\"") != NULL){
            Debug("recv message stop play\n");
            ret = send(fd, POST_AVTransportStop, strlen(POST_AVTransportStop), 0);
        }
    }else if(strstr(recv_packet, "POST")!= NULL){
        // Debug("接收到POST消息:\n%s",recv_packet);
    }else{
       Debug("接收到未知消息:\n%s",recv_packet);
    }
    return ret;
}

int http_reply(int cli_fd) {
    int ret;
    int close_flag;
    char recv_buff[MaxBuffLen];
    char *reply_packet = (char *) malloc(MaxHttpReplyLen);
    if (reply_packet == NULL) {
        Debug("tcp reply string memory alloc fail");
        return -1;
    }
    memset(recv_buff, 0, MaxBuffLen);

    if ((ret = recv(cli_fd, recv_buff, MaxBuffLen, 0)) > 0) {
        if(strstr(recv_buff, "Connection: close") != NULL)
            close_flag = 1;
        else if(strstr(recv_buff, "Connection: keep-alive") != NULL)
            close_flag = -1;
        if ((ret = http_reply_packet_process(recv_buff, cli_fd)) < 0) {
            // if (send(cli_fd, reply_packet, strlen(reply_packet), 0) < 0)
                Debug("tcp send http requset reply head fail");
            // Debug("%s\n",reply_packet);
        }else if (ret > 0) {
            char *str = NULL;
            if ((str = strstr(recv_buff, "<CurrentURI>")) != NULL) {
                sscanf(str, "<CurrentURI>%[^<]</CurrentURI>", play_video_url);
                replaceAmp(play_video_url);
                if (first_play == 0) {
                    first_play = 1;
                    strcpy(play_video_url_next, play_video_url);
                    Debug("get video url is :\n%s\n", play_video_url_next);
                    pthread_create(&player, NULL, play_url, play_video_url_next);
                } else {
                    if (strcmp(play_video_url, play_video_url_next) != 0) {
                        Debug("play next video");
                        strcpy(play_video_url_next, play_video_url);
                    } else {
                        Debug("get video url is playing continue this request");
                    }
                }
            }
        }
    }
    free(reply_packet);
    reply_packet = NULL;
    if(close_flag == 1)
        return -5;
    else if(close_flag == -1)
        return 5;
    else 
        return 0;
}


#if 0

void *http_reply_thread(void *arg) {
    if (arg == NULL)
        return NULL;
    fd_set read_fd;
    fd_set tmp_fd;
    int ret;
    char recv_buff[MaxBuffLen];
    int cli_fd = *((int *) arg);
    FD_ZERO(&read_fd);
    FD_ZERO(&tmp_fd);
    FD_SET(cli_fd, &read_fd);

    struct timeval timeout;
    timeout.tv_sec = 60;
    timeout.tv_usec = 0;
    char *reply_packet = (char *) malloc(MaxHttpReplyLen);
    if (reply_packet == NULL) {
        Debug("tcp reply string memory alloc fail");
        return NULL;
    }

    while (1) {
        tmp_fd = read_fd;
        memset(recv_buff, 0, MaxBuffLen);
        if (select(cli_fd + 1, &tmp_fd, NULL, NULL, &timeout) > 0) {
            if (FD_ISSET(cli_fd, &tmp_fd)) {
                if ((ret = recv(cli_fd, recv_buff, MaxBuffLen, 0)) > 0) {
                    fwrite(recv_buff, 1, ret, p);
                    if ((ret = http_reply_packet_process(recv_buff, reply_packet))== 0) {
                        if (send(cli_fd, reply_packet, strlen(reply_packet) + 1, 0) < 0) {
                            Debug("tcp send http requset reply head fail");
                            break;
                        }
                    } else if (ret > 0) {
                        char *str = NULL;
                        if ((str = strstr(recv_buff, "<CurrentURI>")) != NULL) {
                            sscanf(str, "<CurrentURI>%[^<]</CurrentURI>", play_video_url);
                            replaceAmp(play_video_url);
                            if(first_play == 0){
                                first_play = 1;
                                strcpy(play_video_url_next,play_video_url);
                                Debug("get video url is :\n%s\n",play_video_url_next);
                                pthread_create(&player, NULL, play_url, play_video_url_next);
                            }else{
                                if(strcmp(play_video_url,play_video_url_next) != 0){
                                    Debug("play next video");
                                    strcpy(play_video_url_next,play_video_url);
//                                    play_url(play_video_url);
                                }else{
                                    Debug("get video url is playing continue this request");
                                    goto exit;
                                }
                            }
                        }else{
                            goto exit;
                        }
                    }
                } else if (ret < 0) {
                    Debug("recv fail");
                    break;
                }
            }
        }else if(ret <= 0){
            Debug("wait message timeout exit pthread\n");
            break;
        }
    }

    exit:
    printf("--------> %d <--------------exit !!!!!!!!",getpid());
    close(cli_fd);
    free(reply_packet);
    return NULL;
}

#endif

void *tcp_recv_thread(void *arg) {
    struct sockaddr_in tcp_recv;
    int timer_fd;//定时器
    struct itimerspec timeout;
    struct list_head list_head, *tmp, *pos;
    int tcp_sock;
    int opt = 1;
    int epoll_fd;
    int epoll_event_count;
    struct epoll_event event, timer_event, event_val[MAX_EPOLL_EVENT];
    int new_sockt;
    struct sockaddr_in new_addr;
    socklen_t addr_len = sizeof(new_addr);
    char client_ip[INET_ADDRSTRLEN];
    tcp_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int tcp_sock_fd[30] = {0};
    memset(tcp_sock_fd, -1, 30);
    int i = 0;
    if (tcp_sock < 0) {
        Debug("tcp create socket fail");
        return NULL;
    }
    if (setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        Debug("set set tcp socket ip addr resueadde fail");
        goto err_setsockopt;
    }
    if (setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        Debug("set udp ip reuseaddr fail")
        goto err_setsockopt;
    }
    memset(&tcp_recv, 0, sizeof(tcp_recv));
    memset(&new_addr, 0, sizeof(new_addr));
    tcp_recv.sin_addr.s_addr = INADDR_ANY;
    tcp_recv.sin_family = AF_INET;
    tcp_recv.sin_port = htons(UDP_PROT);
    if (bind(tcp_sock, (struct sockaddr *) &tcp_recv, sizeof(tcp_recv)) < 0) {
        Debug("tcp bind ip fail is %s", strerror(errno));
        goto err_setsockopt;
    }
    if (listen(tcp_sock, 30) < 0) {
        Debug("tcp set listen is %s", strerror(errno));
        goto err_setsockopt;
    }
    Debug("set listen ok");
    //定时初始化部分
    timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    timeout.it_value.tv_sec = 0;
    timeout.it_value.tv_nsec = 0;
    timeout.it_interval .tv_sec = 2;
    timeout.it_interval.tv_nsec = 0;

    //epoll初始化部分
    if((epoll_fd = epoll_create1(0))== -1){
        Debug("epoll create fail\n");
        goto err_setsockopt;
    }
    event.data.fd = tcp_sock;
    event.events = EPOLLIN;

    timer_event.data.fd = timer_fd;
    timer_event.events = EPOLLIN;

    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tcp_sock, &event) == -1){
        Debug("epoll create fail %s\n", strerror(errno));
        goto err_setsockopt;
    }

    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timer_fd, &timer_event) == -1){
        Debug("epoll create fail %s\n", strerror(errno));
        goto err_setsockopt;
    }

    Debug("start epoll\n");

    //内核链表初始化
    INIT_LIST_HEAD(&list_head);

    if (timerfd_settime(timer_fd, 0, &timeout, NULL) == -1) {
        perror("timerfd_settime");
        goto err_setsockopt;
    }

    while (1) {
        epoll_event_count = epoll_wait(epoll_fd, event_val, MAX_EPOLL_EVENT,-1);
        if(epoll_event_count == -1){
            Debug("epoll wait fail %s\n", strerror(errno));
            goto err_setsockopt;
        }

        for(int k = 0; k < epoll_event_count; k++){
            if(event_val[k].data.fd == tcp_sock){
                //TCP连接事件
                if ((new_sockt = accept(tcp_sock, (struct sockaddr *) &new_addr, &addr_len)) < 0) {
                    Debug("tcp accept fail is %s", strerror(errno));
                    goto err_setsockopt;
                }
                //转换客户端IP
                // if (inet_ntop(AF_INET, &(new_addr.sin_addr), client_ip, INET_ADDRSTRLEN) == NULL) {
                //     Debug("get des udp ip fail");
                // }
                //加入epoll轮询
                event.events = EPOLLIN;
                event.data.fd = new_sockt;
                if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_sockt, &event) == -1){
                    Debug("epoll ctl fail %s\n", strerror(errno));
                    goto err_setsockopt;
                }
//                Debug("new tcp connect ip is --------------> %s %d", client_ip, new_sockt);
                //添加new sock到链表
                List_Node *new_node = (List_Node *) malloc(sizeof(List_Node));
                if(new_node == NULL){
                    Debug("memory is samll malloc fail\n");
                    break;
                }
                new_node->keep_live = 0;
                new_node->cli_fd = new_sockt;
                new_node->time_interval = av_gettime_relative() / 1000000.0;
                list_add_tail(&new_node->list,&list_head);
                 Debug("add new tcp sock success\n");
            }else if(event_val[k].data.fd == timer_fd){
                //触发定时器
                Debug("timer start");
                uint64_t expirations;
                if(!list_empty(&list_head)){
                    double now_time = av_gettime_relative() / 1000000.0;
                    list_for_each_safe(pos, tmp, &list_head){
                        List_Node *node = list_entry(pos, List_Node, list);
                        if(node->keep_live != 1 && (now_time - node->time_interval > 3.0)){
                            if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, node->cli_fd, NULL) == -1){
                                Debug("epoll ctl fail\n");
                                break;
                            }
                            close(node->cli_fd);
                            list_del(&node->list);
                            free(node);
                            node = NULL;
                            Debug("timeout free cli_fd\n");
                        }
                    }
                }

                if(read(timer_fd, &expirations, sizeof(expirations)) < 0){
                    Debug("timer expirations fail\n");
                    break;
                }
            }else{
                //更新最后通信的时间
                int ret = http_reply(event_val[k].data.fd);
                list_for_each_safe(pos, tmp, &list_head){
                    List_Node *node = list_entry(pos,List_Node,list);
                    if(node->cli_fd == event_val[k].data.fd){
                        if(ret == -5){
                            node->keep_live = -1;
                        }else if(ret == 5){
                            node->keep_live = 1;
                        }else{
                            node->time_interval = av_gettime_relative() / 1000000.0; //更新最后的连接时间
                        }
                        break;
                    }
                }
            }
        }

    }

    list_for_each_safe(pos, tmp, &list_head){
        List_Node *node = list_entry(pos,List_Node,list);
        close(node->cli_fd);
        list_del(&node->list);
        free(node);
    }

    close(tcp_sock);
    return NULL;
    err_setsockopt:
        close(tcp_sock);
    return NULL;
}

int main(void) {
    int ret;
    int opt = 1;
    socklen_t len;
    /*DLNA 组播端口 和 IP*/
    struct ip_mreq Mulitcast_addr;
    struct sockaddr_in udp_addr;
    struct sockaddr_in recv_addr;
    int sock_fd;
    char buff[MaxBuffLen] = {0};
    fd_set read_fd;
    fd_set tmp_fd;
    pthread_t tcp_pid;
    FD_ZERO(&read_fd);
    FD_ZERO(&tmp_fd);

    memset(&Mulitcast_addr, 0, sizeof(Mulitcast_addr));
    memset(&udp_addr, 0, sizeof(udp_addr));
    // 设置的组播IP和需要加入组播的IP
    inet_pton(AF_INET, Mulitcast_IP, &Mulitcast_addr.imr_multiaddr);
    inet_pton(AF_INET, UDP_IP, &Mulitcast_addr.imr_interface);

    udp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_port = htons(Mulitcast_PORT);

    sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_fd < 0) {
        Debug("create socket fail");
        return -1;
    }
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        Debug("set udp ip reuseaddr fail");
        goto err_setsockopt;
    }

    if (setsockopt(sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &Mulitcast_addr, sizeof(Mulitcast_addr)) < 0) {
        Debug("set udp add Mulitcast fail is %s", strerror(errno));
        goto err_setsockopt;
    }

    if (bind(sock_fd, (struct sockaddr *) &udp_addr, sizeof(udp_addr)) < 0) {
        Debug("udp bind ipaddr fail is %s", strerror(errno));
        goto err_setsockopt;
    }

    FD_SET(sock_fd, &read_fd);
    len = sizeof(recv_addr);
    struct sockaddr_in Mulicast_udp;
    memset(&Mulicast_udp, 0, sizeof(Mulicast_udp));
    /*创建后续http交互线程*/
    pthread_create(&tcp_pid, NULL, tcp_recv_thread, NULL);
    Debug("create tcp success");

    while (1) {
        tmp_fd = read_fd;
        memset(buff, 0, MaxBuffLen);
        if (select(sock_fd + 1, &tmp_fd, NULL, NULL, NULL) > 0) {
            if (FD_ISSET(sock_fd, &tmp_fd)) {
                if (recvfrom(sock_fd, buff, MaxBuffLen, 0, (struct sockaddr *) &recv_addr, &len) > 0) {
                    /*监测Source发送的M-SEARCH包后，作出回复*/
                    if (strstr(buff, "M-SEARCH") != NULL) {
//                        char ip[INET_ADDRSTRLEN];
//                        if (inet_ntop(AF_INET, &(recv_addr.sin_addr), ip, INET_ADDRSTRLEN) == NULL) {
//                            Debug("get des udp ip fail");
//                        }
                        memset(buff, 0, MaxBuffLen);
                        sprintf(buff, udp_reply_packet, UDP_IP, UDP_PROT);
                        ret = sendto(sock_fd, buff, strlen(buff) + 1, 0, (struct sockaddr *) &recv_addr, len);
                        if (ret < 0) {
                            Debug("send fail!!!");
                        }
                    }
                    memset(buff, 0, MaxBuffLen);
                }else{
                    Debug("recv udp message fail");
                    break;
                }
            }
        }
    }
    close(sock_fd);
    Debug("player is playing\n");
    pthread_join(tcp_pid, NULL);
    return 0;

    err_recv:
        pthread_join(tcp_pid,NULL);
    err_setsockopt:
        close(sock_fd);
        return -1;
}