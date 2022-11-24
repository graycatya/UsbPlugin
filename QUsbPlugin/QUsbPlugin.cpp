#include "QUsbPlugin.h"

#include "libusb.h"
#include <iostream>

#include <chrono>

libusb_device_handle *handle = NULL;

QUsbPlugin* QUsbPlugin::_instance = nullptr;
std::mutex* QUsbPlugin::m_pMutex = new std::mutex;
std::thread* QUsbPlugin::m_pHotplug_thread = nullptr;
bool QUsbPlugin::m_bHotplugThreadStop = true;

int verbose = 1;

static void print_endpoint_comp(const struct libusb_ss_endpoint_companion_descriptor *ep_comp)
{
    printf("      USB 3.0 Endpoint Companion:\n");
    printf("        bMaxBurst:           %u\n", ep_comp->bMaxBurst);
    printf("        bmAttributes:        %02xh\n", ep_comp->bmAttributes);
    printf("        wBytesPerInterval:   %u\n", ep_comp->wBytesPerInterval);
}

static void print_endpoint(const struct libusb_endpoint_descriptor *endpoint)
{
    int i, ret;

    printf("      Endpoint:\n");
    printf("        bEndpointAddress:    %02xh\n", endpoint->bEndpointAddress);
    printf("        bmAttributes:        %02xh\n", endpoint->bmAttributes);
    printf("        wMaxPacketSize:      %u\n", endpoint->wMaxPacketSize);
    printf("        bInterval:           %u\n", endpoint->bInterval);
    printf("        bRefresh:            %u\n", endpoint->bRefresh);
    printf("        bSynchAddress:       %u\n", endpoint->bSynchAddress);

    for (i = 0; i < endpoint->extra_length;) {
        if (LIBUSB_DT_SS_ENDPOINT_COMPANION == endpoint->extra[i + 1]) {
            struct libusb_ss_endpoint_companion_descriptor *ep_comp;

            ret = libusb_get_ss_endpoint_companion_descriptor(NULL, endpoint, &ep_comp);
            if (LIBUSB_SUCCESS != ret)
                continue;

            print_endpoint_comp(ep_comp);

            libusb_free_ss_endpoint_companion_descriptor(ep_comp);
        }

        i += endpoint->extra[i];
    }
}

static void print_altsetting(const struct libusb_interface_descriptor *interface)
{
    uint8_t i;

    printf("    Interface:\n");
    printf("      bInterfaceNumber:      %u\n", interface->bInterfaceNumber);
    printf("      bAlternateSetting:     %u\n", interface->bAlternateSetting);
    printf("      bNumEndpoints:         %u\n", interface->bNumEndpoints);
    printf("      bInterfaceClass:       %u\n", interface->bInterfaceClass);
    printf("      bInterfaceSubClass:    %u\n", interface->bInterfaceSubClass);
    printf("      bInterfaceProtocol:    %u\n", interface->bInterfaceProtocol);
    printf("      iInterface:            %u\n", interface->iInterface);

    for (i = 0; i < interface->bNumEndpoints; i++)
        print_endpoint(&interface->endpoint[i]);
}

static void print_2_0_ext_cap(struct libusb_usb_2_0_extension_descriptor *usb_2_0_ext_cap)
{
    printf("    USB 2.0 Extension Capabilities:\n");
    printf("      bDevCapabilityType:    %u\n", usb_2_0_ext_cap->bDevCapabilityType);
    printf("      bmAttributes:          %08xh\n", usb_2_0_ext_cap->bmAttributes);
}

static void print_ss_usb_cap(struct libusb_ss_usb_device_capability_descriptor *ss_usb_cap)
{
    printf("    USB 3.0 Capabilities:\n");
    printf("      bDevCapabilityType:    %u\n", ss_usb_cap->bDevCapabilityType);
    printf("      bmAttributes:          %02xh\n", ss_usb_cap->bmAttributes);
    printf("      wSpeedSupported:       %u\n", ss_usb_cap->wSpeedSupported);
    printf("      bFunctionalitySupport: %u\n", ss_usb_cap->bFunctionalitySupport);
    printf("      bU1devExitLat:         %u\n", ss_usb_cap->bU1DevExitLat);
    printf("      bU2devExitLat:         %u\n", ss_usb_cap->bU2DevExitLat);
}

