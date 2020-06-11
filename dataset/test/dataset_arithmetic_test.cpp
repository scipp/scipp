// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <initializer_list>

#include <gtest/gtest-matchers.h>
#include <gtest/gtest.h>

#include "scipp/core/dimensions.h"
#include "scipp/dataset/dataset.h"
#include "scipp/variable/arithmetic.h"

#include "../variable/test/make_events.h"
#include "dataset_test_common.h"
#include "test_macros.h"
#include "test_operations.h"

using namespace scipp;
using namespace scipp::dataset;

DatasetFactory3D datasetFactory() {
  static DatasetFactory3D factory;
  return factory;
}

template <class Op>
class DataArrayViewBinaryEqualsOpTest
    : public ::testing::Test,
      public ::testing::WithParamInterface<Op> {
protected:
  Op op;
};

template <class Op>
class DatasetBinaryEqualsOpTest : public ::testing::Test,
                                  public ::testing::WithParamInterface<Op> {
protected:
  Op op;
};

template <class Op>
class DatasetViewBinaryEqualsOpTest : public ::testing::Test,
                                      public ::testing::WithParamInterface<Op> {
protected:
  Op op;
};

template <class Op>
class DatasetMaskSlicingBinaryOpTest
    : public ::testing::Test,
      public ::testing::WithParamInterface<Op> {
protected:
  Op op;
};

std::tuple<Dataset, Dataset> generateBinaryOpTestCase() {
  constexpr auto lx = 5;
  constexpr auto ly = 5;

  Random rand;

  const auto coordX = rand(lx);
  const auto coordY = rand(ly);
  const auto labelT =
      makeVariable<double>(Dimensions{Dim::Y, ly}, Values(rand(ly)));
  const auto masks = makeVariable<bool>(Dimensions{Dim::Y, ly},
                                        Values(make_bools(ly, {false, true})));

  Dataset a;
  {
    a.setCoord(Dim::X,
               makeVariable<double>(Dims{Dim::X}, Shape{lx}, Values(coordX)));
    a.setCoord(Dim::Y,
               makeVariable<double>(Dims{Dim::Y}, Shape{ly}, Values(coordY)));

    a.setCoord(Dim("t"), labelT);
    a.setMask("mask", masks);
    a.setData("data_a",
              makeVariable<double>(Dimensions{Dim::X, lx}, Values(rand(lx))));
    a.setData("data_b",
              makeVariable<double>(Dimensions{Dim::Y, ly}, Values(rand(ly))));
  }

  Dataset b;
  {
    b.setCoord(Dim::X,
               makeVariable<double>(Dims{Dim::X}, Shape{lx}, Values(coordX)));
    b.setCoord(Dim::Y,
               makeVariable<double>(Dims{Dim::Y}, Shape{ly}, Values(coordY)));

    b.setCoord(Dim("t"), labelT);
    b.setMask("mask", masks);

    b.setData("data_a",
              makeVariable<double>(Dimensions{Dim::Y, ly}, Values(rand(ly))));
  }

  return std::make_tuple(a, b);
}

TYPED_TEST_SUITE(DataArrayViewBinaryEqualsOpTest, BinaryEquals);
TYPED_TEST_SUITE(DatasetBinaryEqualsOpTest, BinaryEquals);
TYPED_TEST_SUITE(DatasetViewBinaryEqualsOpTest, BinaryEquals);

TYPED_TEST(DataArrayViewBinaryEqualsOpTest, other_data_unchanged) {
  const auto dataset_b = datasetFactory().make();

  for (const auto &item : dataset_b) {
    auto dataset_a = datasetFactory().make();
    const auto original_a(dataset_a);
    auto target = dataset_a["data_zyx"];

    ASSERT_NO_THROW(TestFixture::op(target, item));

    for (const auto &data : dataset_a) {
      if (data.name() != "data_zyx") {
        EXPECT_EQ(data, original_a[data.name()]);
      }
    }
  }
}

TYPED_TEST(DataArrayViewBinaryEqualsOpTest, lhs_with_variance) {
  const auto dataset_b = datasetFactory().make();

  for (const auto &item : dataset_b) {
    const bool randomMasks = true;
    auto dataset_a = datasetFactory().make(randomMasks);
    auto target = dataset_a["data_zyx"];
    auto data_array = copy(target);

    Variable reference(target.data());
    TestFixture::op(reference, item.data());

    ASSERT_NO_THROW(target = TestFixture::op(target, item));
    EXPECT_EQ(target.data(), reference);
    EXPECT_EQ(TestFixture::op(data_array, item), target);
  }
}

TYPED_TEST(DataArrayViewBinaryEqualsOpTest, lhs_without_variance) {
  const auto dataset_b = datasetFactory().make();

  for (const auto &item : dataset_b) {
    const bool randomMasks = true;
    auto dataset_a = datasetFactory().make(randomMasks);
    auto target = dataset_a["data_xyz"];
    auto data_array = copy(target);

    if (item.hasVariances()) {
      ASSERT_ANY_THROW(TestFixture::op(target, item));
    } else {
      Variable reference(target.data());
      TestFixture::op(reference, item.data());

      ASSERT_NO_THROW(target = TestFixture::op(target, item));
      EXPECT_EQ(target.data(), reference);
      EXPECT_FALSE(target.hasVariances());
      EXPECT_EQ(TestFixture::op(data_array, item), target);
    }
  }
}

