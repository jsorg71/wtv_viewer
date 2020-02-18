
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <fx.h>

#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#include <xcb/render.h>

#include "parse.h"
#include "wtv_calls.h"

class GUIObject : public FXObject
{
    FXDECLARE(GUIObject)
public:
    GUIObject();
    GUIObject(int argc, char** argv, struct wtv_info* wtv);
    virtual ~GUIObject();
    int mainLoop();
    int checkWrite();
    int schedAudio();
public:
    struct wtv_info* m_wtv;
    FXApp* m_app;
    FXMainWindow* m_mw;
    FXImage* m_image;
    int m_width;
    int m_height;
    int m_cap_mstime;
public:
    long onConfigure(FXObject* obj, FXSelector sel, void* ptr);
    long onResizeTimeout(FXObject* obj, FXSelector sel, void* ptr);
    long onPaint(FXObject* obj, FXSelector sel, void* ptr);
    long onUpdate(FXObject* obj, FXSelector sel, void* ptr);
    long onEventRead(FXObject* obj, FXSelector sel, void* ptr);
    long onEventWrite(FXObject* obj, FXSelector sel, void* ptr);
    long onFrameTimeout(FXObject* obj, FXSelector sel, void* ptr);
    long onAudioTimeout(FXObject* obj, FXSelector sel, void* ptr);
    long onStatsTimeout(FXObject* obj, FXSelector sel, void* ptr);
    enum _ids
    {
        ID_MAINWINDOW = 0,
        ID_SOCKET,
        ID_FRAME,
        ID_AUDIO,
        ID_STATS,
        ID_LAST
    };
};

/*****************************************************************************/
GUIObject::GUIObject() : FXObject()
{
    m_wtv = NULL;
    m_app = NULL;
    m_mw = NULL;
    m_image = NULL;
    m_width = 0;
    m_height = 0;
    m_cap_mstime = 0;
}

/*****************************************************************************/
GUIObject::GUIObject(int argc, char** argv, struct wtv_info* wtv) : FXObject()
{
    FXInputHandle ih;

    m_wtv = wtv;
    m_app = new FXApp("wtv_viewer", "wtv_viewer");
    m_mw = new FXMainWindow(m_app, "wtv_viewer", NULL, NULL, DECOR_ALL,
                            0, 0, 640, 480);
    m_app->init(argc, argv);
    m_app->create();
    m_mw->show(PLACEMENT_SCREEN);
    m_mw->setTarget(this);
    m_mw->setSelector(GUIObject::ID_MAINWINDOW);
    m_image = NULL;
    m_width = 0;
    m_height = 0;
    ih = (FXInputHandle)(wtv->sck);
    m_app->addInput(ih, INPUT_READ, this, GUIObject::ID_SOCKET);
    m_app->addTimeout(this, GUIObject::ID_FRAME, 100, NULL);
    m_app->addTimeout(this, GUIObject::ID_STATS, 60000, NULL);
    m_cap_mstime = 0;
}

/*****************************************************************************/
GUIObject::~GUIObject()
{
    delete m_app;
    delete m_image;
}

/*****************************************************************************/
int
GUIObject::mainLoop()
{
    m_app->run();
    return 0;
}

/*****************************************************************************/
int GUIObject::checkWrite()
{
    FXInputHandle ih;

    ih = (FXInputHandle)(m_wtv->sck);
    if (m_wtv->out_s_head == NULL)
    {
        m_app->removeInput(ih, INPUT_WRITE);
    }
    else
    {
        m_app->addInput(ih, INPUT_WRITE, this, GUIObject::ID_SOCKET);
    }
    return 0;
}

/*****************************************************************************/
int GUIObject::schedAudio()
{
    //printf("GUIObject::schedAudio:\n");
    m_app->addTimeout(this, GUIObject::ID_AUDIO, 33, NULL);
    return 0;
}

/*****************************************************************************/
long
GUIObject::onConfigure(FXObject* obj, FXSelector sel, void* ptr)
{
    //printf("GUIObject::onConfigure:\n");
    m_app->addTimeout(this, GUIObject::ID_MAINWINDOW, 0, NULL);
    return 1;
}

