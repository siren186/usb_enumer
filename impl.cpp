#include "stdafx.h"
#include "impl.h"

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
                EnumerateHub(sExtHubName);
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

VOID Impl::EnumerateHostController(_In_ HANDLE hHCDev, _In_ HDEVINFO hDeviceInfo, _In_ PSP_DEVINFO_DATA deviceInfoData)
{
    CString sDrvKeyName = GetHCDDriverKeyName(hHCDev);
    if (FALSE == sDrvKeyName.IsEmpty())
    {
        CString sRootHubName = GetRootHubName(hHCDev);
        if (!sRootHubName.IsEmpty() && sRootHubName.GetLength() < MAX_DRIVER_KEY_NAME)
        {
            EnumerateHub(sRootHubName);
        }
    }
}

VOID Impl::EnumerateHub(_In_ const CString& sHubName)
{
    PUSB_NODE_INFORMATION hubInfo    = NULL;
    HANDLE                hHubDevice = INVALID_HANDLE_VALUE;
    CString               sFullHubName;
    ULONG                 nBytes = 0;
    BOOL                  bSuc = FALSE;

    hubInfo = (PUSB_NODE_INFORMATION)ALLOC(sizeof(USB_NODE_INFORMATION));
    if (hubInfo == NULL)
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
        hubInfo,
        sizeof(USB_NODE_INFORMATION),
        hubInfo,
        sizeof(USB_NODE_INFORMATION),
        &nBytes,
        NULL);
    if (!bSuc)
    {
        goto Exit0;
    }

    EnumerateHubPorts(hHubDevice, hubInfo->u.HubInformation.HubDescriptor.bNumberOfPorts);

Exit0:
    if (hHubDevice != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hHubDevice);
        hHubDevice = INVALID_HANDLE_VALUE;
    }

    if (hubInfo)
    {
        FREE(hubInfo);
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

VOID Impl::EnumerateHostControllers()
{
    HANDLE hHCDev = INVALID_HANDLE_VALUE;
    m_nTotalDevicesConnected = 0;
    m_nTotalHubs = 0;

    SP_DEVINFO_DATA deviceInfoData;
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    SP_DEVICE_INTERFACE_DATA stDeviceInterfaceData;
    HDEVINFO hDevInfo = SetupDiGetClassDevs((LPGUID)&GUID_CLASS_USB_HOST_CONTROLLER, NULL, NULL, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));
    for (DWORD index=0; SetupDiEnumDeviceInfo(hDevInfo, index, &deviceInfoData); index++)
    {
        stDeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        BOOL bSuc = SetupDiEnumDeviceInterfaces(hDevInfo, 0, (LPGUID)&GUID_CLASS_USB_HOST_CONTROLLER, index, &stDeviceInterfaceData);
        if (!bSuc)
        {
            break;
        }

        CString sDevPath = _GetDevPath(hDevInfo, stDeviceInterfaceData);
        if (sDevPath.IsEmpty())
        {
            break;
        }

        hHCDev = CreateFile(sDevPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (hHCDev != INVALID_HANDLE_VALUE)
        {
            EnumerateHostController(hHCDev, hDevInfo, &deviceInfoData);
            CloseHandle(hHCDev);
            hHCDev = INVALID_HANDLE_VALUE;
        }
    }

    if (INVALID_HANDLE_VALUE != hHCDev)
    {
        CloseHandle(hHCDev);
        hHCDev = INVALID_HANDLE_VALUE;
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    return;
}

BOOL Impl::IsAdbDevice( const CString& sFatherHubName, int nPortNum )
{
    BOOL bFindInterface0xff42 = FALSE;

    CString sFullHubName;
    sFullHubName.Format(_T("\\\\.\\%s"), sFatherHubName);
    HANDLE hHubDevice = CreateFile(sFullHubName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hHubDevice != INVALID_HANDLE_VALUE)
    {
        PUSB_DESCRIPTOR_REQUEST configDesc = GetConfigDescriptor(hHubDevice, nPortNum, 0);
        _ReadUsbDescriptorRequest(configDesc, bFindInterface0xff42);
        if (configDesc)
        {
            FREE(configDesc);
        }
    }

    return bFindInterface0xff42;
}

void Impl::_ReadUsbDescriptorRequest( PUSB_DESCRIPTOR_REQUEST pRequest, BOOL& bFindInterface0xff42)
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

CString Impl::_GetDevPath( HDEVINFO hDevInfo, SP_DEVICE_INTERFACE_DATA stDeviceInterfaceData )
{
    CString sDevPath;
    PSP_DEVICE_INTERFACE_DETAIL_DATA pDetailData = NULL;

    // 读大小
    ULONG requiredLength = 0;
    BOOL bSuc = SetupDiGetDeviceInterfaceDetail(hDevInfo, &stDeviceInterfaceData, NULL, 0, &requiredLength, NULL);
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
    bSuc = SetupDiGetDeviceInterfaceDetail(hDevInfo, &stDeviceInterfaceData, pDetailData, requiredLength, &requiredLength, NULL);
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