TYPED_TEST(DataArrayViewBinaryEqualsOpTest, slice_lhs_with_variance) {
  const auto dataset_b = datasetFactory().make();

  for (const auto &item : dataset_b) {
    const bool randomMasks = true;
    auto dataset_a = datasetFactory().make(randomMasks);
    auto target = dataset_a["data_zyx"];
    const auto &dims = item.dims();

    for (const Dim dim : dims.labels()) {
      Variable reference(target.data());
      TestFixture::op(reference, item.data().slice({dim, 2}));

      // Fails if any *other* multi-dimensional coord also depends on the
      // slicing dimension, since it will have mismatching values. Note that
      // this behavior is intended and important. It is crucial for preventing
      // operations between misaligned data in case a coordinate is
      // multi-dimensional.
      const auto coords = item.coords();
      if (std::all_of(coords.begin(), coords.end(), [dim](const auto &coord) {
            return dim_of_coord(coord.second, coord.first) == dim ||
                   !coord.second.dims().contains(dim);
          })) {
        ASSERT_NO_THROW(TestFixture::op(target, item.slice({dim, 2})));
        EXPECT_EQ(target.data(), reference);
      } else {
        ASSERT_ANY_THROW(TestFixture::op(target, item.slice({dim, 2})));
      }
    }
  }
}

// DataArrayViewBinaryEqualsOpTest ensures correctness of operations between
// DataArrayView with itself, so we can rely on that for building the reference.
TYPED_TEST(DatasetBinaryEqualsOpTest, return_value) {
  auto a = datasetFactory().make();
  auto b = datasetFactory().make();

  ASSERT_TRUE(
      (std::is_same_v<decltype(TestFixture::op(a, b["data_scalar"].data())),
                      Dataset &>));
  {
    const auto &result = TestFixture::op(a, b["data_scalar"].data());
    ASSERT_EQ(&result, &a);
  }

  ASSERT_TRUE((std::is_same_v<decltype(TestFixture::op(a, b["data_scalar"])),
                              Dataset &>));
  {
    const auto &result = TestFixture::op(a, b["data_scalar"]);
    ASSERT_EQ(&result, &a);
  }

  ASSERT_TRUE((std::is_same_v<decltype(TestFixture::op(a, b)), Dataset &>));
  {
    const auto &result = TestFixture::op(a, b);
    ASSERT_EQ(&result, &a);
  }

  ASSERT_TRUE(
      (std::is_same_v<decltype(TestFixture::op(a, b.slice({Dim::Z, 3}))),
                      Dataset &>));
  {
    const auto &result = TestFixture::op(a, b.slice({Dim::Z, 3}));
    ASSERT_EQ(&result, &a);
  }

  ASSERT_TRUE((std::is_same_v<decltype(TestFixture::op(a, 5.0 * units::one)),
                              Dataset &>));
  {
    const auto &result = TestFixture::op(a, 5.0 * units::one);
    ASSERT_EQ(&result, &a);
  }
}

TYPED_TEST(DatasetBinaryEqualsOpTest, rhs_DataArrayView_self_overlap) {
  auto dataset = datasetFactory().make();
  auto original(dataset);
  auto reference(dataset);

  ASSERT_NO_THROW(TestFixture::op(dataset, dataset["data_scalar"]));
  for (const auto &item : dataset) {
    EXPECT_EQ(item,
              TestFixture::op(reference[item.name()], original["data_scalar"]));
  }
}

TYPED_TEST(DatasetBinaryEqualsOpTest, rhs_Variable_self_overlap) {
  auto dataset = datasetFactory().make();
  auto original(dataset);
  auto reference(dataset);

  ASSERT_NO_THROW(TestFixture::op(dataset, dataset["data_scalar"].data()));
  for (const auto &item : dataset) {
    EXPECT_EQ(item, TestFixture::op(reference[item.name()],
                                    original["data_scalar"].data()));
  }
}

TYPED_TEST(DatasetBinaryEqualsOpTest, rhs_DataArrayView_self_overlap_slice) {
  auto dataset = datasetFactory().make();
  auto original(dataset);
  auto reference(dataset);

  ASSERT_NO_THROW(
      TestFixture::op(dataset, dataset["values_x"].slice({Dim::X, 1})));
  for (const auto &item : dataset) {
    EXPECT_EQ(item, TestFixture::op(reference[item.name()],
                                    original["values_x"].slice({Dim::X, 1})));
  }
}

TYPED_TEST(DatasetBinaryEqualsOpTest, rhs_Dataset) {
  auto a = datasetFactory().make();
  auto b = datasetFactory().make();
  auto reference(a);

  ASSERT_NO_THROW(TestFixture::op(a, b));
  for (const auto &item : a) {
    EXPECT_EQ(item, TestFixture::op(reference[item.name()], b[item.name()]));
  }
}

TYPED_TEST(DatasetBinaryEqualsOpTest, rhs_Dataset_coord_mismatch) {
  auto a = datasetFactory().make();
  DatasetFactory3D otherCoordsFactory;
  auto b = otherCoordsFactory.make();

  ASSERT_THROW(TestFixture::op(a, b), except::CoordMismatchError);
}

