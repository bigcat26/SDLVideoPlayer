#ifndef _MEDIA_DECODER_H_
#define _MEDIA_DECODER_H_

#include <stdint.h>

typedef enum MediaType {
    MediaTypeVideo,
    MediaTypeAudio
} MediaType;

typedef struct MediaInfo {
    MediaType type;
    union {
        struct {
            int width;
            int height;
        } video;

        struct {
            int bitRate;
            int sampleRate;
            int channels;
        } audio;
    } u;
} MediaInfo;

typedef struct MediaDecoder MediaDecoder;

typedef void (*MediaDecoderOnPacket)(MediaDecoder*, int, const uint8_t*, int);

void mediaDecoderDestroy(MediaDecoder* decoder);

MediaDecoder* mediaDecoderCreate(void);

int mediaDecoderOpen(MediaDecoder* decoder, const char* file);

int mediaDecoderStreamIsVideo(MediaDecoder* decoder, int streamId);

int mediaDecoderStreamIsAudio(MediaDecoder* decoder, int streamId);

void mediaDecoderDumpFormat(MediaDecoder* decoder, const char* file);

int mediaDecoderReadFrame(MediaDecoder* decoder);

void mediaDecoderInit(void);

#endif // _MEDIA_DECODER_H_
