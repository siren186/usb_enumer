#include "stdafx.h"
#include "impl.h"

// List of enumerated host controllers.
//
LIST_ENTRY EnumeratedHCListHead = { &EnumeratedHCListHead, &EnumeratedHCListHead };


//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

//
//  PLIST_ENTRY
//  RemoveHeadList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveHeadList(ListHead) \
    (ListHead)->Flink; \
{RemoveEntryList((ListHead)->Flink)}

//
//  VOID
//  RemoveEntryList(
//      PLIST_ENTRY Entry
//      );
//

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink; \
    PLIST_ENTRY _EX_Flink; \
    _EX_Flink = (Entry)->Flink; \
    _EX_Blink = (Entry)->Blink; \
    _EX_Blink->Flink = _EX_Flink; \
    _EX_Flink->Blink = _EX_Blink; \
}

//
//  VOID
//  InsertTailList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink; \
    PLIST_ENTRY _EX_ListHead; \
    _EX_ListHead = (ListHead); \
    _EX_Blink = _EX_ListHead->Blink; \
    (Entry)->Flink = _EX_ListHead; \
    (Entry)->Blink = _EX_Blink; \
    _EX_Blink->Flink = (Entry); \
    _EX_ListHead->Blink = (Entry); \
}

PTSTR ConnectionStatuses[] =
{
    _T(""),                   // 0  - NoDeviceConnected
    _T(""),                   // 1  - DeviceConnected
    _T("FailedEnumeration"),  // 2  - DeviceFailedEnumeration
    _T("GeneralFailure"),     // 3  - DeviceGeneralFailure
    _T("Overcurrent"),        // 4  - DeviceCausedOvercurrent
    _T("NotEnoughPower"),     // 5  - DeviceNotEnoughPower
    _T("NotEnoughBandwidth"), // 6  - DeviceNotEnoughBandwidth
    _T("HubNestedTooDeeply"), // 7  - DeviceHubNestedTooDeeply
    _T("InLegacyHub"),        // 8  - DeviceInLegacyHub
    _T("Enumerating"),        // 9  - DeviceEnumerating
    _T("Reset")               // 10 - DeviceReset
};

VOID Impl::EnumerateHubPorts(HANDLE hHubDevice, ULONG NumPorts)

