// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "codec.h"
#include <algorithm>

namespace audio
{
namespace codec
{

template <typename RawSampleType, typename CodedSampleType, typename SingleSampleType = int16_t>
class CodecAdpcm: public Codec<RawSampleType, CodedSampleType>
{
public:

    explicit CodecAdpcm(const uint32_t sampling_frequency)
    : _sampling_frequency{sampling_frequency}
    {
        static_assert(sizeof(RawSampleType) == 4 * sizeof(CodedSampleType), "Wrong types configuration for ADPCM");
    }
    CodecAdpcm(const CodecAdpcm&) = delete;
    CodecAdpcm(CodecAdpcm&&) = delete;
    CodecAdpcm& operator= (const CodecAdpcm&) = delete;
    CodecAdpcm& operator= (CodecAdpcm&&) = delete;
    ~CodecAdpcm() = default;

    result::Result init();

    result::Result encode(RawSampleType& input, CodedSampleType& output) override;

    // No need in decoding on MCU
    result::Result decode(CodedSampleType& input, RawSampleType& output) override { return result::Result::ERROR_NOT_IMPLEMENTED; }
    void start() override;
private:
    const uint32_t _sampling_frequency;
    bool _is_next_frame_first{false};
    static constexpr uint32_t max_codec_descriptor_size{std::min(static_cast<uint32_t>(sizeof(CodedSampleType)), static_cast<uint32_t>(16UL))};

};

}
}

#include "codec_adpcm.hpp"
