#pragma once

#define MAX_DRIVER_KEY_NAME 256
#define MAX_DEVICE_PROP 200
#define NUM_STRING_DESC_TO_GET 32
//
// A collection of device properties. The device can be hub, host controller or usb device
//
typedef struct _USB_DEVICE_PNP_STRINGS
{
    TCHAR DeviceId[MAX_DRIVER_KEY_NAME];
    TCHAR DeviceDesc[MAX_DRIVER_KEY_NAME];
    TCHAR HwId[MAX_DRIVER_KEY_NAME];
    TCHAR Service[MAX_DRIVER_KEY_NAME];
    TCHAR DeviceClass[MAX_DRIVER_KEY_NAME];
    TCHAR PowerState[MAX_DRIVER_KEY_NAME];
} USB_DEVICE_PNP_STRINGS, *PUSB_DEVICE_PNP_STRINGS;

//
// Structures assocated with TreeView items through the lParam.  When an item
// is selected, the lParam is retrieved and the structure it which it points
// is used to display information in the edit control.
//
typedef enum _USBDEVICEINFOTYPE
{
    HostControllerInfo,
    RootHubInfo,
    ExternalHubInfo,
    DeviceInfo
} USBDEVICEINFOTYPE, *PUSBDEVICEINFOTYPE;

//
// Structure used to build a linked list of String Descriptors
// retrieved from a device.
typedef struct _STRING_DESCRIPTOR_NODE
{
    struct _STRING_DESCRIPTOR_NODE *Next;
    UCHAR                           DescriptorIndex;
    USHORT                          LanguageID;
    USB_STRING_DESCRIPTOR           StringDescriptor[1];
} STRING_DESCRIPTOR_NODE, *PSTRING_DESCRIPTOR_NODE;

typedef struct _DEVICE_INFO_NODE
{
    HDEVINFO                         hDevInfo;
    LIST_ENTRY                       ListEntry;
    SP_DEVINFO_DATA                  stDevInfoData;
    SP_DEVICE_INTERFACE_DATA         stDevInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA DeviceDetailData;
    TCHAR                            DeviceDescName[MAX_DRIVER_KEY_NAME];
    ULONG                            DeviceDescNameLength;
    TCHAR                            DeviceDriverName[MAX_DRIVER_KEY_NAME];
    ULONG                            DeviceDriverNameLength;
    DEVICE_POWER_STATE               LatestDevicePowerState;
} DEVICE_INFO_NODE, *PDEVICE_INFO_NODE;

typedef struct _DEVICE_GUID_LIST
{
    HDEVINFO   hDevInfo;
    LIST_ENTRY ListHead;
} DEVICE_GUID_LIST, *PDEVICE_GUID_LIST;

// HubInfo, HubName may be in USBDEVICEINFOTYPE, so they can be removed
typedef struct
{
    USBDEVICEINFOTYPE                      DeviceInfoType;
    PUSB_NODE_INFORMATION                  HubInfo;          // NULL if not a HUB
    PUSB_HUB_INFORMATION_EX                HubInfoEx;        // NULL if not a HUB
    PCHAR                                  HubName;          // NULL if not a HUB
    PUSB_NODE_CONNECTION_INFORMATION_EX    ConnectionInfo;   // NULL if root HUB
    PUSB_PORT_CONNECTOR_PROPERTIES         PortConnectorProps;
    PUSB_DESCRIPTOR_REQUEST                ConfigDesc;       // NULL if root HUB
    PUSB_DESCRIPTOR_REQUEST                BosDesc;          // NULL if root HUB
    PSTRING_DESCRIPTOR_NODE                StringDescs;
    PUSB_NODE_CONNECTION_INFORMATION_EX_V2 ConnectionInfoV2; // NULL if root HUB
    PUSB_DEVICE_PNP_STRINGS                UsbDeviceProperties;
    PDEVICE_INFO_NODE                      DeviceInfoNode;
    PUSB_HUB_CAPABILITIES_EX               HubCapabilityEx;  // NULL if not a HUB
} USBDEVICEINFO, *PUSBDEVICEINFO;