/*****************************************************************************/
long
GUIObject::onResizeTimeout(FXObject* obj, FXSelector sel, void* ptr)
{
    int width;
    int height;

    //printf("GUIObject::onResizeTimeout:\n");
    width = m_mw->getWidth();
    height = m_mw->getHeight();
    if ((width != m_width) || (height != m_height))
    {
        printf("GUIObject::onResizeTimeout: resize to %dx%d\n", width, height);
        m_wtv->drawable_width = m_width = width;
        m_wtv->drawable_height = m_height = height;
        delete m_image;
        m_image = new FXImage(m_app, NULL, 0, m_width, m_height);
    }
    return 1;
}

/*****************************************************************************/
long
GUIObject::onPaint(FXObject* obj, FXSelector sel, void* ptr)
{
    printf("GUIObject::onPaint:\n");
    return 1;
}

/*****************************************************************************/
long
GUIObject::onUpdate(FXObject* obj, FXSelector sel, void* ptr)
{
    //printf("GUIObject::onUpdate:\n");
    return 1;
}

/*****************************************************************************/
long
GUIObject::onEventRead(FXObject* obj, FXSelector sel, void* ptr)
{
    FXInputHandle ih;

    //printf("GUIObject::onEventRead:\n");
    ih = (FXInputHandle)(m_wtv->sck);
    if (wtv_read(m_wtv) != 0)
    {
        printf("GUIObject::onEventRead: wtv_read failed\n");
        m_app->removeInput(ih, INPUT_READ);
        m_app->removeInput(ih, INPUT_WRITE);
        return 0;
    }
    checkWrite();
    return 1;
}

/*****************************************************************************/
long
GUIObject::onEventWrite(FXObject* obj, FXSelector sel, void* ptr)
{
    FXInputHandle ih;

    //printf("GUIObject::onEventWrite:\n");
    ih = (FXInputHandle)(m_wtv->sck);
    if (wtv_write(m_wtv) != 0)
    {
        printf("GUIObject::onEventWrite: wtv_write failed\n");
        m_app->removeInput(ih, INPUT_READ);
        m_app->removeInput(ih, INPUT_WRITE);
        return 0;
    }
    checkWrite();
    return 1;
}

#define FRAME_MSTIME 33

/*****************************************************************************/
long
GUIObject::onFrameTimeout(FXObject* obj, FXSelector sel, void* ptr)
{
    wtv_request_frame(m_wtv);
    m_app->addTimeout(this, GUIObject::ID_FRAME, FRAME_MSTIME, NULL);
    return 1;
}

/*****************************************************************************/
long GUIObject::onAudioTimeout(FXObject* obj, FXSelector sel, void* ptr)
{
    //printf("GUIObject::onAudioTimeout:\n");
    wtv_check_audio(m_wtv);
    return 1;
}

/*****************************************************************************/
long
GUIObject::onStatsTimeout(FXObject* obj, FXSelector sel, void* ptr)
{
    printf("GUIObject::onStatsTimeout:\n");
    wtv_print_stats(m_wtv);
    m_app->addTimeout(this, GUIObject::ID_STATS, 60000, NULL);
    return 1;
}

FXDEFMAP(GUIObject) GUIObjectMap[] =
{
    FXMAPFUNC(SEL_CONFIGURE, GUIObject::ID_MAINWINDOW, GUIObject::onConfigure),
    FXMAPFUNC(SEL_TIMEOUT, GUIObject::ID_MAINWINDOW, GUIObject::onResizeTimeout),
    FXMAPFUNC(SEL_PAINT, GUIObject::ID_MAINWINDOW, GUIObject::onPaint),
    FXMAPFUNC(SEL_UPDATE, GUIObject::ID_MAINWINDOW, GUIObject::onUpdate),
    FXMAPFUNC(SEL_IO_READ, GUIObject::ID_SOCKET, GUIObject::onEventRead),
    FXMAPFUNC(SEL_IO_WRITE, GUIObject::ID_SOCKET, GUIObject::onEventWrite),
    FXMAPFUNC(SEL_TIMEOUT, GUIObject::ID_FRAME, GUIObject::onFrameTimeout),
    FXMAPFUNC(SEL_TIMEOUT, GUIObject::ID_AUDIO, GUIObject::onAudioTimeout),
    FXMAPFUNC(SEL_TIMEOUT, GUIObject::ID_STATS, GUIObject::onStatsTimeout)
};

