#include "stdafx.h"
#include "UsbEnumer.h"


void CUsbEnumer::_GetConnInfo(
    ULONG ulConnectionIndex,
    HANDLE hHubDevice,
    PUSB_NODE_CONNECTION_INFORMATION_EX* ppConnInfoEx,
    PUSB_NODE_CONNECTION_INFORMATION_EX_V2* ppConnInfoExV2
    )
{
    // 先取ConnInfoExV2
    // 再取ConnInfoEx,若失败,再取ConnInfo
    PUSB_NODE_CONNECTION_INFORMATION_EX    pConnInfoEx   = NULL;
    PUSB_NODE_CONNECTION_INFORMATION_EX_V2 pConnInfoExV2 = NULL;
    ULONG nBytes = 0;
    BOOL bSuc = FALSE;

    ULONG nBytesEx = sizeof(USB_NODE_CONNECTION_INFORMATION_EX) + (sizeof(USB_PIPE_INFO) * 30);
    pConnInfoEx = (PUSB_NODE_CONNECTION_INFORMATION_EX)ALLOC(nBytesEx);
    if (pConnInfoEx == NULL)
    {
        goto Exit0;
    }
    pConnInfoEx->ConnectionIndex = ulConnectionIndex;

    pConnInfoExV2 = (PUSB_NODE_CONNECTION_INFORMATION_EX_V2)ALLOC(sizeof(USB_NODE_CONNECTION_INFORMATION_EX_V2));
    if (pConnInfoExV2 == NULL)
    {
        goto Exit0;
    }
    pConnInfoExV2->ConnectionIndex = ulConnectionIndex;
    pConnInfoExV2->Length = sizeof(USB_NODE_CONNECTION_INFORMATION_EX_V2);
    pConnInfoExV2->SupportedUsbProtocols.Usb300 = 1;

    // ConnInfoExV2
    nBytes = 0;
    bSuc = DeviceIoControl(
        hHubDevice,
        IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX_V2,
        pConnInfoExV2,
        sizeof(USB_NODE_CONNECTION_INFORMATION_EX_V2),
        pConnInfoExV2,
        sizeof(USB_NODE_CONNECTION_INFORMATION_EX_V2),
        &nBytes,
        NULL);
    if (!bSuc || nBytes < sizeof(USB_NODE_CONNECTION_INFORMATION_EX_V2))
    {
        FREE(pConnInfoExV2);
        pConnInfoExV2 = NULL;
    }

    // ConnInfoEx
    bSuc = DeviceIoControl(
        hHubDevice,
        IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX,
        pConnInfoEx,
        nBytesEx,
        pConnInfoEx,
        nBytesEx,
        &nBytesEx,
        NULL);
    if (bSuc)
    {
        if (pConnInfoExV2)
        {
            if (pConnInfoEx->Speed == UsbHighSpeed && pConnInfoExV2->Flags.DeviceIsOperatingAtSuperSpeedOrHigher)
            {
                pConnInfoEx->Speed = UsbSuperSpeed;
            }
        }
    }
    else
    {
        nBytes = sizeof(USB_NODE_CONNECTION_INFORMATION) + sizeof(USB_PIPE_INFO) * 30;
        PUSB_NODE_CONNECTION_INFORMATION pConnInfo = (PUSB_NODE_CONNECTION_INFORMATION)ALLOC(nBytes);
        if (pConnInfo == NULL)
        {
            goto Exit0;
        }

        pConnInfo->ConnectionIndex = ulConnectionIndex;

        // ConnInfo
        bSuc = DeviceIoControl(
            hHubDevice,
            IOCTL_USB_GET_NODE_CONNECTION_INFORMATION,
            pConnInfo,
            nBytes,
            pConnInfo,
            nBytes,
            &nBytes,
            NULL);
        if (!bSuc)
        {
            FREE(pConnInfo);
            goto Exit0;
        }

        pConnInfoEx->ConnectionIndex = pConnInfo->ConnectionIndex;
        pConnInfoEx->DeviceDescriptor = pConnInfo->DeviceDescriptor;
        pConnInfoEx->CurrentConfigurationValue = pConnInfo->CurrentConfigurationValue;
        pConnInfoEx->Speed = pConnInfo->LowSpeed ? UsbLowSpeed : UsbFullSpeed;
        pConnInfoEx->DeviceIsHub = pConnInfo->DeviceIsHub;
        pConnInfoEx->DeviceAddress = pConnInfo->DeviceAddress;
        pConnInfoEx->NumberOfOpenPipes = pConnInfo->NumberOfOpenPipes;
        pConnInfoEx->ConnectionStatus = pConnInfo->ConnectionStatus;
        memcpy(&pConnInfoEx->PipeList[0], &pConnInfo->PipeList[0], sizeof(USB_PIPE_INFO)* 30);

        FREE(pConnInfo);
    }

    bSuc = TRUE; // 成功
Exit0:

    if (FALSE == bSuc)
    {
        if (pConnInfoEx)
        {
            FREE(pConnInfoEx);
            pConnInfoEx = NULL;
        }

        if (pConnInfoExV2)
        {
            FREE(pConnInfoExV2);
            pConnInfoExV2 = NULL;
        }
    }

    if (ppConnInfoEx)
    {
        *ppConnInfoEx = pConnInfoEx;
    }
    if (ppConnInfoExV2)
    {
        *ppConnInfoExV2 = pConnInfoExV2;
    }
}

