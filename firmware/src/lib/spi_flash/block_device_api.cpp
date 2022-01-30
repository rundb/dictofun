#include "lfs.h"
#include "block_device_api.h"
#include "spi_flash.h"
#include "app_timer.h"
#include "nrf_log.h"
    
static const uint32_t BLOCK_SIZE = 256;
static const uint32_t SECTOR_SIZE = 4096;

uint32_t _lastTransactionTimestamp{0};
static const uint32_t SPI_TRANSACTION_DELAY = 2U;

struct ProfileEntry
{
    uint32_t timestamp;
    uint32_t event;
    uint32_t address;
    uint32_t size;
};

static const size_t STATS_SIZE = 500;
static int _statsIdx{0};
static ProfileEntry _stats[STATS_SIZE];
#define OP_READ    1
#define OP_WRITE   2
#define OP_ERASE   3
inline void putStatsEntry(const int event, const uint32_t address, const uint32_t size)
{
    if (_statsIdx >= STATS_SIZE) return;
    _stats[_statsIdx].timestamp = app_timer_cnt_get();
    _stats[_statsIdx].address = address;
    _stats[_statsIdx].size = size;
    _stats[_statsIdx].event = event;
    ++_statsIdx;
}

void waitBetweenFrames()
{
    uint32_t timestamp;
    do {
        timestamp = app_timer_cnt_get();
    } while ((timestamp - _lastTransactionTimestamp) < SPI_TRANSACTION_DELAY);
}

int spi_flash_block_device_read(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size)
{
    waitBetweenFrames();
    const auto address = (block * BLOCK_SIZE) + off;
    putStatsEntry(OP_READ, address, size);
    auto& flashmem = flash::SpiFlash::getInstance();
    const auto res = flashmem.read(address, static_cast<uint8_t*>(buffer), size);
    if (res != flash::SpiFlash::Result::OK)
    {
        return -1;
    }
    while (flashmem.isBusy());

    _lastTransactionTimestamp = app_timer_cnt_get();
    return 0;
}

int spi_flash_block_device_prog(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size)
{
    waitBetweenFrames();
    const auto address = (block * BLOCK_SIZE) + off;
    putStatsEntry(OP_WRITE, address, size);
    auto& flashmem = flash::SpiFlash::getInstance();
    const auto res = flashmem.program(address, static_cast<const uint8_t *>(buffer), size);
    if (res != flash::SpiFlash::Result::OK)
    {
        return -1;
    }
    
    while (flashmem.isBusy());

    return 0;
}

int spi_flash_block_device_erase(const struct lfs_config *c, lfs_block_t block)
{
    waitBetweenFrames();
    const auto address = (block * SECTOR_SIZE);
    putStatsEntry(OP_ERASE, address, SECTOR_SIZE);
    auto& flashmem = flash::SpiFlash::getInstance();
    const auto res = flashmem.eraseSector(address);
    if (res != flash::SpiFlash::Result::OK)
    {
        return -1;
    }
    while (flashmem.isBusy());

    return 0;
}

int spi_flash_block_device_sync(const struct lfs_config *c)
{
    return 0;
}

static const size_t CACHE_SIZE = BLOCK_SIZE;
static const size_t LOOKAHEAD_BUFFER_SIZE = 16;
static uint8_t prog_buffer[CACHE_SIZE];
static uint8_t read_buffer[CACHE_SIZE];
static uint8_t lookahead_buffer[CACHE_SIZE];

// configuration of the filesystem is provided by this struct
const struct lfs_config lfs_configuration = {
    // block device operations
    .read  = spi_flash_block_device_read,
    .prog  = spi_flash_block_device_prog,
    .erase = spi_flash_block_device_erase,
    .sync  = spi_flash_block_device_sync,

    // block device configuration
    .read_size = 16,
    .prog_size = BLOCK_SIZE,
    .block_size = SECTOR_SIZE,
    .block_count = 4096,
    .block_cycles = 500,
    .cache_size = CACHE_SIZE,
    .lookahead_size = LOOKAHEAD_BUFFER_SIZE,
    .read_buffer = read_buffer,
    .prog_buffer = prog_buffer,
    .lookahead_buffer = lookahead_buffer,
};

static size_t ROTU_dbgIdx{0};
void ROTU_printDebugInfo()
{
    if (ROTU_dbgIdx >= _statsIdx) return;

    NRF_LOG_INFO("%d;%s;0x%x;%d", _stats[ROTU_dbgIdx].timestamp, ((_stats[ROTU_dbgIdx].event == 1) ? "read" : (_stats[ROTU_dbgIdx].event == 2) ? "prog" : "erase"), _stats[ROTU_dbgIdx].address, _stats[ROTU_dbgIdx].size)
    ++ROTU_dbgIdx;
}