{
    ULONG       index = 0;
    BOOL        success = 0;
    HRESULT     hr = S_OK;
    CString sDrvKeyName;
    PUSB_DEVICE_PNP_STRINGS pDevProps;
    DWORD       dwSizeOfLeafName = 0;
    CHAR        leafName[512];
    int         icon = 0;

    PUSB_NODE_CONNECTION_INFORMATION_EX    connectionInfoEx;
    PUSB_PORT_CONNECTOR_PROPERTIES         pPortConnectorProps;
    USB_PORT_CONNECTOR_PROPERTIES          portConnectorProps;
    PUSB_DESCRIPTOR_REQUEST                configDesc;
    PUSB_NODE_CONNECTION_INFORMATION_EX_V2 connectionInfoExV2;

    // Loop over all ports of the hub.
    //
    // Port indices are 1 based, not 0 based.
    //
    for (index = 1; index <= NumPorts; index++)
    {
        ULONG nBytesEx;
        ULONG nBytes = 0;

        connectionInfoEx = NULL;
        pPortConnectorProps = NULL;
        ZeroMemory(&portConnectorProps, sizeof(portConnectorProps));
        configDesc = NULL;
        connectionInfoExV2 = NULL;
        pDevProps = NULL;
        ZeroMemory(leafName, sizeof(leafName));

        //
        // Allocate space to hold the connection info for this port.
        // For now, allocate it big enough to hold info for 30 pipes.
        //
        // Endpoint numbers are 0-15.  Endpoint number 0 is the standard
        // control endpoint which is not explicitly listed in the Configuration
        // Descriptor.  There can be an IN endpoint and an OUT endpoint at
        // endpoint numbers 1-15 so there can be a maximum of 30 endpoints
        // per device configuration.
        //
        // Should probably size this dynamically at some point.
        //

        nBytesEx = sizeof(USB_NODE_CONNECTION_INFORMATION_EX)+
            (sizeof(USB_PIPE_INFO)* 30);

        connectionInfoEx = (PUSB_NODE_CONNECTION_INFORMATION_EX)ALLOC(nBytesEx);

        if (connectionInfoEx == NULL)
        {
            break;
        }

        connectionInfoExV2 = (PUSB_NODE_CONNECTION_INFORMATION_EX_V2)
            ALLOC(sizeof(USB_NODE_CONNECTION_INFORMATION_EX_V2));

        if (connectionInfoExV2 == NULL)
        {
            FREE(connectionInfoEx);
            break;
        }

        //
        // Now query USBHUB for the structures
        // for this port.  This will tell us if a device is attached to this
        // port, among other things.
        // The fault tolerate code is executed first.
        //

        portConnectorProps.ConnectionIndex = index;

        success = DeviceIoControl(hHubDevice,
            IOCTL_USB_GET_PORT_CONNECTOR_PROPERTIES,
            &portConnectorProps,
            sizeof(USB_PORT_CONNECTOR_PROPERTIES),
            &portConnectorProps,
            sizeof(USB_PORT_CONNECTOR_PROPERTIES),
            &nBytes,
            NULL);

        if (success && nBytes == sizeof(USB_PORT_CONNECTOR_PROPERTIES))
        {
            pPortConnectorProps = (PUSB_PORT_CONNECTOR_PROPERTIES)
                ALLOC(portConnectorProps.ActualLength);

            if (pPortConnectorProps != NULL)
            {
                pPortConnectorProps->ConnectionIndex = index;

                success = DeviceIoControl(hHubDevice,
                    IOCTL_USB_GET_PORT_CONNECTOR_PROPERTIES,
                    pPortConnectorProps,
                    portConnectorProps.ActualLength,
                    pPortConnectorProps,
                    portConnectorProps.ActualLength,
                    &nBytes,
                    NULL);

                if (!success || nBytes < portConnectorProps.ActualLength)
                {
                    FREE(pPortConnectorProps);
                    pPortConnectorProps = NULL;
                }
            }
        }

        connectionInfoExV2->ConnectionIndex = index;
        connectionInfoExV2->Length = sizeof(USB_NODE_CONNECTION_INFORMATION_EX_V2);
        connectionInfoExV2->SupportedUsbProtocols.Usb300 = 1;

        success = DeviceIoControl(hHubDevice,
            IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX_V2,
            connectionInfoExV2,
            sizeof(USB_NODE_CONNECTION_INFORMATION_EX_V2),
            connectionInfoExV2,
            sizeof(USB_NODE_CONNECTION_INFORMATION_EX_V2),
            &nBytes,
            NULL);

        if (!success || nBytes < sizeof(USB_NODE_CONNECTION_INFORMATION_EX_V2))
        {
            FREE(connectionInfoExV2);
            connectionInfoExV2 = NULL;
        }

        connectionInfoEx->ConnectionIndex = index;

        success = DeviceIoControl(hHubDevice,
            IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX,
            connectionInfoEx,
            nBytesEx,
            connectionInfoEx,
            nBytesEx,
            &nBytesEx,
            NULL);

        if (success)
        {
            //
            // Since the USB_NODE_CONNECTION_INFORMATION_EX is used to display
            // the device speed, but the hub driver doesn't support indication
            // of superspeed, we overwrite the value if the super speed
            // data structures are available and indicate the device is operating
            // at SuperSpeed.
            // 

            if (connectionInfoEx->Speed == UsbHighSpeed
                && connectionInfoExV2 != NULL
                && connectionInfoExV2->Flags.DeviceIsOperatingAtSuperSpeedOrHigher)
            {
                connectionInfoEx->Speed = UsbSuperSpeed;
            }
        }
        else
        {
            PUSB_NODE_CONNECTION_INFORMATION    connectionInfo = NULL;

            // Try using IOCTL_USB_GET_NODE_CONNECTION_INFORMATION
            // instead of IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX
            //

            nBytes = sizeof(USB_NODE_CONNECTION_INFORMATION)+
                sizeof(USB_PIPE_INFO)* 30;

            connectionInfo = (PUSB_NODE_CONNECTION_INFORMATION)ALLOC(nBytes);

            if (connectionInfo == NULL)
            {
                FREE(connectionInfoEx);
                if (pPortConnectorProps != NULL)
                {
                    FREE(pPortConnectorProps);
                }
                if (connectionInfoExV2 != NULL)
                {
                    FREE(connectionInfoExV2);
                }
                continue;
            }

            connectionInfo->ConnectionIndex = index;

            success = DeviceIoControl(hHubDevice,
                IOCTL_USB_GET_NODE_CONNECTION_INFORMATION,
                connectionInfo,
                nBytes,
                connectionInfo,
                nBytes,
                &nBytes,
                NULL);

            if (!success)
            {
                FREE(connectionInfo);
                FREE(connectionInfoEx);
                if (pPortConnectorProps != NULL)
                {
                    FREE(pPortConnectorProps);
                }
                if (connectionInfoExV2 != NULL)
                {
                    FREE(connectionInfoExV2);
                }
                continue;
            }

            // Copy IOCTL_USB_GET_NODE_CONNECTION_INFORMATION into
            // IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX structure.
            //
            connectionInfoEx->ConnectionIndex = connectionInfo->ConnectionIndex;
            connectionInfoEx->DeviceDescriptor = connectionInfo->DeviceDescriptor;
            connectionInfoEx->CurrentConfigurationValue = connectionInfo->CurrentConfigurationValue;
            connectionInfoEx->Speed = connectionInfo->LowSpeed ? UsbLowSpeed : UsbFullSpeed;
            connectionInfoEx->DeviceIsHub = connectionInfo->DeviceIsHub;
            connectionInfoEx->DeviceAddress = connectionInfo->DeviceAddress;
            connectionInfoEx->NumberOfOpenPipes = connectionInfo->NumberOfOpenPipes;
            connectionInfoEx->ConnectionStatus = connectionInfo->ConnectionStatus;

            memcpy(&connectionInfoEx->PipeList[0],
                &connectionInfo->PipeList[0],
                sizeof(USB_PIPE_INFO)* 30);

            FREE(connectionInfo);
        }

        // Update the count of connected devices
        //
        if (connectionInfoEx->ConnectionStatus == DeviceConnected)
        {
            m_nTotalDevicesConnected++;
        }

        if (connectionInfoEx->DeviceIsHub)
        {
            m_nTotalHubs++;
        }

        // If there is a device connected, get the Device Description
        //
        if (connectionInfoEx->ConnectionStatus != NoDeviceConnected)
        {
            sDrvKeyName = GetDriverKeyName(hHubDevice, index);

            if (!sDrvKeyName.IsEmpty() &&
                sDrvKeyName.GetLength() < MAX_DRIVER_KEY_NAME)
            {
                pDevProps = DriverNameToDeviceProperties(sDrvKeyName);
            }
        }

        // If there is a device connected to the port, try to retrieve the
        // Configuration Descriptor from the device.
        //
        if (connectionInfoEx->ConnectionStatus == DeviceConnected)
        {
            configDesc = GetConfigDescriptor(hHubDevice, index, 0);
        }
        else
        {
            configDesc = NULL;
        }

        // If the device connected to the port is an external hub, get the
        // name of the external hub and recursively enumerate it.
        //
        if (connectionInfoEx->DeviceIsHub)
        {
            CString sExtHubName = GetExternalHubName(hHubDevice, index);
            if (!sExtHubName.IsEmpty() &&
                sExtHubName.GetLength() < MAX_DRIVER_KEY_NAME)
            {
                EnumerateHub(sExtHubName,
                    connectionInfoEx,
                    connectionInfoExV2,
                    pPortConnectorProps,
                    configDesc,
                    pDevProps);
            }
        }
        else
        {
            // Allocate some space for a USBDEVICEINFO structure to hold the
            // hub info, hub name, and connection info pointers.  GPTR zero
            // initializes the structure for us.
            //
            if (connectionInfoEx->ConnectionStatus == NoDeviceConnected)
            {
                if (connectionInfoExV2 != NULL &&
                    connectionInfoExV2->SupportedUsbProtocols.Usb300 == 1)
                {
                    icon = NoSsDeviceIcon;
                }
                else
                {
                    icon = NoDeviceIcon;
                }
            }
            else if (connectionInfoEx->CurrentConfigurationValue)
            {
                if (connectionInfoEx->Speed == UsbSuperSpeed)
                {
                    icon = GoodSsDeviceIcon;
                }
                else
                {
                    icon = GoodDeviceIcon;
                }
            }
            else
            {
                icon = BadDeviceIcon;
            }
        }
    } // for
}

