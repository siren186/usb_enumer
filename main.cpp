// usb_enum_demo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "impl.h"


int _tmain(int argc, _TCHAR* argv[])
{
    ULONG ulDeviceCnt = 0;

    Impl impl;
    impl.Init();
    impl.EnumerateHostControllers(&ulDeviceCnt);
	return 0;
}

