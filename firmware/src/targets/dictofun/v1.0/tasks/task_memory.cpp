#include "tasks/task_memory.h"
#include "spi_access.h"

namespace memory
{

bool isMemoryErased()
{
    spi_flash_trigger_read(0x00, 8);
    while (spi_flash_is_spi_bus_busy()) ;
    uint8_t tmp[8];
    spi_flash_copy_received_data(tmp, 8);
    if (tmp[0] == 0xFF && tmp[1] == 0xFF)
    {
        return true;
    }
    else
    {
        return false;
    }
}

}