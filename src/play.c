#include <stdio.h>
#include <unistd.h>
#include "SDL.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "queue.h"


unsigned int audio_device = 0;
int decoder_end = 0;
static int64_t audio_callback_time;
uint8_t *out_buffer;

AVCodecContext *decoder_init(AVFormatContext *is,int stream_index)
{
    int ret;
    AVCodec *codec = NULL;
    AVCodecContext *decoder = avcodec_alloc_context3(NULL);
    if(decoder == NULL){
        return NULL;
    }

    ret = avcodec_parameters_to_context(decoder,is->streams[stream_index]->codecpar);
    if(ret < 0){
        printf("%s\n", av_err2str(ret));
        return NULL;
    }

    codec = (AVCodec *) avcodec_find_decoder(decoder->codec_id);
    if(codec == NULL)
        goto err_find_decoder;

    ret = avcodec_open2(decoder,codec,NULL);
    if(ret < 0){
        printf("%s\n", av_err2str(ret));
        goto err_find_decoder;
    }
    return decoder;

err_find_decoder:
    avcodec_free_context(&decoder);
    return NULL;
}


int SDL_Window_init(SDLInfo *is)
{
    is->win = SDL_CreateWindow("video",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,is->win_width,is->win_height,SDL_WINDOW_SHOWN);
    if(!is->win){
        printf("%s\n",SDL_GetError());
        return -1;
    }

    is->renderer = SDL_CreateRenderer(is->win,-1,SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(!is->renderer){
        printf("%s\n",SDL_GetError());
        goto err_create_renderer;
    }

    is->texture = SDL_CreateTexture(is->renderer,SDL_PIXELFORMAT_IYUV,SDL_TEXTUREACCESS_STREAMING,is->win_width,is->win_height);
    if(!is->texture){
        printf("%s\n",SDL_GetError());
        goto err_create_texture;
    }

    return 0;
err_create_texture:
    SDL_DestroyRenderer(is->renderer);
err_create_renderer:
    SDL_DestroyWindow(is->win);
    SDL_Quit();
    return -1;
}

int decoder_frame(PlayStream *is, AVFrame *frame, AVCodecContext *decoder, PacketQueue *packet_queue)
{
    int ret = AVERROR(EAGAIN);
    int pending = 0;
    AVPacket *pkt = NULL;
    pkt = av_packet_alloc();
    if(pkt == NULL){
        printf("avpavket alloc is fail\n");
        return -1;
    }

    for(;;){
        do{
            ret = avcodec_receive_frame(decoder,frame);
            if(ret == AVERROR_EOF){
                printf("decoder over\n");
                decoder_end = 1;
                avcodec_flush_buffers(decoder);
                ret = 1;
                goto get_success;
            }else if(ret >= 0){
                ret = 0;
                goto get_success;
            }
        }while(ret != AVERROR(EAGAIN));

        if(packet_queue->nb_packet == 0)
            SDL_CondSignal(is->media_info.read_continue);
        if(pending){
            pending = 0;
        }else{
            if(packet_queue_get(packet_queue, pkt, &packet_queue->serial))
                goto err_get_packet;
        }

        ret = avcodec_send_packet(decoder,pkt);
        if(ret == AVERROR(EAGAIN))
            pending = 1;
        else if(ret == 0)
            av_packet_unref(pkt);
    }

get_success:
    av_packet_free(&pkt);
    return ret;
err_get_packet:
    av_packet_free(&pkt);
    return -1;
}

int audio_decode_frame(PlayStream *is)
{
    int data_size,resample_size;
    int wanted_nb_samples;
    av_unused double audio_clock0;
    Frame_t *vp = NULL;

    do{
        if((vp = frame_queue_peek_readable(&is->media_info.audio_frame_queue)) == NULL){
            printf("get frame fail\n");
            return -1;
        }
        frame_queue_next(&is->media_info.audio_frame_queue);
    }while(vp->serial != is->media_info.audio_packet_queue.serial);

    wanted_nb_samples = vp->frame->nb_samples;
    data_size = av_samples_get_buffer_size(NULL, vp->frame->ch_layout.nb_channels, vp->frame->nb_samples,
                                           vp->frame->format,1);


    if((is->audio_params.format != vp->frame->format || \
    is->audio_params.freq != vp->frame->sample_rate) || \
    av_channel_layout_compare(&vp->frame->ch_layout, &is->media_info.audio_decoder->ch_layout)){
        swr_free(&is->media_info.swr);
        swr_alloc_set_opts2(&is->media_info.swr, &is->audio_params.ch_layout, is->audio_params.format,
                            is->audio_params.freq, &vp->frame->ch_layout, vp->frame->format,
                            vp->frame->sample_rate,0, NULL);
        if(!is->media_info.swr || swr_init(is->media_info.swr) < 0){
            printf("audio swr init fail\n");
            swr_free(&is->media_info.swr);
            return -1;
        }

        if(av_channel_layout_copy(&is->audio_src.ch_layout,&vp->frame->ch_layout) < 0){
            printf("copy audio ch_layout fail\n");
            return -1;
        }
        /** 这里只是记录一下 set重采样后的参数，如果后续的frame参数一致则不需要重复
         * 初始化，直接跳到重采样，直到frame的参数发生改变后，才需要重新set 重采样参数。
         * */
        is->audio_src.freq = vp->frame->sample_rate;
        is->audio_src.format = vp->frame->format;
    }

    if(is->media_info.swr){
        /**需要重采样*/
        const uint8_t **in = (const uint8_t **) (const uint8_t **) vp->frame->extended_data;
        uint8_t **out = &is->media_info.sample_audio_buf;
        int out_count = vp->frame->nb_samples * is->audio_params.freq / (vp->frame->sample_rate + 256);
        int out_size = av_samples_get_buffer_size(NULL,is->audio_params.ch_layout.nb_channels,
                                                  out_count,is->audio_params.format,0);
        int len2;
        if(out_size < 0){
            printf("%s", av_err2str(out_size));
            return -1;
        }
        av_fast_malloc(&is->media_info.sample_audio_buf,&is->media_info.audio_buf_size,out_size);
        if(!is->media_info.sample_audio_buf){
            printf("sample audio buf szie alloc fail\n");
            return -1;
        }
        len2 = swr_convert(is->media_info.swr,out,out_count,in,vp->frame->nb_samples);
        if(len2 < 0){
            printf("swr convert audio frame fail is %s\n", av_err2str(len2));
            return -1;
        }
        /**表示还缓冲区开辟的内存可能不够*/
        if(len2 == out_count){
            if(swr_init(is->media_info.swr) < 0)
                swr_free(&is->media_info.swr);
        }
        is->media_info.audio_buf = is->media_info.sample_audio_buf;
        resample_size = is->audio_params.ch_layout.nb_channels * len2 * av_get_bytes_per_sample(is->audio_params.format);
    }else{
        /**不需要重采样*/
        is->media_info.audio_buf = vp->frame->data[0];
        resample_size = data_size;
    }

    audio_clock0 = is->media_info.audio_clock_val;

    if(!isnan(vp->pts))
        is->media_info.audio_clock_val = vp->pts + (double) vp->frame->nb_samples / vp->frame->sample_rate;
    else
        is->media_info.audio_clock_val = NAN;
    return resample_size;
}


int video_thread(void *arg)
{
    PlayStream *is = (PlayStream *)arg;
    int ret;
    AVFrame *frame = NULL;
    Frame_t *new_frame = NULL;
    AVRational tb = is->media_info.url->streams[is->media_info.video_index]->time_base;
    AVRational frame_rate = av_guess_frame_rate(is->media_info.url,is->media_info.url->streams[is->media_info.video_index],NULL);
    frame = av_frame_alloc();
    if(frame == NULL){
        printf("frame alloc fail\n");
        return -1;
    }

    while (1){
        if(is->media_info.video_packet_queue.abourt_requst)
            break;
        ret = decoder_frame(is,frame,is->media_info.video_decoder,&is->media_info.video_packet_queue);
        if(ret > 0){
            printf("decoder deocde video frame over\n");
            goto the_end;
        }else if(ret < 0){
            printf("read video frame fail\n");
            continue;
        }else if(ret == 0){
//            frame->pts = frame->best_effort_timestamp;
//            double dpts = NAN;
//            if(frame->pts != AV_NOPTS_VALUE)
//                dpts = frame->pts * av_q2d(is->media_info.url->streams[is->media_info.video_index]->time_base);
//
//            frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(is->media_info.url, is->media_info.url->streams[is->media_info.video_index], frame);
//            if(is->media_info.clock_sync_type != SYNC_TYPE_MASTER_VIDEO){
//                if(frame->pts != AV_NOPTS_VALUE){
//                    double diff = dpts - get_master_clock(is);
//                    if(!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD && diff < 0 \
//                       && is->media_info.video_packet_queue.nb_packet){
//                        printf("跳过一帧\n");
//                        av_frame_unref(frame);
//                        continue;
//                    }
//                }
//            }
            new_frame = frame_queue_peek_writable(&is->media_info.video_frame_queue);
            new_frame->format = frame->format;
            new_frame->width = frame->width;
            new_frame->height = frame->height;
            new_frame->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : (frame->pts * av_q2d(tb));
            new_frame->serial = is->media_info.video_packet_queue.serial;
            new_frame->duration = (frame_rate.num  && frame_rate.den ? av_q2d(frame_rate) : 0);
            av_frame_move_ref(new_frame->frame,frame);
            frame_queue_push(&is->media_info.video_frame_queue);
            av_frame_unref(frame);
        }
    }
    av_frame_free(&frame);
    return 0;
the_end:
    av_frame_free(&frame);
    return 0;
}

int audio_thread(void *arg)
{
    int ret;
    AVRational tb;
    PlayStream *is = (PlayStream *)arg;
    Frame_t *new_frame = NULL;
    AVFrame *frame = NULL;
    frame = av_frame_alloc();
    if(frame == NULL) {
        printf("audio frame alloc fail\n");
        return -1;
    }

    while (1) {
        ret = decoder_frame(is, frame, is->media_info.audio_decoder, &is->media_info.audio_packet_queue);
        if (ret > 0) {
            printf("audio frame decoder over\n");
            goto the_end;
        } else if (ret == 0) {
            if (!(new_frame = frame_queue_peek_writable(&is->media_info.audio_frame_queue)))
                goto the_end;

            tb = (AVRational) {1, frame->sample_rate};
//            if(frame->pts != AV_NOPTS_VALUE)
//                frame->pts = av_rescale_q(frame->pts, is->media_info.audio_decoder->pkt_timebase, tb);
//            else if(is->media_info.next_pts != AV_NOPTS_VALUE)
//                frame->pts = av_rescale_q(is->media_info.next_pts, is->media_info.pts_tb, tb);
//
//            if(frame->pts != AV_NOPTS_VALUE){
//                is->media_info.next_pts = frame->pts + frame->nb_samples;
//                is->media_info.pts_tb = tb;
//            }

            new_frame->serial = -1;
            new_frame->pos = frame->pkt_pos;
            new_frame->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : av_q2d(tb) * frame->pts;
            new_frame->duration = av_q2d((AVRational) {frame->nb_samples, frame->sample_rate});
            av_frame_move_ref(new_frame->frame, frame);
            frame_queue_push(&is->media_info.audio_frame_queue);
        }else{
            printf("get audio frame fail\n");
            continue;
        }
    }
the_end:
    printf("audio play complete\n");
    av_frame_free(&frame);
    return -1;
}

static void sdl_audio_callback(void *opaque, Uint8 *stream, int len)
{
    int len1, audio_buf_size;
    audio_callback_time = av_gettime_relative();
    PlayStream *is = (PlayStream *)opaque;
    while (len > 0){
        if(is->media_info.audio_buf_index >= is->media_info.audio_buf_size){
            audio_buf_size = audio_decode_frame(is);
            if(audio_buf_size < 0){
                printf("get audio frame fail\n");
                is->media_info.audio_buf = NULL;
                is->media_info.audio_buf_size = SDL_AUDIO_MIN_BUFFER_SIZE / is->audio_params.frame_size * is->audio_params.frame_size;
            }else{
                is->media_info.audio_buf_size = audio_buf_size;
            }
            //重新获取到新的一帧audio frame，把index 置零
            is->media_info.audio_buf_index = 0;
        }
        len1 = is->media_info.audio_buf_size - is->media_info.audio_buf_index;
        if(len1 > len)
            len1 = len;
        memset(stream, 0, len1);
        SDL_MixAudioFormat(stream, (uint8_t *)is->media_info.audio_buf + is->media_info.audio_buf_index, AUDIO_S16SYS, len1, 100);
        len -= len1;
        stream += len1;
        is->media_info.audio_buf_index += len1;
    }

    is->media_info.audio_write_buf_size = is->media_info.audio_buf_size - is->media_info.audio_buf_index;
    if(!isnan(is->media_info.audio_clock_val)){
        set_clock_at(&is->media_info.audio_clock,
                    is->media_info.audio_clock_val - (double)(2 * is->media_info.audio_hw_buf_size + is->media_info.audio_write_buf_size) / is->audio_params.bytes_per_sec,
                     -1, audio_callback_time / 1000000.0);
        sync_clock_to_slave(&is->media_info.ext_clock, &is->media_info.audio_clock);
    }
}

static int audio_open(void *opaque, AVChannelLayout *wanted_channel_layout, int wanted_sample_rate, struct AudioParams *audio_hw_params)
{
    SDL_AudioSpec wanted_spec, spec;
    const char *env;
    static const int next_nb_channels[] = {0, 0, 1, 6, 2, 6, 4, 6};/*音频通道数*/
    static const int next_sample_rates[] = {0, 44100, 48000, 96000, 192000};/*采样率数组*/
    int next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;/*采样率数组下标 从最大值开始*/
    int wanted_nb_channels = wanted_channel_layout->nb_channels;/*通道数*/

    env = SDL_getenv("SDL_AUDIO_CHANNELS");
    if (env) {
        wanted_nb_channels = atoi(env);
        av_channel_layout_uninit(wanted_channel_layout); /*释放 wanted_channel_layout 通道布局中 任何已经分配的数据,并重置通道数*/
        av_channel_layout_default(wanted_channel_layout, wanted_nb_channels);/*获取给定数量通道的默认通道布局*/
    }
    if (wanted_channel_layout->order != AV_CHANNEL_ORDER_NATIVE) { /*本机通道顺序*/
        av_channel_layout_uninit(wanted_channel_layout);
        av_channel_layout_default(wanted_channel_layout, wanted_nb_channels);
    }
    wanted_nb_channels = wanted_channel_layout->nb_channels;
    wanted_spec.channels = wanted_nb_channels;/*给sdl音频结构体 赋值 采样通道数*/
    wanted_spec.freq = wanted_sample_rate; /*给sdl音频结构体 赋值 采样率*/
    if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
        av_log(NULL, AV_LOG_ERROR, "Invalid sample rate or channel count!\n");
        return -1;
    }
    /*通过采用率 找到采样率数组中的 小于等于 指定采样率的最大值 返回索引*/
    while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq)
        next_sample_rate_idx--;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.silence = 0;
    wanted_spec.samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));/*SDl每次回调读取的 数据大小*/
    wanted_spec.callback = sdl_audio_callback;
    wanted_spec.userdata = opaque;

    /*尝试多种组合 让SDL打开音频设备*/
    while (!(audio_device = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE))) {
        av_log(NULL, AV_LOG_WARNING, "SDL_OpenAudio (%d channels, %d Hz): %s\n",
               wanted_spec.channels, wanted_spec.freq, SDL_GetError());
        /*先尝试降低通道数*/
        wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
        /*当通道数为最小时,尝试降低采样率*/
        if (!wanted_spec.channels) {
            wanted_spec.freq = next_sample_rates[next_sample_rate_idx--];
            wanted_spec.channels = wanted_nb_channels;
            if (!wanted_spec.freq) {
                av_log(NULL, AV_LOG_ERROR,
                       "No more combinations to try, audio open failed\n");
                return -1;
            }
        }
        /*设置新的采样率 和通道数*/
        av_channel_layout_default(wanted_channel_layout, wanted_spec.channels);
    }
    /*specs 是sdl实际打开audio设备的参数*/
    if (spec.format != AUDIO_S16SYS) {
        av_log(NULL, AV_LOG_ERROR,
               "SDL advised audio format %d is not supported!\n", spec.format);
        return -1;
    }

    if (spec.channels != wanted_spec.channels) {
        av_channel_layout_uninit(wanted_channel_layout);
        av_channel_layout_default(wanted_channel_layout, spec.channels);
        if (wanted_channel_layout->order != AV_CHANNEL_ORDER_NATIVE) {
            av_log(NULL, AV_LOG_ERROR,
                   "SDL advised channel count %d is not supported!\n", spec.channels);
            return -1;
        }
    }

    audio_hw_params->format = AV_SAMPLE_FMT_S16;
    audio_hw_params->freq = spec.freq;
    if (av_channel_layout_copy(&audio_hw_params->ch_layout, wanted_channel_layout) < 0)
        return -1;
    audio_hw_params->frame_size = av_samples_get_buffer_size(NULL, audio_hw_params->ch_layout.nb_channels, 1, audio_hw_params->format, 1);
    audio_hw_params->bytes_per_sec = av_samples_get_buffer_size(NULL, audio_hw_params->ch_layout.nb_channels, audio_hw_params->freq, audio_hw_params->format, 1);
    if (audio_hw_params->bytes_per_sec <= 0 || audio_hw_params->frame_size <= 0) {
        av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size failed\n");
        return -1;
    }
    return spec.size;
}

