//
// Created by yueci on 23-9-19.
//
#include "queue.h"


int packet_queue_init(PacketQueue *q)
{
    q->first = q->last = NULL;
    q->size = q->nb_packet = 0;
    q->mutex = NULL;
    q->cond = NULL;
    q->abourt_requst = 0;
    q->serial = -1;

    q->mutex = SDL_CreateMutex();
    q->cond = SDL_CreateCond();
    if(q->mutex == NULL || q->cond == NULL)
        return -1;
    return 0;
}

int packet_queue_get(PacketQueue *q, AVPacket *pkt, int *serial)
{
    int ret;
    MyAVPacketList *pkt1;
    SDL_LockMutex(q->mutex);
    for(;;){
        if(q->abourt_requst){
            ret = -1;
            break;
        }
        if(q->nb_packet <= 0){
            SDL_CondWait(q->cond,q->mutex);
        }else if(q->nb_packet > 0){
            pkt1 = q->first;
            av_packet_move_ref(pkt,&pkt1->pkt);
            if(q->first == NULL)
                q->last = NULL;
            else
                q->first = q->first->next;
            if(serial)
                *serial = pkt1->serial;
            q->nb_packet--;
            q->size -= (sizeof(pkt) + pkt->size);
            av_free(pkt1);
            ret = 0;
            break;
        }else{
            ret = -1;
            break;
        }
    }
    SDL_UnlockMutex(q->mutex);
    return ret;
}

int packet_queue_put(PacketQueue *q, AVPacket *pkt)
{
    MyAVPacketList *new_pkt = NULL;
    new_pkt = av_malloc(sizeof(MyAVPacketList));
    if(new_pkt == NULL)
        return -1;
    av_packet_move_ref(&new_pkt->pkt,pkt);
    new_pkt->next = NULL;
    new_pkt->serial = q->serial;

    SDL_LockMutex(q->mutex);
    if(q->first == NULL){
        q->first = new_pkt;
    }else{
        q->last->next = new_pkt;
    }
    q->last = new_pkt;
    q->nb_packet++;
    q->size += (pkt->size + sizeof(pkt));

    SDL_UnlockMutex(q->mutex);
    SDL_CondSignal(q->cond);
    return 0;
}

void packet_queue_flush(PacketQueue *q)
{
    MyAVPacketList *pkt = NULL;
    MyAVPacketList *pkt_head = NULL;
    pkt_head = q->first;
    SDL_LockMutex(q->mutex);
    while (pkt_head!= NULL){
        pkt = pkt_head->next;
        av_packet_unref(&pkt_head->pkt);
        av_free(pkt_head);
        pkt_head = pkt;
    }
    q->size = 0;
    q->nb_packet = 0;
    q->first = NULL;
    q->last = NULL;
    SDL_UnlockMutex(q->mutex);
}

void packet_queue_destory(PacketQueue *q)
{
    packet_queue_flush(q);
    SDL_DestroyMutex(q->mutex);
    SDL_DestroyCond(q->cond);
}

void frame_queue_destory(FrameQueue *q)
{
    for(int i = 0;i < q->max_size;i++){
        av_frame_free(&q->frame_queue[i].frame);
    }

    SDL_DestroyMutex(q->mutex);
    SDL_DestroyCond(q->cond);
}


int frame_queue_init(FrameQueue *q, PacketQueue *p, int max_size, int keep_last)
{
    memset(q, 0, sizeof(FrameQueue));
    q->mutex = SDL_CreateMutex();
    if(!q->mutex){
        printf("%s\n",SDL_GetError());
        return -1;
    }
    q->cond = SDL_CreateCond();
    if(!q->cond){
        printf("%s\n",SDL_GetError());
        return -1;
    }

    q->max_size = FFMIN(max_size, MAX_FRAME_SIZE);
    for(int i = 0;i < q->max_size; i++){
        if(!(q->frame_queue[i].frame = av_frame_alloc()))
            return -1;
    }

    q->size = 0;
    q->rindex = q->windex = 0;
    q->pause = 0;
    q->packet_queue = p;
    q->keep_last = !!keep_last;
    return 0;
}

