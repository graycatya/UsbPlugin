#include "QUsbPlugin.h"


int main(int argc, char *argv[])
{
    QUsbPlugin::Instance()->Register_Hotplug();

    QUsbPlugin::Instance()->Deregister_Hotplug();
}