int read_file_thread(void * arg)
{
    PlayStream *is = (PlayStream *)arg;
    int ret;
    is->media_info.read_end = 0;
    AVPacket *pkt = NULL;
    is->media_info.read_continue = SDL_CreateCond();
    if(!is->media_info.read_continue){
        printf("%s\n",SDL_GetError());
        return  -1;
    }
    is->media_info.read_wait = NULL;
    is->media_info.read_wait = SDL_CreateMutex();
    if(!is->media_info.read_wait){
        printf("%s\n",SDL_GetError());
        return -1;
    }

    pkt = av_packet_alloc();
    if(!pkt){
        printf(" alloc avpacket fail\n");
        return -1;
    }

    while (1){
        if(is->media_info.read_end)
            break;
        if(is->media_info.video_frame_queue.size + is->media_info.audio_frame_queue.size > MAX_FRAME_SIZE){
            SDL_LockMutex(is->media_info.read_wait);
            SDL_CondWaitTimeout(is->media_info.read_continue,is->media_info.read_wait,10);
            SDL_UnlockMutex(is->media_info.read_wait);
            continue;
        }

        ret = av_read_frame(is->media_info.url,pkt);
        if(ret == AVERROR_EOF){
            is->media_info.read_end = 1;
            packet_queue_put(&is->media_info.video_packet_queue,pkt);
            packet_queue_put(&is->media_info.audio_packet_queue,pkt);
        }else if(ret == 0){
            if(pkt->stream_index == is->media_info.video_index){
                packet_queue_put(&is->media_info.video_packet_queue,pkt);
            }else if(pkt->stream_index == is->media_info.audio_index){
                packet_queue_put(&is->media_info.audio_packet_queue,pkt);
            }
        }else if(ret < 0){
            printf("packet read fial %s\n", av_err2str(ret));
        }
        av_packet_unref(pkt);
    }
    printf("read file over is %d  %d\n",is->media_info.video_packet_queue.nb_packet,
           is->media_info.audio_packet_queue.nb_packet);
    av_packet_free(&pkt);
    return 0;
}

