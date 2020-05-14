
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
#include "wtv_picker.h"

#define FRAME_MSTIME 33

class FXMyWindow:public FXTopWindow
{
    FXDECLARE(FXMyWindow)
public:
    FXMyWindow();
    FXMyWindow(FXWindow* p);
    virtual ~FXMyWindow();
    virtual bool doesOverrideRedirect() const;
public:
    struct wtv_info* m_wtv;
    FXApp* m_app;
    FXImage* m_image;
    int m_image_width;
    int m_image_height;
public:
    long onConfigure(FXObject* obj, FXSelector sel, void* ptr);
    long onPaint(FXObject* obj, FXSelector sel, void* ptr);
    long onLeftBtnPress(FXObject* obj, FXSelector sel, void* ptr);
    long onLeftBtnRelease(FXObject* obj, FXSelector sel, void* ptr);
};

/*****************************************************************************/
FXMyWindow::FXMyWindow()
{
    m_wtv = NULL;
    m_app = NULL;
    m_image = NULL;
    m_image_width = 0;
    m_image_height = 0;
}

/*****************************************************************************/
FXMyWindow::FXMyWindow(FXWindow* p):
            FXTopWindow(p, "full", NULL, NULL, 0, 10, 10, 10, 10,
                        0, 0, 0, 0, 0, 0)
{
    m_wtv = NULL;
    m_app = NULL;
    m_image = NULL;
    m_image_width = 0;
    m_image_height = 0;
    flags |= FLAG_ENABLED;
}

/*****************************************************************************/
FXMyWindow::~FXMyWindow()
{
    LOGLN10((m_wtv, LOG_INFO, LOGS, LOGP));
    if (m_image->id() == m_wtv->drawable_id)
    {
        m_wtv->drawable_id = 0;
    }
    delete m_image;
}

/*****************************************************************************/
bool
FXMyWindow::doesOverrideRedirect() const
{
    LOGLN10((m_wtv, LOG_INFO, LOGS, LOGP));
    return true;
}

/*****************************************************************************/
long
FXMyWindow::onConfigure(FXObject* obj, FXSelector sel, void* ptr)
{
    int width;
    int height;
    FXDCWindow* dc;

    LOGLN10((m_wtv, LOG_INFO, LOGS, LOGP));
    FXTopWindow::onConfigure(obj, sel, ptr);
    width = getWidth();
    height = getHeight();
    if ((width != m_image_width) || (height != m_image_height))
    {
        m_image_width = width;
        m_image_height = height;
        delete m_image;
        m_image = new FXImage(m_app, NULL, 0, m_image_width, m_image_height);
        m_image->create();
        dc = new FXDCWindow(m_image);
        dc->fillRectangle(0, 0, m_image_width, m_image_height);
        delete dc;
        if (m_wtv != NULL)
        {
            m_wtv->drawable_id = m_image->id();
            m_wtv->drawable_width = m_image_width;
            m_wtv->drawable_height = m_image_height;
        }
    }
    return 1;
}

/*****************************************************************************/
long
FXMyWindow::onPaint(FXObject* obj, FXSelector sel, void* ptr)
{
    FXEvent* evt;
    FXRegion* reg;
    FXDCWindow* dc;

    (void)obj;
    (void)sel;

    LOGLN10((m_wtv, LOG_INFO, LOGS, LOGP));
    evt = (FXEvent*)ptr;
    reg = new FXRegion(evt->rect.x, evt->rect.y, evt->rect.w, evt->rect.h);
    if (!reg->empty())
    {
        dc = new FXDCWindow(this);
        if (m_image != NULL)
        {
            dc->setClipRegion(*reg);
            dc->drawImage(m_image, 0, 0);
        }
        delete dc;
    }
    delete reg;
    return 1;
}

/*****************************************************************************/
long
FXMyWindow::onLeftBtnPress(FXObject* obj, FXSelector sel, void* ptr)
{
    (void)obj;
    (void)sel;
    (void)ptr;

    LOGLN10((m_wtv, LOG_INFO, LOGS, LOGP));
    return 1;
}

