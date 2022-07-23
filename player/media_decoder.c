
#if defined(__cplusplus)
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#include <libavutil/buffer.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>

#if defined(__cplusplus)
}
#endif

#include "media_decoder.h"
#include "player.h"


#if 0
void SaveFrame(AVFrame *pFrame, int width, int height, int index, int bpp)
{
    int y;
    BITMAPFILEHEADER bmpheader;
    BITMAPINFOHEADER bmpinfo;
    FILE *fp;
    char szFilename[32];

    // Open file
    sprintf_s(szFilename, sizeof(szFilename), "bmp\\frame%d.bmp", index);
    fopen_s(&fp, szFilename, "wb");
    if (fp == NULL)
        return;
    bmpheader.bfType = 0x4d42;
    bmpheader.bfReserved1 = 0;
    bmpheader.bfReserved2 = 0;
    bmpheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmpheader.bfSize = bmpheader.bfOffBits + width*height*bpp / 8;
    bmpinfo.biSize = sizeof(BITMAPINFOHEADER);
    bmpinfo.biWidth = width;
    bmpinfo.biHeight = height;
    bmpinfo.biPlanes = 1;
    bmpinfo.biBitCount = bpp;
    bmpinfo.biCompression = BI_RGB;
    bmpinfo.biSizeImage = (width*bpp + 31) / 32 * 4 * height;
    bmpinfo.biXPelsPerMeter = 100;
    bmpinfo.biYPelsPerMeter = 100;
    bmpinfo.biClrUsed = 0;
    bmpinfo.biClrImportant = 0;
    fwrite(&bmpheader, sizeof(bmpheader), 1, fp);
    fwrite(&bmpinfo, sizeof(bmpinfo), 1, fp);
    // fwrite (pFrame->data[0], width*height*bpp/8, 1, fp);
    for (y = height - 1; y >= 0; y--)
        fwrite(pFrame->data[0] + y*pFrame->linesize[0], 1, width * 3, fp);
    fclose(fp);
}
#endif

#define MEDIA_DECODER_ENABLE_BSF 1
#define MEDIA_DECODER_ENABLE_DECODER 1
#define MEDIA_DECODER_MAX_STREAMS 2

struct MediaDecoder {
    AVFormatContext* afc;
    // AVCodec* codec[MEDIA_DECODER_MAX_STREAMS];
    MEDIADECODER_ONPACKET onRawPacket;
    MEDIADECODER_ONPACKET onDecodedFrame;
#if MEDIA_DECODER_ENABLE_DECODER
    AVCodecContext* avcc[MEDIA_DECODER_MAX_STREAMS];
#endif
#if MEDIA_DECODER_ENABLE_BSF
    AVBSFContext* bsf[MEDIA_DECODER_MAX_STREAMS];
    MEDIADECODER_ONPACKET onFilteredPacket;
#endif
};

void mediaDecoderDestroy(MediaDecoder* decoder) {
    int i;
    if (decoder) {
        for (i = 0; i < MEDIA_DECODER_MAX_STREAMS; ++i) {
            if (decoder->bsf[i]) {
                av_bsf_free(&decoder->bsf[i]);
            }
        }
        for (i = 0; i < MEDIA_DECODER_MAX_STREAMS; ++i) {
            if (decoder->avcc[i]) {
                avcodec_free_context(&decoder->avcc[i]);
            }
        }
        if (decoder->afc) {
            avformat_close_input(&decoder->afc);
        }
        free(decoder);
    }
}

MediaDecoder* mediaDecoderCreate(void) {
    MediaDecoder* decoder = malloc(sizeof(MediaDecoder));
    if (decoder) {
        memset(decoder, 0, sizeof(*decoder));
    }
    return decoder;
}