int video_display(PlayStream *is,AVFrame *frame,AVFrame *yuvframe)
{
    int ret;
    sws_scale(is->media_info.sws,(const uint8_t * const*)frame->data,frame->linesize,0,frame->height,yuvframe->data,yuvframe->linesize);
    ret = SDL_UpdateYUVTexture(is->sdl_info.texture,NULL,yuvframe->data[0],yuvframe->linesize[0],
                               yuvframe->data[1],yuvframe->linesize[1],
                               yuvframe->data[2],yuvframe->linesize[2]);

    if(ret!=0)
    {
        printf("yuv data show err\n");
    }
    SDL_SetRenderDrawColor(is->sdl_info.renderer, 0, 0, 0, 255);
    ret = SDL_RenderClear(is->sdl_info.renderer);


    SDL_YUV_CONVERSION_MODE mode = SDL_YUV_CONVERSION_AUTOMATIC;
    if (frame && (frame->format == AV_PIX_FMT_YUV420P || frame->format == AV_PIX_FMT_YUYV422 || frame->format == AV_PIX_FMT_UYVY422)) {
        if (frame->color_range == AVCOL_RANGE_JPEG)
            mode = SDL_YUV_CONVERSION_JPEG;
        else if (frame->colorspace == AVCOL_SPC_BT709)
            mode = SDL_YUV_CONVERSION_BT709;
        else if (frame->colorspace == AVCOL_SPC_BT470BG || frame->colorspace == AVCOL_SPC_SMPTE170M)
            mode = SDL_YUV_CONVERSION_BT601;
    }
    SDL_SetYUVConversionMode(mode);


    if(ret!=0)
    {
        printf("render clear is err\n");
        return -1;
    }

    ret = SDL_RenderCopy(is->sdl_info.renderer,is->sdl_info.texture,NULL,NULL);
    if(ret)
    {
        printf("rendercopy dete err %s\n",SDL_GetError());
        return -1;
    }
    SDL_RenderPresent(is->sdl_info.renderer);
}