/*****************************************************************************/
long
FXMyWindow::onLeftBtnRelease(FXObject* obj, FXSelector sel, void* ptr)
{
    FXEvent* evt;

    (void)obj;
    (void)sel;

    evt = (FXEvent*)ptr;
    LOGLN10((m_wtv, LOG_INFO, LOGS "click_count %d", LOGP, evt->click_count));
    if (evt->click_count > 1)
    {
        close(TRUE);
    }
    return 1;
}

FXDEFMAP(FXMyWindow) FXMyWindowMap[] =
{
    FXMAPFUNC(SEL_CONFIGURE, 0, FXMyWindow::onConfigure),
    FXMAPFUNC(SEL_PAINT, 0, FXMyWindow::onPaint),
    FXMAPFUNC(SEL_LEFTBUTTONPRESS, 0, FXMyWindow::onLeftBtnPress),
    FXMAPFUNC(SEL_LEFTBUTTONRELEASE, 0, FXMyWindow::onLeftBtnRelease)
};

FXIMPLEMENT(FXMyWindow, FXWindow, FXMyWindowMap, ARRAYNUMBER(FXMyWindowMap))

class FXMyMainWindow:public FXMainWindow
{
    FXDECLARE(FXMyMainWindow)
public:
    FXMyMainWindow();
    FXMyMainWindow(FXApp* app);
    virtual ~FXMyMainWindow();
public:
    struct wtv_info* m_wtv;
    FXApp* m_app;
    FXImage* m_image;
    int m_image_width;
    int m_image_height;
    int m_left_offset;
    int m_top_offset;
    int m_right_offset;
    int m_bottom_offset;
public:
    long onConfigure(FXObject* obj, FXSelector sel, void* ptr);
    long onPaint(FXObject* obj, FXSelector sel, void* ptr);
    long onLeftBtnPress(FXObject* obj, FXSelector sel, void* ptr);
    long onLeftBtnRelease(FXObject* obj, FXSelector sel, void* ptr);
};

/*****************************************************************************/
FXMyMainWindow::FXMyMainWindow()
{
    m_wtv = NULL;
    m_app = NULL;
    m_image = NULL;
    m_image_width = 0;
    m_image_height = 0;
    m_left_offset = 0;
    m_top_offset = 0;
    m_right_offset = 0;
    m_bottom_offset = 0;
}

/*****************************************************************************/
FXMyMainWindow::FXMyMainWindow(FXApp* app):
                FXMainWindow(app, "wtv_viewer", NULL, NULL, DECOR_ALL,
                             0, 0, 640, 480)
{
    m_wtv = NULL;
    m_app = app;
    m_image = NULL;
    m_image_width = 0;
    m_image_height = 0;
    m_left_offset = 0;
    m_top_offset = 0;
    m_right_offset = 0;
    m_bottom_offset = 0;
    flags |= FLAG_ENABLED;
}

/*****************************************************************************/
FXMyMainWindow::~FXMyMainWindow()
{
    delete m_image;
}

/*****************************************************************************/
long
FXMyMainWindow::onConfigure(FXObject* obj, FXSelector sel, void* ptr)
{
    int width;
    int height;
    FXDCWindow* dc;

    FXMainWindow::onConfigure(obj, sel, ptr);
    width = getWidth() - m_left_offset - m_right_offset;
    height = getHeight() - m_top_offset - m_bottom_offset;
    if ((width != m_image_width) || (height != m_image_height))
    {
        m_image_width = width;
        m_image_height = height;
        delete m_image;
        m_image = new FXImage(m_app, NULL, 0, m_image_width, m_image_height);
        m_image->create();
        dc = new FXDCWindow(m_image);
        dc->fillRectangle(0, 0, m_image_width, m_image_height);
        delete dc;
        if (m_wtv != NULL)
        {
            m_wtv->drawable_id = m_image->id();
            m_wtv->drawable_width = m_image_width;
            m_wtv->drawable_height = m_image_height;
        }
    }
    return 1;
}

