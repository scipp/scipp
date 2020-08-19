// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/variable/sort.h"
#include "scipp/variable/variable.h"

using namespace scipp;
using namespace scipp::variable;

TEST(Variable, sort_ascending) {
    auto var = makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 3}}, Values{1.0,3.0,2.0,4.0,0.0,5.0});
    EXPECT_EQ(sort(var, Dim::X, SortOrder::Ascending), makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 3}}, Values{1.0,2.0,3.0,0.0,4.0,5.0}));
}

TEST(Variable, sort_descending) {
    auto var = makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 3}}, Values{1.0,3.0,2.0,4.0,0.0,5.0});
    EXPECT_EQ(sort(var, Dim::X, SortOrder::Descending), makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 3}}, Values{3.0,2.0,1.0,5.0,4.0,0.0}));
}

TEST(Variable, sort_ascending_var) {
    auto var = makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 3}}, Values{1.0,3.0,2.0,4.0,0.0,5.0}, Variances{1.0,2.0,3.0,3.0,2.0,1.0});
    EXPECT_EQ(sort(var, Dim::X, SortOrder::Ascending), makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 3}}, Values{1.0,2.0,3.0,0.0,4.0,5.0}, Variances{1.0,3.0,2.0,2.0,3.0,1.0}));
}

TEST(Variable, sort_descending_var) {
    auto var = makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 3}}, Values{1.0,3.0,2.0,4.0,0.0,5.0}, Variances{1.0,2.0,3.0,3.0,2.0,1.0});
    EXPECT_EQ(sort(var, Dim::X, SortOrder::Descending), makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 3}}, Values{3.0,2.0,1.0,5.0,4.0,0.0}, Variances{2.0,3.0,1.0,1.0,3.0,2.0}));
}