VOID CUsbEnumer::EnumerateHubPorts(HANDLE hHubDevice, ULONG NumPorts)
{
    BOOL bSuc = 0;
    int icon = 0;

    PUSB_NODE_CONNECTION_INFORMATION_EX    pConnectionInfoEx   = NULL;
    PUSB_NODE_CONNECTION_INFORMATION_EX_V2 pConnectionInfoExV2 = NULL;
    UsbDevicePnpStrings                    stDevPnpStrings;

    for (ULONG index = 1; index <= NumPorts; index++)
    {
        pConnectionInfoEx   = NULL;
        pConnectionInfoExV2 = NULL;

        ULONG nBytesEx = sizeof(USB_NODE_CONNECTION_INFORMATION_EX) + (sizeof(USB_PIPE_INFO) * 30);
        pConnectionInfoEx = (PUSB_NODE_CONNECTION_INFORMATION_EX)ALLOC(nBytesEx);
        if (pConnectionInfoEx == NULL)
        {
            break;
        }
        pConnectionInfoEx->ConnectionIndex = index;

        pConnectionInfoExV2 = (PUSB_NODE_CONNECTION_INFORMATION_EX_V2)ALLOC(sizeof(USB_NODE_CONNECTION_INFORMATION_EX_V2));
        if (pConnectionInfoExV2 == NULL)
        {
            FREE(pConnectionInfoEx);
            break;
        }
        pConnectionInfoExV2->ConnectionIndex = index;
        pConnectionInfoExV2->Length = sizeof(USB_NODE_CONNECTION_INFORMATION_EX_V2);
        pConnectionInfoExV2->SupportedUsbProtocols.Usb300 = 1;

        ULONG nBytes = 0;
        bSuc = DeviceIoControl(
            hHubDevice,
            IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX_V2,
            pConnectionInfoExV2,
            sizeof(USB_NODE_CONNECTION_INFORMATION_EX_V2),
            pConnectionInfoExV2,
            sizeof(USB_NODE_CONNECTION_INFORMATION_EX_V2),
            &nBytes,
            NULL);
        if (!bSuc || nBytes < sizeof(USB_NODE_CONNECTION_INFORMATION_EX_V2))
        {
            FREE(pConnectionInfoExV2);
            pConnectionInfoExV2 = NULL;
        }

        bSuc = DeviceIoControl(
            hHubDevice,
            IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX,
            pConnectionInfoEx,
            nBytesEx,
            pConnectionInfoEx,
            nBytesEx,
            &nBytesEx,
            NULL);
        if (bSuc)
        {
            if (pConnectionInfoEx->Speed == UsbHighSpeed
                && pConnectionInfoExV2 != NULL
                && pConnectionInfoExV2->Flags.DeviceIsOperatingAtSuperSpeedOrHigher)
            {
                pConnectionInfoEx->Speed = UsbSuperSpeed;
            }
        }
        else
        {
            nBytes = sizeof(USB_NODE_CONNECTION_INFORMATION) + sizeof(USB_PIPE_INFO) * 30;

            PUSB_NODE_CONNECTION_INFORMATION pConnectionInfo = (PUSB_NODE_CONNECTION_INFORMATION)ALLOC(nBytes);
            if (pConnectionInfo == NULL)
            {
                FREE(pConnectionInfoEx);
                if (pConnectionInfoExV2 != NULL)
                {
                    FREE(pConnectionInfoExV2);
                }
                continue;
            }

            pConnectionInfo->ConnectionIndex = index;

            bSuc = DeviceIoControl(
                hHubDevice,
                IOCTL_USB_GET_NODE_CONNECTION_INFORMATION,
                pConnectionInfo,
                nBytes,
                pConnectionInfo,
                nBytes,
                &nBytes,
                NULL);
            if (!bSuc)
            {
                FREE(pConnectionInfo);
                FREE(pConnectionInfoEx);
                if (pConnectionInfoExV2 != NULL)
                {
                    FREE(pConnectionInfoExV2);
                }
                continue;
            }

            pConnectionInfoEx->ConnectionIndex = pConnectionInfo->ConnectionIndex;
            pConnectionInfoEx->DeviceDescriptor = pConnectionInfo->DeviceDescriptor;
            pConnectionInfoEx->CurrentConfigurationValue = pConnectionInfo->CurrentConfigurationValue;
            pConnectionInfoEx->Speed = pConnectionInfo->LowSpeed ? UsbLowSpeed : UsbFullSpeed;
            pConnectionInfoEx->DeviceIsHub = pConnectionInfo->DeviceIsHub;
            pConnectionInfoEx->DeviceAddress = pConnectionInfo->DeviceAddress;
            pConnectionInfoEx->NumberOfOpenPipes = pConnectionInfo->NumberOfOpenPipes;
            pConnectionInfoEx->ConnectionStatus = pConnectionInfo->ConnectionStatus;
            memcpy(&pConnectionInfoEx->PipeList[0], &pConnectionInfo->PipeList[0], sizeof(USB_PIPE_INFO)* 30);

            FREE(pConnectionInfo);
        }

        if (pConnectionInfoEx->ConnectionStatus == DeviceConnected)
        {
            //pConfigDesc = GetConfigDescriptor(hHubDevice, index, 0);
        }

        if (pConnectionInfoEx->ConnectionStatus != NoDeviceConnected)
        {
            CString sDrvKeyName = GetDriverKeyName(hHubDevice, index);
            if (!sDrvKeyName.IsEmpty() && sDrvKeyName.GetLength() < MAX_DRIVER_KEY_NAME)
            {
                BOOL b = _DriverNameToDeviceProperties(sDrvKeyName, stDevPnpStrings);
            }
        }

        if (pConnectionInfoEx->DeviceIsHub)
        {
            CString sExtHubName = GetExternalHubName(hHubDevice, index);
            if (!sExtHubName.IsEmpty() && sExtHubName.GetLength() < MAX_DRIVER_KEY_NAME)
            {
                EnumerateHub(sExtHubName);
            }
        }
        else
        {
            if (pConnectionInfoEx->ConnectionStatus == NoDeviceConnected)
            {
                if (pConnectionInfoExV2 != NULL &&
                    pConnectionInfoExV2->SupportedUsbProtocols.Usb300 == 1)
                    icon = NoSsDeviceIcon;
                else
                    icon = NoDeviceIcon;
            }
            else if (pConnectionInfoEx->CurrentConfigurationValue)
            {
                pConnectionInfoEx->Speed == UsbSuperSpeed ? icon = GoodSsDeviceIcon : icon = GoodDeviceIcon;
            }
            else
            {
                icon = BadDeviceIcon;
            }
        }
    }
}

