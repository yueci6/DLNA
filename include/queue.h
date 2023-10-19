//
// Created by yueci on 23-9-19.
//

#ifndef SDL_AUDIO_QUEUE_H
#define SDL_AUDIO_QUEUE_H

#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include "SDL.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/avutil.h"
#include "libavutil/error.h"
#include "libavutil/time.h"
#include "libavutil/opt.h"



#define MAX_FRAME_SIZE 16
#define SDL_AUDIO_MIN_BUFFER_SIZE 512

#define AV_SYNC_THRESHOLD_MIN 0.04
#define AV_SYNC_THRESHOLD_MAX 0.1
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30
#define AV_NOSYNC_THRESHOLD 10.0

enum Clock_sync_type{
    SYNC_TYPE_MASTER_AUDIO,
    SYNC_TYPE_MASTER_VIDEO,
    SYNC_TYPE_MASTER_EXTERN,
}typedef Clock_sync_type;


typedef struct MyAVPacketList{
    AVPacket pkt;
    int serial;
    struct MyAVPacketList *next;
} MyAVPacketList;

struct PacketQueue{
    MyAVPacketList *first,*last;
    int serial;
    long size;
    int nb_packet;
    int abourt_requst;
    SDL_mutex *mutex;
    SDL_cond *cond;
}typedef PacketQueue;

struct Frame_t{
    AVFrame *frame;
    double pts;
    double duration;
    int64_t pos;
    int serial;
    int format;
    int width;
    int height;
}typedef Frame_t;

struct FrameQueue{
    Frame_t frame_queue[MAX_FRAME_SIZE];
    int max_size;
    int size;
    int rindex_show;
    int pause;
    int rindex;
    int windex;
    int keep_last;
    SDL_cond *cond;
    SDL_mutex *mutex;
    PacketQueue *packet_queue;
}typedef FrameQueue;

struct Decoder_t{
    AVCodecContext *decoder;
    FrameQueue *frame_queue;
    PacketQueue *packet_queue;
    int serial;
};

struct Clock_t{
    double pts;
    double pts_drits;
    double last_update;
    double speed;
    int serial;
    int paused;
    int *queue_serial;
}typedef Clock_t;

struct SDLInfo{
    int win_width;
    int win_height;

    SDL_Window * win;
    SDL_Renderer *renderer;
    SDL_Texture *texture;

    SDL_AudioSpec want_spec;
    SDL_AudioSpec spec;
}typedef SDLInfo;

struct AudioParams{
    int freq;
    AVChannelLayout ch_layout;
    enum AVSampleFormat format;
    int frame_size;
    int bytes_per_sec;
}typedef AudioParams;

struct AVMediaStatus{
    int video_index;
    int audio_index;
    int read_end;
    int audio_buf_index;
    int audio_write_buf_size;
    double audio_clock_val;
    double max_frame_duration;
    double frame_timer;
    unsigned int audio_buf_size;
    unsigned int audio_hw_buf_size;

    AVRational pts_tb;
    int64_t next_pts;
    uint8_t *audio_buf;
    uint8_t *sample_audio_buf;
    Clock_sync_type clock_sync_type;

    AVFormatContext *url;
    AVCodecContext *video_decoder;
    AVCodecContext *audio_decoder;
    struct SwrContext *swr;
    struct SwsContext *sws;

    FrameQueue video_frame_queue;
    FrameQueue audio_frame_queue;
    PacketQueue video_packet_queue;
    PacketQueue audio_packet_queue;

    Clock_t video_clock;
    Clock_t audio_clock;
    Clock_t ext_clock;

    SDL_Thread *read_file;
    SDL_Thread *video_decoder_thread;
    SDL_Thread *audio_decoder_thread;
    SDL_Thread *video_play_thread;

    SDL_mutex *read_wait;
    SDL_cond *read_continue;
}typedef AVMediaStatus;

struct PlayStream{
    SDLInfo sdl_info;
    AVMediaStatus media_info;
    AudioParams audio_params;
    AudioParams audio_src; //这个参数主要用来记录audio frame每次设置的重采样参数，以判断下一帧frame是否需要重新初始化重采样参数
}typedef PlayStream;

/***
 *初始化Packet 队列
 * @param q Packet 队列
 * @return
 * 成功 0
 * 失败 -1
 */
extern int packet_queue_init(PacketQueue *q);
/***
 *从Packet 队列取出一包packet数据，从头开始拿数据，
 * 先进先出
 * @param q Packet 队列对象
 * @param pkt 需要取出数据的载体
 * @param serial 传入serial，会更新成最新的serial；
 * @return
 * 成功 0
 * 失败 -1
 */
