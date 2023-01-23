// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "ble_fts.h"

#include "ble_srv_common.h"
#include "ble.h"
#include "nrf_sdh_ble.h"

namespace ble
{
namespace fts
{

void on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);

__attribute__ ((section("." STRINGIFY(sdh_ble_observers4)))) __attribute__((used))
nrf_sdh_ble_evt_observer_t observer = {
    .handler = on_ble_evt,
    .p_context = nullptr,
};

FtsService::FtsService(FileSystemInterface& fs_if)
: _fs_if(fs_if)
{
    observer.p_context = &_context;
}

void init()
{
    // TODO: replace all calls to Nordic-dependent functions with a reference to _ble_if
    
    // declare the FT service

    // declare all characteristics of the service
}

}
}