CString CUsbEnumer::GetDriverKeyName(HANDLE hHub, ULONG ConnectionIndex)
{
    CString sRetDrvKeyName;
    PUSB_NODE_CONNECTION_DRIVERKEY_NAME pDriverKeyName = NULL;

    // 读大小
    USB_NODE_CONNECTION_DRIVERKEY_NAME stDriverKeyName = {0};
    stDriverKeyName.ConnectionIndex = ConnectionIndex;
    ULONG ulBytes = 0;
    BOOL bSuc = ::DeviceIoControl(
        hHub,
        IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME,
        &stDriverKeyName,
        sizeof(stDriverKeyName),
        &stDriverKeyName,
        sizeof(stDriverKeyName),
        &ulBytes,
        NULL);
    if (!bSuc)
    {
        goto Exit0;
    }

    ulBytes = stDriverKeyName.ActualLength;
    if (ulBytes <= sizeof(stDriverKeyName))
    {
        goto Exit0;
    }

    // 申请内存
    pDriverKeyName = (PUSB_NODE_CONNECTION_DRIVERKEY_NAME)ALLOC(ulBytes);
    if (pDriverKeyName == NULL)
    {
        goto Exit0;
    }

    // 取值
    pDriverKeyName->ConnectionIndex = ConnectionIndex;
    bSuc = ::DeviceIoControl(hHub, IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME, pDriverKeyName, ulBytes, pDriverKeyName, ulBytes, &ulBytes, NULL);
    if (!bSuc)
    {
        goto Exit0;
    }

    sRetDrvKeyName = (LPCTSTR)pDriverKeyName->DriverKeyName;

Exit0:
    if (pDriverKeyName != NULL)
    {
        FREE(pDriverKeyName);
        pDriverKeyName = NULL;
    }

    return sRetDrvKeyName;
}

BOOL CUsbEnumer::_DriverNameToDeviceProperties(const CString& sDrvKeyName, UsbDevicePnpStrings& stPnpStrings)
{
    BOOL  bSuc = FALSE;
    ULONG ulSize = 0;
    DWORD lastError = 0;
    TCHAR* pDevInstanceId = NULL;

    SP_DEVINFO_DATA deviceInfoData = {0};
    HDEVINFO hDevInfo = _DriverNameToDeviceInst(sDrvKeyName, deviceInfoData);
    if (INVALID_HANDLE_VALUE == hDevInfo)
    {
        goto Exit0;
    }

    ulSize = 0;
    bSuc = ::SetupDiGetDeviceInstanceId(hDevInfo, &deviceInfoData, NULL, 0, &ulSize);
    lastError = GetLastError();
    if (bSuc != FALSE && lastError != ERROR_INSUFFICIENT_BUFFER)
    {
        bSuc = FALSE;
        goto Exit0;
    }

    // An extra byte is required for the terminating character
    pDevInstanceId = new TCHAR[++ulSize];
    bSuc = SetupDiGetDeviceInstanceId(hDevInfo, &deviceInfoData, pDevInstanceId, MAX_DRIVER_KEY_NAME, &ulSize);
    if (bSuc)
    {
        stPnpStrings.sDeviceInstanceId = pDevInstanceId;
        stPnpStrings.sDeviceDesc       = _GetDeviceProperty(hDevInfo, &deviceInfoData, SPDRP_DEVICEDESC);
        stPnpStrings.sHardwareId       = _GetDeviceProperty(hDevInfo, &deviceInfoData, SPDRP_HARDWAREID);
        stPnpStrings.sService          = _GetDeviceProperty(hDevInfo, &deviceInfoData, SPDRP_SERVICE);
        stPnpStrings.sDeviceClass      = _GetDeviceProperty(hDevInfo, &deviceInfoData, SPDRP_CLASS);
    }

Exit0:
    if (pDevInstanceId)
    {
        delete pDevInstanceId;
    }

    if (INVALID_HANDLE_VALUE != hDevInfo)
    {
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }

    return bSuc;
}

