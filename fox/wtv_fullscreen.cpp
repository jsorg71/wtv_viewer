
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fx.h>
#include <fxkeys.h>

#include "wtv_calls.h"
#include "wtv_fullscreen.h"

/*****************************************************************************/
FXFullScreenWindow::FXFullScreenWindow()
{
    m_wtv = NULL;
    m_app = NULL;
    m_image = NULL;
    m_image_width = 0;
    m_image_height = 0;
}

/*****************************************************************************/
FXFullScreenWindow::FXFullScreenWindow(FXWindow* p):
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
FXFullScreenWindow::~FXFullScreenWindow()
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
FXFullScreenWindow::doesOverrideRedirect() const
{
    LOGLN10((m_wtv, LOG_INFO, LOGS, LOGP));
    return true;
}

/*****************************************************************************/
long
FXFullScreenWindow::onConfigure(FXObject* obj, FXSelector sel, void* ptr)
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
FXFullScreenWindow::onPaint(FXObject* obj, FXSelector sel, void* ptr)
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
FXFullScreenWindow::onLeftBtnPress(FXObject* obj, FXSelector sel, void* ptr)
{
    (void)obj;
    (void)sel;
    (void)ptr;

    LOGLN10((m_wtv, LOG_INFO, LOGS, LOGP));
    return 1;
}

/*****************************************************************************/
long
FXFullScreenWindow::onLeftBtnRelease(FXObject* obj, FXSelector sel, void* ptr)
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

/*****************************************************************************/
long
FXFullScreenWindow::onKeyPress(FXObject* obj, FXSelector sel, void* ptr)
{
    (void)obj;
    (void)sel;
    (void)ptr;

    LOGLN10((m_wtv, LOG_INFO, LOGS, LOGP));
    return 1;
}

/*****************************************************************************/
long
FXFullScreenWindow::onKeyRelease(FXObject* obj, FXSelector sel, void* ptr)
{
    FXEvent* evt;

    (void)obj;
    (void)sel;

    LOGLN10((m_wtv, LOG_INFO, LOGS, LOGP));
    evt = (FXEvent*)ptr;
    if ((evt->code == KEY_Escape) || (evt->code == KEY_f) ||
        (evt->code == KEY_F))
    {
        close(TRUE);
    }
    return 1;
}

FXDEFMAP(FXFullScreenWindow) FXFullScreenWindowMap[] =
{
    FXMAPFUNC(SEL_CONFIGURE, 0, FXFullScreenWindow::onConfigure),
    FXMAPFUNC(SEL_PAINT, 0, FXFullScreenWindow::onPaint),
    FXMAPFUNC(SEL_LEFTBUTTONPRESS, 0, FXFullScreenWindow::onLeftBtnPress),
    FXMAPFUNC(SEL_LEFTBUTTONRELEASE, 0, FXFullScreenWindow::onLeftBtnRelease),
    FXMAPFUNC(SEL_KEYPRESS, 0, FXFullScreenWindow::onKeyPress),
    FXMAPFUNC(SEL_KEYRELEASE, 0, FXFullScreenWindow::onKeyRelease)
};

FXIMPLEMENT(FXFullScreenWindow, FXTopWindow, FXFullScreenWindowMap,
            ARRAYNUMBER(FXFullScreenWindowMap))