FXIMPLEMENT(GUIObject, FXObject, GUIObjectMap, ARRAYNUMBER(GUIObjectMap))

/*****************************************************************************/
static xcb_render_pictformat_t
find_format_for_visual(xcb_render_query_pict_formats_reply_t* formats,
                       xcb_visualid_t visual)
{
    xcb_render_pictscreen_iterator_t screens;
    xcb_render_pictdepth_iterator_t depths;
    xcb_render_pictvisual_iterator_t visuals;

    for (screens = xcb_render_query_pict_formats_screens_iterator(formats);
         screens.rem; xcb_render_pictscreen_next(&screens))
    {
        for (depths = xcb_render_pictscreen_depths_iterator(screens.data);
             depths.rem; xcb_render_pictdepth_next(&depths))
        {
            for (visuals = xcb_render_pictdepth_visuals_iterator(depths.data);
                 visuals.rem; xcb_render_pictvisual_next(&visuals))
            {
                if (visuals.data->visual == visual)
                {
                    xcb_render_pictformat_t format = visuals.data->format;
                    return format;
                }
            }
        }
    }
    return XCB_NONE;
}

/*****************************************************************************/
static int
gui_create(int argc, char** argv, struct wtv_info** wtv)
{
    GUIObject* go;
    xcb_connection_t* xcb;
    xcb_screen_t *screen;
    xcb_render_query_pict_formats_reply_t *formats;
    xcb_render_query_pict_formats_cookie_t cookie;

    *wtv = (struct wtv_info*)calloc(1, sizeof(struct wtv_info));
    (*wtv)->sck = wtv_connect_to_uds(NULL);
    go = new GUIObject(argc, argv, *wtv);
    (*wtv)->gui_obj = go;
    xcb = XGetXCBConnection((Display*)(go->m_app->getDisplay()));
    (*wtv)->xcb = xcb;
    (*wtv)->drawable = go->m_mw->id();
    cookie = xcb_render_query_pict_formats(xcb);
    formats = xcb_render_query_pict_formats_reply(xcb, cookie, NULL);
    screen = xcb_setup_roots_iterator(xcb_get_setup(xcb)).data;
    (*wtv)->pict_format_default =
            find_format_for_visual(formats, screen->root_visual);
    free(formats);
    wtv_start(*wtv);
    return 0;
}

/*****************************************************************************/
static int
gui_main_loop(struct wtv_info* wtv)
{
    GUIObject* go;

    go = (GUIObject*)(wtv->gui_obj);
    go->mainLoop();
    return 0;
}

/*****************************************************************************/
static int
gui_delete(struct wtv_info* wtv)
{
    GUIObject* go;

    go = (GUIObject*)(wtv->gui_obj);
    go->m_app->exit(); /* close display, write registry */
    delete go;
    if (wtv->sck != -1)
    {
        close(wtv->sck);
    }
    free(wtv);
    return 0;
}

/*****************************************************************************/
int
main(int argc, char** argv)
{
    struct wtv_info* wtv;

    wtv = NULL;
    gui_create(argc, argv, &wtv);
    gui_main_loop(wtv);
    gui_delete(wtv);
    return 0;
}

/*****************************************************************************/
int
wtv_check_write(struct wtv_info* wtv)
{
    GUIObject* go;

    go = (GUIObject*)(wtv->gui_obj);
    go->checkWrite();
    return 0;
}

/*****************************************************************************/
int
wtv_sched_audio(struct wtv_info* wtv)
{
    GUIObject* go;

    go = (GUIObject*)(wtv->gui_obj);
    go->schedAudio();
    return 0;
}

