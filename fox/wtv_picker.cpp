
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include <fx.h>

#include "wtv_picker.h"
#include "wtv_calls.h"

FXDEFMAP(PickerDialog) PickerDialogMap[] =
{
    FXMAPFUNC(SEL_COMMAND, PickerDialog::ID_BUTTON, PickerDialog::onPress),
    FXMAPFUNC(SEL_TIMEOUT, PickerDialog::ID_STARTUP, PickerDialog::onStartupTimeout)
};

FXIMPLEMENT(PickerDialog, FXDialogBox, PickerDialogMap, ARRAYNUMBER(PickerDialogMap))

/*****************************************************************************/
PickerDialog::PickerDialog() : FXDialogBox()
{
    m_ok_but = NULL;
    m_app = NULL;
    m_wtv = NULL;
}

/*****************************************************************************/
PickerDialog::PickerDialog(FXApp* app, FXWindow* parent, struct wtv_info* wtv) : FXDialogBox(parent, "Pick")
{
    FXuint flags;
    FXSelector sel;
    //FXString ver;

    //LOGLN0((fi, LOG_INFO, LOGS, LOGP));
    setWidth(400);
    setHeight(400);

    flags = BUTTON_NORMAL | LAYOUT_EXPLICIT | BUTTON_DEFAULT;
    sel = PickerDialog::ID_BUTTON;
    m_ok_but = new FXButton(this, "&Ok", NULL, this, sel, flags, 200, 360, 80, 30);

    flags = BUTTON_NORMAL | LAYOUT_EXPLICIT;
    sel = PickerDialog::ID_BUTTON;
    m_cancel_but = new FXButton(this, "&Cancel", NULL, this, sel, flags, 300, 360, 80, 30);

    flags = LAYOUT_EXPLICIT | LIST_NORMAL;
    m_list = new FXList(this, NULL, 0, flags, 10, 10, 380, 340);

    m_app = app;
    m_wtv = wtv;
    m_ok_but->setFocus();

    m_app->addTimeout(this, PickerDialog::ID_STARTUP, 100, NULL);
}

/*****************************************************************************/
PickerDialog::~PickerDialog()
{
    //LOGLN0((m_fi, LOG_INFO, LOGS, LOGP));
}

/*****************************************************************************/
long
PickerDialog::onPress(FXObject* obj, FXSelector sel, void* ptr)
{
    //LOGLN0((m_fi, LOG_INFO, LOGS, LOGP));
    if (obj == m_ok_but)
    {
        return onCmdAccept(obj, sel, ptr);
    }
    return onCmdCancel(obj, sel, ptr);
}

/*****************************************************************************/
long
PickerDialog::onStartupTimeout(FXObject* obj, FXSelector sel, void* ptr)
{
    DIR * ldir;
    struct dirent * entry;
    char filename[256];
    int index;

    printf("PickerDialog::onStartupTimeout:\n");
    ldir = opendir("/tmp");
    index = -1;
    if (ldir != NULL)
    {
        entry = readdir(ldir);
        while (entry != NULL)
        {
            if (strncmp(entry->d_name, "wtv_", 3) == 0)
            {
                if (entry->d_type == DT_SOCK)
                {
                    snprintf(filename, 255, "/tmp/%s", entry->d_name);
                    index = m_list->appendItem(filename);
                }
            }
            entry = readdir(ldir);
        }
        closedir(ldir);
    }
    if (index >= 0)
    {
        m_list->selectItem(index);
        m_list->setFocus();
    }
    return 1;
}

