// usb_enum_demo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "UsbEnumer.h"

void foo()
{
    TiXmlDocument *myDocument = new TiXmlDocument();

    TiXmlElement *RootElement = new TiXmlElement("Persons");
    myDocument->LinkEndChild(RootElement);

    TiXmlElement *PersonElement = new TiXmlElement("Person");
    RootElement->LinkEndChild(PersonElement);

    PersonElement->SetAttribute("ID", "1");

    TiXmlElement *NameElement = new TiXmlElement("name");
    TiXmlElement *AgeElement = new TiXmlElement("age");
    PersonElement->LinkEndChild(NameElement);
    PersonElement->LinkEndChild(AgeElement);
    TiXmlText *NameContent = new TiXmlText("周星星");
    TiXmlText *AgeContent = new TiXmlText("20");
    NameElement->LinkEndChild(NameContent);
    AgeElement->LinkEndChild(AgeContent);
    myDocument->SaveFile("c:\\1.xml");
}


int _tmain(int argc, _TCHAR* argv[])
{
    foo();
    CString sHubName = L"USB#VID_1A40&PID_0101#5&1b23b54f&0&1#{f18a0e88-c30c-11d0-8815-00a0c906bed8}";
    int     nPort    = 1;

    CUsbEnumer usb;
    usb.EnumerateHostControllers();
//    BOOL bIsAdb = usb.IsAdbDevice(sHubName, nPort);

	return 0;
}
