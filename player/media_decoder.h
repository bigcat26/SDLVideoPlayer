#ifndef _MEDIA_DECODER_H_
#define _MEDIA_DECODER_H_

#include <stdint.h>

typedef enum MediaType {
    MediaTypeVideo,
    MediaTypeAudio,
    MediaTypeUnknown,
} MediaType;

typedef struct MediaStreamInfo {
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
} MediaStreamInfo;

typedef struct MediaDecoder MediaDecoder;

typedef void (*MEDIADECODER_ONPACKET)(MediaDecoder*, int, const uint8_t*, int);

void mediaDecoderDestroy(MediaDecoder* decoder);

MediaDecoder* mediaDecoderCreate(void);

int mediaDecoderOpen(MediaDecoder* decoder, const char* file);

int mediaDecoderStreams(MediaDecoder* decoder);

int mediaDecoderStreamInfo(MediaDecoder* decoder, int streamId, MediaStreamInfo* info);

void mediaDecoderSetOnRawPacket(MediaDecoder* decoder, MEDIADECODER_ONPACKET onRawPacket);

MEDIADECODER_ONPACKET mediaDecoderGetOnRawPacket(MediaDecoder* decoder);

void mediaDecoderSetOnDecodedFrame(MediaDecoder* decoder, MEDIADECODER_ONPACKET onDecodedFrame);

MEDIADECODER_ONPACKET mediaDecoderGetOnDecodedFrame(MediaDecoder* decoder);

// int mediaDecoderStreamIsVideo(MediaDecoder* decoder, int streamId);

// int mediaDecoderStreamIsAudio(MediaDecoder* decoder, int streamId);

void mediaDecoderDumpFormat(MediaDecoder* decoder, const char* file);

int mediaDecoderReadFrame(MediaDecoder* decoder);

void mediaDecoderInit(void);

#endif // _MEDIA_DECODER_H_