/*****************************************************************************/
long
FXMyMainWindow::onPaint(FXObject* obj, FXSelector sel, void* ptr)
{
    FXEvent* evt;
    FXRegion* reg;
    FXRegion* image_reg;
    FXDCWindow* dc;
    int width;
    int height;

    (void)obj;
    (void)sel;

    LOGLN10((m_wtv, LOG_INFO, LOGS, LOGP));
    evt = (FXEvent*)ptr;
    reg = new FXRegion(evt->rect.x, evt->rect.y, evt->rect.w, evt->rect.h);
    if (!reg->empty())
    {
        width = getWidth();
        height = getHeight();
        dc = new FXDCWindow(this);
        if (m_image != NULL)
        {
            dc->setClipRegion(*reg);
            image_reg = new FXRegion(m_left_offset, m_top_offset,
                                     width - m_left_offset - m_right_offset,
                                     height - m_top_offset - m_bottom_offset);
            *reg -= *image_reg;
            dc->drawImage(m_image, m_left_offset, m_top_offset);
            delete image_reg;
        }
        if (!reg->empty())
        {
            dc->setClipRegion(*reg);
            dc->setForeground(backColor);
            dc->fillRectangle(0, 0, width, height);
        }
        delete dc;
    }
    delete reg;
    return 1;
}

/*****************************************************************************/
long
FXMyMainWindow::onLeftBtnPress(FXObject* obj, FXSelector sel, void* ptr)
{
    (void)obj;
    (void)sel;
    (void)ptr;

    LOGLN10((m_wtv, LOG_INFO, LOGS, LOGP));
    return 1;
}

/*****************************************************************************/
long
FXMyMainWindow::onLeftBtnRelease(FXObject* obj, FXSelector sel, void* ptr)
{
    FXEvent* evt;

    (void)obj;
    (void)sel;

    evt = (FXEvent*)ptr;
    LOGLN10((m_wtv, LOG_INFO, LOGS "click_count %d", LOGP, evt->click_count));
    if (evt->click_count > 1)
    {
        if (target != NULL)
        {
            target->tryHandle(this, FXSEL(SEL_COMMAND, message), NULL);
        }
    }
    return 1;
}

FXDEFMAP(FXMyMainWindow) FXMyMainWindowMap[] =
{
    FXMAPFUNC(SEL_CONFIGURE, 0, FXMyMainWindow::onConfigure),
    FXMAPFUNC(SEL_PAINT, 0, FXMyMainWindow::onPaint),
    FXMAPFUNC(SEL_LEFTBUTTONPRESS, 0, FXMyMainWindow::onLeftBtnPress),
    FXMAPFUNC(SEL_LEFTBUTTONRELEASE, 0, FXMyMainWindow::onLeftBtnRelease)
};

FXIMPLEMENT(FXMyMainWindow, FXMainWindow, FXMyMainWindowMap,
            ARRAYNUMBER(FXMyMainWindowMap))

