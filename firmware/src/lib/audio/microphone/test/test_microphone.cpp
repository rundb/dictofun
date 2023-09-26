// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "mock/microphone_mock.h"

#include <gtest/gtest.h>

namespace
{
using namespace audio;

TEST(MicrophoneInstantiation, BasicCheck)
{
    MicrophoneMock cut;
}

} // namespace
