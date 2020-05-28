
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/dri3.h>
#include <xcb/render.h>

#include "wtv_calls.h"
#include "wtv_xcb.h"

#define ToFixed(f) ((int) ((f) * 65536))

/*****************************************************************************/
int
wtv_fd_to_drawable(struct wtv_info* wtv, int fd, int fd_width, int fd_height,
                   int fd_stride, int fd_size, int fd_bpp)
{
    xcb_pixmap_t pixmap;
    xcb_connection_t* xcb;
    xcb_void_cookie_t cookie;
    xcb_generic_error_t* xcb_error;
    xcb_render_picture_t src_picture;
    xcb_render_picture_t dst_picture;
    xcb_render_transform_t trans;
    xcb_rectangle_t rectangles[2];
    float xscale;
    float yscale;
    int dst_width;
    int dst_height;
    int ratio;
    int dst_x;
    int dst_y;

    (void)fd_bpp;

    LOGLN10((wtv, LOG_INFO, LOGS "drawable_id %d", LOGP, wtv->drawable_id));
    if (wtv->drawable_id == 0)
    {
        return 1;
    }
    xcb = (xcb_connection_t*)(wtv->xcb);
    pixmap = xcb_generate_id(xcb);
    cookie = xcb_dri3_pixmap_from_buffer(xcb, pixmap, wtv->drawable_id,
                                         fd_size, fd_width, fd_height,
                                         fd_stride, 24, 32, fd);
    xcb_error = xcb_request_check(xcb, cookie);
    free(xcb_error);
    if ((fd_width == wtv->drawable_width) &&
        (fd_height == wtv->drawable_height))
    {
        /* fast path, do not need to resize */
        LOGLN10((wtv, LOG_INFO, LOGS "no resize", LOGP));
        xcb_copy_area(xcb, pixmap, wtv->drawable_id, wtv->gc, 0, 0, 0, 0,
                      fd_width, fd_height);
        xcb_free_pixmap(xcb, pixmap);
        return 0;
    }
    /* need to resize */
    src_picture = xcb_generate_id(xcb);
    xcb_render_create_picture(xcb, src_picture, pixmap,
                              wtv->pict_format_default, 0, NULL);
    ratio = (fd_width << 16) / fd_height;
    dst_height = wtv->drawable_height;
    dst_width = (dst_height * ratio + 0x8000) >> 16;
    if (dst_width > wtv->drawable_width)
    {
        ratio = (fd_height << 16) / fd_width;
        dst_width = wtv->drawable_width;
        dst_height = (dst_width * ratio + 0x8000) >> 16;
    }
    dst_x = 0;
    if (dst_width < wtv->drawable_width)
    {
        dst_x += (wtv->drawable_width - dst_width) / 2;
        /* fill any extra on left and right */
        memset(rectangles, 0, sizeof(rectangles));
        rectangles[0].width = dst_x;
        rectangles[0].height = wtv->drawable_height;
        rectangles[1].x = dst_x + dst_width;
        rectangles[1].width = dst_x;
        rectangles[1].height = wtv->drawable_height;
        xcb_poly_fill_rectangle(xcb, wtv->drawable_id, wtv->gc, 2, rectangles);
    }
    dst_y = 0;
    if (dst_height < wtv->drawable_height)
    {
        dst_y += (wtv->drawable_height - dst_height) / 2;
        /* fill any extra on top and bottom */
        memset(rectangles, 0, sizeof(rectangles));
        rectangles[0].width = wtv->drawable_width;
        rectangles[0].height = dst_y;
        rectangles[1].y = dst_y + dst_height;
        rectangles[1].width = wtv->drawable_width;
        rectangles[1].height = dst_y;
        xcb_poly_fill_rectangle(xcb, wtv->drawable_id, wtv->gc, 2, rectangles);
    }
    memset(&trans, 0, sizeof(trans));
    xscale = fd_width;
    xscale /= dst_width;
    yscale = fd_height;
    yscale /= dst_height;
    trans.matrix11 = ToFixed(xscale);
    trans.matrix22 = ToFixed(yscale);
    trans.matrix33 = ToFixed(1);
    xcb_render_set_picture_transform(xcb, src_picture, trans);
    dst_picture = xcb_generate_id(xcb);
    xcb_render_create_picture(xcb, dst_picture, wtv->drawable_id,
                              wtv->pict_format_default, 0, NULL);
    xcb_render_composite(xcb, XCB_RENDER_PICT_OP_SRC,
                         src_picture, XCB_RENDER_PICTURE_NONE, dst_picture,
                         0, 0, 0, 0, dst_x, dst_y, dst_width, dst_height);
    xcb_render_free_picture(xcb, src_picture);
    xcb_render_free_picture(xcb, dst_picture);
    xcb_free_pixmap(xcb, pixmap);
    return 0;
}
