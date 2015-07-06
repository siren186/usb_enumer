枚举USB设备的小例子

> * usb_host_controler <i class="icon-chevron-sign-right"></i> root_hub <i class="icon-chevron-sign-right"></i> ext_hub <i class="icon-chevron-sign-right"></i> port <i class="icon-chevron-sign-right"></i> device


### 结果示例:

```xml
<conputer>
    <controller>
        <root_hub>
            <port>
                <ext_hub>
                    <port />
                    <port />
                    <port />
                    <port />
                    <port />
                    <port />
                    <port />
                    <port />
                </ext_hub>
            </port>
            <port />
        </root_hub>
    </controller>
    <controller>
        <root_hub>
            <port>
                <ext_hub>
                    <port>
                        <device instance_id="USB\VID_046A&amp;PID_0011\6&amp;20D5D7D2&amp;0&amp;1">
                            <interface_descriptor bInterfaceClass="3" bInterfaceSubClass="1" bInterfaceProtocol="1" />
                        </device>
                    </port>
                    <port />
                    <port>
                        <device instance_id="USB\VID_046A&amp;PID_0011\6&amp;20D5D7D2&amp;0&amp;1">
                            <interface_descriptor bInterfaceClass="3" bInterfaceSubClass="1" bInterfaceProtocol="1" />
                            <interface_descriptor bInterfaceClass="3" bInterfaceSubClass="1" bInterfaceProtocol="2" />
                            <interface_descriptor bInterfaceClass="3" bInterfaceSubClass="0" bInterfaceProtocol="0" />
                        </device>
                    </port>
                    <port />
                    <port />
                    <port />
                </ext_hub>
            </port>
            <port />
        </root_hub>
    </controller>
    <controller>
        <root_hub>
            <port />
            <port />
            <port />
            <port />
            <port />
            <port />
            <port />
            <port />
        </root_hub>
    </controller>
</conputer>
```
