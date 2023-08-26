// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "ble_fts.h"

namespace ble
{

void task_ble(void * context_ptr);

struct Context
{
    QueueHandle_t command_queue{nullptr};
    QueueHandle_t requests_queue{nullptr};
    QueueHandle_t keepalive_queue{nullptr};
    QueueHandle_t command_to_mem_queue{nullptr};
    QueueHandle_t status_from_mem_queue{nullptr};
    QueueHandle_t data_from_mem_queue{nullptr};
    QueueHandle_t commands_to_rtc_queue{nullptr};
};

enum class Command
{
    START,
    STOP,
    RESET_PAIRING,
    CONNECT_FS,
};

struct CommandQueueElement
{
    Command command_id;
};

enum class Request
{
    LED
};

struct RequestQueueElement
{
    Request request;
    static constexpr size_t max_args_count{1};
    uint32_t args[max_args_count];
};

enum class KeepaliveEvent
{
    CONNECTED,
    FILESYSTEM_EVENT,
    DISCONNECTED,
};

struct KeepaliveQueueElement
{
    KeepaliveEvent event;
};

enum class CommandToMemory
{
    GET_FILES_LIST,
    GET_FILE_INFO,
    OPEN_FILE,
    GET_FILE_DATA,
    CLOSE_FILE,
    GET_FS_STATUS,
    GET_FILES_LIST_NEXT,
};

struct CommandToMemoryQueueElement
{
    CommandToMemory command_id;
    ble::fts::file_id_type file_id;
};

enum class StatusFromMemory
{
    OK,
    ERROR_PERMISSION_DENIED,
    ERROR_FILE_NOT_FOUND,
    ERROR_FS_CORRUPT,
    ERROR_FILE_ALREADY_OPEN,
    ERROR_OTHER,
};

struct StatusFromMemoryQueueElement
{
    StatusFromMemory status;
    uint32_t data_size;
};

struct FileDataFromMemoryQueueElement
{
    static constexpr size_t element_max_size{256};
    uint32_t size{0};
    uint8_t data[element_max_size];
};

}