class GUIObject:public FXObject
{
    FXDECLARE(GUIObject)
public:
    GUIObject();
    GUIObject(int argc, char** argv, struct wtv_info* wtv);
    virtual ~GUIObject();
    int mainLoop();
    int schedWrite();
    int schedAudio();
    int drawDrawable();
    int doOpenDialog();
public:
    struct wtv_info* m_wtv;
    FXApp* m_app;
    FXMyMainWindow* m_mw;
    FXDockSite* m_topdock;
    FXToolBarShell* m_tbs;
    FXMenuBar* m_mb;
    FXMenuPane* m_filemenu;
    FXMenuPane* m_helpmenu;
    FXStatusBar* m_sb;
    FXLabel* m_sbl1;
    xcb_connection_t* xcb;
    FXMyWindow* m_fullscreen;
public:
    long onEventRead(FXObject* obj, FXSelector sel, void* ptr);
    long onEventWrite(FXObject* obj, FXSelector sel, void* ptr);
    long onFrameTimeout(FXObject* obj, FXSelector sel, void* ptr);
    long onAudioTimeout(FXObject* obj, FXSelector sel, void* ptr);
    long onStatsTimeout(FXObject* obj, FXSelector sel, void* ptr);
    long onStartupTimeout(FXObject* obj, FXSelector sel, void* ptr);
    long onCmdOpen(FXObject* obj, FXSelector sel, void* ptr);
    long onCmdFullscreenToggle(FXObject* obj, FXSelector sel, void* ptr);
    long onFullscreenClose(FXObject* obj, FXSelector sel, void* ptr);
    enum _ids
    {
        ID_MAINWINDOW = 1,
        ID_SOCKET,
        ID_FRAME,
        ID_AUDIO,
        ID_STATS,
        ID_STARTUP,
        ID_OPEN,
        ID_FULLSCREEN_TOGGLE,
        ID_FULLSCREEN_CLOSE,
        ID_EXIT,
        ID_HELP,
        ID_ABOUT,
        ID_LAST
    };
};

/*****************************************************************************/
GUIObject::GUIObject():FXObject()
{
    m_wtv = NULL;
    m_app = NULL;
    m_mw = NULL;
    m_fullscreen = NULL;
}

/*****************************************************************************/
GUIObject::GUIObject(int argc, char** argv, struct wtv_info* wtv):FXObject()
{
    FXuint flags;
    FXSelector sel;
    FXCursor* cur;

    m_wtv = wtv;
    m_app = new FXApp("wtv_viewer", "wtv_viewer");
    cur = new FXCursor(m_app, FX::CURSOR_ARROW);
    m_app->setDefaultCursor(DEF_RARROW_CURSOR, cur);
    m_app->init(argc, argv);
    m_mw = new FXMyMainWindow(m_app);
    m_mw->setTarget(this);
    m_mw->setSelector(GUIObject::ID_MAINWINDOW);
    m_mw->m_wtv = wtv;
    /* menu */
    flags = LAYOUT_SIDE_TOP | LAYOUT_FILL_X;
    m_topdock = new FXDockSite(m_mw, flags);
    flags = FRAME_RAISED;
    m_tbs = new FXToolBarShell(m_mw, flags);
    flags = LAYOUT_DOCK_NEXT | LAYOUT_SIDE_TOP | LAYOUT_FILL_X | FRAME_RAISED;
    m_mb = new FXMenuBar(m_topdock, m_tbs, flags);
    m_filemenu = new FXMenuPane(m_mw);
    new FXMenuTitle(m_mb, "&File", NULL, m_filemenu);
    sel = GUIObject::ID_OPEN;
    new FXMenuCommand(m_filemenu, "&Open\t\tOpen a new source.", NULL,
                      this, sel);
    sel = GUIObject::ID_FULLSCREEN_TOGGLE;
    new FXMenuCommand(m_filemenu, "&Fullscreen\t\tToggle full screen.", NULL,
                      this, sel);
    sel = GUIObject::ID_EXIT;
    new FXMenuCommand(m_filemenu, "&Exit\t\tExit the application.", NULL,
                      this, sel);
    m_helpmenu = new FXMenuPane(m_mw);
    new FXMenuTitle(m_mb, "&Help", NULL, m_helpmenu);
    sel = GUIObject::ID_HELP;
    new FXMenuCommand(m_helpmenu, "&Help...\t\tDisplay help information.",
                      NULL, this, sel);
    sel = GUIObject::ID_ABOUT;
    new FXMenuCommand(m_helpmenu, "&About\t\tDisplay version information.",
                      NULL, this, sel);
    /* status bar */
    flags = LAYOUT_SIDE_BOTTOM | LAYOUT_FILL_X | STATUSBAR_WITH_DRAGCORNER |
            FRAME_RAISED;
    m_sb = new FXStatusBar(m_mw, flags);
    m_sbl1 = new FXLabel(m_sb, "", NULL, LAYOUT_RIGHT | LAYOUT_CENTER_Y);
    m_app->create();
    m_mw->show(PLACEMENT_SCREEN);
    /* set the video image offsets */
    m_mw->m_left_offset = 4;
    m_mw->m_top_offset = m_mb->getHeight() + 4;
    m_mw->m_right_offset = 4;
    m_mw->m_bottom_offset = m_sb->getHeight() + 4;
    m_app->addTimeout(this, GUIObject::ID_STARTUP, 100, NULL);
    m_fullscreen = NULL;
}

