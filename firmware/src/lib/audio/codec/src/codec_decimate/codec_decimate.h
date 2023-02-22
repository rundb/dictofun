// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "codec.h"

namespace audio
{
namespace codec
{

template <typename RawSampleType, typename CodedSampleType>
class DecimatorCodec: public Codec<RawSampleType, CodedSampleType>
{
    static constexpr size_t decimation_step = sizeof(RawSampleType) / sizeof(CodedSampleType);
public:

    explicit DecimatorCodec(const size_t sample_size, const uint32_t sampling_frequency) 
    : _sampling_frequency{sampling_frequency}
    , _sample_size{sample_size}
    {
        static_assert(sizeof(RawSampleType) % sizeof(CodedSampleType) == 0, "Decimator codec wrongly configured");
    }
    DecimatorCodec(const DecimatorCodec&) = delete;
    DecimatorCodec(DecimatorCodec&&) = delete;
    DecimatorCodec& operator= (const DecimatorCodec&) = delete;
    DecimatorCodec& operator= (DecimatorCodec&&) = delete;
    ~DecimatorCodec() = default;

    void start() override;
    result::Result encode(RawSampleType& input, CodedSampleType& output) override;

    // Decoding can't be implementing in decimator, as we lose the data when decimation is performed
    result::Result decode(CodedSampleType& input, RawSampleType& output) override { return result::Result::ERROR_GENERAL; }
private:
    bool _is_next_frame_first{false};
    const uint32_t _sampling_frequency;
    const size_t _sample_size;

    static constexpr uint32_t max_codec_descriptor_size{std::min(sizeof(CodedSampleType), 24U)};
};

}
}

#include "codec_decimate.hpp"
