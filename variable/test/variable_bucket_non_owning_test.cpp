// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <fix_typed_test_suite_warnings.h>

#include "scipp/variable/bins.h"
#include "scipp/variable/operations.h"
#include "scipp/variable/string.h"

using namespace scipp;

class VariableBucketNonOwningTest : public ::testing::Test {
protected:
  Variable buffer =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  const Variable indices = makeVariable<index_pair>(
      Dims{Dim::Y}, Shape{3},
      Values{index_pair{0, 1}, index_pair{1, 3}, index_pair{3, 4}});
  Variable view = make_non_owning_bins(indices, Dim::X, VariableView(buffer));
};

TEST_F(VariableBucketNonOwningTest, slicing) {
  const auto sliced_indices = indices.slice({Dim::Y, 1, 3});
  const auto view_sliced =
      make_non_owning_bins(sliced_indices, Dim::X, VariableView(buffer));
  EXPECT_EQ(view_sliced, view.slice({Dim::Y, 1, 3}));
}

TEST_F(VariableBucketNonOwningTest, copy) {
  // Still a non-owning view, no copy of data or indices is made.
  Variable copy(view);
  EXPECT_EQ(copy, view);
  view.values<bucket<VariableView>>()[0] +=
      view.values<bucket<VariableView>>()[2];
  EXPECT_EQ(copy, view);
}

TEST_F(VariableBucketNonOwningTest, assign) {
  Variable buffer_copy(buffer);
  Variable copy =
      make_non_owning_bins(indices, Dim::X, VariableView(buffer_copy));
  view.values<bucket<VariableView>>()[0] +=
      view.values<bucket<VariableView>>()[2];
  EXPECT_NE(copy, view);
  // Assignment changes referenced buffer rather than assigning values
  copy = view;
  EXPECT_EQ(copy, view);
  view.values<bucket<VariableView>>()[0] +=
      view.values<bucket<VariableView>>()[2];
  EXPECT_EQ(copy, view);
}

TEST_F(VariableBucketNonOwningTest, copy_view) {
  // Should still be a non-owning view, no copy of data or indices is made, but
  // not implemented right now.
  EXPECT_ANY_THROW(Variable(view.slice({Dim::Y, 0, 2})));
}

template <class T>
class VariableBucketNonOwningTypedTest : public VariableBucketNonOwningTest {
protected:
  using ViewType = T;
  Variable view = make_non_owning_bins(indices, Dim::X, T(buffer));
  T get_view() { return view; }
  T get_buffer() { return buffer; }
};
using VariableBucketNonOwningTypes =
    ::testing::Types<VariableConstView, VariableView>;
TYPED_TEST_SUITE(VariableBucketNonOwningTypedTest,
                 VariableBucketNonOwningTypes);

TYPED_TEST(VariableBucketNonOwningTypedTest, constituents) {
  auto [idx, dim, buf] =
      TestFixture::get_view()
          .template constituents<bucket<typename TestFixture::ViewType>>();
  EXPECT_EQ(idx, this->indices);
  EXPECT_EQ(dim, Dim::X);
  EXPECT_EQ(buf, this->buffer);
}

TYPED_TEST(VariableBucketNonOwningTypedTest, constituents_slice) {
  auto [idx, dim, buf] =
      TestFixture::get_view()
          .slice({Dim::Y, 1, 3})
          .template constituents<bucket<typename TestFixture::ViewType>>();
  EXPECT_EQ(idx, this->indices.slice({Dim::Y, 1, 3}));
  EXPECT_EQ(dim, Dim::X);
  EXPECT_EQ(buf, this->buffer);
}

TYPED_TEST(VariableBucketNonOwningTypedTest, constituents_slice_of_slice) {
  const auto sliced_indices = this->indices.slice({Dim::Y, 1, 3});
  const auto view_sliced =
      make_non_owning_bins(sliced_indices, Dim::X, TestFixture::get_buffer());
  auto [idx, dim, buf] =
      view_sliced.slice({Dim::Y, 1, 2})
          .template constituents<bucket<typename TestFixture::ViewType>>();
  EXPECT_EQ(idx, this->indices.slice({Dim::Y, 2, 3}));
  EXPECT_EQ(dim, Dim::X);
  EXPECT_EQ(buf, this->buffer);
}
