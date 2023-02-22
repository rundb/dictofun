// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */
#pragma once

#include <cstring>

namespace audio
{
namespace codec
{
template <typename RawSampleType, typename CodedSampleType>
void DecimatorCodec<RawSampleType, CodedSampleType>::start()
{
    _is_next_frame_first = true;
}

template <typename RawSampleType, typename CodedSampleType>
result::Result DecimatorCodec<RawSampleType, CodedSampleType>::encode(RawSampleType& input, CodedSampleType& output)
{
    for (auto i = 0U; i < sizeof(input.data); i += _sample_size * decimation_step)
    {
        // TODO: get rid of explicit use of uint16_t (probably need to pass this as a template parameter)
        uint16_t input_sample = *reinterpret_cast<uint16_t *>(&input.data[i]);
        *reinterpret_cast<uint16_t *>(&output.data[i / decimation_step]) = input_sample;
    }
    if (_is_next_frame_first)
    {
        _is_next_frame_first = false;
        snprintf(reinterpret_cast<char *>(output.data), max_codec_descriptor_size, "%s;%lu;%d;%d", "decim", _sampling_frequency, _sample_size, decimation_step);
    }
    return result::Result::OK;
}
}
}
