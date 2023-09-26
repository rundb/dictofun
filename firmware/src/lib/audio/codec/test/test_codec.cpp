// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "codec.h"
#include "codec_adpcm.h"

#include <gtest/gtest.h>

#include <iostream>
using namespace std;

namespace
{
using namespace audio;

struct TestCodecSample
{
    uint8_t data[8];
};

class DummyCodec : public codec::Codec<TestCodecSample, TestCodecSample>
{
public:
    void start() override final { }
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

TEST(AdpcmInstantiation, BasicCheck)
{
    using InputSampleType = codec::Sample<64>;
    using OutputSampleType = codec::Sample<16>;
    codec::CodecAdpcm<InputSampleType, OutputSampleType> cut{16000};
    cut.init();
}

TEST(AdpcmEncoding, Basic)
{
    using InputSampleType = codec::Sample<64>;
    using OutputSampleType = codec::Sample<16>;
    codec::CodecAdpcm<InputSampleType, OutputSampleType> cut{16000};
    cut.init();
    InputSampleType input{{0xca, 0x13, 0xaa, 0x13, 0xe9, 0x13, 0x11, 0x0,  0x17, 0x0,  0x22,
                           0x0,  0x4a, 0x0,  0x6c, 0x0,  0x9a, 0x0,  0xcd, 0x0,  0x35, 0x1,
                           0xae, 0x1,  0x4f, 0x2,  0x57, 0x3,  0x24, 0x5,  0x92, 0x9,  0x74,
                           0x28, 0x2f, 0xd8, 0x2b, 0xe4, 0xf2, 0xe6, 0x54, 0xe8, 0x23, 0xe9,
                           0xb9, 0xe9, 0x34, 0xea, 0x6f, 0xea, 0xb5, 0xea, 0xe0, 0xea, 0xeb,
                           0xea, 0x15, 0xeb, 0x1a, 0xeb, 0x2c, 0xeb, 0x40, 0xeb}};
    OutputSampleType output;
    cut.encode(input, output);
    EXPECT_TRUE(output.data[0] != 0 || output.data[1] != 0);
}

} // namespace