static void print_bos(libusb_device_handle *handle)
{
    struct libusb_bos_descriptor *bos;
    uint8_t i;
    int ret;

    ret = libusb_get_bos_descriptor(handle, &bos);
    if (ret < 0)
        return;

    printf("  Binary Object Store (BOS):\n");
    printf("    wTotalLength:            %u\n", bos->wTotalLength);
    printf("    bNumDeviceCaps:          %u\n", bos->bNumDeviceCaps);

    for (i = 0; i < bos->bNumDeviceCaps; i++) {
        struct libusb_bos_dev_capability_descriptor *dev_cap = bos->dev_capability[i];

        if (dev_cap->bDevCapabilityType == LIBUSB_BT_USB_2_0_EXTENSION) {
            struct libusb_usb_2_0_extension_descriptor *usb_2_0_extension;

            ret = libusb_get_usb_2_0_extension_descriptor(NULL, dev_cap, &usb_2_0_extension);
            if (ret < 0)
                return;

            print_2_0_ext_cap(usb_2_0_extension);
            libusb_free_usb_2_0_extension_descriptor(usb_2_0_extension);
        } else if (dev_cap->bDevCapabilityType == LIBUSB_BT_SS_USB_DEVICE_CAPABILITY) {
            struct libusb_ss_usb_device_capability_descriptor *ss_dev_cap;

            ret = libusb_get_ss_usb_device_capability_descriptor(NULL, dev_cap, &ss_dev_cap);
            if (ret < 0)
                return;

            print_ss_usb_cap(ss_dev_cap);
            libusb_free_ss_usb_device_capability_descriptor(ss_dev_cap);
        }
    }

    libusb_free_bos_descriptor(bos);
}

static void print_interface(const struct libusb_interface *interface)
{
    int i;

    for (i = 0; i < interface->num_altsetting; i++)
        print_altsetting(&interface->altsetting[i]);
}

static void print_configuration(struct libusb_config_descriptor *config)
{
    uint8_t i;

    printf("  Configuration:\n");
    printf("    wTotalLength:            %u\n", config->wTotalLength);
    printf("    bNumInterfaces:          %u\n", config->bNumInterfaces);
    printf("    bConfigurationValue:     %u\n", config->bConfigurationValue);
    printf("    iConfiguration:          %u\n", config->iConfiguration);
    printf("    bmAttributes:            %02xh\n", config->bmAttributes);
    printf("    MaxPower:                %u\n", config->MaxPower);

    for (i = 0; i < config->bNumInterfaces; i++)
        print_interface(&config->interface[i]);
}

static void print_device(libusb_device *dev, libusb_device_handle *handle)
{
    struct libusb_device_descriptor desc;
    unsigned char string[256];
    const char *speed;
    int ret;
    uint8_t i;
    uint8_t port_number;

    switch (libusb_get_device_speed(dev)) {
    case LIBUSB_SPEED_LOW:		speed = "1.5M"; break;
    case LIBUSB_SPEED_FULL:		speed = "12M"; break;
    case LIBUSB_SPEED_HIGH:		speed = "480M"; break;
    case LIBUSB_SPEED_SUPER:	speed = "5G"; break;
    case LIBUSB_SPEED_SUPER_PLUS:	speed = "10G"; break;
    default:			speed = "Unknown";
    }

    ret = libusb_get_device_descriptor(dev, &desc);
    if (ret < 0) {
        fprintf(stderr, "failed to get device descriptor");
        return;
    }
    port_number = libusb_get_port_number(dev);
    printf("Dev (bus %u, port_number %d, device %u): %04X - %04X speed: %s\n",
           libusb_get_bus_number(dev), port_number, libusb_get_device_address(dev),
           desc.idVendor, desc.idProduct, speed);

    if (!handle)
        libusb_open(dev, &handle);

    if (handle) {
        if (desc.iManufacturer) {
            ret = libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, string, sizeof(string));
            if (ret > 0)
                printf("  Manufacturer:              %s\n", (char *)string);
        }

        if (desc.iProduct) {
            ret = libusb_get_string_descriptor_ascii(handle, desc.iProduct, string, sizeof(string));
            if (ret > 0)
                printf("  Product:                   %s\n", (char *)string);
        }

        if (desc.iSerialNumber) {
            ret = libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, string, sizeof(string));
            if (ret > 0)
                printf("  Serial Number:             %s\n", (char *)string);
        }
    }

    if (verbose) {
        for (i = 0; i < desc.bNumConfigurations; i++) {
            struct libusb_config_descriptor *config;

            ret = libusb_get_config_descriptor(dev, i, &config);
            if (LIBUSB_SUCCESS != ret) {
                printf("  Couldn't retrieve descriptors\n");
                continue;
            }

            print_configuration(config);

            libusb_free_config_descriptor(config);
        }

        if (handle && desc.bcdUSB >= 0x0201)
            print_bos(handle);
    }

    if (handle)
        libusb_close(handle);

    fflush(stdout);
}

int QUsbPlugin::Register_Hotplug()
{
    if (!libusb_has_capability (LIBUSB_CAP_HAS_HOTPLUG)) {
        std::cout << "Hotplug capabilities are not supported on this platform" << std::endl;
        _instance->Register_Thread_Hotplug();
    } else {

    }

    return 0;
}

int QUsbPlugin::Deregister_Hotplug()
{
    if (!libusb_has_capability (LIBUSB_CAP_HAS_HOTPLUG)) {
        std::cout << "Hotplug capabilities are not supported on this platform" << std::endl;
        _instance->Deregister_Thread_Hotplug();
    } else {

    }

    return 0;
}

int QUsbPlugin::GetHotplug_SleepMs()
{
    return m_nHotplug_sleepMs;
}

void QUsbPlugin::Register_Hotplug_Callback(std::function<void (std::list<Usb_Device>, std::list<Usb_Device>)> callback)
{
    if(m_pHotplug_callback == nullptr)
    {
        m_pHotplug_callback = callback;
    }
}

