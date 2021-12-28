/*
 * Copyright (c) 2021 Roman Turkin 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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