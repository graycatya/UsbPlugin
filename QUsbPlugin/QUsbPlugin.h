#ifndef QUSBPLUGIN_H
#define QUSBPLUGIN_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

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

private:
    QUsbPlugin();
    ~QUsbPlugin();

    void Register_LibUsb_Hotplug();
    void Register_Thread_Hotplug();
    void Deregister_LibUsb_Hotplug();
    void Deregister_Thread_Hotplug();

private:
    static QUsbPlugin* _instance;
    static std::mutex* m_pMutex;
    static std::thread* m_pNotplug_thread;
    static bool m_bNotplugThreadStop;

    void *ctx = nullptr;
};

#endif // QUSBPLUGIN_H