void QUsbPlugin::Deregister_Hotplug_Callback()
{
    if(m_pHotplug_callback)
    {
        m_pHotplug_callback = nullptr;
    }
}

void QUsbPlugin::SetHotplug_SleepMs(int ms)
{
    m_nHotplug_sleepMs = ms;
}

QUsbPlugin::QUsbPlugin()
{
    int rc = libusb_init(reinterpret_cast<libusb_context**>(ctx));
    if (rc < 0)
    {
        std::cout << "failed to initialise libusb: " << libusb_error_name(rc) << std::endl;
        std::cout.flush();
        return;
    }
    m_nHotplug_sleepMs = 1000;
    m_pUsb_devices.clear();
    Init_Device_List();
}

QUsbPlugin::~QUsbPlugin()
{
    libusb_exit(reinterpret_cast<libusb_context*>(ctx));
}

void QUsbPlugin::Init_Device_List()
{
    libusb_device **devs;
    ssize_t cnt;
    cnt = libusb_get_device_list(reinterpret_cast<libusb_context*>(ctx), &devs);
    if (cnt < 0) {
        libusb_exit(NULL);
        return;
    }
    for (int i = 0; devs[i]; i++)
    {
        _instance->Add_Device(static_cast<libusb_device*>(devs[i]), NULL, m_pUsb_devices);
        print_device(devs[i], NULL);
    }

    libusb_free_device_list(devs, 1);
}

void QUsbPlugin::Register_LibUsb_Hotplug()
{

}

void QUsbPlugin::Register_Thread_Hotplug()
{
    m_bHotplugThreadStop = false;
    m_pHotplug_thread = new std::thread([]{
         while(!m_bHotplugThreadStop)
         {
             libusb_device **devs;
             ssize_t cnt;
             cnt = libusb_get_device_list(reinterpret_cast<libusb_context*>(_instance->ctx), &devs);
             if (cnt < 0) {
                 libusb_exit(NULL);
                 return 1;
             }
             std::map<std::string, Usb_Device> devices;
             for (int i = 0; devs[i]; i++)
                 _instance->Add_Device(static_cast<libusb_device*>(devs[i]), NULL, devices);
             libusb_free_device_list(devs, 1);
             _instance->Compare_Devices(_instance->m_pUsb_devices, devices);
             std::this_thread::sleep_for(std::chrono::milliseconds(_instance->m_nHotplug_sleepMs));
         }
    });
}

void QUsbPlugin::Deregister_LibUsb_Hotplug()
{

}

void QUsbPlugin::Deregister_Thread_Hotplug()
{
    if(m_pHotplug_thread != nullptr)
    {
        m_bHotplugThreadStop = true;
        m_pHotplug_thread->join();
        delete m_pHotplug_thread;
        m_pHotplug_thread = nullptr;
    }

}

void QUsbPlugin::Add_Device(void *dev, void *handle, std::map<std::string, Usb_Device> &devices)
{
    struct libusb_device_descriptor desc;
    libusb_device* t_dev = static_cast<libusb_device*>(dev);
    libusb_device_handle * t_dev_handle = static_cast<libusb_device_handle*>(handle);

    Usb_Device usbdevice;
    int ret = libusb_get_device_descriptor(t_dev, &desc);
    if (ret < 0) {
        return;
    }
    usbdevice.speed = libusb_get_device_speed(t_dev);
    usbdevice.port_number = libusb_get_port_number(t_dev);
    usbdevice.device_address = libusb_get_device_address(t_dev);
    usbdevice.bus_number = libusb_get_bus_number(t_dev);
    usbdevice.vid = desc.idVendor;
    usbdevice.pid = desc.idProduct;
    devices[usbdevice.get_device_key()] = usbdevice;
}

void QUsbPlugin::Compare_Devices(std::map<std::string, Usb_Device> &original, std::map<std::string, Usb_Device> &current)
{
    std::map<std::string, Usb_Device> t_original = original;
    std::map<std::string, Usb_Device> t_current = current;
    std::list<Usb_Device> adds;
    std::list<Usb_Device> dels;

    std::list<std::string> delkeys;
    if(t_original.size() < t_current.size())
    {
        for(auto it = t_original.begin(); it != t_original.end(); ++it)
        {
            if(t_current.count(it->first) > 0)
            {
                delkeys.push_back(it->first);
            }
        }
    } else {
        for(auto it = t_current.begin(); it != t_current.end(); ++it)
        {
            if(t_current.count(it->first) > 0)
            {
                delkeys.push_back(it->first);
            }
        }
    }
    for(auto it = delkeys.begin(); it != delkeys.end(); ++it)
    {
        t_original.erase(it->data());
        t_current.erase(it->data());
    }
    for (auto it = t_original.begin(); it != t_original.end() ; ++it) {
        dels.push_back(it->second);
    }
    for (auto it = t_current.begin(); it != t_current.end() ; ++it) {
        adds.push_back(it->second);
    }
    if(m_pHotplug_callback)
    {
        m_pHotplug_callback(adds, dels);
    }
    original.clear();
    original = current;
}