double vp_duration(PlayStream *is, Frame_t *vp, Frame_t *last_vp)
{
    double duration;
    if(vp->serial == last_vp->serial){
        duration = vp->pts - last_vp->pts;
        if(isnan(duration) || duration <= 0 || duration > is->media_info.max_frame_duration)
            return last_vp->duration;
        else
            return duration;
    }else{
        return 0.0;
    }
}

double compute_target_delay(PlayStream *is, double delay)
{
    double diff;
    double sync_threshold;

    if(is->media_info.clock_sync_type != SYNC_TYPE_MASTER_VIDEO){
        diff = get_clock(&is->media_info.video_clock) - get_master_clock(is);
        sync_threshold =  FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
        if(!isnan(diff) && fabs(diff) < is->media_info.max_frame_duration){
            if(diff <= -sync_threshold){
                delay = FFMAX(0, delay + diff);
            }else if(diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD){
                delay = delay + diff;
            }else if(diff >= sync_threshold)
                delay = 2 * delay;
        }
    }
    return delay;
}

void play_exit(PlayStream *is)
{
    SDL_PauseAudioDevice(audio_device,1);
    SDL_CloseAudioDevice(audio_device);

    avformat_close_input(&is->media_info.url);
    avcodec_close(is->media_info.audio_decoder);
    avcodec_close(is->media_info.video_decoder);
    avcodec_free_context(&is->media_info.audio_decoder);
    avcodec_free_context(&is->media_info.video_decoder);

    swr_free(&is->media_info.swr);
    sws_freeContext(is->media_info.sws);
    packet_queue_destory(&is->media_info.audio_packet_queue);
    packet_queue_destory(&is->media_info.video_packet_queue);
    frame_queue_destory(&is->media_info.video_frame_queue);
    frame_queue_destory(&is->media_info.audio_frame_queue);

    SDL_DestroyCond(is->media_info.read_continue);
    SDL_DestroyMutex(is->media_info.read_wait);


    SDL_DestroyRenderer(is->sdl_info.renderer);
    SDL_DestroyTexture(is->sdl_info.texture);
    SDL_DestroyWindow(is->sdl_info.win);

}

