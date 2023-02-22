// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */
#pragma once

#include "dvi_adpcm.h"
#include <cstdio>

namespace audio
{
namespace codec
{

static dvi_adpcm_state_t adpcm_encoder_state;

template <typename RawSampleType, typename CodedSampleType, typename SingleSampleType>
result::Result CodecAdpcm<RawSampleType, CodedSampleType, SingleSampleType>::init()
{
    dvi_adpcm_init_state(&adpcm_encoder_state);
    return result::Result::OK;
}

template <typename RawSampleType, typename CodedSampleType, typename SingleSampleType>
void CodecAdpcm<RawSampleType, CodedSampleType, SingleSampleType>::start()
{
    _is_next_frame_first = true;
}

template <typename RawSampleType, typename CodedSampleType, typename SingleSampleType>
result::Result CodecAdpcm<RawSampleType, CodedSampleType, SingleSampleType>::encode(RawSampleType& input, CodedSampleType& output)
{
    int out_size{0};
    bool header_flag{false};
    const auto adpcm_encode_result = dvi_adpcm_encode(&input.data, sizeof(input), &output.data, &out_size, &adpcm_encoder_state, header_flag);
    if (0 != adpcm_encode_result)
    {
        return result::Result::ERROR_GENERAL;
    }

    if (_is_next_frame_first)
    {
        // insert codec info to the start of the output frame
        snprintf(reinterpret_cast<char *>(output.data), max_codec_descriptor_size, "%s;%lu;%d", "adpcm", _sampling_frequency, sizeof(SingleSampleType));
        _is_next_frame_first = false;
    }

    return result::Result::OK;
}
}
}