HDEVINFO CUsbEnumer::_DriverNameToDeviceInst(const CString& sDrvKeyName, SP_DEVINFO_DATA& stDevInfoData)
{
    ULONG ulIndex = 0;
    BOOL  bSuc   = FALSE;

    // We cannot walk the device tree with CM_Get_Sibling etc. unless we assume
    // the device tree will stabilize. Any devnode removal (even outside of USB)
    // would force us to retry. Instead we use Setup API to snapshot all devices.
    HDEVINFO hDevInfo = ::SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (INVALID_HANDLE_VALUE == hDevInfo)
    {
        goto Exit0;
    }

    ulIndex = 0;
    memset(&stDevInfoData, 0, sizeof(SP_DEVINFO_DATA));
    stDevInfoData.cbSize = sizeof(stDevInfoData);

    while (bSuc == FALSE)
    {
        if (!::SetupDiEnumDeviceInfo(hDevInfo, ulIndex, &stDevInfoData))
        {
            break;
        }

        CString sDriverName = _GetDeviceProperty(hDevInfo, &stDevInfoData, SPDRP_DRIVER);
        if (!sDriverName.IsEmpty() && sDrvKeyName.CompareNoCase(sDriverName) == 0)
        {
            bSuc = TRUE; // 成功
            break;
        }

        ulIndex++;

        if (ulIndex >= 100) // 这个地方,微软的usbview中没有,加这个,是为了防止死循环.
        {
            break;
        }
    }

Exit0:
    if (FALSE == bSuc)
    {
        if (INVALID_HANDLE_VALUE != hDevInfo)
        {
            ::SetupDiDestroyDeviceInfoList(hDevInfo);
            hDevInfo = INVALID_HANDLE_VALUE;
        }
        memset(&stDevInfoData, 0, sizeof(SP_DEVINFO_DATA));
    }

    return hDevInfo;
}

CString CUsbEnumer::_GetDeviceProperty(
    _In_ HDEVINFO hDevInfo,
    _In_ PSP_DEVINFO_DATA pDevInfoData,
    _In_ DWORD dwProperty)
{
    CString sRetBuffer;
    LPTSTR lpBuf = NULL;

//    DWORD nBufSize = nBufLen * sizeof(TCHAR);
    DWORD dwDataBytes = 0;

//     if (pBuffer == NULL)
//     {
//         return FALSE;
//     }

    BOOL bResult = SetupDiGetDeviceRegistryProperty(hDevInfo, pDevInfoData, dwProperty, NULL, NULL, 0, &dwDataBytes);
    DWORD lastError = GetLastError();

    if ((dwDataBytes == 0) || (bResult != FALSE && lastError != ERROR_INSUFFICIENT_BUFFER))
    {
        goto Exit0;
    }

    lpBuf = (LPTSTR)ALLOC(dwDataBytes + 1);
    if (NULL == lpBuf)
    {
        goto Exit0;
    }
    memset(lpBuf, 0, dwDataBytes + 1);

    bResult = SetupDiGetDeviceRegistryProperty(
        hDevInfo,
        pDevInfoData,
        dwProperty,
        NULL,
        (PBYTE)lpBuf,
        dwDataBytes,
        &dwDataBytes);

    if (bResult)
    {
        sRetBuffer.Append(lpBuf, dwDataBytes / sizeof(TCHAR));
    }

Exit0:
    if (lpBuf)
    {
        FREE(lpBuf);
    }
    return sRetBuffer;
}

//*****************************************************************************
//
// GetConfigDescriptor()
//
// hHubDevice - Handle of the hub device containing the port from which the
// Configuration Descriptor will be requested.
//
// ConnectionIndex - Identifies the port on the hub to which a device is
// attached from which the Configuration Descriptor will be requested.
//
// DescriptorIndex - Configuration Descriptor index, zero based.
//
//*****************************************************************************