// Common Class Interface Descriptor
//
typedef struct _USB_INTERFACE_DESCRIPTOR2
{
    UCHAR  bLength;             // offset 0, size 1
    UCHAR  bDescriptorType;     // offset 1, size 1
    UCHAR  bInterfaceNumber;    // offset 2, size 1
    UCHAR  bAlternateSetting;   // offset 3, size 1
    UCHAR  bNumEndpoints;       // offset 4, size 1
    UCHAR  bInterfaceClass;     // offset 5, size 1
    UCHAR  bInterfaceSubClass;  // offset 6, size 1
    UCHAR  bInterfaceProtocol;  // offset 7, size 1
    UCHAR  iInterface;          // offset 8, size 1
    USHORT wNumClasses;         // offset 9, size 2
} USB_INTERFACE_DESCRIPTOR2, *PUSB_INTERFACE_DESCRIPTOR2;

// IAD Descriptor
//
typedef struct _USB_IAD_DESCRIPTOR
{
    UCHAR   bLength;
    UCHAR   bDescriptorType;
    UCHAR   bFirstInterface;
    UCHAR   bInterfaceCount;
    UCHAR   bFunctionClass;
    UCHAR   bFunctionSubClass;
    UCHAR   bFunctionProtocol;
    UCHAR   iFunction;
} USB_IAD_DESCRIPTOR, *PUSB_IAD_DESCRIPTOR;

typedef struct _USBEXTERNALHUBINFO
{
    USBDEVICEINFOTYPE                      DeviceInfoType;
    PUSB_NODE_INFORMATION                  HubInfo;
    PUSB_HUB_INFORMATION_EX                HubInfoEx;
    TCHAR                                  HubName[MAX_DRIVER_KEY_NAME];
    PUSB_NODE_CONNECTION_INFORMATION_EX    ConnectionInfo;
    PUSB_PORT_CONNECTOR_PROPERTIES         PortConnectorProps;
    PUSB_DESCRIPTOR_REQUEST                ConfigDesc;
    PUSB_DESCRIPTOR_REQUEST                BosDesc;
    PSTRING_DESCRIPTOR_NODE                StringDescs;
    PUSB_NODE_CONNECTION_INFORMATION_EX_V2 ConnectionInfoV2; // NULL if root HUB
    PUSB_DEVICE_PNP_STRINGS                UsbDeviceProperties;
    PDEVICE_INFO_NODE                      DeviceInfoNode;
    PUSB_HUB_CAPABILITIES_EX               HubCapabilityEx;
} USBEXTERNALHUBINFO, *PUSBEXTERNALHUBINFO;

typedef struct _USBROOTHUBINFO
{
    USBDEVICEINFOTYPE                   DeviceInfoType;
    PUSB_NODE_INFORMATION               HubInfo;
    PUSB_HUB_INFORMATION_EX             HubInfoEx;
    TCHAR                               HubName[MAX_DRIVER_KEY_NAME];
    PUSB_PORT_CONNECTOR_PROPERTIES      PortConnectorProps;
    PUSB_DEVICE_PNP_STRINGS             UsbDeviceProperties;
    PDEVICE_INFO_NODE                   DeviceInfoNode;
    PUSB_HUB_CAPABILITIES_EX            HubCapabilityEx;

} USBROOTHUBINFO, *PUSBROOTHUBINFO;


typedef struct _USBHOSTCONTROLLERINFO
{
    USBDEVICEINFOTYPE                   DeviceInfoType;
    LIST_ENTRY                          ListEntry;
    TCHAR                               DriverKey[MAX_DRIVER_KEY_NAME];
    ULONG                               VendorID;
    ULONG                               DeviceID;
    ULONG                               SubSysID;
    ULONG                               Revision;
    PUSB_CONTROLLER_INFO_0              ControllerInfo;
    PUSB_DEVICE_PNP_STRINGS             UsbDeviceProperties;
} USBHOSTCONTROLLERINFO, *PUSBHOSTCONTROLLERINFO;

typedef enum _TREEICON
{
    ComputerIcon,
    HubIcon,
    NoDeviceIcon,
    GoodDeviceIcon,
    BadDeviceIcon,
    GoodSsDeviceIcon,
    NoSsDeviceIcon
} TREEICON;

//
//USB 2.0 Specification Changes - New Descriptors
//
#define USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_TYPE  0x07
#define USB_INTERFACE_POWER_DESCRIPTOR_TYPE            0x08
#define USB_OTG_DESCRIPTOR_TYPE                        0x09
#define USB_DEBUG_DESCRIPTOR_TYPE                      0x0A
#define USB_IAD_DESCRIPTOR_TYPE                        0x0B