CString Impl::GetDriverKeyName(HANDLE Hub, ULONG ConnectionIndex)
{
    CString sDrvKeyName;
    BOOL                                success = 0;
    ULONG                               nBytes = 0;
    USB_NODE_CONNECTION_DRIVERKEY_NAME  stDriverKeyName;
    PUSB_NODE_CONNECTION_DRIVERKEY_NAME pDriverKeyName = NULL;
    //PCHAR                               driverKeyNameA = NULL;

    // Get the length of the name of the driver key of the device attached to
    // the specified port.
    //
    stDriverKeyName.ConnectionIndex = ConnectionIndex;

    success = DeviceIoControl(Hub,
        IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME,
        &stDriverKeyName,
        sizeof(stDriverKeyName),
        &stDriverKeyName,
        sizeof(stDriverKeyName),
        &nBytes,
        NULL);

    if (!success)
    {
        goto Exit0;
    }

    // Allocate space to hold the driver key name
    //
    nBytes = stDriverKeyName.ActualLength;

    if (nBytes <= sizeof(stDriverKeyName))
    {
        goto Exit0;
    }

    pDriverKeyName = (PUSB_NODE_CONNECTION_DRIVERKEY_NAME)ALLOC(nBytes);
    if (pDriverKeyName == NULL)
    {
        goto Exit0;
    }

    // Get the name of the driver key of the device attached to
    // the specified port.
    //
    pDriverKeyName->ConnectionIndex = ConnectionIndex;

    success = DeviceIoControl(Hub,
        IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME,
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

    //driverKeyNameA = WideStrToMultiStr(driverKeyNameW->DriverKeyName, nBytes);
    // TODO: 这个地方, 赋值会不会只是赋了一个字符
    sDrvKeyName = (LPCTSTR)pDriverKeyName->DriverKeyName;

Exit0:
    if (pDriverKeyName != NULL)
    {
        FREE(pDriverKeyName);
        pDriverKeyName = NULL;
    }

    return sDrvKeyName;
}

/*****************************************************************************

DriverNameToDeviceProperties()

Returns the Device properties of the DevNode with the matching DriverName.
Returns NULL if the matching DevNode is not found.

The caller should free the returned structure using FREE() macro

*****************************************************************************/
PUSB_DEVICE_PNP_STRINGS Impl::DriverNameToDeviceProperties(const CString& sDrvKeyName)
{
    HDEVINFO        deviceInfo = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA deviceInfoData = { 0 };
    ULONG           len;
    BOOL            status;
    PUSB_DEVICE_PNP_STRINGS DevProps = NULL;
    DWORD           lastError;

    CString sTmpBuf;

    // Allocate device propeties structure
    DevProps = (PUSB_DEVICE_PNP_STRINGS)ALLOC(sizeof(USB_DEVICE_PNP_STRINGS));

    if (NULL == DevProps)
    {
        status = FALSE;
        goto Done;
    }

    // Get device instance
    status = DriverNameToDeviceInst(sDrvKeyName, &deviceInfo, &deviceInfoData);
    if (status == FALSE)
    {
        goto Done;
    }

    len = 0;
    status = SetupDiGetDeviceInstanceId(deviceInfo,
        &deviceInfoData,
        NULL,
        0,
        &len);
    lastError = GetLastError();


    if (status != FALSE && lastError != ERROR_INSUFFICIENT_BUFFER)
    {
        status = FALSE;
        goto Done;
    }

    //
    // An extra byte is required for the terminating character
    //

    len++;

    status = SetupDiGetDeviceInstanceId(deviceInfo,
        &deviceInfoData,
        DevProps->DeviceId,
        MAX_DRIVER_KEY_NAME,
        &len);
    if (status == FALSE)
    {
        goto Done;
    }

    sTmpBuf = GetDeviceProperty(deviceInfo,
        &deviceInfoData,
        SPDRP_DEVICEDESC);
    if (!sTmpBuf.IsEmpty())
    {
        wcsncpy_s(DevProps->DeviceDesc, MAX_DRIVER_KEY_NAME, sTmpBuf, sTmpBuf.GetLength());
    }

    //    
    // We don't fail if the following registry query fails as these fields are additional information only
    //

    sTmpBuf = GetDeviceProperty(deviceInfo,
        &deviceInfoData,
        SPDRP_HARDWAREID);
    if (!sTmpBuf.IsEmpty())
    {
        wcsncpy_s(DevProps->HwId, MAX_DRIVER_KEY_NAME, sTmpBuf, sTmpBuf.GetLength());
    }

    sTmpBuf = GetDeviceProperty(deviceInfo,
        &deviceInfoData,
        SPDRP_SERVICE);
    if (!sTmpBuf.IsEmpty())
    {
        wcsncpy_s(DevProps->Service, MAX_DRIVER_KEY_NAME, sTmpBuf, sTmpBuf.GetLength());
    }

    GetDeviceProperty(deviceInfo,
        &deviceInfoData,
        SPDRP_CLASS);
    if (!sTmpBuf.IsEmpty())
    {
        wcsncpy_s(DevProps->DeviceClass, MAX_DRIVER_KEY_NAME, sTmpBuf, sTmpBuf.GetLength());
    }
Done:

    if (deviceInfo != INVALID_HANDLE_VALUE)
    {
        SetupDiDestroyDeviceInfoList(deviceInfo);
    }

    if (status == FALSE)
    {
        if (DevProps != NULL)
        {
            FreeDeviceProperties(&DevProps);
        }
    }
    return DevProps;
}

/*****************************************************************************

DriverNameToDeviceInst()

Finds the Device instance of the DevNode with the matching DriverName.
Returns FALSE if the matching DevNode is not found and TRUE if found

*****************************************************************************/
BOOL Impl::DriverNameToDeviceInst(const CString& sDrvKeyName, _Out_ HDEVINFO *pDevInfo, _Out_writes_bytes_(sizeof(SP_DEVINFO_DATA)) PSP_DEVINFO_DATA pDevInfoData)
{
    HDEVINFO         deviceInfo = INVALID_HANDLE_VALUE;
    BOOL             status = TRUE;
    ULONG            deviceIndex;
    SP_DEVINFO_DATA  deviceInfoData;
    BOOL             bResult = FALSE;
    TCHAR szBuf[MAX_DRIVER_KEY_NAME] = { 0 };
    BOOL             done = FALSE;

    if (pDevInfo == NULL || pDevInfoData == NULL)
    {
        return FALSE;
    }

    memset(pDevInfoData, 0, sizeof(SP_DEVINFO_DATA));

    *pDevInfo = INVALID_HANDLE_VALUE;

    //
    // We cannot walk the device tree with CM_Get_Sibling etc. unless we assume
    // the device tree will stabilize. Any devnode removal (even outside of USB)
    // would force us to retry. Instead we use Setup API to snapshot all
    // devices.
    //

    // Examine all present devices to see if any match the given DriverName
    //
    deviceInfo = SetupDiGetClassDevs(NULL,
        NULL,
        NULL,
        DIGCF_ALLCLASSES | DIGCF_PRESENT);

    if (deviceInfo == INVALID_HANDLE_VALUE)
    {
        status = FALSE;
        goto Done;
    }

    deviceIndex = 0;
    deviceInfoData.cbSize = sizeof(deviceInfoData);

    while (done == FALSE)
    {
        //
        // Get devinst of the next device
        //

        status = SetupDiEnumDeviceInfo(deviceInfo,
            deviceIndex,
            &deviceInfoData);

        deviceIndex++;

        if (!status)
        {
            //
            // This could be an error, or indication that all devices have been
            // processed. Either way the desired device was not found.
            //

            done = TRUE;
            break;
        }

        //
        // Get the DriverName value
        //

        CString sTmpBuf = GetDeviceProperty(deviceInfo,
            &deviceInfoData,
            SPDRP_DRIVER);

        if (!sTmpBuf.IsEmpty())
        {
            bResult = TRUE;
        }

        // If the DriverName value matches, return the DeviceInstance
        //
        if (!sTmpBuf.IsEmpty() && sDrvKeyName.CompareNoCase(sTmpBuf) == 0)
        {
            done = TRUE;
            *pDevInfo = deviceInfo;
            CopyMemory(pDevInfoData, &deviceInfoData, sizeof(deviceInfoData));
            break;
        }
    }

Done:

    if (bResult == FALSE)
    {
        if (deviceInfo != INVALID_HANDLE_VALUE)
        {
            SetupDiDestroyDeviceInfoList(deviceInfo);
        }
    }

    return status;
}

CString Impl::GetDeviceProperty(
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


VOID Impl::FreeDeviceProperties(_In_ PUSB_DEVICE_PNP_STRINGS *ppDevProps)
{
    if (ppDevProps == NULL)
    {
        return;
    }

    if (*ppDevProps == NULL)
    {
        return;
    }

    //
    // The following are not necessary, but left in case
    // in the future there is a later failure where these
    // pointer fields would be allocated.
    //

    FREE(*ppDevProps);
    *ppDevProps = NULL;
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

PUSB_DESCRIPTOR_REQUEST Impl::GetConfigDescriptor(HANDLE hHubDevice, ULONG ConnectionIndex, UCHAR DescriptorIndex)
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



//*****************************************************************************
//
// GetBOSDescriptor()
//
// hHubDevice - Handle of the hub device containing the port from which the
// Configuration Descriptor will be requested.
//
// ConnectionIndex - Identifies the port on the hub to which a device is
// attached from which the BOS Descriptor will be requested.
//
//*****************************************************************************

PUSB_DESCRIPTOR_REQUEST Impl::GetBOSDescriptor(HANDLE hHubDevice, ULONG ConnectionIndex)

{
    BOOL    success = 0;
    ULONG   nBytes = 0;
    ULONG   nBytesReturned = 0;

    UCHAR   bosDescReqBuf[sizeof(USB_DESCRIPTOR_REQUEST)+
        sizeof(USB_BOS_DESCRIPTOR)];

    PUSB_DESCRIPTOR_REQUEST bosDescReq = NULL;
    PUSB_BOS_DESCRIPTOR     bosDesc = NULL;


    // Request the BOS Descriptor the first time using our
    // local buffer, which is just big enough for the BOS
    // Descriptor itself.
    //
    nBytes = sizeof(bosDescReqBuf);

    bosDescReq = (PUSB_DESCRIPTOR_REQUEST)bosDescReqBuf;
    bosDesc = (PUSB_BOS_DESCRIPTOR)(bosDescReq + 1);

    // Zero fill the entire request structure
    //
    memset(bosDescReq, 0, nBytes);

    // Indicate the port from which the descriptor will be requested
    //
    bosDescReq->ConnectionIndex = ConnectionIndex;

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
    bosDescReq->SetupPacket.wValue = (USB_BOS_DESCRIPTOR_TYPE << 8);

    bosDescReq->SetupPacket.wLength = (USHORT)(nBytes - sizeof(USB_DESCRIPTOR_REQUEST));

    // Now issue the get descriptor request.
    //
    success = DeviceIoControl(hHubDevice,
        IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
        bosDescReq,
        nBytes,
        bosDescReq,
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

    if (bosDesc->wTotalLength < sizeof(USB_BOS_DESCRIPTOR))
    {
        return NULL;
    }

    // Now request the entire BOS Descriptor using a dynamically
    // allocated buffer which is sized big enough to hold the entire descriptor
    //
    nBytes = sizeof(USB_DESCRIPTOR_REQUEST)+bosDesc->wTotalLength;

    bosDescReq = (PUSB_DESCRIPTOR_REQUEST)ALLOC(nBytes);

    if (bosDescReq == NULL)
    {
        return NULL;
    }

    bosDesc = (PUSB_BOS_DESCRIPTOR)(bosDescReq + 1);

    // Indicate the port from which the descriptor will be requested
    //
    bosDescReq->ConnectionIndex = ConnectionIndex;

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
    bosDescReq->SetupPacket.wValue = (USB_BOS_DESCRIPTOR_TYPE << 8);

    bosDescReq->SetupPacket.wLength = (USHORT)(nBytes - sizeof(USB_DESCRIPTOR_REQUEST));

    // Now issue the get descriptor request.
    //

    success = DeviceIoControl(hHubDevice,
        IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
        bosDescReq,
        nBytes,
        bosDescReq,
        nBytes,
        &nBytesReturned,
        NULL);

    if (!success)
    {
        FREE(bosDescReq);
        return NULL;
    }

    if (nBytes != nBytesReturned)
    {
        FREE(bosDescReq);
        return NULL;
    }

    if (bosDesc->wTotalLength != (nBytes - sizeof(USB_DESCRIPTOR_REQUEST)))
    {
        FREE(bosDescReq);
        return NULL;
    }

    return bosDescReq;
}




//*****************************************************************************
//
// AreThereStringDescriptors()
//
// DeviceDesc - Device Descriptor for which String Descriptors should be
// checked.
//
// ConfigDesc - Configuration Descriptor (also containing Interface Descriptor)
// for which String Descriptors should be checked.
//
//*****************************************************************************

BOOL Impl::AreThereStringDescriptors(PUSB_DEVICE_DESCRIPTOR DeviceDesc, PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc)

{
    PUCHAR                  descEnd = NULL;
    PUSB_COMMON_DESCRIPTOR  commonDesc = NULL;

    //
    // Check Device Descriptor strings
    //

    if (DeviceDesc->iManufacturer ||
        DeviceDesc->iProduct ||
        DeviceDesc->iSerialNumber
        )
    {
        return TRUE;
    }


    //
    // Check the Configuration and Interface Descriptor strings
    //

    descEnd = (PUCHAR)ConfigDesc + ConfigDesc->wTotalLength;

    commonDesc = (PUSB_COMMON_DESCRIPTOR)ConfigDesc;

    while ((PUCHAR)commonDesc + sizeof(USB_COMMON_DESCRIPTOR) < descEnd &&
        (PUCHAR)commonDesc + commonDesc->bLength <= descEnd)
    {
        switch (commonDesc->bDescriptorType)
        {
        case USB_CONFIGURATION_DESCRIPTOR_TYPE:
        case USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_TYPE:
            if (commonDesc->bLength != sizeof(USB_CONFIGURATION_DESCRIPTOR))
            {
                break;
            }
            if (((PUSB_CONFIGURATION_DESCRIPTOR)commonDesc)->iConfiguration)
            {
                return TRUE;
            }
            commonDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)commonDesc + commonDesc->bLength);
            continue;

        case USB_INTERFACE_DESCRIPTOR_TYPE:
            if (commonDesc->bLength != sizeof(USB_INTERFACE_DESCRIPTOR) &&
                commonDesc->bLength != sizeof(USB_INTERFACE_DESCRIPTOR2))
            {
                break;
            }
            if (((PUSB_INTERFACE_DESCRIPTOR)commonDesc)->iInterface)
            {
                return TRUE;
            }
            commonDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)commonDesc + commonDesc->bLength);
            continue;

        default:
            commonDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)commonDesc + commonDesc->bLength);
            continue;
        }
        break;
    }

    return FALSE;
}

