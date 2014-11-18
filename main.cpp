// usb_enum_demo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "UsbEnumer.h"

int _tmain(int argc, _TCHAR* argv[])
{
    CString sHubName = L"USB#VID_1A40&PID_0101#5&1b23b54f&0&1#{f18a0e88-c30c-11d0-8815-00a0c906bed8}";
    int     nPort    = 1;

    CUsbEnumer usb;
    usb.EnumAllUsbDevices();
    //usb.EnumerateHostControllers();
//    BOOL bIsAdb = usb.IsAdbDevice(sHubName, nPort);

	return 0;
}
