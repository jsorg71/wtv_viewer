
#if !defined(WTV_MAINWINDOW_H)
#define WTV_MAINWINDOW_H

class FXWTVMainWindow:public FXMainWindow
{
    FXDECLARE(FXWTVMainWindow)
public:
    FXWTVMainWindow();
    FXWTVMainWindow(FXApp* app);
    virtual ~FXWTVMainWindow();
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
    long onKeyPress(FXObject* obj, FXSelector sel, void* ptr);
    long onKeyRelease(FXObject* obj, FXSelector sel, void* ptr);
};

#endif