//*****************************************************************************
//
// GetAllStringDescriptors()
//
// hHubDevice - Handle of the hub device containing the port from which the
// String Descriptors will be requested.
//
// ConnectionIndex - Identifies the port on the hub to which a device is
// attached from which the String Descriptors will be requested.
//
// DeviceDesc - Device Descriptor for which String Descriptors should be
// requested.
//
// ConfigDesc - Configuration Descriptor (also containing Interface Descriptor)
// for which String Descriptors should be requested.
//
//*****************************************************************************

PSTRING_DESCRIPTOR_NODE Impl::GetAllStringDescriptors(HANDLE hHubDevice, ULONG ConnectionIndex, PUSB_DEVICE_DESCRIPTOR DeviceDesc, PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc)

{
    PSTRING_DESCRIPTOR_NODE supportedLanguagesString = NULL;
    ULONG                   numLanguageIDs = 0;
    USHORT                  *languageIDs = NULL;

    PUCHAR                  descEnd = NULL;
    PUSB_COMMON_DESCRIPTOR  commonDesc = NULL;
    UCHAR                   uIndex = 1;
    UCHAR                   bInterfaceClass = 0;
    BOOL                    getMoreStrings = FALSE;
    HRESULT                 hr = S_OK;

    //
    // Get the array of supported Language IDs, which is returned
    // in String Descriptor 0
    //
    supportedLanguagesString = GetStringDescriptor(hHubDevice,
        ConnectionIndex,
        0,
        0);

    if (supportedLanguagesString == NULL)
    {
        return NULL;
    }

    numLanguageIDs = (supportedLanguagesString->StringDescriptor->bLength - 2) / 2;

    languageIDs = (USHORT *)&supportedLanguagesString->StringDescriptor->bString[0];

    //
    // Get the Device Descriptor strings
    //

    if (DeviceDesc->iManufacturer)
    {
        GetStringDescriptors(hHubDevice,
            ConnectionIndex,
            DeviceDesc->iManufacturer,
            numLanguageIDs,
            languageIDs,
            supportedLanguagesString);
    }

    if (DeviceDesc->iProduct)
    {
        GetStringDescriptors(hHubDevice,
            ConnectionIndex,
            DeviceDesc->iProduct,
            numLanguageIDs,
            languageIDs,
            supportedLanguagesString);
    }

    if (DeviceDesc->iSerialNumber)
    {
        GetStringDescriptors(hHubDevice,
            ConnectionIndex,
            DeviceDesc->iSerialNumber,
            numLanguageIDs,
            languageIDs,
            supportedLanguagesString);
    }

    //
    // Get the Configuration and Interface Descriptor strings
    //

    descEnd = (PUCHAR)ConfigDesc + ConfigDesc->wTotalLength;

    commonDesc = (PUSB_COMMON_DESCRIPTOR)ConfigDesc;

    while ((PUCHAR)commonDesc + sizeof(USB_COMMON_DESCRIPTOR) < descEnd &&
        (PUCHAR)commonDesc + commonDesc->bLength <= descEnd)
    {
        switch (commonDesc->bDescriptorType)
        {
        case USB_CONFIGURATION_DESCRIPTOR_TYPE:
            if (commonDesc->bLength != sizeof(USB_CONFIGURATION_DESCRIPTOR))
            {
                break;
            }
            if (((PUSB_CONFIGURATION_DESCRIPTOR)commonDesc)->iConfiguration)
            {
                GetStringDescriptors(hHubDevice,
                    ConnectionIndex,
                    ((PUSB_CONFIGURATION_DESCRIPTOR)commonDesc)->iConfiguration,
                    numLanguageIDs,
                    languageIDs,
                    supportedLanguagesString);
            }
            commonDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)commonDesc + commonDesc->bLength);
            continue;

        case USB_IAD_DESCRIPTOR_TYPE:
            if (commonDesc->bLength < sizeof(USB_IAD_DESCRIPTOR))
            {
                break;
            }
            if (((PUSB_IAD_DESCRIPTOR)commonDesc)->iFunction)
            {
                GetStringDescriptors(hHubDevice,
                    ConnectionIndex,
                    ((PUSB_IAD_DESCRIPTOR)commonDesc)->iFunction,
                    numLanguageIDs,
                    languageIDs,
                    supportedLanguagesString);
            }
            commonDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)commonDesc + commonDesc->bLength);
            continue;

        case USB_INTERFACE_DESCRIPTOR_TYPE:
            if (commonDesc->bLength != sizeof(USB_INTERFACE_DESCRIPTOR) &&
                commonDesc->bLength != sizeof(USB_INTERFACE_DESCRIPTOR2))
            {
                break;
            }
            if (((PUSB_INTERFACE_DESCRIPTOR)commonDesc)->iInterface)
            {
                GetStringDescriptors(hHubDevice,
                    ConnectionIndex,
                    ((PUSB_INTERFACE_DESCRIPTOR)commonDesc)->iInterface,
                    numLanguageIDs,
                    languageIDs,
                    supportedLanguagesString);
            }

            //
            // We need to display more string descriptors for the following
            // interface classes
            //
            bInterfaceClass = ((PUSB_INTERFACE_DESCRIPTOR)commonDesc)->bInterfaceClass;
            if (bInterfaceClass == USB_DEVICE_CLASS_VIDEO)
            {
                getMoreStrings = TRUE;
            }
            commonDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)commonDesc + commonDesc->bLength);
            continue;

        default:
            commonDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)commonDesc + commonDesc->bLength);
            continue;
        }
        break;
    }

    if (getMoreStrings)
    {
        //
        // We might need to display strings later that are referenced only in
        // class-specific descriptors. Get String Descriptors 1 through 32 (an
        // arbitrary upper limit for Strings needed due to "bad devices"
        // returning an infinite repeat of Strings 0 through 4) until one is not
        // found.
        //
        // There are also "bad devices" that have issues even querying 1-32, but
        // historically USBView made this query, so the query should be safe for
        // video devices.
        //
        for (uIndex = 1; SUCCEEDED(hr) && (uIndex < NUM_STRING_DESC_TO_GET); uIndex++)
        {
            hr = GetStringDescriptors(hHubDevice,
                ConnectionIndex,
                uIndex,
                numLanguageIDs,
                languageIDs,
                supportedLanguagesString);
        }
    }

    return supportedLanguagesString;
}