static int mediaDecoderDumpError(const char* text, int code) {
    char msg[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(code, msg, AV_ERROR_MAX_STRING_SIZE);
    LOGE("%s: %s (%d)\n", text, msg, code);
    return code;
}

int mediaDecoderInitDecoder(MediaDecoder* decoder, int streamId, AVCodec* codec) {
#if MEDIA_DECODER_ENABLE_DECODER
    int res;

    res = avcodec_open2(decoder->avcc[streamId], codec, NULL);
    if (res < 0) {
        mediaDecoderDumpError("codec open error", res);
        return res;
    }
#endif
    return 0;
}

int mediaDecoderInitBitstreamFilter(MediaDecoder* decoder, int streamId, AVCodec* codec) {
#if MEDIA_DECODER_ENABLE_BSF
    int res;
    const AVBitStreamFilter* bsf = NULL;

    if (codec->id == AV_CODEC_ID_H264) {
        bsf = av_bsf_get_by_name("h264_mp4toannexb");
        LOGD("stream #%d codec id is h264, bsf=%p\n", streamId, bsf);
    } else if (codec->id == AV_CODEC_ID_HEVC) {
        bsf = av_bsf_get_by_name("hevc_mp4toannexb");
        LOGD("stream #%d codec id is hevc, bsf=%p\n", streamId, bsf);
    }

    if (bsf != NULL) {
        res = av_bsf_alloc(bsf, &decoder->bsf[streamId]);
        if (res != 0) {
            mediaDecoderDumpError("unable to alloc bsf", res);
        } else {
            avcodec_parameters_copy(
                decoder->bsf[streamId]->par_in,
                decoder->afc->streams[streamId]->codecpar);
            res = av_bsf_init(decoder->bsf[streamId]);
            LOGD("stream #%d bsf init res=%d\n", streamId, res);
        }
    }
#endif
    return 0;
}

int mediaDecoderOpen(MediaDecoder* decoder, const char* file) {
    int i;
    int res;

    res = avformat_open_input(&decoder->afc, file, NULL, NULL);
    if (res != 0) {
        return mediaDecoderDumpError("unable to open input", res);
    }

    res = avformat_find_stream_info(decoder->afc, NULL);
    if (res < 0) {
        return mediaDecoderDumpError("unable to find stream info", res);
    }

    for (i = 0; i < decoder->afc->nb_streams; ++i) {
        AVCodecParameters* codecpar = decoder->afc->streams[i]->codecpar;
        decoder->avcc[i] = avcodec_alloc_context3(NULL);
        if (!decoder->avcc[i]) {
            LOGE("alloc decoder context error");
            return -1;
        }

        res = avcodec_parameters_to_context(decoder->avcc[i], codecpar);
        if (res < 0) {
            return mediaDecoderDumpError("unable to copy codec parameters", res);
        }

        AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
        LOGD("find decoder for codec tag: %c%c%c%c result %p\n",
             codecpar->codec_tag & 0xff, (codecpar->codec_tag >> 8) & 0xff,
             (codecpar->codec_tag >> 16) & 0xff, codecpar->codec_tag >> 24,
             codec);
        if (codec) {
            if (i < MEDIA_DECODER_MAX_STREAMS) {
                mediaDecoderInitDecoder(decoder, i, codec);
                mediaDecoderInitBitstreamFilter(decoder, i, codec);
            } else {
                LOGE("warn: too many streams %d\n", i);
            }
        }
    }

    return 0;
}

int mediaDecoderStreams(MediaDecoder* decoder) {
    if (!decoder || !decoder->afc) {
        return -1;
    }
    return decoder->afc->nb_streams;
}

int mediaDecoderSaveYUVImage(const char *file, AVFrame *frame)
{
    FILE *fp;
    uint32_t pitchY = frame->linesize[0];
    uint32_t pitchU = frame->linesize[1];
    uint32_t pitchV = frame->linesize[2];
    uint8_t *avY = frame->data[0];
    uint8_t *avU = frame->data[1];
    uint8_t *avV = frame->data[2];

    fp = fopen(file, "wb");
    if (!fp) {
        LOGE("open file %s failed: %s", file, strerror(errno));
        return -1;
    }

    for (uint32_t i = 0; i < frame->height; i++) {
        fwrite(avY, frame->width, 1, fp);
        avY += pitchY;
    }

    for (uint32_t i = 0; i < frame->height/2; i++) {
        fwrite(avU, frame->width / 2, 1, fp);
        avU += pitchU;
    }

    for (uint32_t i = 0; i < frame->height/2; i++) {
        fwrite(avV, frame->width / 2, 1, fp);
        avV += pitchV;
    }

    printf("file size: %ld\n", ftell(fp));

    fclose(fp);
    return 0;
}

int mediaDecoderStreamInfo(MediaDecoder* decoder, int streamId, MediaStreamInfo* info) {
    AVStream *stream;
    AVCodecContext *avcc;
    if (!decoder || !decoder->afc) {
        return -1;
    }

    stream = decoder->afc->streams[streamId];
    avcc = decoder->avcc[streamId];
    if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        info->type = MediaTypeVideo;
        info->u.video.width = avcc->width;
        info->u.video.height = avcc->height;
        info->u.video.pixelFormat = avcc->pix_fmt;
        // info->u.video.fps = 
        // int fps = st->avg_frame_rate.den && st->avg_frame_rate.num;
        // int tbr = st->r_frame_rate.den && st->r_frame_rate.num;
        // int tbn = st->time_base.den && st->time_base.num;

    } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        info->type = MediaTypeAudio;
        info->u.audio.bitRate = avcc->bit_rate;
    } else {
        info->type = MediaTypeUnknown;
        return 0; 
    }
    return 0;
}

void mediaDecoderSetOnRawPacket(MediaDecoder* decoder, MEDIADECODER_ONPACKET onRawPacket) {
    decoder->onRawPacket = onRawPacket;
}