TYPED_TEST(DatasetBinaryEqualsOpTest, rhs_Dataset_with_missing_items) {
  auto a = datasetFactory().make();
  a.setData("extra", makeVariable<double>(Values{double{}}));
  auto b = datasetFactory().make();
  auto reference(a);

  ASSERT_NO_THROW(TestFixture::op(a, b));
  for (const auto &item : a) {
    if (item.name() == "extra") {
      EXPECT_EQ(item, reference[item.name()]);
    } else {
      EXPECT_EQ(item, TestFixture::op(reference[item.name()], b[item.name()]));
    }
  }
}

TYPED_TEST(DatasetBinaryEqualsOpTest, rhs_Dataset_with_extra_items) {
  auto a = datasetFactory().make();
  auto b = datasetFactory().make();
  b.setData("extra", makeVariable<double>(Values{double{}}));

  ASSERT_ANY_THROW(TestFixture::op(a, b));
}

TYPED_TEST(DatasetBinaryEqualsOpTest, rhs_DatasetView_self_overlap) {
  auto dataset = datasetFactory().make();
  const auto slice = dataset.slice({Dim::Z, 3});
  auto reference(dataset);

  ASSERT_NO_THROW(TestFixture::op(dataset, slice));
  for (const auto &item : dataset) {
    // Items independent of Z are removed when creating `slice`.
    if (item.dims().contains(Dim::Z)) {
      EXPECT_EQ(item,
                TestFixture::op(reference[item.name()],
                                reference[item.name()].slice({Dim::Z, 3})));
    } else {
      EXPECT_EQ(item, reference[item.name()]);
    }
  }
}

TYPED_TEST(DatasetBinaryEqualsOpTest, rhs_DatasetView_coord_mismatch) {
  auto dataset = datasetFactory().make();

  // Non-range sliced throws for X and Y due to multi-dimensional coords.
  ASSERT_THROW(TestFixture::op(dataset, dataset.slice({Dim::X, 3})),
               except::CoordMismatchError);
  ASSERT_THROW(TestFixture::op(dataset, dataset.slice({Dim::Y, 3})),
               except::CoordMismatchError);

  ASSERT_THROW(TestFixture::op(dataset, dataset.slice({Dim::X, 3, 4})),
               except::CoordMismatchError);
  ASSERT_THROW(TestFixture::op(dataset, dataset.slice({Dim::Y, 3, 4})),
               except::CoordMismatchError);
  ASSERT_THROW(TestFixture::op(dataset, dataset.slice({Dim::Z, 3, 4})),
               except::CoordMismatchError);
}

TYPED_TEST(DatasetBinaryEqualsOpTest,
           with_single_var_with_single_events_dimensions_sized_same) {
  Dataset a = make_simple_events({1.1, 2.2});
  Dataset b = make_simple_events({3.3, 4.4});
  Dataset c = TestFixture::op(a, b);
  auto c_data = c["events"].data().values<event_list<double>>()[0];
  ASSERT_EQ(c_data[0], TestFixture::op(1.1, 3.3));
  ASSERT_EQ(c_data[1], TestFixture::op(2.2, 4.4));
}

TYPED_TEST(DatasetBinaryEqualsOpTest,
           with_single_var_dense_and_events_dimension) {
  Dataset a = make_events_2d({1.1, 2.2});
  Dataset b = make_events_2d({3.3, 4.4});
  Dataset c = TestFixture::op(a, b);
  ASSERT_EQ(c["events"].data().values<event_list<double>>().size(), 2);
  auto c_data = c["events"].data().values<event_list<double>>()[0];
  ASSERT_EQ(c_data[0], TestFixture::op(1.1, 3.3));
  ASSERT_EQ(c_data[1], TestFixture::op(2.2, 4.4));
}

TYPED_TEST(DatasetBinaryEqualsOpTest, with_multiple_variables) {
  Dataset a = make_simple_events({1.1, 2.2});
  a.setData("events2", a["events"].data());
  Dataset b = make_simple_events({3.3, 4.4});
  b.setData("events2", b["events"].data());
  Dataset c = TestFixture::op(a, b);
  ASSERT_EQ(c.size(), 2);
  auto c_data = c["events"].data().values<event_list<double>>()[0];
  ASSERT_EQ(c_data[0], TestFixture::op(1.1, 3.3));
  ASSERT_EQ(c_data[1], TestFixture::op(2.2, 4.4));
  c_data = c["events2"].data().values<event_list<double>>()[1];
  ASSERT_EQ(c_data[0], TestFixture::op(1.1, 3.3));
  ASSERT_EQ(c_data[1], TestFixture::op(2.2, 4.4));
}

TYPED_TEST(DatasetBinaryEqualsOpTest,
           with_events_dimensions_of_different_sizes) {
  Dataset a = make_simple_events({1.1, 2.2});
  Dataset b = make_simple_events({3.3, 4.4, 5.5});
  ASSERT_THROW(TestFixture::op(a, b), std::runtime_error);
}

TYPED_TEST(DatasetBinaryEqualsOpTest, masks_propagate) {
  auto a = datasetFactory().make();
  auto b = datasetFactory().make();
  const auto expectedMasks =
      makeVariable<bool>(Dimensions{Dim::X, datasetFactory().lx},
                         Values(make_bools(datasetFactory().lx, true)));

  b.setMask("masks_x", expectedMasks);

  TestFixture::op(a, b);

  EXPECT_EQ(a.masks()["masks_x"], expectedMasks);
}

