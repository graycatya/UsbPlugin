#ifndef QUSBPLUGIN_H
#define QUSBPLUGIN_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <map>
#include <list>
#include <string>

class Usb_Device

{
public:
    uint8_t bus_number;
    uint8_t port_number;
    uint8_t device_address;
    uint16_t vid;
    uint16_t pid;
    int speed;

    std::string get_device_key() {
        std::string key;
        key = std::to_string(bus_number) + ","
                + std::to_string(port_number) + ","
                + std::to_string(device_address) + ","
                + std::to_string(vid) + ","
                + std::to_string(pid);
        return key;
    }

private:
    void *device_handle = nullptr;
};

class QUsbPlugin
{
public:
    static QUsbPlugin* Instance() noexcept
    {
        if(_instance == nullptr)
        {
            std::unique_lock<std::mutex>lock(*m_pMutex);
            if(_instance == nullptr)
            {
                _instance = new QUsbPlugin();
            }
        }
        return _instance;
    }

    static void Delete( void ) noexcept
    {
        if(_instance != nullptr)
        {

            if(m_pMutex != nullptr)
            {
                delete m_pMutex;
                m_pMutex = nullptr;
            }
            delete _instance;
            _instance = nullptr;
        }
    }
    static int Register_Hotplug();
    static int Deregister_Hotplug();

    void SetHotplug_SleepMs(int ms);
    int GetHotplug_SleepMs();

    void Register_Hotplug_Callback(std::function<void(std::list<Usb_Device>, std::list<Usb_Device>)> callback);
    void Deregister_Hotplug_Callback();

private:
    QUsbPlugin();
    ~QUsbPlugin();

    void Init_Device_List();

    void Register_LibUsb_Hotplug();
    void Register_Thread_Hotplug();
    void Deregister_LibUsb_Hotplug();
    void Deregister_Thread_Hotplug();

    void Add_Device(void *dev, void *handle, std::map<std::string, Usb_Device> &devices);
    void Compare_Devices(std::map<std::string, Usb_Device> &original, std::map<std::string, Usb_Device> &current);

private:

    static QUsbPlugin* _instance;
    static std::mutex* m_pMutex;
    static std::thread* m_pHotplug_thread;
    static bool m_bHotplugThreadStop;
    int m_nHotplug_sleepMs;

    void *ctx = nullptr;

    std::map<std::string, Usb_Device> m_pUsb_devices;

    std::function<void(std::list<Usb_Device>, std::list<Usb_Device>)> m_pHotplug_callback = nullptr;

    friend class Usb_Device;
};

#endif // QUSBPLUGIN_H
