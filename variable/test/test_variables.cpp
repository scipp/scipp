// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <gtest/gtest.h>

#include "test_variables.h"

using namespace scipp;

INSTANTIATE_TEST_SUITE_P(
    Scalar, DenseVariablesTest,
    testing::Values(makeVariable<double>(Values{1.2}),
                    makeVariable<double>(Values{1.2}, Variances{1.3}),
                    makeVariable<float>(Values{1.2}, units::m),
                    makeVariable<int64_t>(Values{12}),
                    makeVariable<int32_t>(Values{4}, units::s),
                    makeVariable<std::string>(Values{"abc"})));

INSTANTIATE_TEST_SUITE_P(
    1D, DenseVariablesTest,
    testing::Values(makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m,
                                         Values{1, 2}, Variances{3, 4}),
                    makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m,
                                         Values{1, 2}),
                    makeVariable<double>(Dims{Dim::Y}, Shape{3}, units::s,
                                         Values{1, 2, 3}, Variances{3, 4, 5}),
                    makeVariable<std::string>(Dims{Dim::Row}, Shape{3},
                                              Values{"abc", "de", "f"})));

INSTANTIATE_TEST_SUITE_P(
    2D, DenseVariablesTest,
    testing::Values(makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                         units::m, Values{1, 2, 3, 4, 5, 6},
                                         Variances{1, 1, 2, 2, 3, 3})));
