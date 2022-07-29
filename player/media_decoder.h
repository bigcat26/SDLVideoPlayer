#ifndef _MEDIA_DECODER_H_
#define _MEDIA_DECODER_H_

#include <stdint.h>

#include "mp4_common.h"

typedef struct MediaDecoder MediaDecoder;

typedef int (*MEDIADECODER_ONPACKET)(MediaDecoder*, int, const uint8_t*, int);
typedef int (*MEDIADECODER_ONFRAME)(MediaDecoder*, int, uint8_t, const int*, const uint8_t**);

void mediaDecoderDestroy(MediaDecoder* decoder);

MediaDecoder* mediaDecoderCreate(void);

int mediaDecoderOpen(MediaDecoder* decoder, const char* file);

int mediaDecoderStreams(MediaDecoder* decoder);

int mediaDecoderStreamInfo(MediaDecoder* decoder, int streamId, MP4MediaStreamInfo* info);

void mediaDecoderSetOnRawPacket(MediaDecoder* decoder, MEDIADECODER_ONPACKET onRawPacket);

MEDIADECODER_ONPACKET mediaDecoderGetOnRawPacket(MediaDecoder* decoder);

void mediaDecoderSetOnDecodedFrame(MediaDecoder* decoder, MEDIADECODER_ONFRAME onDecodedFrame);

MEDIADECODER_ONFRAME mediaDecoderGetOnDecodedFrame(MediaDecoder* decoder);

void mediaDecoderDumpFormat(MediaDecoder* decoder, const char* file);

int mediaDecoderReadFrame(MediaDecoder* decoder);

void mediaDecoderInit(void);

#endif // _MEDIA_DECODER_H_