MEDIADECODER_ONPACKET mediaDecoderGetOnRawPacket(MediaDecoder* decoder) {
    return decoder->onRawPacket;
}

void mediaDecoderSetOnDecodedFrame(MediaDecoder* decoder, MEDIADECODER_ONPACKET onDecodedFrame) {
    decoder->onDecodedFrame = onDecodedFrame;
}

MEDIADECODER_ONPACKET mediaDecoderGetOnDecodedFrame(MediaDecoder* decoder) {
    return decoder->onDecodedFrame;
}

void mediaDecoderDumpFormat(MediaDecoder* decoder, const char* file) {
    av_dump_format(decoder->afc, 0, file, 0);
}

static void mediaDecoderProcessDecode(MediaDecoder* decoder, AVPacket* pkt, int streamId) {
    int res;
    AVFrame* frame = NULL;

    if (decoder->avcc[streamId] == NULL) {
        return;
    }

    frame = av_frame_alloc();
    if (!frame) {
        LOGE("alloc frame failed");
        return;
    }

    res = avcodec_send_packet(decoder->avcc[streamId], pkt);
    if (res != 0) {
        LOGE("avcodec_send_packet failed streamId=%d\n", pkt->stream_index);
        mediaDecoderDumpError("codec send packet error", res);
        goto cleanup;
    }
    av_packet_unref(pkt);

    for (;;) {
        res = avcodec_receive_frame(decoder->avcc[streamId], frame);
        if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
            break;
        } else if (res < 0) {
            mediaDecoderDumpError("bsf receive packet error", res);
            break;
        }

        mediaDecoderSaveYUVImage("sample.yuv", frame);

        AVBufferRef *buf = NULL;
        buf = av_frame_get_plane_buffer(frame, 0);
        printf("plane 0 size: %d\n", buf->size);
        av_buffer_unref(&buf);
        buf = av_frame_get_plane_buffer(frame, 1);
        printf("plane 1 size: %d\n", buf->size);
        av_buffer_unref(&buf);
        buf = av_frame_get_plane_buffer(frame, 2);
        printf("plane 2 size: %d\n", buf->size);
        av_buffer_unref(&buf);
        buf = av_frame_get_plane_buffer(frame, 3);
        av_buffer_unref(&buf);

        if (decoder->onDecodedFrame) {
            decoder->onDecodedFrame(decoder, streamId, frame->data, frame->pkt_size);
        }

        av_frame_unref(frame);
    }

cleanup:
    if (frame) {
        av_frame_free(&frame);
    }
}

static void mediaDecoderProcessBitstreamFilter(MediaDecoder* decoder, AVPacket* pkt) {
    int res;
    int streamId = pkt->stream_index;
    AVPacket* filteredPacket = NULL;

    if (streamId >= MEDIA_DECODER_MAX_STREAMS) {
        return;
    }

    if (decoder->bsf[streamId] == NULL) {
        mediaDecoderProcessDecode(decoder, pkt, streamId);
        return;
    }

    filteredPacket = av_packet_alloc();
    if (!filteredPacket) {
        LOGE("av_packet_alloc failed");
        return;
    }

    res = av_bsf_send_packet(decoder->bsf[streamId], pkt);
    if (res != 0) {
        mediaDecoderDumpError("bsf send packet error", res);
        goto cleanup;
    }

    for (;;) {
        res = av_bsf_receive_packet(decoder->bsf[streamId], filteredPacket);
        if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
            break;
        } else if (res < 0) {
            mediaDecoderDumpError("bsf receive packet error", res);
            break;
        }

        if (decoder->onFilteredPacket) {
            decoder->onFilteredPacket(decoder, streamId, filteredPacket->data, filteredPacket->size);
        }

        mediaDecoderProcessDecode(decoder, filteredPacket, streamId);
        av_packet_unref(filteredPacket);
    }

cleanup:
    if (filteredPacket) {
        av_packet_free(&filteredPacket);
    }
}

int mediaDecoderReadFrame(MediaDecoder* decoder) {
    int res;
    AVPacket pkt;

    res = av_read_frame(decoder->afc, &pkt);
    if (AVERROR_EOF == res) {
        LOGD("read frame reaches EOF\n");
        return 0;
    }

    if (res < 0) {
        return mediaDecoderDumpError("read frame error", res);
    }

    if (decoder->onRawPacket) {
        decoder->onRawPacket(decoder, pkt.stream_index, pkt.data, pkt.size);
    }

    mediaDecoderProcessBitstreamFilter(decoder, &pkt);
    av_packet_unref(&pkt);
    return 1;
}

void mediaDecoderInit(void) {
    av_log_set_level(AV_LOG_VERBOSE);
}