void frame_queue_push(FrameQueue *q)
{
    if(++q->windex == q->max_size)
        q->windex = 0;
    SDL_LockMutex(q->mutex);
    q->size++;
    SDL_UnlockMutex(q->mutex);
}

void frame_queue_next(FrameQueue *q)
{
    if(q->keep_last && !q->rindex_show){
        q->rindex_show = 1;
        return;
    }

    av_frame_unref(q->frame_queue[q->rindex].frame);
    if(++q->rindex == q->max_size)
        q->rindex = 0;
    SDL_LockMutex(q->mutex);
    q->size--;
    SDL_CondSignal(q->cond);
    SDL_UnlockMutex(q->mutex);
}

Frame_t *frame_queue_peek_writable(FrameQueue *q)
{
    SDL_LockMutex(q->mutex);
    while(q->size >= q->max_size && !q->packet_queue->abourt_requst){
        SDL_CondWait(q->cond, q->mutex);
    }
    SDL_UnlockMutex(q->mutex);
    if(q->packet_queue->abourt_requst)
        return NULL;
    return &q->frame_queue[q->windex];
}

Frame_t *frame_queue_peek_readable(FrameQueue *q)
{
    SDL_LockMutex(q->mutex);
    while (q->size - q->rindex_show <= 0 && !q->packet_queue->abourt_requst){
        SDL_CondWait(q->cond,q->mutex);
    }
    SDL_UnlockMutex(q->mutex);
    if(q->packet_queue->abourt_requst)
        return NULL;

    return &q->frame_queue[(q->rindex + q->rindex_show) % q->max_size];
}

Frame_t *frame_queue_peek_last(FrameQueue *q)
{
    return &q->frame_queue[q->rindex];
}

Frame_t *frame_queue_peek(FrameQueue *q)
{
    return &q->frame_queue[(q->rindex + 1) % q->max_size];
}

Frame_t *frame_queue_peek_next(FrameQueue *q)
{
    return &q->frame_queue[(q->rindex + 2) % q->max_size];
}

void set_clock_at(Clock_t *c, double pts, int serial, double time)
{
    c->pts = pts;
    c->serial = serial;
    c->last_update = time;
    c->pts_drits = c->pts - time;
}

void set_clock(Clock_t *c, double pts, int serial)
{
    double time = (double) av_gettime_relative() / 1000000.0;
    set_clock_at(c, pts, serial, time);
}

void clock_init(Clock_t *c, int *queue_serial)
{
    c->paused = 0;
    c->speed = 1.0;
    c->queue_serial = queue_serial;
    set_clock(c, NAN, -1);
}

double get_clock(Clock_t *c)
{
    if(c->paused)
        return c->pts;
    if(*c->queue_serial != c->serial)
        return NAN;

    double time = (double) av_gettime_relative() / 1000000.0;
    return time + c->pts_drits + (time - c->last_update) * c->speed;
}

double get_master_clock(PlayStream *is)
{
    double time;
    switch (is->media_info.clock_sync_type) {
        case SYNC_TYPE_MASTER_AUDIO:
            time = get_clock(&is->media_info.audio_clock);
            break;
        case SYNC_TYPE_MASTER_VIDEO:
            time = get_clock(&is->media_info.video_clock);
            break;
        case SYNC_TYPE_MASTER_EXTERN:
            time = get_clock(&is->media_info.ext_clock);
            break;
        default:
            break;
    }
    return time;
}

void sync_clock_to_slave(Clock_t *c, Clock_t *slave)
{
    double clock = get_clock(c);
    double slave_clock = get_clock(slave);
    if(fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD || !(isnan(slave_clock)) && isnan(clock))
        set_clock(c, slave_clock, slave->serial);
}



