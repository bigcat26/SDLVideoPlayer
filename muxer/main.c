#include <assert.h>
#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#if defined(__cplusplus)
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>

#if defined(__cplusplus)
}
#endif

#include "muxer.h"

enum {
    H264_NAL_UNSPECIFIED     = 0,
    H264_NAL_SLICE           = 1,
    H264_NAL_DPA             = 2,
    H264_NAL_DPB             = 3,
    H264_NAL_DPC             = 4,
    H264_NAL_IDR_SLICE       = 5,
    H264_NAL_SEI             = 6,
    H264_NAL_SPS             = 7,
    H264_NAL_PPS             = 8,
    H264_NAL_AUD             = 9,
    H264_NAL_END_SEQUENCE    = 10,
    H264_NAL_END_STREAM      = 11,
    H264_NAL_FILLER_DATA     = 12,
    H264_NAL_SPS_EXT         = 13,
    H264_NAL_PREFIX          = 14,
    H264_NAL_SUB_SPS         = 15,
    H264_NAL_DPS             = 16,
    H264_NAL_RESERVED17      = 17,
    H264_NAL_RESERVED18      = 18,
    H264_NAL_AUXILIARY_SLICE = 19,
    H264_NAL_EXTEN_SLICE     = 20,
    H264_NAL_DEPTH_EXTEN_SLICE = 21,
    H264_NAL_RESERVED22      = 22,
    H264_NAL_RESERVED23      = 23,
    H264_NAL_UNSPECIFIED24   = 24,
    H264_NAL_UNSPECIFIED25   = 25,
    H264_NAL_UNSPECIFIED26   = 26,
    H264_NAL_UNSPECIFIED27   = 27,
    H264_NAL_UNSPECIFIED28   = 28,
    H264_NAL_UNSPECIFIED29   = 29,
    H264_NAL_UNSPECIFIED30   = 30,
    H264_NAL_UNSPECIFIED31   = 31,
};

#define H264_NAL_TYPE(x) (((x)[4]) & 0x1F)
#define H264_NAL_REF_IDC(x) ((((x)[4]) & 0x60) >> 5)
#define H264_NALU_IS_KEYFRAME(x) ((x) >= (uint8_t)H264_NAL_IDR_SLICE && (x) <= (uint8_t)H264_NAL_PPS)
#define H265_NALU_TYPE(x) ((((x)[4]) & 0x7E) >> 1)
#define H265_NALU_IS_KEYFRAME(x) ((x) >= (uint8_t)HEVC_NAL_BLA_W_LP && (x) <= (uint8_t)HEVC_NAL_CRA_NUT)


int muxerWriteFrame(MP4_MUXER *muxer, const char *file, int index) {
    int ret;
#if 0
    AVPacket pkt;
    AVFormatContext *ctx = NULL;
    AVCodecParameters *codecpar = NULL;

    ret = avformat_open_input(&ctx, file, NULL, NULL);
    if (ret < 0) {
        printf("%s open file error: %s\n", file, av_err2str(ret));
        return -1;
    }

    ret = avformat_find_stream_info(ctx, NULL);
    if (ret < 0) {
        printf("%s find stream info error: %s\n", file, av_err2str(ret));
        return -2;
    }

    // codecpar = ctx->streams[0]->codecpar;

    ret = av_read_frame(ctx, &pkt);
    if (ret < 0) {
        printf("%s read frame error: %s\n", file, av_err2str(ret));
        return -3;
    }

    av_dump_format(ctx, 0, file, 0);

    ret = mp4MuxerWriteFrame(muxer, 0, pkt.data, pkt.size, index * 6000, index * 6000, pkt.flags);
    if (ret) {
        printf("write frame error: %d\n", ret);
        return -4;
    }

    avformat_free_context(ctx);
#else
    FILE *fp;
    size_t size;
    uint8_t *shaped, *buff;

    fp = fopen(file, "rb");
    if (!fp) {
        printf("open file %s error: %s\n", file, strerror(errno));
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    buff = (uint8_t *)malloc(size);
    if (!buff) {
        printf("OOM!\n");
        fclose(fp);
        return -2;
    }

    if (size != fread(buff, 1, size, fp)) {
        printf("read failed!\n");
        fclose(fp);
        return -3;
    }

    shaped = buff;
    if (shaped[4] == H264_NAL_AUD) {
        shaped += 5;
        size -= 5;
    }
    if (shaped[0] == 0x30) {
        shaped[0] = 0;
    } else if (shaped[0] == 0x10) {
        shaped += 1;
        size -= 1;
    }

    // PKT_FLAG_KEY
    int flags = 0;
    uint8_t nalType = shaped[4] & 0x1F;
    printf("frame #%d file=%s data=%02x %02x\n", index, file, shaped[4], shaped[5]);
    if (H264_NALU_IS_KEYFRAME(nalType)) {
        flags = 1; // PKT_FLAG_KEY
    }

    ret = mp4MuxerWriteFrame(muxer, 0, shaped, size, flags);
    if (ret) {
        printf("write frame error: %d\n", ret);
        fclose(fp);
        return -4;
    }
    // printf("write frame(%s)\n", file);
    fclose(fp);
    free(buff);
#endif
    return 0;
}

int muxerWalkDir(MP4_MUXER *muxer, const char *path) {
    DIR *dirp;
    int index = 0;
    struct dirent *d;
    char file[PATH_MAX];

    dirp = opendir(path);
    if (!dirp) {
        printf("opendir %s error: %s\n", path, strerror(errno));
        return -1;
    }

    while ((d = readdir(dirp)) != NULL) {
        if (d->d_type == DT_REG) {
            snprintf(file, PATH_MAX - 1, "%s/%s", path, d->d_name);
            muxerWriteFrame(muxer, file, index++);
        }
    }

    return closedir(dirp);
}

int main(int argc, char **argv) {
    int res;
    const char *filename = "output.mp4";
    MP4_MUXER_VIDEO_PARAMS vopts;
    MP4_MUXER_AUDIO_PARAMS aopts;

    MP4_MUXER *muxer = NULL;

    res = mp4MuxerNew(&muxer);
    assert(res == 0);

    vopts.width = 640;
    vopts.height = 480;
    vopts.frameRate = 15;
    vopts.gopSize = 45;
    res = mp4MuxerAddH264Stream(muxer, &vopts);
    assert(res >= 0);

    // aopts.bitRate = 16000;
    // aopts.sampleRate = 221000;
    // aopts.channelLayout = MP4_MUXER_AUDIO_CHANNEL_LAYOUT_MONO;
    // res = mp4MuxerAddAACStream(muxer, &aopts);
    // assert(res >= 0);

    res = mp4MuxerOpen(muxer, filename);
    assert(res == 0);

    muxerWalkDir(muxer, "/root/work/p2p/third_party/producer/samples/h264SampleFrames");
    // muxerWalkDir(muxer, "/root/work/p2p/third_party/producer/samples/aacSampleFrames");

    res = mp4MuxerClose(muxer);
    assert(res == 0);

    res = mp4MuxerFree(&muxer);
    assert(res == 0);

    return 0;
}