/*****************************************************************************/
GUIObject::~GUIObject()
{
    delete m_app;
}

/*****************************************************************************/
int
GUIObject::mainLoop()
{
    m_app->run();
    return 0;
}

/*****************************************************************************/
int
GUIObject::schedWrite()
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
int
GUIObject::schedAudio()
{
    LOGLN10((m_wtv, LOG_INFO, LOGS, LOGP));
    m_app->addTimeout(this, GUIObject::ID_AUDIO, 16, NULL);
    return 0;
}

/*****************************************************************************/
int
GUIObject::drawDrawable()
{
    LOGLN10((m_wtv, LOG_INFO, LOGS "m_fullscreen %p", LOGP, m_fullscreen));
    if (m_fullscreen != NULL)
    {
        m_fullscreen->update(0, 0, m_wtv->drawable_width,
                             m_wtv->drawable_height);
        return 0;
    }
    m_mw->update(m_mw->m_left_offset, m_mw->m_top_offset,
                 m_wtv->drawable_width, m_wtv->drawable_height);
    return 0;
}

/*****************************************************************************/
long
GUIObject::onEventRead(FXObject* obj, FXSelector sel, void* ptr)
{
    FXInputHandle ih;

    (void)obj;
    (void)sel;
    (void)ptr;

    LOGLN10((m_wtv, LOG_INFO, LOGS, LOGP));
    ih = (FXInputHandle)(m_wtv->sck);
    if (wtv_read(m_wtv) != 0)
    {
        LOGLN0((m_wtv, LOG_ERROR, LOGS "wtv_read failed", LOGP));
        m_app->removeInput(ih, INPUT_READ);
        m_app->removeInput(ih, INPUT_WRITE);
        return 0;
    }
    schedWrite();
    return 1;
}

/*****************************************************************************/
long
GUIObject::onEventWrite(FXObject* obj, FXSelector sel, void* ptr)
{
    FXInputHandle ih;

    (void)obj;
    (void)sel;
    (void)ptr;

    LOGLN10((m_wtv, LOG_INFO, LOGS, LOGP));
    ih = (FXInputHandle)(m_wtv->sck);
    if (wtv_write(m_wtv) != 0)
    {
        LOGLN0((m_wtv, LOG_ERROR, LOGS "wtv_write failed", LOGP));
        m_app->removeInput(ih, INPUT_READ);
        m_app->removeInput(ih, INPUT_WRITE);
        return 0;
    }
    schedWrite();
    return 1;
}

/*****************************************************************************/
long
GUIObject::onFrameTimeout(FXObject* obj, FXSelector sel, void* ptr)
{
    (void)obj;
    (void)sel;
    (void)ptr;

    wtv_request_frame(m_wtv);
    m_app->addTimeout(this, GUIObject::ID_FRAME, FRAME_MSTIME, NULL);
    return 1;
}

/*****************************************************************************/
long
GUIObject::onAudioTimeout(FXObject* obj, FXSelector sel, void* ptr)
{
    (void)obj;
    (void)sel;
    (void)ptr;

    LOGLN10((m_wtv, LOG_INFO, LOGS, LOGP));
    wtv_check_audio(m_wtv);
    return 1;
}

