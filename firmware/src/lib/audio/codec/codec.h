// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#pragma once

#include "result.h"
#include <stdint.h>

namespace audio
{
namespace codec
{

template <uint32_t BufferSize>
struct Sample
{
    uint8_t data[BufferSize];
};

/// @brief This class represents an interface of a codec used in the system. 
/// @tparam CodedType type of the data to be encoded
/// @tparam RawType type of the data that leaves the encoder
template<typename RawType, typename CodedType>
class Codec
{
public:
    virtual result::Result encode(RawType& input, CodedType& output) = 0;
    virtual result::Result decode(CodedType& input, RawType& output) = 0;

    // Codec is responsible for providing information regarding the used codec and it's parameters to the user of data stream.
    // This method allows audio_processor to tell codec that next frame to be encoded shall be the first one.
    virtual void start() = 0;
};

}
}
