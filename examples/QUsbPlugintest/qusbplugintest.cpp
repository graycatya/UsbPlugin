#include "QUsbPlugin.h"

#include <thread>


int main(int argc, char *argv[])
{
    QUsbPlugin::Instance()->Register_Hotplug();

    std::thread threads([](){
        while(true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));//睡眠1000毫秒
        }
    });
    threads.join();
    QUsbPlugin::Instance()->Deregister_Hotplug();

}
