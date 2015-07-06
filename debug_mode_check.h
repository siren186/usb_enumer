#pragma once
#include <algorithm>
#include "usb_enumer.h"
#include "usb_enumer_helper.h"

class CDebugModeCheck
{
public:
    static BOOL IsDebugModeOpen(LPCTSTR lpDevInstanceId)
    {
        BOOL bRet = FALSE;
        if (lpDevInstanceId)
        {
            CUsbEnumer usb;
            usb.EnumAllUsbDevices();
            const TiXmlDocument* pDoc = usb.GetEnumResult();

            std::vector<CString> vecDebugModeOpen;
            CUsbEnumerHelper::GetDebugModeOpenDevList(pDoc, vecDebugModeOpen);
            std::vector<CString>::const_iterator it = std::find(vecDebugModeOpen.begin(), vecDebugModeOpen.end(), lpDevInstanceId);
            if (it != vecDebugModeOpen.end())
            {
                bRet = TRUE;
            }
            usb.ClearEnumResult();
        }

        return bRet;
    }
};