CString Impl::GetExternalHubName(HANDLE Hub, ULONG ConnectionIndex)
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
    //extHubNameA = WideStrToMultiStr(extHubNameW->NodeName, nBytes);
    // TODO: 这个地方的赋值会不会只赋了一个字符?
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

//*****************************************************************************
//
// EnumerateHostController()
//
// hTreeParent - Handle of the TreeView item under which host controllers
// should be added.
//
//*****************************************************************************

VOID Impl::EnumerateHostController(
HANDLE                   hHCDev,
_In_    HANDLE           hDeviceInfo,
_In_    PSP_DEVINFO_DATA deviceInfoData
)
{
    CString sDrvKeyName;
    CString sRootHubName;
    PLIST_ENTRY             listEntry = NULL;
    PUSBHOSTCONTROLLERINFO  hcInfo = NULL;
    PUSBHOSTCONTROLLERINFO  hcInfoInList = NULL;
    DWORD                   dwSuccess;
    BOOL                    success = FALSE;
    ULONG                   deviceAndFunction = 0;
    PUSB_DEVICE_PNP_STRINGS DevProps = NULL;


    // Allocate a structure to hold information about this host controller.
    //
    hcInfo = (PUSBHOSTCONTROLLERINFO)ALLOC(sizeof(USBHOSTCONTROLLERINFO));

    // just return if could not alloc memory
    if (NULL == hcInfo)
        return;

    hcInfo->DeviceInfoType = HostControllerInfo;

    // Obtain the driver key name for this host controller.
    //
    sDrvKeyName = GetHCDDriverKeyName(hHCDev);

    if (sDrvKeyName.IsEmpty())
    {
        // Failure obtaining driver key name.
        FREE(hcInfo);
        return;
    }
    wcscpy_s(hcInfo->DriverKey, MAX_DRIVER_KEY_NAME, sDrvKeyName);

    // Don't enumerate this host controller again if it already
    // on the list of enumerated host controllers.
    //
    listEntry = EnumeratedHCListHead.Flink;

    while (listEntry != &EnumeratedHCListHead)
    {
        hcInfoInList = CONTAINING_RECORD(listEntry,
            USBHOSTCONTROLLERINFO,
            ListEntry);

        if (sDrvKeyName.CompareNoCase(hcInfoInList->DriverKey) == 0)
        {
            // Already on the list, exit
            //
            FREE(hcInfo);
            return;
        }

        listEntry = listEntry->Flink;
    }

    // Obtain host controller device properties
    // 此处可以获取usb host的名称,见DevProps.DeviceDesc字段
    DevProps = DriverNameToDeviceProperties(sDrvKeyName);

    if (DevProps)
    {
        ULONG   ven, dev, subsys, rev;
        ven = dev = subsys = rev = 0;

        if (swscanf_s(DevProps->DeviceId,
            _T("PCI\\VEN_%x&DEV_%x&SUBSYS_%x&REV_%x"),
            &ven, &dev, &subsys, &rev) != 4)
        {
        }

        hcInfo->VendorID = ven;
        hcInfo->DeviceID = dev;
        hcInfo->SubSysID = subsys;
        hcInfo->Revision = rev;
        hcInfo->UsbDeviceProperties = DevProps;
    }

    // Get the USB Host Controller power map
    dwSuccess = GetHostControllerPowerMap(hHCDev, hcInfo);

    // Get bus, device, and function
    //
    hcInfo->BusDeviceFunctionValid = FALSE;

    success = SetupDiGetDeviceRegistryProperty(hDeviceInfo,
        deviceInfoData,
        SPDRP_BUSNUMBER,
        NULL,
        (PBYTE)&hcInfo->BusNumber,
        sizeof(hcInfo->BusNumber),
        NULL);

    if (success)
    {
        success = SetupDiGetDeviceRegistryProperty(hDeviceInfo,
            deviceInfoData,
            SPDRP_ADDRESS,
            NULL,
            (PBYTE)&deviceAndFunction,
            sizeof(deviceAndFunction),
            NULL);
    }

    if (success)
    {
        hcInfo->BusDevice = (USHORT)(deviceAndFunction >> 16);
        hcInfo->BusFunction = (USHORT)(deviceAndFunction & 0xffff);
        hcInfo->BusDeviceFunctionValid = TRUE;
    }

    // Get the USB Host Controller info
    dwSuccess = GetHostControllerInfo(hHCDev, hcInfo);

    // Add this host controller to the list of enumerated
    // host controllers.
    //
    InsertTailList(&EnumeratedHCListHead,
        &hcInfo->ListEntry);

    // Get the name of the root hub for this host
    // controller and then enumerate the root hub.
    //
    sRootHubName = GetRootHubName(hHCDev);

    if (!sRootHubName.IsEmpty() &&
        sRootHubName.GetLength() < MAX_DRIVER_KEY_NAME)
    {
        EnumerateHub(
            sRootHubName,
            NULL,       // ConnectionInfo
            NULL,       // ConnectionInfoV2
            NULL,       // PortConnectorProps
            NULL,       // ConfigDesc
            NULL);      // We do not pass DevProps for RootHub
    }
    return;
}


