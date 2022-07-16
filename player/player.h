#ifndef _PLAYER_H_
#define _PLAYER_H_


#ifdef WIN32
#define MP4_FILE "D:\\code\\sample.mp4"
#else
#define MP4_FILE "~/Downloads/Incoming/video.mp4"
#endif

#ifdef WIN32
#define LOGD(fmt, ...) printf("[%s:%d] " fmt "\n", __FILE__, __LINE__, __VA_ARGS__)
#define LOGE(fmt, ...) printf("[%s:%d] " fmt "\n", __FILE__, __LINE__, __VA_ARGS__)
#else
#define LOGD(fmt, args...) printf("[%s:%d] " fmt "\n", __FILE__, __LINE__, ##args)
#define LOGE(fmt, args...) printf("[%s:%d] " fmt "\n", __FILE__, __LINE__, ##args)
#endif


#endif // _PLAYER_H_