/*****************************************************************************/
long
GUIObject::onStatsTimeout(FXObject* obj, FXSelector sel, void* ptr)
{
    (void)obj;
    (void)sel;
    (void)ptr;

    LOGLN0((m_wtv, LOG_INFO, LOGS, LOGP));
    wtv_print_stats(m_wtv);
    m_app->addTimeout(this, GUIObject::ID_STATS, 60000, NULL);
    return 1;
}

/*****************************************************************************/
int
GUIObject::doOpenDialog()
{
    FXInputHandle ih;
    FXbool ok;
    FXListItem* item;
    FXString str;
    PickerDialog* picker;
    int index;
    int count;

    LOGLN0((m_wtv, LOG_INFO, LOGS, LOGP));
    picker = new PickerDialog(m_app, m_mw, m_wtv);
    ok = picker->execute(PLACEMENT_OWNER);
    if (!ok)
    {
        return 0;
    }
    count = picker->m_list->getNumItems();
    for (index = 0; index < count; index++)
    {
        if (picker->m_list->isItemSelected(index))
        {
            item = picker->m_list->getItem(index);
            str = item->getText();
            break;
        }
    }
    delete picker;
    if (index == count)
    {
        return 0;
    }
    if (m_wtv->sck > 0)
    {
        ih = (FXInputHandle)(m_wtv->sck);
        m_app->removeInput(ih, INPUT_READ);
        m_app->removeInput(ih, INPUT_WRITE);
        close(m_wtv->sck);
        m_wtv->sck = 0;
    }
    m_app->removeTimeout(this, GUIObject::ID_FRAME);
    m_app->removeTimeout(this, GUIObject::ID_STATS);
    m_app->removeTimeout(this, GUIObject::ID_AUDIO);
    wtv_stop(m_wtv);
    m_wtv->sck = wtv_connect_to_uds(m_wtv, str.text());
    if (m_wtv->sck == -1)
    {
        return 0;
    }
    m_mw->setTitle(str + " - wtv_viewer");
    wtv_start(m_wtv);
    ih = (FXInputHandle)(m_wtv->sck);
    m_app->addInput(ih, INPUT_READ, this, GUIObject::ID_SOCKET);
    m_app->addTimeout(this, GUIObject::ID_FRAME, 100, NULL);
    m_app->addTimeout(this, GUIObject::ID_STATS, 60000, NULL);
    return 0;
}

/*****************************************************************************/
long
GUIObject::onStartupTimeout(FXObject* obj, FXSelector sel, void* ptr)
{
    (void)obj;
    (void)sel;
    (void)ptr;

    doOpenDialog();
    return 1;
}

/*****************************************************************************/
long
GUIObject::onCmdOpen(FXObject* obj, FXSelector sel, void* ptr)
{
    (void)obj;
    (void)sel;
    (void)ptr;

    doOpenDialog();
    return 1;
}

/*****************************************************************************/
long
GUIObject::onCmdFullscreenToggle(FXObject* obj, FXSelector sel, void* ptr)
{
    FXRootWindow* rootWindow;

    (void)obj;
    (void)sel;
    (void)ptr;

    LOGLN10((m_wtv, LOG_INFO, LOGS, LOGP));
    if (m_fullscreen != NULL)
    {
        m_fullscreen->close(TRUE);
        m_fullscreen = NULL;
        m_wtv->drawable_id = m_mw->m_image->id();
        m_wtv->drawable_width = m_mw->m_image_width;
        m_wtv->drawable_height = m_mw->m_image_height;
        return 1;
    }
    m_fullscreen = new FXMyWindow(m_mw);
    m_fullscreen->setTarget(this);
    m_fullscreen->setSelector(GUIObject::ID_FULLSCREEN_CLOSE);
    m_fullscreen->m_app = m_app;
    m_fullscreen->m_wtv = m_wtv;
    m_fullscreen->create();
    m_fullscreen->show();
    rootWindow = m_app->getRootWindow();
    m_fullscreen->position(0, 0, rootWindow->getWidth(),
                           rootWindow->getHeight());
    return 1;
}