VOID Impl::EnumerateHub(
const CString&                                  HubName,
_In_opt_ PUSB_NODE_CONNECTION_INFORMATION_EX    ConnectionInfo,
_In_opt_ PUSB_NODE_CONNECTION_INFORMATION_EX_V2 ConnectionInfoV2,
_In_opt_ PUSB_PORT_CONNECTOR_PROPERTIES         PortConnectorProps,
_In_opt_ PUSB_DESCRIPTOR_REQUEST                ConfigDesc,
_In_opt_ PUSB_DEVICE_PNP_STRINGS                DevProps
)
{
    // Initialize locals to not allocated state so the error cleanup routine
    // only tries to cleanup things that were successfully allocated.
    //
    PUSB_NODE_INFORMATION    hubInfo = NULL;
    PUSB_HUB_INFORMATION_EX  hubInfoEx = NULL;
    PUSB_HUB_CAPABILITIES_EX hubCapabilityEx = NULL;
    HANDLE                  hHubDevice = INVALID_HANDLE_VALUE;
    //PCHAR                 deviceName = NULL;
    CString sFullHubName;
    ULONG                   nBytes = 0;
    BOOL                    success = 0;
    DWORD                   dwSizeOfLeafName = 0;
    //CHAR                    leafName[512] = { 0 };
    HRESULT                 hr = S_OK;
    //size_t                  cchHeader = 0;
    //size_t                  cchFullHubName = 0;

    // Allocate some space for a USB_NODE_INFORMATION structure for this Hub
    //
    hubInfo = (PUSB_NODE_INFORMATION)ALLOC(sizeof(USB_NODE_INFORMATION));
    if (hubInfo == NULL)
    {
        goto EnumerateHubError;
    }

    hubInfoEx = (PUSB_HUB_INFORMATION_EX)ALLOC(sizeof(USB_HUB_INFORMATION_EX));
    if (hubInfoEx == NULL)
    {
        goto EnumerateHubError;
    }

    hubCapabilityEx = (PUSB_HUB_CAPABILITIES_EX)ALLOC(sizeof(USB_HUB_CAPABILITIES_EX));
    if (hubCapabilityEx == NULL)
    {
        goto EnumerateHubError;
    }

    // 
    //     // Allocate a temp buffer for the full hub device name.
    //     //
    //     hr = StringCbLength("\\\\.\\", MAX_DEVICE_PROP, &cchHeader);
    //     if (FAILED(hr))
    //     {
    //         goto EnumerateHubError;
    //     }
    //     cchFullHubName = cchHeader + cbHubName + 1;
    //     deviceName = (PCHAR)ALLOC((DWORD)cchFullHubName);
    //     if (deviceName == NULL)
    //     {
    //         OOPS();
    //         goto EnumerateHubError;
    //     }
    // 
    //     // Create the full hub device name
    //     //
    //     hr = StringCchCopyN(deviceName, cchFullHubName, "\\\\.\\", cchHeader);
    //     if (FAILED(hr))
    //     {
    //         goto EnumerateHubError;
    //     }
    //     hr = StringCchCatN(deviceName, cchFullHubName, HubName, cbHubName);
    //     if (FAILED(hr))
    //     {
    //         goto EnumerateHubError;
    //     }

    // Allocate a temp buffer for the full hub device name.
    //
    sFullHubName.Format(_T("\\\\.\\%s"), HubName);
    // Try to hub the open device
    //
    hHubDevice = CreateFile(sFullHubName,
        GENERIC_WRITE,
        FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    // Done with temp buffer for full hub device name
    //
    //FREE(deviceName);
    if (hHubDevice == INVALID_HANDLE_VALUE)
    {
        goto EnumerateHubError;
    }

    //
    // Now query USBHUB for the USB_NODE_INFORMATION structure for this hub.
    // This will tell us the number of downstream ports to enumerate, among
    // other things.
    //
    success = DeviceIoControl(hHubDevice,
        IOCTL_USB_GET_NODE_INFORMATION,
        hubInfo,
        sizeof(USB_NODE_INFORMATION),
        hubInfo,
        sizeof(USB_NODE_INFORMATION),
        &nBytes,
        NULL);

    if (!success)
    {
        goto EnumerateHubError;
    }

    success = DeviceIoControl(hHubDevice,
        IOCTL_USB_GET_HUB_INFORMATION_EX,
        hubInfoEx,
        sizeof(USB_HUB_INFORMATION_EX),
        hubInfoEx,
        sizeof(USB_HUB_INFORMATION_EX),
        &nBytes,
        NULL);

    //
    // Fail gracefully for downlevel OS's from Win8
    //
    if (!success || nBytes < sizeof(USB_HUB_INFORMATION_EX))
    {
        FREE(hubInfoEx);
        hubInfoEx = NULL;
    }

    //
    // Obtain Hub Capabilities
    //
    success = DeviceIoControl(hHubDevice,
        IOCTL_USB_GET_HUB_CAPABILITIES_EX,
        hubCapabilityEx,
        sizeof(USB_HUB_CAPABILITIES_EX),
        hubCapabilityEx,
        sizeof(USB_HUB_CAPABILITIES_EX),
        &nBytes,
        NULL);

    //
    // Fail gracefully
    //
    if (!success || nBytes < sizeof(USB_HUB_CAPABILITIES_EX))
    {
        FREE(hubCapabilityEx);
        hubCapabilityEx = NULL;
    }
    // Now recursively enumerate the ports of this hub.
    //
    EnumerateHubPorts(hHubDevice, hubInfo->u.HubInformation.HubDescriptor.bNumberOfPorts);


    CloseHandle(hHubDevice);
    return;

EnumerateHubError:
    //
    // Clean up any stuff that got allocated
    //

    if (hHubDevice != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hHubDevice);
        hHubDevice = INVALID_HANDLE_VALUE;
    }

    if (hubInfo)
    {
        FREE(hubInfo);
    }

    if (hubInfoEx)
    {
        FREE(hubInfoEx);
    }

    if (ConnectionInfo)
    {
        FREE(ConnectionInfo);
    }

    if (ConfigDesc)
    {
        FREE(ConfigDesc);
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

PSTRING_DESCRIPTOR_NODE Impl::GetStringDescriptor(
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
    PSTRING_DESCRIPTOR_NODE stringDescNode = NULL;

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

    stringDescNode = (PSTRING_DESCRIPTOR_NODE)ALLOC(sizeof(STRING_DESCRIPTOR_NODE)+
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



//*****************************************************************************
//
// GetStringDescriptors()
//
// hHubDevice - Handle of the hub device containing the port from which the
// String Descriptor will be requested.
//
// ConnectionIndex - Identifies the port on the hub to which a device is
// attached from which the String Descriptor will be requested.
//
// DescriptorIndex - String Descriptor index.
//
// NumLanguageIDs -  Number of languages in which the string should be
// requested.
//
// LanguageIDs - Languages in which the string should be requested.
//
// StringDescNodeHead - First node in linked list of device's string descriptors
//
// Return Value: HRESULT indicating whether the string is on the list
//
//*****************************************************************************

HRESULT Impl::GetStringDescriptors(
_In_ HANDLE                         hHubDevice,
_In_ ULONG                          ConnectionIndex,
_In_ UCHAR                          DescriptorIndex,
_In_ ULONG                          NumLanguageIDs,
_In_reads_(NumLanguageIDs) USHORT  *LanguageIDs,
_In_ PSTRING_DESCRIPTOR_NODE        StringDescNodeHead
)

{
    PSTRING_DESCRIPTOR_NODE tail = NULL;
    PSTRING_DESCRIPTOR_NODE trailing = NULL;
    ULONG i = 0;

    //
    // Go to the end of the linked list, searching for the requested index to
    // see if we've already retrieved it
    //
    for (tail = StringDescNodeHead; tail != NULL; tail = tail->Next)
    {
        if (tail->DescriptorIndex == DescriptorIndex)
        {
            return S_OK;
        }

        trailing = tail;
    }

    tail = trailing;

    //
    // Get the next String Descriptor. If this is NULL, then we're done (return)
    // Otherwise, loop through all Language IDs
    //
    for (i = 0; (tail != NULL) && (i < NumLanguageIDs); i++)
    {
        tail->Next = GetStringDescriptor(hHubDevice,
            ConnectionIndex,
            DescriptorIndex,
            LanguageIDs[i]);

        tail = tail->Next;
    }

    if (tail == NULL)
    {
        return E_FAIL;
    }
    else {
        return S_OK;
    }
}


CString Impl::GetHCDDriverKeyName(HANDLE HCD)
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

//*****************************************************************************
//
// GetHostControllerPowerMap()
//
// HANDLE hHCDev
//      - handle to USB Host Controller
//
// PUSBHOSTCONTROLLERINFO hcInfo
//      - data structure to receive the Power Map Info
//
// return DWORD dwError
//      - return ERROR_SUCCESS or last error
//
//*****************************************************************************

DWORD Impl::GetHostControllerPowerMap(
HANDLE hHCDev,
PUSBHOSTCONTROLLERINFO hcInfo)
{
    USBUSER_POWER_INFO_REQUEST UsbPowerInfoRequest;
    PUSB_POWER_INFO            pUPI = &UsbPowerInfoRequest.PowerInformation;
    DWORD                      dwError = 0;
    DWORD                      dwBytes = 0;
    BOOL                       bSuccess = FALSE;
    int                        nIndex = 0;
    int                        nPowerState = WdmUsbPowerSystemWorking;

    for (; nPowerState <= WdmUsbPowerSystemShutdown; nIndex++, nPowerState++)
    {
        // zero initialize our request
        memset(&UsbPowerInfoRequest, 0, sizeof(UsbPowerInfoRequest));

        // set the header and request sizes
        UsbPowerInfoRequest.Header.UsbUserRequest = USBUSER_GET_POWER_STATE_MAP;
        UsbPowerInfoRequest.Header.RequestBufferLength = sizeof(UsbPowerInfoRequest);
        UsbPowerInfoRequest.PowerInformation.SystemState = (WDMUSB_POWER_STATE)nPowerState;

        //
        // Now query USBHUB for the USB_POWER_INFO structure for this hub.
        // For Selective Suspend support
        //
        bSuccess = DeviceIoControl(hHCDev,
            IOCTL_USB_USER_REQUEST,
            &UsbPowerInfoRequest,
            sizeof(UsbPowerInfoRequest),
            &UsbPowerInfoRequest,
            sizeof(UsbPowerInfoRequest),
            &dwBytes,
            NULL);

        if (!bSuccess)
        {
            dwError = GetLastError();
        }
        else
        {
            // copy the data into our USB Host Controller's info structure
            memcpy(&(hcInfo->USBPowerInfo[nIndex]), pUPI, sizeof(USB_POWER_INFO));
        }
    }

    return dwError;
}


//*****************************************************************************
//
// GetHostControllerInfo()
//
// HANDLE hHCDev
//      - handle to USB Host Controller
//
// PUSBHOSTCONTROLLERINFO hcInfo
//      - data structure to receive the Power Map Info
//
// return DWORD dwError
//      - return ERROR_SUCCESS or last error
//
//*****************************************************************************

DWORD Impl::GetHostControllerInfo(
HANDLE hHCDev,
PUSBHOSTCONTROLLERINFO hcInfo)
{
    USBUSER_CONTROLLER_INFO_0 UsbControllerInfo;
    DWORD                      dwError = 0;
    DWORD                      dwBytes = 0;
    BOOL                       bSuccess = FALSE;

    memset(&UsbControllerInfo, 0, sizeof(UsbControllerInfo));

    // set the header and request sizes
    UsbControllerInfo.Header.UsbUserRequest = USBUSER_GET_CONTROLLER_INFO_0;
    UsbControllerInfo.Header.RequestBufferLength = sizeof(UsbControllerInfo);

    //
    // Query for the USB_CONTROLLER_INFO_0 structure
    //
    bSuccess = DeviceIoControl(hHCDev,
        IOCTL_USB_USER_REQUEST,
        &UsbControllerInfo,
        sizeof(UsbControllerInfo),
        &UsbControllerInfo,
        sizeof(UsbControllerInfo),
        &dwBytes,
        NULL);

    if (!bSuccess)
    {
        dwError = GetLastError();
    }
    else
    {
        hcInfo->ControllerInfo = (PUSB_CONTROLLER_INFO_0)ALLOC(sizeof(USB_CONTROLLER_INFO_0));
        if (NULL == hcInfo->ControllerInfo)
        {
            dwError = GetLastError();
        }
        else
        {
            // copy the data into our USB Host Controller's info structure
            memcpy(hcInfo->ControllerInfo, &UsbControllerInfo.Info0, sizeof(USB_CONTROLLER_INFO_0));
        }
    }
    return dwError;
}

CString Impl::GetRootHubName(HANDLE HostController)
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

VOID Impl::EnumerateHostControllers(ULONG *DevicesConnected)
{
    HANDLE                           hHCDev = NULL;
    SP_DEVINFO_DATA                  deviceInfoData;
    SP_DEVICE_INTERFACE_DATA         stDeviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA pDeviceInterfaceDetailData = NULL;
    ULONG                            requiredLength = 0;

    m_nTotalDevicesConnected = 0;
    m_nTotalHubs = 0;

    // Iterate over host controllers using the new GUID based interface
    //
    HDEVINFO hDevInfo = SetupDiGetClassDevs((LPGUID)&GUID_CLASS_USB_HOST_CONTROLLER, NULL, NULL, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (DWORD index = 0; SetupDiEnumDeviceInfo(hDevInfo, index, &deviceInfoData); index++)
    {
        stDeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        BOOL bSuc = SetupDiEnumDeviceInterfaces(hDevInfo, 0, (LPGUID)&GUID_CLASS_USB_HOST_CONTROLLER, index, &stDeviceInterfaceData);

        if (!bSuc)
        {
            break;
        }

        bSuc = SetupDiGetDeviceInterfaceDetail(hDevInfo, &stDeviceInterfaceData, NULL, 0, &requiredLength, NULL);

        if (!bSuc && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            break;
        }

        pDeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)ALLOC(requiredLength);
        if (pDeviceInterfaceDetailData == NULL)
        {
            break;
        }

        pDeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        bSuc = SetupDiGetDeviceInterfaceDetail(hDevInfo,
            &stDeviceInterfaceData,
            pDeviceInterfaceDetailData,
            requiredLength,
            &requiredLength,
            NULL);

        if (!bSuc)
        {
            break;
        }

        hHCDev = CreateFile(pDeviceInterfaceDetailData->DevicePath,
            GENERIC_WRITE,
            FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);

        // If the handle is valid, then we've successfully opened a Host
        // Controller.  Display some info about the Host Controller itself,
        // then enumerate the Root Hub attached to the Host Controller.
        //
        if (hHCDev != INVALID_HANDLE_VALUE)
        {
            EnumerateHostController(hHCDev, hDevInfo, &deviceInfoData);
            CloseHandle(hHCDev);
        }

        FREE(pDeviceInterfaceDetailData);
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    *DevicesConnected = m_nTotalDevicesConnected;

    return;
}

void Impl::MyDebugModeTest( const CString& sFatherHubName, int nPortNum )
{
    CString sFullHubName;
    sFullHubName.Format(_T("\\\\.\\%s"), sFatherHubName);

    HANDLE hHubDevice = CreateFile(sFullHubName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hHubDevice != INVALID_HANDLE_VALUE)
    {
        PUSB_DESCRIPTOR_REQUEST configDesc = GetConfigDescriptor(hHubDevice, 1, 0);
        MyReadUsbDescriptorRequest(configDesc);
    }
}

void Impl::MyReadUsbDescriptorRequest( PUSB_DESCRIPTOR_REQUEST pRequest )
{
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
            break;
        }
    } while ((commonDesc = GetNextDescriptor((PUSB_COMMON_DESCRIPTOR)ConfigDesc, ConfigDesc->wTotalLength, commonDesc, -1)) != NULL);
}

PUSB_COMMON_DESCRIPTOR Impl::GetNextDescriptor(
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


PUSB_COMMON_DESCRIPTOR Impl::NextDescriptor(_In_ PUSB_COMMON_DESCRIPTOR Descriptor)
{
    if (Descriptor->bLength == 0)
    {
        return NULL;
    }
    return (PUSB_COMMON_DESCRIPTOR)((PUCHAR)Descriptor + Descriptor->bLength);
}