TYPED_TEST_SUITE(DatasetMaskSlicingBinaryOpTest, Binary);

TYPED_TEST(DatasetMaskSlicingBinaryOpTest, binary_op_on_sliced_masks) {
  auto a = make_1d_masked();

  const auto expectedMasks =
      makeVariable<bool>(Dimensions{Dim::X, 3}, Values(make_bools(3, true)));

  // these are conveniently 0 1 0 and 1 0 1
  const auto slice1 = a.slice({Dim::X, 0, 3});
  const auto slice2 = a.slice({Dim::X, 3, 6});

  const auto slice3 = TestFixture::op(slice1, slice2);

  EXPECT_EQ(slice3.masks()["masks_x"], expectedMasks);
}

TYPED_TEST(DatasetViewBinaryEqualsOpTest, return_value) {
  auto a = datasetFactory().make();
  auto b = datasetFactory().make();
  DatasetView view(a);

  ASSERT_TRUE((std::is_same_v<decltype(TestFixture::op(view, b["data_scalar"])),
                              DatasetView>));
  {
    const auto &result = TestFixture::op(view, b["data_scalar"]);
    EXPECT_EQ(&result["data_scalar"].template values<double>()[0],
              &a["data_scalar"].template values<double>()[0]);
  }

  ASSERT_TRUE(
      (std::is_same_v<decltype(TestFixture::op(view, b)), DatasetView>));
  {
    const auto &result = TestFixture::op(view, b);
    EXPECT_EQ(&result["data_scalar"].template values<double>()[0],
              &a["data_scalar"].template values<double>()[0]);
  }

  ASSERT_TRUE(
      (std::is_same_v<decltype(TestFixture::op(view, b.slice({Dim::Z, 3}))),
                      DatasetView>));
  {
    const auto &result = TestFixture::op(view, b.slice({Dim::Z, 3}));
    EXPECT_EQ(&result["data_scalar"].template values<double>()[0],
              &a["data_scalar"].template values<double>()[0]);
  }

  ASSERT_TRUE(
      (std::is_same_v<decltype(TestFixture::op(view, b["data_scalar"].data())),
                      DatasetView>));
  {
    const auto &result = TestFixture::op(view, b["data_scalar"].data());
    EXPECT_EQ(&result["data_scalar"].template values<double>()[0],
              &a["data_scalar"].template values<double>()[0]);
  }

  ASSERT_TRUE((std::is_same_v<decltype(TestFixture::op(view, 5.0 * units::one)),
                              DatasetView>));
  {
    const auto &result = TestFixture::op(view, 5.0 * units::one);
    EXPECT_EQ(&result["data_scalar"].template values<double>()[0],
              &a["data_scalar"].template values<double>()[0]);
  }
}

TYPED_TEST(DatasetViewBinaryEqualsOpTest, rhs_DataArrayView_self_overlap) {
  auto dataset = datasetFactory().make();
  auto reference(dataset);
  TestFixture::op(reference, dataset["data_scalar"]);

  for (scipp::index z = 0; z < dataset.coords()[Dim::Z].dims()[Dim::Z]; ++z) {
    for (const auto &item : dataset)
      if (item.dims().contains(Dim::Z)) {
        EXPECT_NE(item, reference[item.name()]);
      }
    ASSERT_NO_THROW(
        TestFixture::op(dataset.slice({Dim::Z, z}), dataset["data_scalar"]));
  }
  for (const auto &item : dataset)
    if (item.dims().contains(Dim::Z)) {
      EXPECT_EQ(item, reference[item.name()]);
    }
}

TYPED_TEST(DatasetViewBinaryEqualsOpTest,
           rhs_DataArrayView_self_overlap_slice) {
  auto dataset = datasetFactory().make();
  auto reference(dataset);
  TestFixture::op(reference, dataset["values_x"].slice({Dim::X, 1}));

  for (scipp::index z = 0; z < dataset.coords()[Dim::Z].dims()[Dim::Z]; ++z) {
    for (const auto &item : dataset)
      if (item.dims().contains(Dim::Z)) {
        EXPECT_NE(item, reference[item.name()]);
      }
    ASSERT_NO_THROW(TestFixture::op(dataset.slice({Dim::Z, z}),
                                    dataset["values_x"].slice({Dim::X, 1})));
  }
  for (const auto &item : dataset)
    if (item.dims().contains(Dim::Z)) {
      EXPECT_EQ(item, reference[item.name()]);
    }
}

TYPED_TEST(DatasetViewBinaryEqualsOpTest, rhs_Dataset_coord_mismatch) {
  DatasetFactory3D otherCoordsFactory;
  auto a = otherCoordsFactory.make();
  auto b = datasetFactory().make();

  ASSERT_THROW(TestFixture::op(DatasetView(a), b), except::CoordMismatchError);
}

TYPED_TEST(DatasetViewBinaryEqualsOpTest, rhs_Dataset_with_missing_items) {
  auto a = datasetFactory().make();
  a.setData("extra", makeVariable<double>(Values{double{}}));
  auto b = datasetFactory().make();
  auto reference(a);

  ASSERT_NO_THROW(TestFixture::op(DatasetView(a), b));
  for (const auto &item : a) {
    if (item.name() == "extra") {
      EXPECT_EQ(item, reference[item.name()]);
    } else {
      EXPECT_EQ(item, TestFixture::op(reference[item.name()], b[item.name()]));
    }
  }
}

