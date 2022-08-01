#ifndef _MUXER_H_
#define _MUXER_H_

#include <libavutil/rational.h>
#include <stdint.h>

#include "mp4_common.h"

#define MP4_MUXER_MAX_STREAMS 2

struct MP4_MUXER_STREAM {
    AVStream *st;
    int64_t nextPts;
    AVRational timeBase;
};

typedef struct MP4_MUXER_STREAM MP4_MUXER_STREAM;

struct MP4_MUXER {
    AVFormatContext *afc;
    MP4_MUXER_STREAM streams[MP4_MUXER_MAX_STREAMS];
};

typedef struct MP4_MUXER_VIDEO_PARAMS {
    int width;
    int height;
    int frameRate;
} MP4_MUXER_VIDEO_PARAMS;

typedef enum MP4_MUXER_AUDIO_CHANNEL_LAYOUT {
    MP4_MUXER_AUDIO_CHANNEL_LAYOUT_MONO = 4,
} MP4_MUXER_AUDIO_CHANNEL_LAYOUT;

typedef struct MP4_MUXER_AUDIO_PARAMS {
    int bitRate;
    int sampleRate;
    int frameSize;
    int channelLayout;
} MP4_MUXER_AUDIO_PARAMS;

typedef struct MP4_MUXER MP4_MUXER;

int mp4MuxerNew(MP4_MUXER **muxer);

int mp4MuxerOpen(MP4_MUXER *muxer, const char *output);

int mp4MuxerClose(MP4_MUXER *muxer);

int mp4MuxerFree(MP4_MUXER **muxer);

int mp4MuxerAddH264Stream(MP4_MUXER *muxer, const MP4_MUXER_VIDEO_PARAMS *params);

int mp4MuxerAddHEVCStream(MP4_MUXER *muxer, const MP4_MUXER_VIDEO_PARAMS *params);

int mp4MuxerAddAACStream(MP4_MUXER *muxer, const MP4_MUXER_AUDIO_PARAMS *params);

int mp4MuxerWriteFrame(MP4_MUXER *muxer, int streamId, const uint8_t *data, int size, int flags);

#endif // _MUXER_H_
