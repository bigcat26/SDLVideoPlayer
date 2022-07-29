#ifndef _MP4_COMMON_H_
#define _MP4_COMMON_H_

#include "common.h"

typedef enum MP4MediaType {
    MP4MediaTypeVideo,
    MP4MediaTypeAudio,
    MP4MediaTypeUnknown,
} MP4MediaType;

typedef enum MP4MediaCodec {
    MP4MediaCodecH264,
    MP4MediaCodecHEVC,
    MP4MediaCodecAAC,
} MP4MediaCodec;

typedef struct MP4MediaStreamInfo {
    MP4MediaType type;
    union {
        struct {
            int fps;
            int width;
            int height;
            int pixelFormat;
        } video;

        struct {
            int bitRate;
            int sampleRate;
            int channels;
        } audio;
    } u;
} MP4MediaStreamInfo;

#endif // _MP4_COMMON_H_