PUSB_DESCRIPTOR_REQUEST CUsbEnumer::GetConfigDescriptor(HANDLE hHubDevice, ULONG ConnectionIndex, UCHAR DescriptorIndex)
{
    BOOL    success = 0;
    ULONG   nBytes = 0;
    ULONG   nBytesReturned = 0;

    UCHAR   configDescReqBuf[sizeof(USB_DESCRIPTOR_REQUEST)+
        sizeof(USB_CONFIGURATION_DESCRIPTOR)];

    PUSB_DESCRIPTOR_REQUEST         configDescReq = NULL;
    PUSB_CONFIGURATION_DESCRIPTOR   configDesc = NULL;


    // Request the Configuration Descriptor the first time using our
    // local buffer, which is just big enough for the Cofiguration
    // Descriptor itself.
    //
    nBytes = sizeof(configDescReqBuf);

    configDescReq = (PUSB_DESCRIPTOR_REQUEST)configDescReqBuf;
    configDesc = (PUSB_CONFIGURATION_DESCRIPTOR)(configDescReq + 1);

    // Zero fill the entire request structure
    //
    memset(configDescReq, 0, nBytes);

    // Indicate the port from which the descriptor will be requested
    //
    configDescReq->ConnectionIndex = ConnectionIndex;

    //
    // USBHUB uses URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE to process this
    // IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION request.
    //
    // USBD will automatically initialize these fields:
    //     bmRequest = 0x80
    //     bRequest  = 0x06
    //
    // We must inititialize these fields:
    //     wValue    = Descriptor Type (high) and Descriptor Index (low byte)
    //     wIndex    = Zero (or Language ID for String Descriptors)
    //     wLength   = Length of descriptor buffer
    //
    configDescReq->SetupPacket.wValue = (USB_CONFIGURATION_DESCRIPTOR_TYPE << 8)
        | DescriptorIndex;

    configDescReq->SetupPacket.wLength = (USHORT)(nBytes - sizeof(USB_DESCRIPTOR_REQUEST));

    // Now issue the get descriptor request.
    //
    success = DeviceIoControl(hHubDevice,
        IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
        configDescReq,
        nBytes,
        configDescReq,
        nBytes,
        &nBytesReturned,
        NULL);

    if (!success)
    {
        return NULL;
    }

    if (nBytes != nBytesReturned)
    {
        return NULL;
    }

    if (configDesc->wTotalLength < sizeof(USB_CONFIGURATION_DESCRIPTOR))
    {
        return NULL;
    }

    // Now request the entire Configuration Descriptor using a dynamically
    // allocated buffer which is sized big enough to hold the entire descriptor
    //
    nBytes = sizeof(USB_DESCRIPTOR_REQUEST)+configDesc->wTotalLength;

    configDescReq = (PUSB_DESCRIPTOR_REQUEST)ALLOC(nBytes);

    if (configDescReq == NULL)
    {
        return NULL;
    }

    configDesc = (PUSB_CONFIGURATION_DESCRIPTOR)(configDescReq + 1);

    // Indicate the port from which the descriptor will be requested
    //
    configDescReq->ConnectionIndex = ConnectionIndex;

    //
    // USBHUB uses URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE to process this
    // IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION request.
    //
    // USBD will automatically initialize these fields:
    //     bmRequest = 0x80
    //     bRequest  = 0x06
    //
    // We must inititialize these fields:
    //     wValue    = Descriptor Type (high) and Descriptor Index (low byte)
    //     wIndex    = Zero (or Language ID for String Descriptors)
    //     wLength   = Length of descriptor buffer
    //
    configDescReq->SetupPacket.wValue = (USB_CONFIGURATION_DESCRIPTOR_TYPE << 8)
        | DescriptorIndex;

    configDescReq->SetupPacket.wLength = (USHORT)(nBytes - sizeof(USB_DESCRIPTOR_REQUEST));

    // Now issue the get descriptor request.
    //

    success = DeviceIoControl(hHubDevice,
        IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
        configDescReq,
        nBytes,
        configDescReq,
        nBytes,
        &nBytesReturned,
        NULL);

    if (!success)
    {
        FREE(configDescReq);
        return NULL;
    }

    if (nBytes != nBytesReturned)
    {
        FREE(configDescReq);
        return NULL;
    }

    if (configDesc->wTotalLength != (nBytes - sizeof(USB_DESCRIPTOR_REQUEST)))
    {
        FREE(configDescReq);
        return NULL;
    }

    return configDescReq;
}

CString CUsbEnumer::GetExternalHubName(HANDLE Hub, ULONG ConnectionIndex)
{
    CString sExtHubName;
    BOOL                        success = 0;
    ULONG                       nBytes = 0;
    USB_NODE_CONNECTION_NAME    stExtHubName;
    PUSB_NODE_CONNECTION_NAME   pExtHubName = NULL;
    //PCHAR                       extHubNameA = NULL;

    // Get the length of the name of the external hub attached to the
    // specified port.
    //
    stExtHubName.ConnectionIndex = ConnectionIndex;

    success = DeviceIoControl(Hub,
        IOCTL_USB_GET_NODE_CONNECTION_NAME,
        &stExtHubName,
        sizeof(stExtHubName),
        &stExtHubName,
        sizeof(stExtHubName),
        &nBytes,
        NULL);

    if (!success)
    {
        goto Exit0;
    }

    // Allocate space to hold the external hub name
    //
    nBytes = stExtHubName.ActualLength;

    if (nBytes <= sizeof(stExtHubName))
    {
        goto Exit0;
    }

    pExtHubName = (PUSB_NODE_CONNECTION_NAME)ALLOC(nBytes);

    if (pExtHubName == NULL)
    {
        goto Exit0;
    }

    // Get the name of the external hub attached to the specified port
    //
    pExtHubName->ConnectionIndex = ConnectionIndex;

    success = DeviceIoControl(Hub,
        IOCTL_USB_GET_NODE_CONNECTION_NAME,
        pExtHubName,
        nBytes,
        pExtHubName,
        nBytes,
        &nBytes,
        NULL);

    if (!success)
    {
        goto Exit0;
    }

    // Convert the External Hub name
    //
    sExtHubName = (LPCTSTR)pExtHubName->NodeName;

Exit0:
    // There was an error, free anything that was allocated
    //
    if (pExtHubName != NULL)
    {
        FREE(pExtHubName);
        pExtHubName = NULL;
    }

    return sExtHubName;
}

VOID CUsbEnumer::EnumerateHostController(_In_ HANDLE hHCDev, _In_ HDEVINFO hDeviceInfo, _In_ PSP_DEVINFO_DATA deviceInfoData)
{
    CString sRootHubName = GetRootHubName(hHCDev);
    if (!sRootHubName.IsEmpty() && sRootHubName.GetLength() < MAX_DRIVER_KEY_NAME)
    {
        EnumerateHub(sRootHubName);
    }
}

