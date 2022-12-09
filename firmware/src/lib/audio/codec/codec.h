// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#pragma once

#include "result.h"

namespace audio
{
/// @brief This class represents an interface of a codec used in the system. 
/// @tparam CodedType type of the data to be encoded
/// @tparam RawType type of the data that leaves the encoder
template<typename RawType, typename CodedType>
class Codec
{
public:
    virtual result::Result encode(RawType& input, CodedType& output) = 0;
    virtual result::Result decode(CodedType& input, RawType& output) = 0;
};

}