TYPED_TEST(DatasetViewBinaryEqualsOpTest, rhs_Dataset_with_extra_items) {
  auto a = datasetFactory().make();
  auto b = datasetFactory().make();
  b.setData("extra", makeVariable<double>(Values{double{}}));

  ASSERT_ANY_THROW(TestFixture::op(DatasetView(a), b));
}

TYPED_TEST(DatasetViewBinaryEqualsOpTest, rhs_DatasetView_self_overlap) {
  auto dataset = datasetFactory().make();
  const auto slice = dataset.slice({Dim::Z, 3});
  auto reference(dataset);

  ASSERT_NO_THROW(TestFixture::op(dataset.slice({Dim::Z, 0, 3}), slice));
  ASSERT_NO_THROW(TestFixture::op(dataset.slice({Dim::Z, 3, 6}), slice));
  for (const auto &item : dataset) {
    // Items independent of Z are removed when creating `slice`.
    if (item.dims().contains(Dim::Z)) {
      EXPECT_EQ(item,
                TestFixture::op(reference[item.name()],
                                reference[item.name()].slice({Dim::Z, 3})));
    } else {
      EXPECT_EQ(item, reference[item.name()]);
    }
  }
}

TYPED_TEST(DatasetViewBinaryEqualsOpTest,
           rhs_DatasetView_self_overlap_undetectable) {
  auto dataset = datasetFactory().make();
  const auto slice = dataset.slice({Dim::Z, 3});
  auto reference(dataset);

  // Same as `rhs_DatasetView_self_overlap` above, but reverse slice order.
  // The second line will see the updated slice 3, and there is no way to
  // detect and prevent this.
  ASSERT_NO_THROW(TestFixture::op(dataset.slice({Dim::Z, 3, 6}), slice));
  ASSERT_NO_THROW(TestFixture::op(dataset.slice({Dim::Z, 0, 3}), slice));
  for (const auto &item : dataset) {
    // Items independent of Z are removed when creating `slice`.
    if (item.dims().contains(Dim::Z)) {
      EXPECT_NE(item,
                TestFixture::op(reference[item.name()],
                                reference[item.name()].slice({Dim::Z, 3})));
    } else {
      EXPECT_EQ(item, reference[item.name()]);
    }
  }
}

TYPED_TEST(DatasetViewBinaryEqualsOpTest, rhs_DatasetView_coord_mismatch) {
  auto dataset = datasetFactory().make();
  const DatasetView view(dataset);

  // Non-range sliced throws for X and Y due to multi-dimensional coords.
  ASSERT_THROW(TestFixture::op(view, dataset.slice({Dim::X, 3})),
               except::CoordMismatchError);
  ASSERT_THROW(TestFixture::op(view, dataset.slice({Dim::Y, 3})),
               except::CoordMismatchError);

  ASSERT_THROW(TestFixture::op(view, dataset.slice({Dim::X, 3, 4})),
               except::CoordMismatchError);
  ASSERT_THROW(TestFixture::op(view, dataset.slice({Dim::Y, 3, 4})),
               except::CoordMismatchError);
  ASSERT_THROW(TestFixture::op(view, dataset.slice({Dim::Z, 3, 4})),
               except::CoordMismatchError);
}

template <class Op>
class DatasetBinaryOpTest : public ::testing::Test,
                            public ::testing::WithParamInterface<Op> {
protected:
  Op op;
};

TYPED_TEST_SUITE(DatasetBinaryOpTest, Binary);

TYPED_TEST(DatasetBinaryOpTest, dataset_lhs_dataset_rhs) {
  const auto [dataset_a, dataset_b] = generateBinaryOpTestCase();

  const auto res = TestFixture::op(dataset_a, dataset_b);

  /* Only one variable should be present in result as only one common name
   * existed between input datasets. */
  EXPECT_EQ(1, res.size());

  /* Test that the dataset contains the equivalent of operating on the Variable
   * directly. */
  /* Correctness of results is tested via Variable tests. */
  const auto reference =
      TestFixture::op(dataset_a["data_a"].data(), dataset_b["data_a"].data());
  EXPECT_EQ(reference, res["data_a"].data());

  /* Expect coordinates to be copied to the result dataset */
  EXPECT_EQ(res.coords(), dataset_a.coords());
  EXPECT_EQ(res.masks(), dataset_a.masks());
}

TYPED_TEST(DatasetBinaryOpTest, dataset_lhs_variableconstview_rhs) {
  const auto [dataset_a, dataset_b] = generateBinaryOpTestCase();

  const auto res = TestFixture::op(dataset_a, dataset_b["data_a"].data());

  const auto reference =
      TestFixture::op(dataset_a["data_a"].data(), dataset_b["data_a"].data());
  EXPECT_EQ(reference, res["data_a"].data());
}

TYPED_TEST(DatasetBinaryOpTest, variableconstview_lhs_dataset_rhs) {
  const auto [dataset_a, dataset_b] = generateBinaryOpTestCase();

  const auto res = TestFixture::op(dataset_a["data_a"].data(), dataset_b);

  const auto reference =
      TestFixture::op(dataset_a["data_a"].data(), dataset_b["data_a"].data());
  EXPECT_EQ(reference, res["data_a"].data());
}

