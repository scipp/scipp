// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/common/index.h"

TEST(IndexTest, size) { ASSERT_EQ(sizeof(scipp::index), 8); }
TEST(IndexTest, sign) { ASSERT_EQ(scipp::index{-1}, int64_t(-1)); }
