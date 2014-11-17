#pragma once
#include "tinyxml\tinyxml.h"



#define ALLOC(dwBytes) GlobalAlloc(GPTR,(dwBytes))
#define FREE(hMem)     GlobalFree((hMem))

#define MAX_DRIVER_KEY_NAME 256
#define MAX_DEVICE_PROP 200
#define NUM_STRING_DESC_TO_GET 32

typedef struct
{
    TCHAR DeviceDesc[MAX_DRIVER_KEY_NAME];
    TCHAR HwId[MAX_DRIVER_KEY_NAME];
    TCHAR Service[MAX_DRIVER_KEY_NAME];
    TCHAR DeviceClass[MAX_DRIVER_KEY_NAME];

    CString sDeviceInstanceId;
    CString sDeviceDesc;
    CString sHwId;
    CString sService;
    CString sDeviceClass;
} UsbDevicePnpStrings, *PUsbDevicePnpStrings;

typedef struct _STRING_DESCRIPTOR_NODE
{
    struct _STRING_DESCRIPTOR_NODE *Next;
    UCHAR                           DescriptorIndex;
    USHORT                          LanguageID;
    USB_STRING_DESCRIPTOR           StringDescriptor[1];
} StringDescriptorNode, *PStringDescriptorNode;

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
    int m_nTotalConnectedDevices;
    int m_nTotalHubs;

public:
    CUsbEnumer() : m_nTotalConnectedDevices(0), m_nTotalHubs(0) {}

    void EnumAllUsb()
    {
        EnumerateHostControllers();
    }

    VOID EnumerateHostControllers();

    BOOL IsAdbDevice(const CString& sFatherHubName, int nPortNum);

private:
    void _MyReadUsbDescriptorRequest(PUSB_DESCRIPTOR_REQUEST pRequest, BOOL& bFindInterface0xff42);
    VOID EnumerateHostController(_In_ HANDLE hHCDev, _In_ HDEVINFO deviceInfo, _In_ PSP_DEVINFO_DATA deviceInfoData);
    VOID EnumerateHub(_In_ const CString& sHubName);
    VOID EnumerateHubPorts(HANDLE hHubDevice, ULONG NumPorts);
    BOOL DriverNameToDeviceProperties(const CString& sDrvKeyName, UsbDevicePnpStrings& stPnpStrings);
    BOOL DriverNameToDeviceInst(const CString& sDrvKeyName, _Out_ HDEVINFO *pDevInfo, _Out_writes_bytes_(sizeof(SP_DEVINFO_DATA)) PSP_DEVINFO_DATA pDevInfoData);
    CString GetDeviceProperty(
        _In_ HDEVINFO DeviceInfoSet,
        _In_ PSP_DEVINFO_DATA DeviceInfoData,
        _In_ DWORD Property);
    PUSB_DESCRIPTOR_REQUEST GetConfigDescriptor(HANDLE hHubDevice, ULONG ConnectionIndex, UCHAR DescriptorIndex);
    CString GetExternalHubName(HANDLE Hub, ULONG ConnectionIndex);
    PStringDescriptorNode GetStringDescriptor(HANDLE hHubDevice, ULONG ConnectionIndex, UCHAR DescriptorIndex, USHORT LanguageID);
    CString GetHCDDriverKeyName(HANDLE HCD);
    CString GetRootHubName(HANDLE HostController);
    PUSB_COMMON_DESCRIPTOR GetNextDescriptor( _In_reads_bytes_(TotalLength) PUSB_COMMON_DESCRIPTOR FirstDescriptor, _In_ ULONG TotalLength, _In_ PUSB_COMMON_DESCRIPTOR StartDescriptor, _In_ long DescriptorType );
    PUSB_COMMON_DESCRIPTOR NextDescriptor(_In_ PUSB_COMMON_DESCRIPTOR Descriptor);

    void _ParsepUsbDescriptorRequest( PUSB_DESCRIPTOR_REQUEST pRequest);

private:
    // Из:"\\?\pci#ven_8086&dev_1e26&subsys_05771028&rev_04#3&11583659&1&e8#{3abf6f2d-71c4-462a-8a92-1e6861e6af27}"
    CString _GetDevPath( HDEVINFO hDevInfo, SP_DEVICE_INTERFACE_DATA stDeviceInterfaceData );
    CString GetDriverKeyName(HANDLE Hub, ULONG ConnectionIndex);
};
