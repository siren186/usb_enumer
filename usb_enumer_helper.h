#pragma once
#include <vector>

class CUsbEnumerHelper
{
public:
    static void GetDebugModeOpenDevList(const TiXmlDocument* pDoc, std::vector<CString>& vecDebugModeOpen)
    {
        vecDebugModeOpen.clear();
        if (pDoc)
        {
            _ParseElement(pDoc, vecDebugModeOpen);
        }
    }

private:
    static void _ParseElement(const TiXmlNode* pElem, std::vector<CString>& vecDebugModeOpen)
    {
        if ( NULL == pElem )
        {
            return;
        }

        for (const TiXmlNode* pElement = pElem->FirstChild(); NULL != pElement; pElement = pElement->NextSibling())
        {
            TiXmlNode::NodeType emType = (TiXmlNode::NodeType)pElement->Type();
            switch (emType)
            {
            case TiXmlNode::TINYXML_ELEMENT:
                if ( 0 == _stricmp( pElement->Value(), "device")) // 找到设备节点
                {
                    const char* p = pElement->ToElement()->Attribute("instance_id");
                    if (p)
                    {
                        std::string sDevInstanceId = p;
                        bool bIsDebugModeOpen = _ParseInterfaceDescriptor(pElement);
                        if (bIsDebugModeOpen)
                        {
                            vecDebugModeOpen.push_back((LPCTSTR)CA2W(sDevInstanceId.c_str()));
                        }
                    }
                }
                else
                {
                    _ParseElement(pElement, vecDebugModeOpen);
                }
                break;
            default:
                break;
            }
        }
    }

    static bool _ParseInterfaceDescriptor(const TiXmlNode* pDeviceNode)
    {
        if ( NULL == pDeviceNode )
        {
            return false;
        }

        for (const TiXmlNode* pElement = pDeviceNode->FirstChild(); NULL != pElement; pElement = pElement->NextSibling())
        {
            TiXmlNode::NodeType emType = (TiXmlNode::NodeType)pElement->Type();
            switch (emType)
            {
            case TiXmlNode::TINYXML_ELEMENT:
                if (0 == _stricmp( pElement->Value(), "interface_descriptor"))
                {
                    int nInterfaceClass = -1;
                    int nInterfaceSubClass = -1;
                    int nInterfaceProtocol = -1;
                    pElement->ToElement()->Attribute("bInterfaceClass",    &nInterfaceClass);
                    pElement->ToElement()->Attribute("bInterfaceSubClass", &nInterfaceSubClass);
                    pElement->ToElement()->Attribute("bInterfaceProtocol", &nInterfaceProtocol);
                    if (0xff == nInterfaceClass &&
                        0x42 == nInterfaceSubClass)
                    {
                        return true;
                    }
                }
                break;
            default:
                break;
            }
        }
        return false;
    }
};
