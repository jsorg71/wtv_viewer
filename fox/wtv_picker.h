
#if !defined(WTV_GUI_PICKER_H)
#define WTV_GUI_PICKER_H

class PickerDialog : public FXDialogBox
{
    FXDECLARE(PickerDialog)
public:
    PickerDialog();
    PickerDialog(FXApp* app, FXWindow* parent, struct wtv_info* wtv);
    virtual ~PickerDialog();
    long onPress(FXObject* obj, FXSelector sel, void* ptr);
    long onStartupTimeout(FXObject* obj, FXSelector sel, void* ptr);
    long onDoubleClicked(FXObject* obj, FXSelector sel, void* ptr);
public:
    enum _ids
    {
        ID_BUTTON = FXDialogBox::ID_LAST,
        ID_LIST,
        ID_STARTUP,
        ID_LAST
    };
public:
    FXApp* m_app;
    FXButton* m_ok_but;
    FXButton* m_cancel_but;
    FXList* m_list;
    struct wtv_info* m_wtv;
};

#endif