#define ALLOC(dwBytes) GlobalAlloc(GPTR,(dwBytes))
#define REALLOC(hMem, dwBytes) GlobalReAlloc((hMem), (dwBytes), (GMEM_MOVEABLE|GMEM_ZEROINIT))
#define FREE(hMem)  GlobalFree((hMem))

class Impl
{
private:
    int m_nTotalDevicesConnected;
    int m_nTotalHubs;

public:
    Impl() : m_nTotalDevicesConnected(0), m_nTotalHubs(0) {}
    VOID EnumerateHostControllers();


    BOOL IsAdbDevice(const CString& sFatherHubName, int nPortNum);
    void _ReadUsbDescriptorRequest(PUSB_DESCRIPTOR_REQUEST pRequest, BOOL& bFindInterface0xff42);

private:
    VOID EnumerateHostController(_In_ HANDLE hHCDev, _In_ HDEVINFO deviceInfo, _In_ PSP_DEVINFO_DATA deviceInfoData);
    VOID EnumerateHub(_In_ const CString& sHubName);
    VOID EnumerateHubPorts(HANDLE hHubDevice, ULONG NumPorts);

    CString GetDriverKeyName(HANDLE Hub, ULONG ConnectionIndex);
    PUSB_DEVICE_PNP_STRINGS DriverNameToDeviceProperties(const CString& sDrvKeyName);
    BOOL DriverNameToDeviceInst(const CString& sDrvKeyName, _Out_ HDEVINFO *pDevInfo, _Out_writes_bytes_(sizeof(SP_DEVINFO_DATA)) PSP_DEVINFO_DATA pDevInfoData);
    CString GetDeviceProperty(
        _In_ HDEVINFO DeviceInfoSet,
        _In_ PSP_DEVINFO_DATA DeviceInfoData,
        _In_ DWORD Property);
    PUSB_DESCRIPTOR_REQUEST GetConfigDescriptor(HANDLE hHubDevice, ULONG ConnectionIndex, UCHAR DescriptorIndex);
    PUSB_DESCRIPTOR_REQUEST GetBOSDescriptor(HANDLE hHubDevice, ULONG ConnectionIndex);
    BOOL AreThereStringDescriptors(PUSB_DEVICE_DESCRIPTOR DeviceDesc, PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc);
    PSTRING_DESCRIPTOR_NODE GetAllStringDescriptors(HANDLE hHubDevice, ULONG ConnectionIndex, PUSB_DEVICE_DESCRIPTOR DeviceDesc, PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc);
    CString GetExternalHubName(HANDLE Hub, ULONG ConnectionIndex);
    VOID FreeDeviceProperties(_In_ PUSB_DEVICE_PNP_STRINGS *ppDevProps);
    PSTRING_DESCRIPTOR_NODE GetStringDescriptor(HANDLE hHubDevice, ULONG ConnectionIndex, UCHAR DescriptorIndex, USHORT LanguageID);
    HRESULT GetStringDescriptors(_In_ HANDLE hHubDevice, _In_ ULONG ConnectionIndex, _In_ UCHAR DescriptorIndex, _In_ ULONG NumLanguageIDs, _In_reads_(NumLanguageIDs) USHORT *LanguageIDs, _In_ PSTRING_DESCRIPTOR_NODE StringDescNodeHead);
    CString GetHCDDriverKeyName(HANDLE HCD);
    DWORD GetHostControllerInfo(HANDLE hHCDev, PUSBHOSTCONTROLLERINFO hcInfo);
    CString GetRootHubName(HANDLE HostController);
    PUSB_COMMON_DESCRIPTOR GetNextDescriptor( _In_reads_bytes_(TotalLength) PUSB_COMMON_DESCRIPTOR FirstDescriptor, _In_ ULONG TotalLength, _In_ PUSB_COMMON_DESCRIPTOR StartDescriptor, _In_ long DescriptorType );
    PUSB_COMMON_DESCRIPTOR NextDescriptor(_In_ PUSB_COMMON_DESCRIPTOR Descriptor);


private:
    // Из:"\\?\pci#ven_8086&dev_1e26&subsys_05771028&rev_04#3&11583659&1&e8#{3abf6f2d-71c4-462a-8a92-1e6861e6af27}"
    CString _GetDevPath( HDEVINFO hDevInfo, SP_DEVICE_INTERFACE_DATA stDeviceInterfaceData );
};