extern int packet_queue_get(PacketQueue *q, AVPacket *pkt, int *serial);
/***
 * 向packet 队列中尾插入一包数据
 * @param q packet队列对象
 * @param pkt 待入队的packet数据
 * @return
 * 成功 0
 * 失败 -1
 */
extern int packet_queue_put(PacketQueue *q, AVPacket *pkt);
/***
 * 清空packet队列中的所有packet，
 * 将其他描述队列信息的参数重置为0。
 * @param q
 */
extern void packet_queue_flush(PacketQueue *q);

/***
* 清空并注销packet队列；
* @param q
*/
extern void packet_queue_destory(PacketQueue *q);

/***
 * 初始化frame队列
 * @param q 待初始化的frame队列
 * @param p 需要绑定对应的packet队列
 * @param max_size frame队列最大可存在的frmae数量
 * @param keep_last 是否保存上一帧在frame队列中，保存上一帧开启时，当窗口大小被调整，
 * 可以重新渲染上一帧显示。
 * @return
 * 成功 0
 * 失败 -1
 */
extern int frame_queue_init(FrameQueue *q, PacketQueue *p, int max_size, int keep_last);

extern void frame_queue_destory(FrameQueue *q);

/***
 * framt队列的frame数组windex下标向后移动一位，
 * 同时size++。
 * @param q frame队列对象
 */
extern void frame_queue_push(FrameQueue *q);

/***
 * frame队列rindex下标向后移动一位，同时
 * size--，并释放一帧frame数据。
 * @param q frame队列对象
 */
extern void frame_queue_next(FrameQueue *q);

/***
 * 从frame队列中获取一包可写入的frame_t
 * @param q frame队列对象
 * @return
 * 成功 返回一包可写入的frame_t
 * 失败 NULL
 */
extern Frame_t *frame_queue_peek_writable(FrameQueue *q);

/***
 * 从frame队列中返回一包frame_t数据
 * @param q frame队列
 * @return
 * 成功 返回一包frame_t数据
 * 失败 NULL
 */
extern Frame_t *frame_queue_peek_readable(FrameQueue *q);

/***
 * 从frame队列中获取当前正在显示的frame_t数据
 * @param q frame队列
 * @return
 * 成功 返回一包frame_t数据
 */
extern Frame_t *frame_queue_peek_last(FrameQueue *q);

/***
 *从frame队列中获取下一包待显示的frame_t数据
 * @param q frame队列
 * @return
 * 成功 返回一包frame_t数据
 */
extern Frame_t *frame_queue_peek(FrameQueue *q);

/***
 *从frame队列中返回下一包待显示的frame的下一包frame
 * @param q frame队列
 * @return
 * 成功 返回一包frame_t数据
 */
extern Frame_t *frame_queue_peek_next(FrameQueue *q);

/***
 *通过最新帧的pts，和系统时间计算出消逝的时间，
 * 同时更新时钟的pts，和当前帧的播放起点时间。
 * @param c 时钟对象
 * @param pts 当前帧的pts
 * @param serial 当前帧的序号
 * @param time 当前的系统时间
 */
extern void set_clock_at(Clock_t *c, double pts, int serial, double time);

/***
 *更新时钟的参数
 * @param c 时钟对象
 * @param pts 当前帧的pts
 * @param serial 当前帧的序号
 */
extern void set_clock(Clock_t *c, double pts, int serial);

/***
 * 初始化时钟
 * 播放速度为 1.00
 * pts 等于 NAN
 * @param c Clock对象
 * @param queue_serial 队列序号
 */
extern void clock_init(Clock_t *c, int *queue_serial);

/***
 * 获取时钟的进度
 * @param c 时钟对象
 * @return
 * 成功返回 当前时钟的播放进度
 * 失败返回 NAN
 */
extern double get_clock(Clock_t *c);

/***
 * 获取主时钟的播放进度
 * @param is PlayStream实例
 * @return
 * 成功返回 主时钟的播放进度
 * 失败返回 NAN
 */
extern double get_master_clock(PlayStream *is);

/***
 * 使用slave时钟的进度值更新同步c的进度
 * @param c 待更新的时钟
 * @param slave 提供同步进度的时钟
 */
extern void sync_clock_to_slave(Clock_t *c, Clock_t *slave);

extern void* play_url(void *arg);

#endif //SDL_AUDIO_QUEUE_H