/*****************************************************************************/
long
GUIObject::onFullscreenClose(FXObject* obj, FXSelector sel, void* ptr)
{
    (void)obj;
    (void)sel;
    (void)ptr;

    LOGLN10((m_wtv, LOG_INFO, LOGS, LOGP));
    if (m_fullscreen != NULL)
    {
        m_fullscreen = NULL;
        m_wtv->drawable_id = m_mw->m_image->id();
        m_wtv->drawable_width = m_mw->m_image_width;
        m_wtv->drawable_height = m_mw->m_image_height;
    }
    return 0; /* allow close */
}

FXDEFMAP(GUIObject) GUIObjectMap[] =
{
    FXMAPFUNC(SEL_COMMAND, GUIObject::ID_OPEN, GUIObject::onCmdOpen),
    FXMAPFUNC(SEL_COMMAND, GUIObject::ID_FULLSCREEN_TOGGLE,
                                            GUIObject::onCmdFullscreenToggle),
    FXMAPFUNC(SEL_COMMAND, GUIObject::ID_MAINWINDOW,
                                            GUIObject::onCmdFullscreenToggle),
    FXMAPFUNC(SEL_CLOSE, GUIObject::ID_FULLSCREEN_CLOSE,
                                            GUIObject::onFullscreenClose),
    FXMAPFUNC(SEL_IO_READ, GUIObject::ID_SOCKET, GUIObject::onEventRead),
    FXMAPFUNC(SEL_IO_WRITE, GUIObject::ID_SOCKET, GUIObject::onEventWrite),
    FXMAPFUNC(SEL_TIMEOUT, GUIObject::ID_FRAME, GUIObject::onFrameTimeout),
    FXMAPFUNC(SEL_TIMEOUT, GUIObject::ID_AUDIO, GUIObject::onAudioTimeout),
    FXMAPFUNC(SEL_TIMEOUT, GUIObject::ID_STATS, GUIObject::onStatsTimeout),
    FXMAPFUNC(SEL_TIMEOUT, GUIObject::ID_STARTUP, GUIObject::onStartupTimeout)
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
    xcb_render_pictformat_t format;

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
                    format = visuals.data->format;
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
    go = new GUIObject(argc, argv, *wtv);
    (*wtv)->gui_obj = go;
    xcb = XGetXCBConnection((Display*)(go->m_app->getDisplay()));
    go->xcb = xcb;
    (*wtv)->xcb = xcb;
    cookie = xcb_render_query_pict_formats(xcb);
    formats = xcb_render_query_pict_formats_reply(xcb, cookie, NULL);
    screen = xcb_setup_roots_iterator(xcb_get_setup(xcb)).data;
    (*wtv)->pict_format_default =
            find_format_for_visual(formats, screen->root_visual);
    free(formats);
    (*wtv)->gc = xcb_generate_id(xcb);
    xcb_create_gc(xcb, (*wtv)->gc, go->m_mw->id(), 0, NULL);
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
    xcb_free_gc(go->xcb, wtv->gc);
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
wtv_gui_sched_write(struct wtv_info* wtv)
{
    GUIObject* go;

    go = (GUIObject*)(wtv->gui_obj);
    go->schedWrite();
    return 0;
}

/*****************************************************************************/
int
wtv_gui_sched_audio(struct wtv_info* wtv)
{
    GUIObject* go;

    go = (GUIObject*)(wtv->gui_obj);
    go->schedAudio();
    return 0;
}

/*****************************************************************************/
int
wtv_gui_writeln(struct wtv_info* wtv, const char* msg)
{
    (void)wtv;

    printf("%s\n", msg);
    return 0;
}

/*****************************************************************************/
int
wtv_gui_draw_drawable(struct wtv_info* wtv)
{
    GUIObject* go;

    go = (GUIObject*)(wtv->gui_obj);
    go->drawDrawable();
    return 0;
}

