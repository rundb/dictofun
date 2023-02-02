#include "ble_fts_glue.h"

using namespace ble::fts;

namespace integration
{

namespace test
{

constexpr size_t test_files_count{2};
ble::fts::file_id_type test_files_ids[test_files_count] {
    0x0102030405060708ULL,
    0x0102030405060709ULL,
};

result::Result dictofun_test_get_file_list(uint32_t& files_count, file_id_type * files_list_ptr)
{
    if (nullptr == files_list_ptr)
    {
        return result::Result::ERROR_INVALID_PARAMETER;
    }
    files_count = test_files_count;
    memcpy(files_list_ptr, test_files_ids, sizeof(test_files_ids));
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
