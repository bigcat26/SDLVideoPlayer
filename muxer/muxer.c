#include <errno.h>
#include <libavcodec/codec_id.h>
#include <string.h>

#if defined(__cplusplus)
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h>
#include <libavformat/avformat.h>
#include <libavutil/avassert.h>
#include <libavutil/avutil.h>
#include <libavutil/buffer.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/mem.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>

#if defined(__cplusplus)
}
#endif

#include "muxer.h"

#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */


int mp4MuxerNew(MP4_MUXER **muxer) {
    MP4_MUXER *m = (MP4_MUXER *)malloc(sizeof(MP4_MUXER));
    if (m) {
        memset(m, 0, sizeof(MP4_MUXER));
        avformat_alloc_output_context2(&m->afc, NULL, NULL, "a.mp4");
        if (!m->afc) {
            avformat_alloc_output_context2(&m->afc, NULL, "mpeg", "a.mp4");
            if (!m->afc) {
                LOGE("alloc output context failed\n");
                return 1;
            }
        }
        *muxer = m;
        return 0;
    }
    return -ENOMEM;
}

int mp4MuxerOpen(MP4_MUXER *muxer, const char *filename) {
    int ret;

    av_dump_format(muxer->afc, 0, NULL, 1);

    ret = avio_open(&muxer->afc->pb, filename, AVIO_FLAG_WRITE);
    if (ret < 0) {
        LOGE("Could not open '%s': %s\n", filename, av_err2str(ret));
        return ret;
    }

    ret = avformat_write_header(muxer->afc, NULL);
    if (ret < 0) {
        LOGE("Error occurred when write header: %s\n", av_err2str(ret));
        return ret;
    }

    return 0;
}

int mp4MuxerClose(MP4_MUXER *muxer) {
    if (muxer->afc) {
        av_write_trailer(muxer->afc);
        if (!(muxer->afc->oformat->flags & AVFMT_NOFILE)) {
            avio_close(muxer->afc->pb);
        }
    }
    return 0;
}

int mp4MuxerFree(MP4_MUXER **muxer) {
    if (muxer && *muxer) {
        MP4_MUXER *m = *muxer;
        if (m->afc) {
            avformat_free_context(m->afc);
        }
        free(m);
        *muxer = NULL;
    }
    return 0;
}

