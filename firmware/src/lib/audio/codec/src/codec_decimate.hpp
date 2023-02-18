// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */
#pragma once

namespace audio
{
namespace codec
{
template <typename RawSampleType, typename CodedSampleType>
result::Result DecimatorCodec<RawSampleType, CodedSampleType>::encode(RawSampleType& input, CodedSampleType& output)
{
    for (auto i = 0U; i < sizeof(input.data); i += decimation_step)
    {
        output.data[i / decimation_step] = input.data[i + 1];
    }
    return result::Result::OK;
}
}
}
