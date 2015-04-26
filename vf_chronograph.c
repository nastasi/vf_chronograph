/*
 * Copyright (c) 2008 Affine Systems, Inc (Michael Sullivan, Bobby Impollonia)
 * Copyright (c) 2013 Andrey Utkin <andrey.krieger.utkin gmail com>
 * Copyright (c) 2015 Matteo Nastasi <matteo.nastasi gmail com>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * Chronograph filter. An example of animated overlay using
 * gdlib on the input frame.
 */

#include <stdio.h>
#include <gd.h>

#include "libavutil/colorspace.h"
#include "libavutil/common.h"
#include "libavutil/opt.h"
#include "libavutil/eval.h"
#include "libavutil/pixdesc.h"
#include "libavutil/parseutils.h"
#include "libavfilter/internal.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/formats.h"
#include "libavformat/avformat.h"

static const char *const var_names[] = {
    "x",
    "y",
    "alpha",

    "dar",

    "hsub", "vsub",
    "in_h", "ih",      ///< height of the input video
    "in_w", "iw",      ///< width  of the input video
    "sar",
    "t",
    "max",
    NULL
};

enum { Y, U, V, A };

enum var_name {
    VAR_X,
    VAR_Y,
    VAR_ALPHA,

    VAR_DAR,

    VAR_HSUB, VAR_VSUB,
    VAR_IN_H, VAR_IH,
    VAR_IN_W, VAR_IW,
    VAR_SAR,
    VAR_T,
    VAR_MAX,
    VARS_NB
};

typedef struct ChronographContext {
    const AVClass *class;
    int x, y;
    int alpha;

    int vsub, hsub;   ///< chroma subsampling
    char *x_s, *y_s, *alpha_s; ///< expression for x and y
    gdImagePtr im_bg, im_decsec, im_sec, im_min, im_clk;
    gdImagePtr imout;
} ChronographContext;

static av_cold int init(AVFilterContext *ctx)
{
    ChronographContext *s = ctx->priv;
    FILE *in;

    s->x = atoi(s->x_s);
    s->y = atoi(s->y_s);
    s->alpha = atoi(s->alpha_s);

    if ((in = fopen("media/dec_of_sec.png", "rb")) == NULL)
        return AVERROR(EINVAL);
    s->im_decsec = gdImageCreateFromPng(in);
    fclose(in);

    if ((in = fopen("media/sec.png", "rb")) == NULL)
        return AVERROR(EINVAL);
    s->im_sec = gdImageCreateFromPng(in);
    fclose(in);

    if ((in = fopen("media/min.png", "rb")) == NULL)
        return AVERROR(EINVAL);
    s->im_min = gdImageCreateFromPng(in);
    fclose(in);

    if ((in = fopen("media/clock.png", "rb")) == NULL)
        return AVERROR(EINVAL);
    s->im_clk = gdImageCreateFromPng(in);
    fclose(in);

    s->imout = gdImageCreateTrueColor(gdImageSX(s->im_clk), gdImageSY(s->im_clk));

    return 0;
}

static int query_formats(AVFilterContext *ctx)
{
    static const enum AVPixelFormat pix_fmts[] = {
        AV_PIX_FMT_YUV444P,  AV_PIX_FMT_YUV422P,  AV_PIX_FMT_YUV420P,
        AV_PIX_FMT_YUV411P,  AV_PIX_FMT_YUV410P,
        AV_PIX_FMT_YUVJ444P, AV_PIX_FMT_YUVJ422P, AV_PIX_FMT_YUVJ420P,
        AV_PIX_FMT_YUV440P,  AV_PIX_FMT_YUVJ440P,
        AV_PIX_FMT_NONE
    };
    AVFilterFormats *fmts_list = ff_make_format_list(pix_fmts);

    if (!fmts_list)
        return AVERROR(ENOMEM);
    return ff_set_common_formats(ctx, fmts_list);
}

static int config_input(AVFilterLink *inlink)
{
    AVFilterContext *ctx = inlink->dst;
    ChronographContext *s = ctx->priv;
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(inlink->format);

    s->hsub = desc->log2_chroma_w;
    s->vsub = desc->log2_chroma_h;

    return 0;
}

