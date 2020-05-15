
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fx.h>
#include <fxkeys.h>

#include "wtv_calls.h"
#include "wtv_mainwindow.h"

/*****************************************************************************/
FXWTVMainWindow::FXWTVMainWindow()
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
FXWTVMainWindow::FXWTVMainWindow(FXApp* app):
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
FXWTVMainWindow::~FXWTVMainWindow()
{
    delete m_image;
}

/*****************************************************************************/
long
FXWTVMainWindow::onConfigure(FXObject* obj, FXSelector sel, void* ptr)
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
FXWTVMainWindow::onPaint(FXObject* obj, FXSelector sel, void* ptr)
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
FXWTVMainWindow::onLeftBtnPress(FXObject* obj, FXSelector sel, void* ptr)
{
    (void)obj;
    (void)sel;
    (void)ptr;

    LOGLN10((m_wtv, LOG_INFO, LOGS, LOGP));
    return 1;
}

/*****************************************************************************/
long
FXWTVMainWindow::onLeftBtnRelease(FXObject* obj, FXSelector sel, void* ptr)
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

/*****************************************************************************/
long
FXWTVMainWindow::onKeyPress(FXObject* obj, FXSelector sel, void* ptr)
{
    (void)obj;
    (void)sel;
    (void)ptr;

    LOGLN10((m_wtv, LOG_INFO, LOGS, LOGP));
    return 1;
}

/*****************************************************************************/
long
FXWTVMainWindow::onKeyRelease(FXObject* obj, FXSelector sel, void* ptr)
{
    FXEvent* evt;

    (void)obj;
    (void)sel;

    LOGLN10((m_wtv, LOG_INFO, LOGS, LOGP));
    evt = (FXEvent*)ptr;
    if ((evt->code == KEY_f) || (evt->code == KEY_F))
    {
        if (target != NULL)
        {
            target->tryHandle(this, FXSEL(SEL_COMMAND, message), NULL);
        }
    }
    return 1;
}

FXDEFMAP(FXWTVMainWindow) FXWTVMainWindowMap[] =
{
    FXMAPFUNC(SEL_CONFIGURE, 0, FXWTVMainWindow::onConfigure),
    FXMAPFUNC(SEL_PAINT, 0, FXWTVMainWindow::onPaint),
    FXMAPFUNC(SEL_LEFTBUTTONPRESS, 0, FXWTVMainWindow::onLeftBtnPress),
    FXMAPFUNC(SEL_LEFTBUTTONRELEASE, 0, FXWTVMainWindow::onLeftBtnRelease),
    FXMAPFUNC(SEL_KEYPRESS, 0, FXWTVMainWindow::onKeyPress),
    FXMAPFUNC(SEL_KEYRELEASE, 0, FXWTVMainWindow::onKeyRelease)
};

FXIMPLEMENT(FXWTVMainWindow, FXMainWindow, FXWTVMainWindowMap,
            ARRAYNUMBER(FXWTVMainWindowMap))