TYPED_TEST(DatasetBinaryOpTest, broadcast) {
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  const auto y = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{1, 2});
  const auto c = makeVariable<double>(Values{2.0});
  Dataset a;
  Dataset b;
  a.setCoord(Dim::X, x);
  a.setData("data1", x);
  a.setData("data2", x);
  b.setData("data1", c);
  b.setData("data2", c + c);
  const auto res = TestFixture::op(a, b);
  EXPECT_EQ(res["data1"].data(), TestFixture::op(x, c));
  EXPECT_EQ(res["data2"].data(), TestFixture::op(x, c + c));
}

TYPED_TEST(DatasetBinaryOpTest, dataset_lhs_scalar_rhs) {
  const auto dataset = std::get<0>(generateBinaryOpTestCase());
  const auto scalar = 4.5 * units::one;

  const auto res = TestFixture::op(dataset, scalar);

  /* Test that the dataset contains the equivalent of operating on the Variable
   * directly. */
  /* Correctness of results is tested via Variable tests. */
  const auto reference = TestFixture::op(dataset["data_a"].data(), scalar);
  EXPECT_EQ(reference, res["data_a"].data());

  /* Expect coordinates to be copied to the result dataset */
  EXPECT_EQ(res.coords(), dataset.coords());
}

TYPED_TEST(DatasetBinaryOpTest, scalar_lhs_dataset_rhs) {
  const auto dataset = std::get<0>(generateBinaryOpTestCase());
  const auto scalar = 4.5 * units::one;

  const auto res = TestFixture::op(scalar, dataset);

  /* Test that the dataset contains the equivalent of operating on the Variable
   * directly. */
  /* Correctness of results is tested via Variable tests. */
  const auto reference = TestFixture::op(scalar, dataset["data_a"].data());
  EXPECT_EQ(reference, res["data_a"].data());

  /* Expect coordinatesto be copied to the result dataset */
  EXPECT_EQ(res.coords(), dataset.coords());
}

TYPED_TEST(DatasetBinaryOpTest, dataset_events_lhs_dataset_events_rhs) {
  const auto dataset_a =
      make_events_with_coords_and_labels({1.1, 2.2}, {1.0, 2.0});
  const auto dataset_b =
      make_events_with_coords_and_labels({3.3, 4.4}, {1.0, 2.0});

  const auto res = TestFixture::op(dataset_a, dataset_b);

  /* Only one variable should be present in result as only one common name
   * existed between input datasets. */
  EXPECT_EQ(1, res.size());

  /* Test that the dataset contains the equivalent of operating on the Variable
   * directly. */
  /* Correctness of results is tested via Variable tests. */
  const auto reference =
      TestFixture::op(dataset_a["events"].data(), dataset_b["events"].data());
  EXPECT_EQ(reference, res["events"].data());

  EXPECT_EQ(dataset_a["events"].coords(), res["events"].coords());
}

TYPED_TEST(DatasetBinaryOpTest,
           dataset_events_lhs_dataarrayconstview_events_rhs) {
  const auto dataset_a =
      make_events_with_coords_and_labels({1.1, 2.2}, {1.0, 2.0});
  const auto dataset_b =
      make_events_with_coords_and_labels({3.3, 4.4}, {1.0, 2.0});

  const auto res = TestFixture::op(dataset_a, dataset_b["events"]);

  EXPECT_EQ(res, TestFixture::op(dataset_a, dataset_b));
}

TYPED_TEST(DatasetBinaryOpTest, events_with_dense_broadcast) {
  Dataset dense;
  dense.setData("a",
                makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 2}));
  Dataset events;
  events.setData("a", makeVariable<event_list<double>>(Dims{}, Shape{}));

  // Note: In the old way of handling event data, the events dim would result in
  // a failure here. Now we just get a broadcast, since `dense` has no coord
  // that would prevent this.
  ASSERT_NO_THROW(TestFixture::op(events, dense));
}

TYPED_TEST(DatasetBinaryOpTest, events_with_dense) {
  Dataset dense;
  dense.setData("a", makeVariable<double>(Values{2.0}));
  const auto events =
      make_events_with_coords_and_labels({1.1, 2.2}, {1.0, 2.0}, "a");

  const auto res = TestFixture::op(events, dense);

  EXPECT_EQ(res.size(), 1);
  EXPECT_TRUE(res.contains("a"));
  EXPECT_EQ(res["a"].data(),
            TestFixture::op(events["a"].data(), dense["a"].data()));
}

TYPED_TEST(DatasetBinaryOpTest, dense_with_events) {
  Dataset dense;
  dense.setData("a", makeVariable<double>(Values{2.0}));
  const auto events =
      make_events_with_coords_and_labels({1.1, 2.2}, {1.0, 2.0}, "a");

  const auto res = TestFixture::op(dense, events);

  EXPECT_EQ(res.size(), 1);
  EXPECT_TRUE(res.contains("a"));
  EXPECT_EQ(res["a"].data(),
            TestFixture::op(dense["a"].data(), events["a"].data()));
}

TYPED_TEST(DatasetBinaryOpTest,
           dataarrayconstview_events_lhs_dataset_events_rhs) {
  const auto dataset_a =
      make_events_with_coords_and_labels({1.1, 2.2}, {1.0, 2.0});
  const auto dataset_b =
      make_events_with_coords_and_labels({3.3, 4.4}, {1.0, 2.0});

  const auto res = TestFixture::op(dataset_a["events"], dataset_b);

  EXPECT_EQ(res, TestFixture::op(dataset_a, dataset_b));
}