static int filter_frame(AVFilterLink *inlink, AVFrame *frame)
{
    ChronographContext *s = inlink->dst->priv;
    int plane, x, y, ang;
    unsigned char *row[4];
    unsigned char pixel[4];

    double instant;

    instant = av_q2d(inlink->time_base) * frame->pts;

    gdImageAlphaBlending(s->imout, 0);
    gdImageCopy(s->imout, s->im_clk, 0,0, 0,0, gdImageSX(s->im_clk), gdImageSY(s->im_clk));
    gdImageAlphaBlending(s->imout, 1);
    
    ang = 360 - (int)(fmod(floor(instant / 60.0), 60.0) * 6.0); /* 0.36 == ( 360.0 / 1000.0 ) */
    gdImageCopyRotated(s->imout, s->im_min, gdImageSX(s->imout)>>1, gdImageSY(s->imout)>>1, 0, 0,
                       gdImageSX(s->im_min), gdImageSY(s->im_min), ang);

    ang = 360 - (int)(fmod(floor(instant), 60.0) * 6.0); /* 0.36 == ( 360.0 / 1000.0 ) */
    gdImageCopyRotated(s->imout, s->im_sec, gdImageSX(s->imout)>>1, gdImageSY(s->imout)>>1, 0, 0,
                       gdImageSX(s->im_sec), gdImageSY(s->im_sec), ang);

    ang = 360 - (int)(floor(fmod(instant , 1.0)  * 1000.0) * 0.36); /* 0.36 == ( 360.0 / 1000.0 ) */
    gdImageCopyRotated(s->imout, s->im_decsec, gdImageSX(s->imout)>>1, gdImageSY(s->imout)>>1, 0, 0,
                       gdImageSX(s->im_decsec), gdImageSY(s->im_decsec), ang);

    // METTI I MINUTI

    gdImageAlphaBlending(s->imout, 0);


    for (y = FFMAX(s->y, 0); y < frame->height && y <  (s->y + gdImageSY(s->imout)) ; y++) {
        row[0] = frame->data[0] + y * frame->linesize[0];

        for (plane = 1; plane < 3; plane++)
            row[plane] = frame->data[plane] +
                 frame->linesize[plane] * (y >> s->vsub);

        for (x = FFMAX(s->x, 0); x < frame->width && x < (s->x + gdImageSX(s->imout)) ; x++) {
            double alpha;
            int c;

            c = gdImageGetPixel(s->imout, x - s->x, y - s->y);

            pixel[Y] = RGB_TO_Y_CCIR(gdImageRed(s->imout, c), gdImageGreen(s->imout, c), gdImageBlue(s->imout, c));
            pixel[U] = RGB_TO_U_CCIR(gdImageRed(s->imout, c), gdImageGreen(s->imout, c), gdImageBlue(s->imout, c), 0);
            pixel[V] = RGB_TO_V_CCIR(gdImageRed(s->imout, c), gdImageGreen(s->imout, c), gdImageBlue(s->imout, c), 0);
            // pixel[A] = 255;
            pixel[A] = gdImageAlpha(s->imout,c);
            alpha = (double)((127.0 - pixel[A]) * ((double)(s->alpha) / 255.0)) / 127.0;

            row[0][x           ] = (1 - alpha) * row[0][x           ] + alpha * pixel[Y];
            row[1][x >> s->hsub] = (1 - alpha) * row[1][x >> s->hsub] + alpha * pixel[U];
            row[2][x >> s->hsub] = (1 - alpha) * row[2][x >> s->hsub] + alpha * pixel[V];
        }
    }

    return ff_filter_frame(inlink->dst->outputs[0], frame);
}

#define OFFSET(x) offsetof(ChronographContext, x)
#define FLAGS AV_OPT_FLAG_VIDEO_PARAM|AV_OPT_FLAG_FILTERING_PARAM

#define CONFIG_CHRONOGRAPH_FILTER 1

#if CONFIG_CHRONOGRAPH_FILTER

static const AVOption chronograph_options[] = {
    { "x",         "set horizontal position of the left chronograph edge", OFFSET(x_s), AV_OPT_TYPE_STRING, { .str="0" },   CHAR_MIN, CHAR_MAX, FLAGS },
    { "y",         "set vertical position of the top chronograph edge",    OFFSET(y_s), AV_OPT_TYPE_STRING, { .str="0" },   CHAR_MIN, CHAR_MAX, FLAGS },
    { "alpha",         "set overhall alpha value",    OFFSET(alpha_s), AV_OPT_TYPE_STRING, { .str="255" },   CHAR_MIN, CHAR_MAX, FLAGS },

    { NULL }
};

#define AVFILTER_DEFINE_CLASS(fname)            \
    static const AVClass fname##_class = {      \
        .class_name = #fname,                   \
        .item_name  = av_default_item_name,     \
        .option     = fname##_options,          \
        .version    = LIBAVUTIL_VERSION_INT,    \
        .category   = AV_CLASS_CATEGORY_FILTER, \
    }


AVFILTER_DEFINE_CLASS(chronograph);

static const AVFilterPad chronograph_inputs[] = {
    {
        .name           = "default",
        .type           = AVMEDIA_TYPE_VIDEO,
        .config_props   = config_input,
        .filter_frame   = filter_frame,
        .needs_writable = 1,
    },
    { NULL }
};

static const AVFilterPad chronograph_outputs[] = {
    {
        .name = "default",
        .type = AVMEDIA_TYPE_VIDEO,
    },
    { NULL }
};

AVFilter ff_vf_chronograph = {
    .name          = "chronograph",
    .description   = "Overlap a chronograph on the input video.",
    .priv_size     = sizeof(ChronographContext),
    .priv_class    = &chronograph_class,
    .init          = init,
    .query_formats = query_formats,
    .inputs        = chronograph_inputs,
    .outputs       = chronograph_outputs,
    .flags         = AVFILTER_FLAG_SUPPORT_TIMELINE_GENERIC,
};
#endif /* CONFIG_CHRONOGRAPH_FILTER */

