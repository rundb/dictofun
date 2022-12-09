// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "codec.h"

#include <gtest/gtest.h>


namespace
{
using namespace audio;

struct TestCodecSample
{
    uint8_t data[8];
};

class DummyCodec: public::Codec<TestCodecSample, TestCodecSample>
{
public:
    result::Result encode(TestCodecSample& input, TestCodecSample& output) override final
    {
        memcpy(&output, &input, sizeof(input));
        return result::Result::OK;
    }
    result::Result decode(TestCodecSample& input, TestCodecSample& output) override final
    {
        memcpy(&output, &input, sizeof(input));
        return result::Result::OK;
    }
};

TEST(CodecInstantiation, BasicCheck)
{
    DummyCodec cut;
}

}