TYPED_TEST(DatasetBinaryOpTest, events_dataarrayconstview_coord_mismatch) {
  const auto dataset_a =
      make_events_with_coords_and_labels({1.1, 2.2}, {1.0, 2.0});
  const auto dataset_b =
      make_events_with_coords_and_labels({3.3, 4.4}, {1.0, 2.1});

  ASSERT_THROW(TestFixture::op(dataset_a, dataset_b["events"]),
               except::CoordMismatchError);
  ASSERT_THROW(TestFixture::op(dataset_a["events"], dataset_b),
               except::CoordMismatchError);
}

TYPED_TEST(DatasetBinaryOpTest,
           dataset_events_lhs_dataset_events_rhs_fail_when_coords_mismatch) {
  auto dataset_a = make_simple_events({1.1, 2.2});
  auto dataset_b = make_simple_events({3.3, 4.4});

  {
    auto var = makeVariable<event_list<double>>(Dims{}, Shape{});
    var.values<event_list<double>>()[0] = {0.5, 1.0};
    dataset_a.coords().set(Dim::X, var);
  }

  {
    auto var = makeVariable<event_list<double>>(Dims{}, Shape{});
    var.values<event_list<double>>()[0] = {0.5, 1.5};
    dataset_b.coords().set(Dim::X, var);
  }

  EXPECT_THROW(TestFixture::op(dataset_a, dataset_b),
               except::CoordMismatchError);
}

TYPED_TEST(DatasetBinaryOpTest,
           dataset_events_lhs_dataset_events_rhs_fail_when_labels_mismatch) {
  auto dataset_a = make_simple_events({1.1, 2.2});
  auto dataset_b = make_simple_events({3.3, 4.4});

  {
    auto var = makeVariable<event_list<double>>(Dims{}, Shape{});
    var.values<event_list<double>>()[0] = {0.5, 1.0};
    dataset_a.coords().set(Dim("l"), var);
  }

  {
    auto var = makeVariable<event_list<double>>(Dims{}, Shape{});
    var.values<event_list<double>>()[0] = {0.5, 1.5};
    dataset_b.coords().set(Dim("l"), var);
  }

  EXPECT_THROW(TestFixture::op(dataset_a, dataset_b),
               except::CoordMismatchError);
}

TYPED_TEST(DatasetBinaryOpTest, dataset_lhs_datasetconstview_rhs) {
  auto dataset_a = datasetFactory().make();
  auto dataset_b = datasetFactory().make();

  DatasetConstView dataset_b_view(dataset_b);
  const auto res = TestFixture::op(dataset_a, dataset_b_view);

  for (const auto &item : res) {
    const auto reference = TestFixture::op(dataset_a[item.name()].data(),
                                           dataset_b[item.name()].data());
    EXPECT_EQ(reference, item.data());
  }
}

TYPED_TEST(DatasetBinaryOpTest, datasetconstview_lhs_dataset_rhs) {
  const auto dataset_a = datasetFactory().make();
  const auto dataset_b = datasetFactory().make().slice({Dim::X, 1});

  DatasetConstView dataset_a_view = dataset_a.slice({Dim::X, 1});
  const auto res = TestFixture::op(dataset_a_view, dataset_b);

  Dataset dataset_a_slice(dataset_a_view);
  const auto reference = TestFixture::op(dataset_a_slice, dataset_b);
  EXPECT_EQ(res, reference);
}

TYPED_TEST(DatasetBinaryOpTest, datasetconstview_lhs_datasetconstview_rhs) {
  auto dataset_a = datasetFactory().make();
  auto dataset_b = datasetFactory().make();

  DatasetConstView dataset_a_view(dataset_a);
  DatasetConstView dataset_b_view(dataset_b);
  const auto res = TestFixture::op(dataset_a_view, dataset_b_view);

  for (const auto &item : res) {
    const auto reference = TestFixture::op(dataset_a[item.name()].data(),
                                           dataset_b[item.name()].data());
    EXPECT_EQ(reference, item.data());
  }
}

TYPED_TEST(DatasetBinaryOpTest, dataset_lhs_dataarrayview_rhs) {
  auto dataset_a = datasetFactory().make();
  auto dataset_b = datasetFactory().make();

  const auto res = TestFixture::op(dataset_a, dataset_b["data_scalar"]);

  for (const auto &item : res) {
    const auto reference = TestFixture::op(dataset_a[item.name()].data(),
                                           dataset_b["data_scalar"].data());
    EXPECT_EQ(reference, item.data());
  }
}

TYPED_TEST(DatasetBinaryOpTest, masks_propagate) {
  auto a = datasetFactory().make();
  auto b = datasetFactory().make();

  const auto expectedMasks =
      makeVariable<bool>(Dimensions{Dim::X, datasetFactory().lx},
                         Values(make_bools(datasetFactory().lx, true)));

  b.setMask("masks_x", expectedMasks);

  const auto res = TestFixture::op(a, b);

  EXPECT_EQ(res.masks()["masks_x"], expectedMasks);
}

