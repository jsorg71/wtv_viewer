
#if !defined(WTV_FULLSCREEN_H)
#define WTV_FULLSCREEN_H

class FXFullScreenWindow:public FXTopWindow
{
    FXDECLARE(FXFullScreenWindow)
public:
    FXFullScreenWindow();
    FXFullScreenWindow(FXWindow* p);
    virtual ~FXFullScreenWindow();
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
    long onKeyPress(FXObject* obj, FXSelector sel, void* ptr);
    long onKeyRelease(FXObject* obj, FXSelector sel, void* ptr);

};

#endif