VOID CUsbEnumer::EnumerateHub(_In_ const CString& sHubName)
{
    HANDLE                hHubDevice = INVALID_HANDLE_VALUE;
    CString               sFullHubName;
    ULONG                 nBytes = 0;
    BOOL                  bSuc = FALSE;
    PUSB_NODE_INFORMATION pNodeInfo = (PUSB_NODE_INFORMATION)ALLOC(sizeof(USB_NODE_INFORMATION));
    if (pNodeInfo == NULL)
    {
        goto Exit0;
    }

    sFullHubName.Format(_T("\\\\.\\%s"), sHubName);
    hHubDevice = CreateFile(sFullHubName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hHubDevice == INVALID_HANDLE_VALUE)
    {
        goto Exit0;
    }

    bSuc = DeviceIoControl(
        hHubDevice,
        IOCTL_USB_GET_NODE_INFORMATION,
        pNodeInfo,
        sizeof(USB_NODE_INFORMATION),
        pNodeInfo,
        sizeof(USB_NODE_INFORMATION),
        &nBytes,
        NULL);
    if (!bSuc)
    {
        goto Exit0;
    }

    EnumerateHubPorts(hHubDevice, pNodeInfo->u.HubInformation.HubDescriptor.bNumberOfPorts);

Exit0:
    if (pNodeInfo)
    {
        FREE(pNodeInfo);
    }
    if (hHubDevice != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hHubDevice);
    }
}


//*****************************************************************************
//
// GetStringDescriptor()
//
// hHubDevice - Handle of the hub device containing the port from which the
// String Descriptor will be requested.
//
// ConnectionIndex - Identifies the port on the hub to which a device is
// attached from which the String Descriptor will be requested.
//
// DescriptorIndex - String Descriptor index.
//
// LanguageID - Language in which the string should be requested.
//
//*****************************************************************************

PStringDescriptorNode CUsbEnumer::GetStringDescriptor(
HANDLE  hHubDevice,
ULONG   ConnectionIndex,
UCHAR   DescriptorIndex,
USHORT  LanguageID
)

{
    BOOL    success = 0;
    ULONG   nBytes = 0;
    ULONG   nBytesReturned = 0;

    UCHAR   stringDescReqBuf[sizeof(USB_DESCRIPTOR_REQUEST)+
        MAXIMUM_USB_STRING_LENGTH];

    PUSB_DESCRIPTOR_REQUEST stringDescReq = NULL;
    PUSB_STRING_DESCRIPTOR  stringDesc = NULL;
    PStringDescriptorNode stringDescNode = NULL;

    nBytes = sizeof(stringDescReqBuf);

    stringDescReq = (PUSB_DESCRIPTOR_REQUEST)stringDescReqBuf;
    stringDesc = (PUSB_STRING_DESCRIPTOR)(stringDescReq + 1);

    // Zero fill the entire request structure
    //
    memset(stringDescReq, 0, nBytes);

    // Indicate the port from which the descriptor will be requested
    //
    stringDescReq->ConnectionIndex = ConnectionIndex;

    //
    // USBHUB uses URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE to process this
    // IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION request.
    //
    // USBD will automatically initialize these fields:
    //     bmRequest = 0x80
    //     bRequest  = 0x06
    //
    // We must inititialize these fields:
    //     wValue    = Descriptor Type (high) and Descriptor Index (low byte)
    //     wIndex    = Zero (or Language ID for String Descriptors)
    //     wLength   = Length of descriptor buffer
    //
    stringDescReq->SetupPacket.wValue = (USB_STRING_DESCRIPTOR_TYPE << 8)
        | DescriptorIndex;

    stringDescReq->SetupPacket.wIndex = LanguageID;

    stringDescReq->SetupPacket.wLength = (USHORT)(nBytes - sizeof(USB_DESCRIPTOR_REQUEST));

    // Now issue the get descriptor request.
    //
    success = DeviceIoControl(hHubDevice,
        IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
        stringDescReq,
        nBytes,
        stringDescReq,
        nBytes,
        &nBytesReturned,
        NULL);

    //
    // Do some sanity checks on the return from the get descriptor request.
    //

    if (!success)
    {
        return NULL;
    }

    if (nBytesReturned < 2)
    {
        return NULL;
    }

    if (stringDesc->bDescriptorType != USB_STRING_DESCRIPTOR_TYPE)
    {
        return NULL;
    }

    if (stringDesc->bLength != nBytesReturned - sizeof(USB_DESCRIPTOR_REQUEST))
    {
        return NULL;
    }

    if (stringDesc->bLength % 2 != 0)
    {
        return NULL;
    }

    //
    // Looks good, allocate some (zero filled) space for the string descriptor
    // node and copy the string descriptor to it.
    //

    stringDescNode = (PStringDescriptorNode)ALLOC(sizeof(StringDescriptorNode)+
        stringDesc->bLength);

    if (stringDescNode == NULL)
    {
        return NULL;
    }

    stringDescNode->DescriptorIndex = DescriptorIndex;
    stringDescNode->LanguageID = LanguageID;

    memcpy(stringDescNode->StringDescriptor,
        stringDesc,
        stringDesc->bLength);

    return stringDescNode;
}