TEST(DatasetSetData, dense_to_dense) {
  auto dense = datasetFactory().make();
  auto d = Dataset(dense.slice({Dim::X, 0, 2}));
  dense.setData("data_x_1", dense["data_x"]);
  EXPECT_EQ(dense["data_x"], dense["data_x_1"]);

  EXPECT_THROW(dense.setData("data_x_2", d["data_x"]),
               except::VariableMismatchError);
}

TEST(DatasetSetData, dense_to_empty) {
  auto ds = Dataset();
  auto dense = datasetFactory().make();
  ds.setData("data_x", dense["data_x"]);
  EXPECT_EQ(dense["data_x"].coords(), ds["data_x"].coords());
  EXPECT_EQ(dense["data_x"].data(), ds["data_x"].data());
}

TEST(DatasetSetData, labels) {
  auto dense = datasetFactory().make();
  dense.setCoord(
      Dim("l"),
      makeVariable<double>(
          Dims{Dim::X}, Shape{dense.coords()[Dim::X].values<double>().size()}));
  auto d = Dataset(dense.slice({Dim::Y, 0}));
  dense.setData("data_x_1", dense["data_x"]);
  EXPECT_EQ(dense["data_x"], dense["data_x_1"]);

  d.setCoord(Dim("l1"), makeVariable<double>(
                            Dims{Dim::X},
                            Shape{d.coords()[Dim::X].values<double>().size()}));
  EXPECT_THROW(dense.setData("data_x_2", d["data_x"]), except::NotFoundError);
}

TEST(DatasetInPlaceStrongExceptionGuarantee, events) {
  auto good = make_events_variable_with_variance<double>();
  set_events_values<double>(good, {{1, 2, 3}, {4}});
  set_events_variances<double>(good, {{5, 6, 7}, {8}});
  auto bad = make_events_variable_with_variance<double>();
  set_events_values<double>(bad, {{0.1, 0.2, 0.3}, {0.4}});
  set_events_variances<double>(bad, {{0.5, 0.6}, {0.8}});
  DataArray good_array(good, {}, {});

  // We have no control over the iteration order in the implementation of binary
  // operations. All we know that data is in some sort of (unordered) map.
  // Therefore, we try all permutations of key names and insertion order, hoping
  // to cover also those that first process good items, then bad items (if bad
  // items are processed first, the exception guarantees of the underlying
  // binary operations for Variable are doing the job on their own, but we need
  // to exercise those for Dataset here).
  for (const auto &keys : {std::pair{"a", "b"}, std::pair{"b", "a"}}) {
    auto &[key1, key2] = keys;
    for (const auto &values : {std::pair{good, bad}, std::pair{bad, good}}) {
      auto &[value1, value2] = values;
      Dataset d;
      d.setData(key1, value1);
      d.setData(key2, value2);
      auto original(d);

      ASSERT_ANY_THROW(d += d);
      ASSERT_EQ(d, original);
      // Note that we should not use an item of d in this test, since then
      // operation is delayed and we me end up bypassing the problem that the
      // "dry run" fixes.
      ASSERT_ANY_THROW(d += good_array);
      ASSERT_EQ(d, original);
    }
  }
}

TEST(DatasetMaskContainer, can_contain_any_type_but_only_OR_EQ_bools) {
  Dataset a;
  a.setMask("double", makeVariable<double>(Dims{Dim::X}, Shape{3},
                                           Values{1.0, 2.0, 3.0}));
  ASSERT_THROW(a.masks()["double"] |= a.masks()["double"], std::runtime_error);
  a.setMask("float",
            makeVariable<float>(Dims{Dim::X}, Shape{3}, Values{1.0, 2.0, 3.0}));
  ASSERT_THROW(a.masks()["float"] |= a.masks()["float"], std::runtime_error);
  a.setMask("int64",
            makeVariable<int64_t>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}));
  ASSERT_THROW(a.masks()["int64"] |= a.masks()["int64"], std::runtime_error);
  a.setMask("int32",
            makeVariable<int32_t>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}));
  ASSERT_THROW(a.masks()["int32"] |= a.masks()["int32"], std::runtime_error);

  // success case
  a.setMask("bool", makeVariable<bool>(Dims{Dim::X}, Shape{3},
                                       Values{false, false, false}));
  ASSERT_NO_THROW(a.masks()["bool"] |= a.masks()["bool"]);
}

TEST(DatasetMaskContainer, can_contain_any_type_but_only_OR_bools) {
  Dataset a;
  a.setMask("double", makeVariable<double>(Dims{Dim::X}, Shape{3},
                                           Values{1.0, 2.0, 3.0}));
  ASSERT_THROW(a.masks()["double"] | a.masks()["double"], std::runtime_error);
  a.setMask("float",
            makeVariable<float>(Dims{Dim::X}, Shape{3}, Values{1.0, 2.0, 3.0}));
  ASSERT_THROW(a.masks()["float"] | a.masks()["float"], std::runtime_error);
  a.setMask("int64",
            makeVariable<int64_t>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}));
  ASSERT_THROW(a.masks()["int64"] | a.masks()["int64"], std::runtime_error);
  a.setMask("int32",
            makeVariable<int32_t>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}));
  ASSERT_THROW(a.masks()["int32"] | a.masks()["int32"], std::runtime_error);

  // success case
  a.setMask("bool", makeVariable<bool>(Dims{Dim::X}, Shape{3},
                                       Values{false, false, false}));
  ASSERT_NO_THROW(a.masks()["bool"] | a.masks()["bool"]);
}