int mp4MuxerAddStream(MP4_MUXER *muxer, AVCodecContext *c) {
    int ret;
    int streamId;
    MP4_MUXER_STREAM *stream = NULL;

    streamId = muxer->afc->nb_streams;
    if (streamId >= MP4_MUXER_MAX_STREAMS) {
        LOGE("no more streams\n");
        return -1;
    }

    stream = &muxer->streams[streamId];
    stream->timeBase = c->time_base;

    // stream->tmp_pkt = av_packet_alloc();
    // if (!stream->tmp_pkt) {
    //     LOGE("Could not allocate AVPacket\n");
    //     return -2;
    // }

    stream->st = avformat_new_stream(muxer->afc, NULL);
    if (!stream->st) {
        LOGE("Could not allocate stream\n");
        return -3;
    }

    stream->st->id = streamId;
    stream->st->time_base = c->time_base;

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(stream->st->codecpar, c);
    if (ret < 0) {
        LOGE("Could not copy the stream parameters\n");
        return -4;
    }

    if (muxer->afc->oformat->flags & AVFMT_GLOBALHEADER) {
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    return streamId;
}

AVCodecContext *mp4MuxerCreateCodecContext(MP4_MUXER *muxer, enum AVCodecID codecId, const AVCodec **codec) {
    AVCodecContext *c;
    /* find the encoder */
    *codec = avcodec_find_encoder(codecId);
    if (!*codec) {
        LOGE("could not find encoder for %s\n", avcodec_get_name(codecId));
        return NULL;
    }

    c = avcodec_alloc_context3(*codec);
    if (!c) {
        LOGE("could not alloc an encoding context\n");
        return NULL;
    }
    c->codec_id = codecId;
    return c;
}


int mp4MuxerAddVideoStream(MP4_MUXER *muxer, const MP4_MUXER_VIDEO_PARAMS *params, enum AVCodecID codecId) {
    int i;
    int streamId;
    AVCodecContext *c;
    const AVCodec *codec;

    c = mp4MuxerCreateCodecContext(muxer, codecId, &codec);
    if (!c || !codec) {
        return -1;
    }

    c->bit_rate = 400000;

    /* Resolution must be a multiple of two. */
    c->width = params->width;
    c->height = params->height;

    /* timebase: This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identical to 1. */
    c->time_base = (AVRational){1, params->frameRate};

    c->gop_size = params->gopSize; /* emit one intra frame every twelve frames at most */
    c->pix_fmt = STREAM_PIX_FMT;
    return mp4MuxerAddStream(muxer, c);
}

// , int width, int height, int frameRate, int gopSize, int pixelFormat
int mp4MuxerAddH264Stream(MP4_MUXER *muxer, const MP4_MUXER_VIDEO_PARAMS *params) {
    return mp4MuxerAddVideoStream(muxer, params, AV_CODEC_ID_H264);
}

int mp4MuxerAddHEVCStream(MP4_MUXER *muxer, const MP4_MUXER_VIDEO_PARAMS *params) {
    return mp4MuxerAddVideoStream(muxer, params, AV_CODEC_ID_HEVC);
}

int mp4MuxerAddAACStream(MP4_MUXER *muxer, const MP4_MUXER_AUDIO_PARAMS *params) {
    int i;
    int streamId;
    AVCodecContext *c;
    const AVCodec *codec;

    c = mp4MuxerCreateCodecContext(muxer, AV_CODEC_ID_AAC, &codec);
    if (!c || !codec) {
        return -1;
    }

    c->sample_fmt  = codec->sample_fmts ? codec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
    c->bit_rate    = params->bitRate;
    c->sample_rate = params->sampleRate;
    c->frame_size  = params->frameSize;

#if 0
    if (codec->supported_samplerates) {
        c->sample_rate = codec->supported_samplerates[0];
        for (i = 0; codec->supported_samplerates[i]; i++) {
            if (codec->supported_samplerates[i] == 44100) {
                c->sample_rate = 44100;
            }
        }
    }
#endif

    c->channels = av_get_channel_layout_nb_channels(params->channelLayout);
    c->channel_layout = params->channelLayout; // AV_CH_LAYOUT_STEREO;
#if 0
    if (codec->channel_layouts) {
        c->channel_layout = codec->channel_layouts[0];
        for (i = 0; codec->channel_layouts[i]; i++) {
            if (codec->channel_layouts[i] == AV_CH_LAYOUT_STEREO) {
                c->channel_layout = AV_CH_LAYOUT_STEREO;
            }
        }
    }
    c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
#endif

    /* timebase: This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identical to 1. */
    c->time_base = (AVRational){1, params->sampleRate};

    return mp4MuxerAddStream(muxer, c);
}

static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);
}

int mp4MuxerWriteFrame(MP4_MUXER *muxer, int streamId, const uint8_t *data, int size, int flags) {
    int ret;
    int64_t pts;
    AVPacket pkt;
    MP4_MUXER_STREAM *stream = NULL;

    if (streamId >= muxer->afc->nb_streams) {
        LOGE("bad stream id: %d\n", streamId);
        return -1;
    }
    stream = &muxer->streams[streamId];

    memset(&pkt, 0, sizeof(pkt));
    pkt.data = (uint8_t *)data;
    pkt.size = size;
    pkt.stream_index = streamId;
    pkt.dts = stream->nextPts;
    pkt.pts = stream->nextPts;
    pkt.flags = flags;
    pkt.pos = -1;
    stream->nextPts += av_rescale_q(1, stream->timeBase, stream->st->time_base);

#if 0
    ret = av_interleaved_write_frame(muxer->afc, pkt);
    /* pkt is now blank (av_interleaved_write_frame() takes ownership of
     * its contents and resets pkt), so that no unreferencing is necessary.
     * This would be different if one used av_write_frame(). */
    if (ret < 0) {
        LOGE("Error while writing output packet: %s\n", av_err2str(ret));
        return ret;
    }
#endif
    ret = av_write_frame(muxer->afc, &pkt);
    log_packet(muxer->afc, &pkt);
    if (ret < 0) {
        LOGE("Error while writing output packet: %s\n", av_err2str(ret));
        return ret;
    }

    return ret == AVERROR_EOF ? 1 : 0;
}
