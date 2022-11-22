#include "QUsbPlugin.h"

#include <thread>
#include <iostream>
#include <string>

static void Hotplug_callback(std::list<Usb_Device> adds, std::list<Usb_Device> dels) {
    std::cout << "adds: " << std::to_string(adds.size()) << " dels: " << std::to_string(dels.size()) << std::endl;
    fflush(stdout);
}

int main(int argc, char *argv[])
{
    QUsbPlugin::Instance()->SetHotplug_SleepMs(200);
    QUsbPlugin::Instance()->Register_Hotplug_Callback(Hotplug_callback);
    QUsbPlugin::Instance()->Register_Hotplug();

    std::thread threads([](){
        while(true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));//睡眠1000毫秒
        }
    });
    threads.join();
    QUsbPlugin::Instance()->Deregister_Hotplug();
    QUsbPlugin::Delete();

}
