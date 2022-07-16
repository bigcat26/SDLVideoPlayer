
#if defined(__cplusplus)
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
#include <libavutil/error.h>

#include <libswscale/swscale.h>

#if defined(__cplusplus)
}
#endif

#include "player.h"
#include "media_decoder.h"

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

#define MEDIA_DECODER_ENABLE_BSF      1
#define MEDIA_DECODER_ENABLE_DECODER  1
#define MEDIA_DECODER_MAX_STREAMS     2


struct MediaDecoder {
    AVFormatContext* afc;
    //AVCodec* codec[MEDIA_DECODER_MAX_STREAMS];
    MediaDecoderOnPacket onRawPacket;
#if MEDIA_DECODER_ENABLE_DECODER
    AVCodecContext* codecCtx[MEDIA_DECODER_MAX_STREAMS];
#endif
#if MEDIA_DECODER_ENABLE_BSF
    AVBSFContext* bsf[MEDIA_DECODER_MAX_STREAMS];
    MediaDecoderOnPacket onFilteredPacket;
#endif
};

void mediaDecoderDestroy(MediaDecoder *decoder) {
    if (decoder) {
        free(decoder);
    }
}

MediaDecoder *mediaDecoderCreate(void) {
    MediaDecoder* decoder = malloc(sizeof(MediaDecoder));
    if (decoder) {
        memset(decoder, 0, sizeof(*decoder));
    }
    return decoder;
}

static int mediaDecoderDumpError(const char *text, int code) {
    char msg[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(code, msg, AV_ERROR_MAX_STRING_SIZE);
    LOGE("%s: %s (%d)", text, msg, code);
    return code;
}

int mediaDecoderInitDecoder(MediaDecoder* decoder, int streamId, AVCodec* codec)
{
#if MEDIA_DECODER_ENABLE_DECODER
    int res;

    decoder->codecCtx[streamId] = avcodec_alloc_context3(codec);
    if (!decoder->codecCtx[streamId])
    {
        LOGE("alloc decoder context error");
        return -1;
    }

    res = avcodec_open2(decoder->codecCtx[streamId], codec, NULL);
    if (res < 0)
    {
        mediaDecoderDumpError("codec open error", res);
        return res;
    }
#endif
    return 0;
}

int mediaDecoderInitBitstreamFilter(MediaDecoder* decoder, int streamId, AVCodec* codec)
{
#if MEDIA_DECODER_ENABLE_BSF
    int res;
    const AVBitStreamFilter* bsf = NULL;

    if (codec->id == AV_CODEC_ID_H264) {
        bsf = av_bsf_get_by_name("h264_mp4toannexb");
    } else if (codec->id == AV_CODEC_ID_HEVC) {
        bsf = av_bsf_get_by_name("hevc_mp4toannexb");
    }

    if (bsf != NULL) {
        res = av_bsf_alloc(bsf, &decoder->bsf[streamId]);
        if (res != 0)
        {
            mediaDecoderDumpError("unable to alloc bsf", res);
        }
        else
        {
            avcodec_parameters_copy(
                decoder->bsf[streamId]->par_in,
                decoder->afc->streams[streamId]->codecpar);
            av_bsf_init(decoder->bsf[streamId]);
        }
    }
#endif
    return 0;
}

int mediaDecoderOpen(MediaDecoder *decoder, const char* file) {
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
        AVCodecContext* acc = decoder->afc->streams[i]->codec;
        AVCodec* codec = avcodec_find_decoder(acc->codec_id);
        if (!codec) {
            LOGE("Unable to find decoder for codec tag: %c%c%c%c", 
                acc->codec_tag & 0xff, (acc->codec_tag >> 8) & 0xff, 
                (acc->codec_tag >> 16) & 0xff, acc->codec_tag >> 24);
            return -1;
        }

        LOGD("found decoder for codec tag: %c%c%c%c",
            acc->codec_tag & 0xff, (acc->codec_tag >> 8) & 0xff,
            (acc->codec_tag >> 16) & 0xff, acc->codec_tag >> 24);

        if (i < MEDIA_DECODER_MAX_STREAMS) {
            // decoder->codec[i] = codec;
            mediaDecoderInitDecoder(decoder, i, codec);
            mediaDecoderInitBitstreamFilter(decoder, i, codec);
        }
    }

    return 0;
}

enum AVMediaType mediaDecoderStreamCodec(MediaDecoder* decoder, int streamId) {
    if (streamId < decoder->afc->nb_streams) {
        return decoder->afc->streams[streamId]->codec->codec_type;
    }
    return AVMEDIA_TYPE_UNKNOWN;
}

int mediaDecoderStreamIsVideo(MediaDecoder* decoder, int streamId)
{
    return mediaDecoderStreamCodec(decoder, streamId) == AVMEDIA_TYPE_VIDEO;
}

int mediaDecoderStreamIsAudio(MediaDecoder* decoder, int streamId)
{
    return mediaDecoderStreamCodec(decoder, streamId) == AVMEDIA_TYPE_AUDIO;
}

void mediaDecoderDumpFormat(MediaDecoder* decoder) {
    av_dump_format(decoder->afc, 0, MP4_FILE, 0);
}

static void mediaDecoderProcessDecode(MediaDecoder* decoder, AVPacket* pkt, int streamId)
{
    int res;
    AVFrame* frame = NULL;

    frame = av_frame_alloc();
    if (!frame) {
        LOGE("alloc frame failed");
        return;
    }

    res = avcodec_send_packet(decoder->codecCtx[streamId], pkt);
    if (res != 0) {
        mediaDecoderDumpError("codec send packet error", res);
        return;
    }

    for (;;) {
        res = avcodec_receive_frame(decoder->codecCtx[streamId], frame);
        if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
            break;
        }
        else if (res < 0)
        {
            mediaDecoderDumpError("bsf receive packet error", res);
            break;
        }

        // TODO: do something with frame...

        av_frame_unref(frame);
    }

    if (frame) {
        av_frame_free(&frame);
    }
}

static void mediaDecoderProcessBitstreamFilter(MediaDecoder* decoder, AVPacket *pkt) {
    int res;
    int streamId = pkt->stream_index;
    AVPacket * filteredPacket = NULL;

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
        return;
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

    if (filteredPacket) {
        av_packet_free(&filteredPacket);
    }
}


int mediaDecoderReadFrame(MediaDecoder* decoder) {
    int res;
    AVPacket pkt;

    res = av_read_frame(decoder->afc, &pkt);
    if (AVERROR_EOF == res) {
        return 0;
    }

    if (res < 0) {
        char msg[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(res, msg, AV_ERROR_MAX_STRING_SIZE);
        LOGE("read frame error: %s (%d)", msg, res);
        return res;
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