int av_play_thread(void *arg)
{
    PlayStream *is = (PlayStream *)arg;
    Frame_t *vp, *last_vp, *nextvp;
    double time;
    AVFrame *yuvframe = av_frame_alloc();
    yuvframe->width = is->sdl_info.win_width;
    yuvframe->height = is->sdl_info.win_height;
    yuvframe->format = AV_PIX_FMT_YUV420P;

    out_buffer = (uint8_t *) av_malloc(av_image_get_buffer_size(yuvframe->format,yuvframe->width,
                                                                yuvframe->height,1));
    av_image_fill_arrays(yuvframe->data,yuvframe->linesize,out_buffer,AV_PIX_FMT_YUV420P,
                         is->sdl_info.win_width,is->sdl_info.win_height,1);
    while(1){
        double duration, delay;
        if(decoder_end && is->media_info.video_frame_queue.size - 1 <= 0 && is->media_info.audio_frame_queue.size -1 <= 0){
            printf("show frame over\n");
            break;
        }

        if(is->media_info.video_frame_queue.size - is->media_info.video_frame_queue.rindex_show == 0){
            printf("wait....\n");
            SDL_Delay(100);
            continue;
        }else{
            last_vp = frame_queue_peek_last(&is->media_info.video_frame_queue);
            vp = frame_queue_peek(&is->media_info.video_frame_queue);

            duration = vp_duration(is, vp, last_vp);
            delay = compute_target_delay(is, duration);
            time = av_gettime_relative() / 1000000.0;

            if(time < delay + is->media_info.frame_timer){
                video_display(is, last_vp->frame,yuvframe);
                continue;
            }
            is->media_info.frame_timer += delay;
            if(delay > 0 && time - is->media_info.frame_timer > AV_SYNC_THRESHOLD_MAX)
                is->media_info.frame_timer = time;

            SDL_LockMutex(is->media_info.video_frame_queue.mutex);
            if(!isnan(vp->pts)){
                set_clock(&is->media_info.video_clock, vp->pts, vp->serial);
                sync_clock_to_slave(&is->media_info.ext_clock,&is->media_info.video_clock);
            }
            SDL_UnlockMutex(is->media_info.video_frame_queue.mutex);

            if(is->media_info.video_frame_queue.size - is->media_info.video_frame_queue.rindex_show > 1){
                nextvp = frame_queue_peek_next(&is->media_info.video_frame_queue);
                duration = vp_duration(is, nextvp, vp);
                delay = compute_target_delay(is, duration);

                if(time > is->media_info.frame_timer + delay && is->media_info.clock_sync_type != SYNC_TYPE_MASTER_VIDEO){
                    frame_queue_next(&is->media_info.video_frame_queue);
                    continue;
                }
                frame_queue_next(&is->media_info.video_frame_queue);
                video_display(is,vp->frame,yuvframe);
            }
        }
    }
    play_exit(is);
    printf("play exit\n");
}

