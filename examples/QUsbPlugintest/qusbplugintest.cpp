#include "QUsbPlugin.h"
#include <thread>
#include <iostream>
#include <string>

static void Hotplug_callback(std::list<Usb_Device> adds, std::list<Usb_Device> dels) {
    std::cout << "adds: " << std::to_string(adds.size()) << " dels: " << std::to_string(dels.size()) << std::endl;
    fflush(stdout);
}

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

static libusb_device_handle *dev_handle = NULL;

int main() {
    int i = 0;
    int ret = 1;
    int transferred = 0;
    ssize_t cnt;
    unsigned char cmd[9] = {0x03, 0x05, 0x06, 0x02, 0x10, 0xF0, 0xF0, 0x00, 0x00};    // 64为上述第3步获取到的缓冲区大小

    struct libusb_device_descriptor desc;
    libusb_device **devs;

    libusb_context *ctx = NULL;

    ret = libusb_init(NULL);
    if(ret < 0) {
        fprintf(stderr, "failed to initialise libusb\n");
        return 1;
    }

    dev_handle = libusb_open_device_with_vid_pid(NULL, 0x4C4A, 0x4155);
    if(dev_handle == NULL){
        perror("Cannot open device\n");
    }else{
        printf("Device Opened\n");
    }

    if(libusb_kernel_driver_active(dev_handle, 0) == 1) {
        printf("Kernel Driver Active\n");
        if(libusb_detach_kernel_driver(dev_handle, 0) == 0){
            printf("Kernel Driver Detached!\n");
        }
    }

    ret = libusb_claim_interface(dev_handle, 1);
    if(ret < 0) {
        perror("Cannot Claim Interface\n");
        return 1;
    }

    ret = libusb_interrupt_transfer(dev_handle, 0x02, cmd, sizeof(cmd), &transferred, 0);
    printf("transferred: %d", transferred);
    if(ret==0){
        printf("write Successful!\n");
    }else{
        printf("write error!\n");
    }

    char buf[1024] = {0};
    ret = libusb_interrupt_transfer(dev_handle, 0x82, (unsigned char*)buf, sizeof(buf), &transferred, 0);
    if (ret != 0) {
        printf("failed to read \n");
    }

    ret = libusb_release_interface(dev_handle, 0);
    if(ret != 0){
        printf("Cannot Released Interface!\n");
    }else{
        printf("Released Interface!\n");
    }

    libusb_close(dev_handle);
    libusb_exit(ctx);

    return 0;
}
