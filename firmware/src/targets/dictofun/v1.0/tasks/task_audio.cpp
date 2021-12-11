#include "tasks/task_audio.h"
#include "tasks/task_state.h"
#include <spi_access.h>
#include <nrfx.h>

drv_audio_frame_t * pending_frame = NULL;
size_t write_pointer = 0;
#define SPI_XFER_DATA_STEP 0x100

void audio_init()
{
    drv_audio_init(audio_frame_cb);   
}

void audio_start_record()
{
    drv_audio_transmission_enable();
}

// TODO: replace with setting a flag that stops the recording in 
//       the interrupt. Thus we can assure that last chunk is saved.
void audio_stop_record()
{
    drv_audio_transmission_disable();
}

void audio_frame_handle()
{
  if(pending_frame) 
  {
    const int data_size = pending_frame->buffer_occupied;
    NRFX_ASSERT(data_size < SPI_XFER_DATA_STEP);
    NRFX_ASSERT(application::getApplicationState() == application::APP_SM_RECORDING);
      //  nrf_gpio_pin_set(17);
    spi_flash_write(write_pointer, data_size, pending_frame->buffer);
       //     nrf_gpio_pin_clear(17);
    write_pointer += SPI_XFER_DATA_STEP;

    pending_frame->buffer_free_flag = true;
    pending_frame->buffer_occupied = 0;
    pending_frame = NULL;
  }
}

void audio_frame_cb(drv_audio_frame_t * frame)
{
  NRFX_ASSERT(NULL == pending_frame);
  pending_frame = frame;
}

// In fact, 2x of size is returned - as we write only half of the available memory
size_t audio_get_record_size()
{
    return write_pointer;
}