void* play_url(void *arg)
{
    char *url = (char *)arg;
    int ret;
    avformat_network_init();
    AVFormatContext *input_file;
    AVCodecContext *audio_decoder = NULL;
    AVCodecContext *video_decoder = NULL;
    AVChannelLayout  ch_layout = {0};

    PlayStream *is = NULL;
    is =  (PlayStream *) malloc(sizeof(PlayStream ));
    if(is == NULL){
        return NULL;
    }

    ret = SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);
    if(ret < 0){
        printf("%s\n",SDL_GetError());
        return NULL;
    }

    input_file = avformat_alloc_context();
    if(input_file == NULL){
        return NULL;
    }

    ret = avformat_open_input(&input_file, url, NULL, NULL);
    if(ret < 0){
        printf("%s\n", av_err2str(ret));
        goto err_open_file;
    }

    ret = avformat_find_stream_info(input_file,NULL);
    if(ret < 0){
        printf("%s\n", av_err2str(ret));
        goto err_open_file;
    }

    is->media_info.audio_index = -1;
    is->media_info.video_index = -1;

    for(int i = 0;i < input_file->nb_streams;i++){
        if(input_file->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            is->media_info.audio_index = i;
        else if(input_file->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            is->media_info.video_index = i;
    }

    int opt = -1;
    clock_init(&is->media_info.video_clock, &opt);
    clock_init(&is->media_info.audio_clock, &opt);
    clock_init(&is->media_info.ext_clock, &opt);

    if(is->media_info.video_index != -1){
        video_decoder = decoder_init(input_file,is->media_info.video_index);
        if(video_decoder == NULL)
            goto err_open_file;

        packet_queue_init(&is->media_info.video_packet_queue);
        frame_queue_init(&is->media_info.video_frame_queue,&is->media_info.video_packet_queue,16,0);
        is->sdl_info.win_height = video_decoder->height;
        is->sdl_info.win_width = video_decoder->width;
        ret = SDL_Window_init(&is->sdl_info);
        is->media_info.sws = NULL;
        if(ret!= 0)
            goto err_audio_decoder_init;
        is->media_info.sws = sws_getCachedContext(is->media_info.sws,video_decoder->width,
                                            video_decoder->height,
                                            video_decoder->pix_fmt,
                                            is->sdl_info.win_width,is->sdl_info.win_height,AV_PIX_FMT_YUV420P,SWS_BICUBIC,
                                            NULL,NULL,NULL);
        if(!is->media_info.sws){
            printf("video sws init fail\n");
            return NULL;
        }
    }

    if(is->media_info.audio_index != -1) {
        audio_decoder = decoder_init(input_file, is->media_info.audio_index);
        packet_queue_init(&is->media_info.audio_packet_queue);
        frame_queue_init(&is->media_info.audio_frame_queue, &is->media_info.audio_packet_queue, 16, 1);
        if (audio_decoder == NULL)
            goto err_audio_decoder_init;

        is->media_info.next_pts = AV_NOPTS_VALUE;
        is->media_info.pts_tb = input_file->streams[is->media_info.audio_index]->time_base;
        //从解码器获取通道布局信息
        ret = av_channel_layout_copy(&ch_layout, &audio_decoder->ch_layout);
        if (ret < 0) {
            printf("%s\n", av_err2str(ret));
            goto err_get_audio_ch_layout;
        }

        int sample_rate = audio_decoder->sample_rate;
        if((ret = audio_open(is, &ch_layout, sample_rate, &is->audio_params)) < 0)
            return NULL;
        is->media_info.audio_hw_buf_size = ret;
        is->audio_src = is->audio_params;
        is->media_info.audio_buf_size = 0;
        is->media_info.audio_buf_index = 0;
    }

    is->media_info.clock_sync_type = SYNC_TYPE_MASTER_AUDIO;
    is->media_info.video_decoder = video_decoder;
    is->media_info.audio_decoder = audio_decoder;
    is->media_info.url = input_file;
    is->media_info.max_frame_duration = (is->media_info.url->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;
    is->media_info.read_file = SDL_CreateThread(read_file_thread,"read_file_thread",(void *)is);
    is->media_info.video_decoder_thread = SDL_CreateThread(video_thread,"video_thread",(void *)is);
    is->media_info.audio_decoder_thread = SDL_CreateThread(audio_thread,"audio_thread",(void *)is);
    SDL_PauseAudioDevice(audio_device,0);
    is->media_info.video_play_thread = SDL_CreateThread(av_play_thread,"av_play_thread",(void *)is);
    printf("Hello, World!\n");

    SDL_WaitThread(is->media_info.read_file, NULL);
    SDL_WaitThread(is->media_info.video_play_thread,NULL);
    return NULL;
err_get_audio_ch_layout:
    avcodec_free_context(&audio_decoder);
err_audio_decoder_init:
    avcodec_free_context(&video_decoder);
err_open_file:
    avformat_free_context(input_file);
    return NULL;
}
