#pragma once
#include "tinyxml\tinyxml.h"

#define MAX_DRIVER_KEY_NAME 256
#define MAX_DEVICE_PROP 200
#define NUM_STRING_DESC_TO_GET 32

typedef struct _USB_DEVICE_PNP_STRINGS
{
    TCHAR DeviceId[MAX_DRIVER_KEY_NAME];
    TCHAR DeviceDesc[MAX_DRIVER_KEY_NAME];
    TCHAR HwId[MAX_DRIVER_KEY_NAME];
    TCHAR Service[MAX_DRIVER_KEY_NAME];
    TCHAR DeviceClass[MAX_DRIVER_KEY_NAME];
    TCHAR PowerState[MAX_DRIVER_KEY_NAME];
} USB_DEVICE_PNP_STRINGS, *PUSB_DEVICE_PNP_STRINGS;

typedef enum _USBDEVICEINFOTYPE
{
    HostControllerInfo,
    RootHubInfo,
    ExternalHubInfo,
    DeviceInfo
} USBDEVICEINFOTYPE, *PUSBDEVICEINFOTYPE;

typedef struct _STRING_DESCRIPTOR_NODE
{
    struct _STRING_DESCRIPTOR_NODE *Next;
    UCHAR                           DescriptorIndex;
    USHORT                          LanguageID;
    USB_STRING_DESCRIPTOR           StringDescriptor[1];
} STRING_DESCRIPTOR_NODE, *PSTRING_DESCRIPTOR_NODE;

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

class CUsbEnumer
{
private:
    int m_nTotalDevicesConnected;
    int m_nTotalHubs;
    TiXmlDocument m_XmlDoc;

public:
    CUsbEnumer() : m_nTotalDevicesConnected(0), m_nTotalHubs(0) {}
    VOID EnumerateHostControllers();
    BOOL IsAdbDevice(const CString& sFatherHubName, int nPortNum);

private:
    void _ReadUsbDescriptorRequest(PUSB_DESCRIPTOR_REQUEST pRequest, BOOL& bFindInterface0xff42);
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
    CString GetExternalHubName(HANDLE Hub, ULONG ConnectionIndex);
    VOID FreeDeviceProperties(_In_ PUSB_DEVICE_PNP_STRINGS *ppDevProps);
    PSTRING_DESCRIPTOR_NODE GetStringDescriptor(HANDLE hHubDevice, ULONG ConnectionIndex, UCHAR DescriptorIndex, USHORT LanguageID);
    CString GetHCDDriverKeyName(HANDLE HCD);
    CString GetRootHubName(HANDLE HostController);
    PUSB_COMMON_DESCRIPTOR GetNextDescriptor( _In_reads_bytes_(TotalLength) PUSB_COMMON_DESCRIPTOR FirstDescriptor, _In_ ULONG TotalLength, _In_ PUSB_COMMON_DESCRIPTOR StartDescriptor, _In_ long DescriptorType );
    PUSB_COMMON_DESCRIPTOR NextDescriptor(_In_ PUSB_COMMON_DESCRIPTOR Descriptor);

    // Из:"\\?\pci#ven_8086&dev_1e26&subsys_05771028&rev_04#3&11583659&1&e8#{3abf6f2d-71c4-462a-8a92-1e6861e6af27}"
    CString _GetDevPath( HDEVINFO hDevInfo, SP_DEVICE_INTERFACE_DATA stDeviceInterfaceData );
};
