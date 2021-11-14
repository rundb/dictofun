#include "BleSystem.h"

ble::BleSystem bleSystem{};

int main()
{
    application_init();
    bleSystem.init();
    for (;;)
    {
        application_cyclic();
        bleSystem.cyclic();
    }
}