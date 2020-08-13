// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/variable/sort.h"
#include "scipp/variable/variable.h"

using namespace scipp;
using namespace scipp::variable;

TEST(Variable, sort1d) {
    auto var = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1.0, 3.0, 2.0});
    EXPECT_EQ(sort(var), makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1.0, 2.0, 3.0}));
}

TEST(Variable, sort2d_x) {
    auto var = makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 3}}, Values{1.0,3.0,2.0,4.0,0.0,5.0});
    EXPECT_EQ(sort(var), makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 3}}, Values{1.0,2.0,3.0,0.0,4.0,5.0}));
}

TEST(Variable, sort2d_y) {
    auto var = makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 3}}, Values{1.0,3.0,2.0,4.0,0.0,5.0});
    EXPECT_EQ(sort(var), makeVariable<double>(Dimensions{{Dim::X,3}, {Dim::Y,2}}, Values{1.0,4.0,0.0,3.0,2.0,5.0}));
}