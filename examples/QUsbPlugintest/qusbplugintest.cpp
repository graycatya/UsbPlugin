#include "QUsbPlugin.h"

#include <thread>
#include <iostream>
#include <string>
#include <QCoreApplication>
#include <QDebug>
#include <QString>
//static void Hotplug_callback(std::list<Usb_Device> adds, std::list<Usb_Device> dels) {
//    std::cout << "adds: " << std::to_string(adds.size()) << " dels: " << std::to_string(dels.size()) << std::endl;
//    fflush(stdout);
//}

//int main(int argc, char *argv[])
//{
//    QUsbPlugin::Instance()->SetHotplug_SleepMs(200);
//    QUsbPlugin::Instance()->Register_Hotplug_Callback(Hotplug_callback);
//    QUsbPlugin::Instance()->Register_Hotplug();

//    std::thread threads([](){
//        while(true)
//        {
//            std::this_thread::sleep_for(std::chrono::milliseconds(1000));//睡眠1000毫秒
//        }
//    });
//    threads.join();
//    QUsbPlugin::Instance()->Deregister_Hotplug();
//    QUsbPlugin::Delete();

//}

#include <string.h>
#include <stdio.h>
#include "libusb.h"


void print_dev(libusb_device **devs)
{
    libusb_device *dev;
        int i = 0, j = 0;
        uint8_t path[8];

        while ((dev = devs[i++]) != NULL)
        {
            struct libusb_device_descriptor desc;
            int r = libusb_get_device_descriptor(dev, &desc);
            if (r < 0) {
                qDebug("failed to get device descriptor");
                return;
            }
        //main_show->ui->textBrowser->setText("aaaa");
#if 1
            qDebug("%04x:%04x (bus %d, device %d)",
                desc.idVendor, desc.idProduct,
                libusb_get_bus_number(dev), libusb_get_device_address(dev));

            r = libusb_get_port_numbers(dev, path, sizeof(path));
            if (r > 0) {
                qDebug(" path: %d", path[0]);
                for (j = 1; j < r; j++)
                    qDebug(".%d", path[j]);
            }
#endif
        }

}

int find_all_of_devices(void)
{
    libusb_device **devs;
    ssize_t cnt;

    cnt = libusb_get_device_list(NULL, &devs);
    print_dev(devs);
    return 0;
}

void test_my_usb_devices(int vid,int pid)
{
    libusb_device_handle *handle;
    libusb_device *dev;
    int bus;

    struct libusb_config_descriptor *conf_desc;
    const struct libusb_endpoint_descriptor *endpoint;
    uint8_t endpoint_in = 0, endpoint_out = 0;
    int endpoint_num, nb_ifaces, altesetting = -1, interfaceNumber = -1;
    struct libusb_ss_endpoint_companion_descriptor *ep_comp = NULL;

    uint8_t buf[63]={0};
    //int i;
    int res = 0;
    int size=0;

    /*打开设备*/
    handle = libusb_open_device_with_vid_pid(NULL, vid, pid);
    if (handle == NULL)
        qDebug("open devices failed");


    dev = libusb_get_device(handle);
    bus = libusb_get_bus_number(dev);
    libusb_get_config_descriptor(dev, 0, &conf_desc);
    nb_ifaces = conf_desc->bNumInterfaces;
    qDebug()<<nb_ifaces;

    if(nb_ifaces>0)
        {
        for(int i = 0; i < nb_ifaces; i++)
        {
            altesetting=conf_desc->interface[i].num_altsetting;
            if(altesetting>0)
            {
                for(int j = 0; j < altesetting; j++)
                {
                    endpoint_num=conf_desc->interface[i].altsetting[j].bNumEndpoints;
                    interfaceNumber=conf_desc->interface[i].altsetting[j].bInterfaceNumber;
                    if(endpoint_num>0)
                    {
                        for(int k=0;k<endpoint_num;k++)
                        {
                            endpoint= &conf_desc->interface[i].altsetting[j].endpoint[k];
                            if ((endpoint->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) & (LIBUSB_TRANSFER_TYPE_BULK | LIBUSB_TRANSFER_TYPE_INTERRUPT)) {
                                if (endpoint->bEndpointAddress & LIBUSB_ENDPOINT_IN) {
                                    if (!endpoint_in)
                                        endpoint_in = endpoint->bEndpointAddress;
                                }
                                else {
                                    if (!endpoint_out)
                                        endpoint_out = endpoint->bEndpointAddress;
                                }
                            }
                        }
                    }
                    else
                        qDebug("endpoint num error");
                }
            }
            else
            {
                qDebug("altsetting error");
            }

            /*卸载驱动内核*/
            res = libusb_set_auto_detach_kernel_driver(handle, 1);
            /*为指定的设备申请接口*/
            res=libusb_claim_interface(handle, interfaceNumber);
            res<0?qDebug("claim interface error"):qDebug("claim interface success");

            memset(&buf[0], 0, 63);

            buf[0] = 0x12;
            buf[1] = 0x13;
            buf[2] = 0x14;
            buf[3] = 0x15;
            buf[4] = 0x15;
            buf[5] = 0x15;
            buf[6] = 0x15;
            buf[7] = 0x15;
            buf[8] = 0x15;
            qDebug("\nTesting interrupt write using endpoint %02X...\n", endpoint_out);
            res = libusb_interrupt_transfer(handle, endpoint_out, buf, 64, &size, 1000);
            //r = libusb_interrupt_transfer(handle, endpoint_out, report_buffer, 64, &size, 1000);
            //r = libusb_interrupt_transfer(handle, endpoint_out, report_buffer, 65, &size, 1000);

            if (res >= 0) {
                qDebug("write ok\n");
            }
            else {
                qDebug("   %s\n", libusb_strerror((enum libusb_error)res));
            }

            qDebug("Testing interrupt read using endpoint %02X...\n", endpoint_in);
            res = libusb_interrupt_transfer(handle, endpoint_in, buf, 9, &size, 1000);
            if (res >= 0) {
                qDebug("read ok:");
                for (uint8_t i = 0; i < 64; i++)
                {
                    qDebug("%02x ", buf[i]);
                }

            }
            else {
                qDebug("   %s\n", libusb_strerror((enum libusb_error)res));
            }

            Sleep(1000);
        }

        }
        else
            qDebug("interface found error");

    /*释放配置描述符*/
    libusb_free_config_descriptor(conf_desc);

}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    libusb_init(NULL);
    find_all_of_devices();
    test_my_usb_devices(0x4c4a, 0x4155);
    //test_my_usb_devices(0x14cd, 0x1212);
    //test_my_usb_devices(0x3243, 0x0122);
    app.exec();
}
