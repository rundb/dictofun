#include "ble_fts_glue.h"

using namespace ble::fts;

namespace integration
{

ble::fts::BleInterface dictofun_ble_if
{

};

namespace test
{

result::Result dictofun_test_get_file_list(uint32_t& files_count, file_id_type * files_list_ptr)
{
    if (nullptr == files_list_ptr)
    {
        return result::Result::ERROR_INVALID_PARAMETER;
    }
    files_count = 0;
    return result::Result::OK;
}

FileSystemInterface dictofun_test_fs_if
{
    dictofun_test_get_file_list,
};

}

namespace target
{

}
}