CString CUsbEnumer::GetHCDDriverKeyName(HANDLE HCD)
{
    CString sDriverKeyName;
    BOOL                    success = 0;
    ULONG                   nBytes = 0;
    USB_HCD_DRIVERKEY_NAME  driverKeyName = { 0 };
    PUSB_HCD_DRIVERKEY_NAME pDriverKeyName = NULL;
    //PCHAR                   driverKeyNameA = NULL;

    ZeroMemory(&driverKeyName, sizeof(driverKeyName));

    // Get the length of the name of the driver key of the HCD
    //
    success = DeviceIoControl(HCD,
        IOCTL_GET_HCD_DRIVERKEY_NAME,
        &driverKeyName,
        sizeof(driverKeyName),
        &driverKeyName,
        sizeof(driverKeyName),
        &nBytes,
        NULL);

    if (!success)
    {
        goto Exit0;
    }

    // Allocate space to hold the driver key name
    //
    nBytes = driverKeyName.ActualLength;
    if (nBytes <= sizeof(driverKeyName))
    {
        goto Exit0;
    }

    // Allocate size of name plus 1 for terminal zero
    pDriverKeyName = (PUSB_HCD_DRIVERKEY_NAME)ALLOC(nBytes + 1);
    if (pDriverKeyName == NULL)
    {
        goto Exit0;
    }

    // Get the name of the driver key of the device attached to
    // the specified port.
    //

    success = DeviceIoControl(HCD,
        IOCTL_GET_HCD_DRIVERKEY_NAME,
        pDriverKeyName,
        nBytes,
        pDriverKeyName,
        nBytes,
        &nBytes,
        NULL);
    if (!success)
    {
        goto Exit0;
    }

    // Convert the driver key name
    //
    //driverKeyNameA = WideStrToMultiStr(driverKeyNameW->DriverKeyName, nBytes);
    sDriverKeyName = pDriverKeyName->DriverKeyName;

Exit0:
    // There was an error, free anything that was allocated
    //
    if (pDriverKeyName != NULL)
    {
        FREE(pDriverKeyName);
        pDriverKeyName = NULL;
    }

    return sDriverKeyName;
}

CString CUsbEnumer::GetRootHubName(HANDLE HostController)
{
    CString sRootHubName;
    BOOL                success = 0;
    ULONG               nBytes = 0;
    USB_ROOT_HUB_NAME   stRootHubName;
    PUSB_ROOT_HUB_NAME  pRootHubName = NULL;
    //PCHAR               rootHubNameA = NULL;

    // Get the length of the name of the Root Hub attached to the
    // Host Controller
    //
    success = DeviceIoControl(HostController,
        IOCTL_USB_GET_ROOT_HUB_NAME,
        0,
        0,
        &stRootHubName,
        sizeof(stRootHubName),
        &nBytes,
        NULL);

    if (!success)
    {
        goto Exit0;
    }

    // Allocate space to hold the Root Hub name
    //
    nBytes = stRootHubName.ActualLength;

    pRootHubName = (PUSB_ROOT_HUB_NAME)ALLOC(nBytes);
    if (pRootHubName == NULL)
    {
        goto Exit0;
    }

    // Get the name of the Root Hub attached to the Host Controller
    //
    success = DeviceIoControl(HostController,
        IOCTL_USB_GET_ROOT_HUB_NAME,
        NULL,
        0,
        pRootHubName,
        nBytes,
        &nBytes,
        NULL);
    if (!success)
    {
        goto Exit0;
    }

    // Convert the Root Hub name
    //
    //rootHubNameA = WideStrToMultiStr(rootHubNameW->RootHubName, nBytes);
    // TODO: 小心这个地方的赋值,是否只是赋值了一个字符
    sRootHubName = (LPCTSTR)pRootHubName->RootHubName;

Exit0:
    if (pRootHubName != NULL)
    {
        FREE(pRootHubName);
        pRootHubName = NULL;
    }
    return sRootHubName;
}

VOID CUsbEnumer::EnumerateHostControllers()
{

    HDEVINFO hDevInfo = SetupDiGetClassDevs((LPGUID)&GUID_CLASS_USB_HOST_CONTROLLER, NULL, NULL, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));
    if (INVALID_HANDLE_VALUE != hDevInfo)
    {
        SP_DEVICE_INTERFACE_DATA stInterfaceData = {0};
        SP_DEVINFO_DATA deviceInfoData = {0};
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        for (DWORD index=0; SetupDiEnumDeviceInfo(hDevInfo, index, &deviceInfoData); index++)
        {
            stInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
            BOOL bSuc = SetupDiEnumDeviceInterfaces(hDevInfo, 0, (LPGUID)&GUID_CLASS_USB_HOST_CONTROLLER, index, &stInterfaceData);
            if (!bSuc)
            {
                break;
            }

            CString sDevPath = _GetDevPath(hDevInfo, stInterfaceData);
            if (sDevPath.IsEmpty())
            {
                break;
            }

            HANDLE hHCDev = CreateFile(sDevPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
            if (hHCDev != INVALID_HANDLE_VALUE)
            {
                CString sDrvKeyName = GetHCDDriverKeyName(hHCDev);
                if (!sDrvKeyName.IsEmpty())
                {
                    EnumerateHostController(hHCDev, hDevInfo, &deviceInfoData);
                }
                CloseHandle(hHCDev);
            }
        }
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }
}

BOOL CUsbEnumer::IsAdbDevice( const CString& sFatherHubName, int nPortNum )
{
    BOOL bFindInterface0xff42 = FALSE;

    CString sFullHubName;
    sFullHubName.Format(_T("\\\\.\\%s"), sFatherHubName);
    HANDLE hHubDevice = CreateFile(sFullHubName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hHubDevice != INVALID_HANDLE_VALUE)
    {
        PUSB_DESCRIPTOR_REQUEST configDesc = GetConfigDescriptor(hHubDevice, nPortNum, 0);
        _MyReadUsbDescriptorRequest(configDesc, bFindInterface0xff42);
        if (configDesc)
        {
            FREE(configDesc);
        }
    }

    return bFindInterface0xff42;
}

