// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <gtest/gtest.h>

#include "scipp/variable/bins.h"
#include "test_variables.h"

using namespace scipp;

INSTANTIATE_TEST_SUITE_P(
    Scalar, DenseVariablesTest,
    testing::Values(makeVariable<double>(Values{1.2}),
                    makeVariable<double>(Values{1.2}, Variances{1.3}),
                    makeVariable<float>(Values{1.2}, sc_units::m),
                    makeVariable<int64_t>(Values{12}),
                    makeVariable<int32_t>(Values{4}, sc_units::s),
                    makeVariable<std::string>(Values{"abc"})));

INSTANTIATE_TEST_SUITE_P(
    1D, DenseVariablesTest,
    testing::Values(makeVariable<double>(Dims{Dim::X}, Shape{0}, sc_units::m),
                    makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m,
                                         Values{1, 2}, Variances{3, 4}),
                    makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m,
                                         Values{1, 2}),
                    makeVariable<double>(Dims{Dim::Y}, Shape{3}, sc_units::s,
                                         Values{1, 2, 3}, Variances{3, 4, 5}),
                    makeVariable<std::string>(Dims{Dim::Row}, Shape{3},
                                              Values{"abc", "de", "f"})));

INSTANTIATE_TEST_SUITE_P(
    2D, DenseVariablesTest,
    testing::Values(
        makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{0, 0}, sc_units::m),
        makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{0, 2}, sc_units::m),
        makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 0}, sc_units::m),
        makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3}, sc_units::m,
                             Values{1, 2, 3, 4, 5, 6},
                             Variances{1, 1, 2, 2, 3, 3})));

namespace {
Variable indices = makeVariable<scipp::index_pair>(
    Dims{Dim::X}, Shape{2}, Values{std::pair{0, 2}, std::pair{2, 4}});
Variable buffer =
    makeVariable<double>(Dims{Dim::Event}, Shape{4}, Values{1, 2, 3, 4});
} // namespace

INSTANTIATE_TEST_SUITE_P(1D, BinnedVariablesTest,
                         testing::Values(make_bins(indices, Dim::Event,
                                                   buffer)));
