
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
    float xscale;
    float yscale;

    (void)fd_bpp;

    xcb = (xcb_connection_t*)(wtv->xcb);
    pixmap = xcb_generate_id(xcb);
    cookie = xcb_dri3_pixmap_from_buffer(xcb, pixmap, wtv->drawable,
                                         fd_size, fd_width, fd_height,
                                         fd_stride, 24, 32, fd);
    xcb_error = xcb_request_check(xcb, cookie);
    free(xcb_error);
    src_picture = xcb_generate_id(xcb);
    xcb_render_create_picture(xcb, src_picture, pixmap,
                              wtv->pict_format_default, 0, NULL);
    memset(&trans, 0, sizeof(trans));
    xscale = fd_width;
    xscale /= wtv->drawable_width;
    yscale = fd_height;
    yscale /= wtv->drawable_height;
    trans.matrix11 = ToFixed(xscale);
    trans.matrix22 = ToFixed(yscale);
    trans.matrix33 = ToFixed(1);
    xcb_render_set_picture_transform(xcb, src_picture, trans);
    dst_picture = xcb_generate_id(xcb);
    xcb_render_create_picture(xcb, dst_picture, wtv->drawable,
                              wtv->pict_format_default, 0, NULL);
    xcb_render_composite(xcb, XCB_RENDER_PICT_OP_SRC,
                         src_picture, XCB_RENDER_PICTURE_NONE, dst_picture,
                         0, 0, 0, 0, 0, 0,
                         wtv->drawable_width, wtv->drawable_height);
    xcb_render_free_picture(xcb, src_picture);
    xcb_render_free_picture(xcb, dst_picture);
    xcb_free_pixmap(xcb, pixmap);
    return 0;
}