void CUsbEnumer::_MyReadUsbDescriptorRequest( PUSB_DESCRIPTOR_REQUEST pRequest, BOOL& bFindInterface0xff42)
{
    if (NULL == pRequest)
        return;

    PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc = (PUSB_CONFIGURATION_DESCRIPTOR)(pRequest + 1);
    PUSB_COMMON_DESCRIPTOR commonDesc = (PUSB_COMMON_DESCRIPTOR)(ConfigDesc);

    do 
    {
        switch (commonDesc->bDescriptorType)
        {
        case USB_INTERFACE_DESCRIPTOR_TYPE:
            UCHAR bInterfaceClass = ((PUSB_INTERFACE_DESCRIPTOR)commonDesc)->bInterfaceClass;
            UCHAR bInterfaceSubClass = ((PUSB_INTERFACE_DESCRIPTOR)commonDesc)->bInterfaceSubClass;
            UCHAR bInterfaceProtocol = ((PUSB_INTERFACE_DESCRIPTOR)commonDesc)->bInterfaceProtocol;
            if (bInterfaceClass == 0xff && bInterfaceSubClass == 0x42)
            {
                bFindInterface0xff42 = TRUE;
            }
            break;
        }
    } while ((commonDesc = GetNextDescriptor((PUSB_COMMON_DESCRIPTOR)ConfigDesc, ConfigDesc->wTotalLength, commonDesc, -1)) != NULL);
}

void CUsbEnumer::_ParsepUsbDescriptorRequest( PUSB_DESCRIPTOR_REQUEST pRequest)
{
    if (NULL == pRequest)
        return;

    PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc = (PUSB_CONFIGURATION_DESCRIPTOR)(pRequest + 1);
    PUSB_COMMON_DESCRIPTOR commonDesc = (PUSB_COMMON_DESCRIPTOR)(ConfigDesc);

    do 
    {
        switch (commonDesc->bDescriptorType)
        {
        case USB_INTERFACE_DESCRIPTOR_TYPE:
            {
//                 TiXmlElement* pXmlElemInterface = new TiXmlElement("interface_descriptor");
//                 pXmlElemInterface->SetAttribute("bInterfaceClass", (int)((PUSB_INTERFACE_DESCRIPTOR)commonDesc)->bInterfaceClass);
//                 pXmlElemInterface->SetAttribute("bInterfaceSubClass", (int)((PUSB_INTERFACE_DESCRIPTOR)commonDesc)->bInterfaceSubClass);
//                 pXmlElemInterface->SetAttribute("bInterfaceProtocol", (int)((PUSB_INTERFACE_DESCRIPTOR)commonDesc)->bInterfaceProtocol);
//                 elem->LinkEndChild(pXmlElemInterface);
            }
            break;
        }
    } while ((commonDesc = GetNextDescriptor((PUSB_COMMON_DESCRIPTOR)ConfigDesc, ConfigDesc->wTotalLength, commonDesc, -1)) != NULL);
}

PUSB_COMMON_DESCRIPTOR CUsbEnumer::GetNextDescriptor(
    _In_reads_bytes_(TotalLength) PUSB_COMMON_DESCRIPTOR FirstDescriptor,
    _In_ ULONG TotalLength,
    _In_ PUSB_COMMON_DESCRIPTOR StartDescriptor,
    _In_ long DescriptorType
    )
{
    PUSB_COMMON_DESCRIPTOR currentDescriptor = NULL;
    PUSB_COMMON_DESCRIPTOR endDescriptor     = NULL;

    endDescriptor = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)FirstDescriptor + TotalLength);

    if (StartDescriptor >= endDescriptor ||
        NextDescriptor(StartDescriptor)>= endDescriptor)
    {
        return NULL;
    }

    if (DescriptorType == -1) // -1 means any type
    {
        return NextDescriptor(StartDescriptor);
    }

    currentDescriptor = StartDescriptor;

    while (((currentDescriptor = NextDescriptor(currentDescriptor)) < endDescriptor)
        && currentDescriptor != NULL)
    {
        if (currentDescriptor->bDescriptorType == (UCHAR)DescriptorType)
        {
            return currentDescriptor;
        }
    }
    return NULL;
}

PUSB_COMMON_DESCRIPTOR CUsbEnumer::NextDescriptor(_In_ PUSB_COMMON_DESCRIPTOR Descriptor)
{
    if (Descriptor->bLength == 0)
    {
        return NULL;
    }
    return (PUSB_COMMON_DESCRIPTOR)((PUCHAR)Descriptor + Descriptor->bLength);
}

CString CUsbEnumer::_GetDevPath( HDEVINFO hDevInfo, SP_DEVICE_INTERFACE_DATA stDeviceInterfaceData )
{
    CString sDevPath;
    PSP_DEVICE_INTERFACE_DETAIL_DATA pDetailData = NULL;

    // 读大小
    ULONG requiredLength = 0;
    BOOL bSuc = ::SetupDiGetDeviceInterfaceDetail(hDevInfo, &stDeviceInterfaceData, NULL, 0, &requiredLength, NULL);
    if (!bSuc && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        goto Exit0; 
    }

    // 申请内存
    pDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)ALLOC(requiredLength);
    if (NULL == pDetailData)
    {
        goto Exit0;        
    }

    // 取值
    pDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    bSuc = ::SetupDiGetDeviceInterfaceDetail(hDevInfo, &stDeviceInterfaceData, pDetailData, requiredLength, &requiredLength, NULL);
    if (bSuc)
    {
        sDevPath = (LPCTSTR)pDetailData->DevicePath;
    }

Exit0:
    if (pDetailData)
    {
        FREE(pDetailData);
        pDetailData = NULL;
    }

    return sDevPath;
}
