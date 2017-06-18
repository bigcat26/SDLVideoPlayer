#include <windows.h>

#include "libavutil/mem.h"
#include "libavutil/imgutils.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_thread.h>

// Old SDLv1
// video: http://dranger.com/ffmpeg/tutorial02.html
// audio: http://dranger.com/ffmpeg/tutorial03.html

// New SDLv2
// https://stackoverflow.com/questions/17579286/sdl2-0-alternative-for-sdl-overlay

// SDLv1 -> SDLv2 Migration Guide
// https://wiki.libsdl.org/MigrationGuide#SDL_1.2_to_2.0_Migration_Guide

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

int main()
{
    int i;
    int fps;
    int res;
    int gpp = 0;
    int stop;
    int rgbsize;
    int video_stream_id = -1;
    uint8_t *rgb_buf;
    AVCodec *decoder = NULL;
    AVFrame *fyuv = NULL;
    AVFrame *frgb = NULL;
    AVPacket packet;
    AVCodecContext *acc = NULL;
    AVFormatContext *afc = NULL;
    struct SwsContext *img_convert_ctx;
    
    SDL_Event sdl_event;
    SDL_Window *sdl_win = NULL;
    SDL_Renderer *sdl_renderer = NULL;
    SDL_Texture *sdl_texture = NULL;
    SDL_Surface *sdl_surface = NULL;
    Uint8 *yPlane, *uPlane, *vPlane;
    size_t yPlaneSz, uvPlaneSz;
    int uvPitch;


    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
    {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }

    av_log_set_level(AV_LOG_INFO);
    av_register_all();
    res = avformat_open_input(&afc, "E:\\BaiduYunDownload\\idoorcam.h264", NULL, NULL);
    //res = avformat_open_input(&afc, "F:\\QQDownload\\’’∆¨\\π¡÷’’∆¨\\photo_by_zll\\MVI_0394.AVI", NULL, NULL);
    if (0 != res)
    {
        printf("ERROR: %08X\n", res);
        return -1;
    }

    res = avformat_find_stream_info(afc, NULL);
    if (0 != res)
    {
        printf("ERROR: %08X\n", res);
        return -1;
    }

    for (i = 0; i < afc->nb_streams; ++i)
    {
        //av_dump_format(afc, i, "", 1);
        if (afc->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_stream_id = i;
            break;
        }
    }

    if (video_stream_id < 0)
    {
        return -1;
    }

    acc = afc->streams[video_stream_id]->codec;
    decoder = avcodec_find_decoder(acc->codec_id);
    if (NULL == decoder)
    {
        return -1;
    }

    res = avcodec_open2(acc, decoder, NULL);
    if (0 != res)
    {
        printf("ERROR: %08X\n", res);
        return -1;
    }

    fyuv = av_frame_alloc();
    frgb = av_frame_alloc();

    // Determine required buffer size and allocate buffer
    rgbsize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, acc->width, acc->height, 1);
    rgb_buf = (uint8_t *)av_malloc(rgbsize * sizeof(uint8_t));

    // Assign appropriate parts of buffer to image planes in pFrameRGB
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
    // of AVPicture
    av_image_fill_arrays(frgb->data, frgb->linesize, rgb_buf, AV_PIX_FMT_RGB24, acc->width, acc->height, 1);

//     img_convert_ctx = sws_getContext(acc->width, acc->height, acc->pix_fmt, 
//         acc->width, acc->height, AV_PIX_FMT_BGR24/*AV_PIX_FMT_RGB24*/,
//         SWS_BICUBIC, NULL, NULL, NULL);

    img_convert_ctx = sws_getContext(acc->width, acc->height, acc->pix_fmt,
        acc->width, acc->height, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, NULL, NULL, NULL);

    sdl_win = SDL_CreateWindow("Video Player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, acc->width, acc->height, SDL_WINDOW_OPENGL);
    //sdl_win = SDL_CreateWindow("Video Player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
    sdl_renderer = SDL_CreateRenderer(sdl_win, -1, 0);
    //SDL_CreateWindowAndRenderer(720, 576, SDL_WINDOW_OPENGL, &sdl_win, &sdl_renderer);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
    SDL_RenderSetLogicalSize(sdl_renderer, acc->width, acc->height);

    //SDL_SetRenderDrawColor(sdl_renderer, 127, 127, 127, 255);
    //SDL_RenderClear(sdl_renderer);
    //SDL_RenderPresent(sdl_renderer);

    sdl_texture = SDL_CreateTexture(sdl_renderer,
        SDL_PIXELFORMAT_YV12, //SDL_PIXELFORMAT_IYUV,
        SDL_TEXTUREACCESS_STREAMING,
        acc->width, acc->height);

    yPlaneSz = acc->width * acc->height;
    uvPlaneSz = acc->width * acc->height / 4;
    yPlane = (Uint8*)malloc(yPlaneSz);
    uPlane = (Uint8*)malloc(uvPlaneSz);
    vPlane = (Uint8*)malloc(uvPlaneSz);

    //SDL_UpdateTexture(sdl_texture, NULL, myPixels, 640 * sizeof(Uint32));

    //sdl_surface = SDL_LoadBMP("timg.bmp");

    uvPitch = acc->width / 2;
    for (i = 0, stop = 0, fps = 0; !stop; ++i)
    {
        res = av_read_frame(afc, &packet);
        if (AVERROR_EOF == res)
        {
            break;
        }
        if (res < 0)
        {
            break;
        }

        if (packet.stream_index != video_stream_id)
        {
            continue;
        }
        
        res = avcodec_decode_video2(acc, fyuv, &gpp, &packet);
        if (res < 0)
        {
            printf("ERROR: %08X\n", res);
            break;
        }

        if (gpp)
        {
            AVPicture pict;

            //sws_scale(img_convert_ctx, fyuv->data, fyuv->linesize,
            //    0, acc->height, frgb->data, frgb->linesize);
            //SaveFrame(frgb, acc->width, acc->height, i, 24);

            pict.data[0] = yPlane;
            pict.data[1] = uPlane;
            pict.data[2] = vPlane;
            pict.linesize[0] = acc->width;
            pict.linesize[1] = uvPitch;
            pict.linesize[2] = uvPitch;

            // Convert the image into YUV format that SDL uses
            sws_scale(img_convert_ctx, (uint8_t const * const *)fyuv->data,
                fyuv->linesize, 0, acc->height, pict.data, pict.linesize);
            //memset(uPlane, 0, uvPlaneSz);
            //memset(vPlane, 0, uvPlaneSz);

            ++fps;

            SDL_UpdateYUVTexture(sdl_texture, NULL, yPlane, acc->width, uPlane, uvPitch, vPlane, uvPitch);

            SDL_RenderClear(sdl_renderer);
            SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, NULL);
            SDL_RenderPresent(sdl_renderer);
        }

        av_free_packet(&packet);

        SDL_PollEvent(&sdl_event);
        switch (sdl_event.type) 
        {
        case SDL_QUIT:
            stop = 1;
            break;
        default:
            break;
        }
    }

    SDL_DestroyTexture(sdl_texture);
    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyWindow(sdl_win);

    free(yPlane);
    free(uPlane);
    free(vPlane);

    sws_freeContext(img_convert_ctx);
    avcodec_close(acc);

    if (NULL != rgb_buf)
    {
        av_free(rgb_buf);
    }
    if (NULL != frgb)
    {
        av_frame_free(&frgb);
    }
    if (NULL != fyuv)
    {
        av_frame_free(&fyuv);
    }
    if (NULL != afc)
    {
        avformat_close_input(&afc);
    }

    return 0;
}
