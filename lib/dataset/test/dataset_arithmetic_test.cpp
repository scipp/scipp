// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <initializer_list>

#include <gtest/gtest-matchers.h>
#include <gtest/gtest.h>

#include "scipp/core/dimensions.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/util.h"

#include "dataset_test_common.h"
#include "test_macros.h"
#include "test_operations.h"

using namespace scipp;
using namespace scipp::dataset;

DatasetFactory3D datasetFactory() {
  static DatasetFactory3D factory;
  return factory;
}

template <class T> auto make_no_variances(T &&factory) {
  auto ds = factory.make();
  for (auto &&x : ds)
    x.data().setVariances({});
  return ds;
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
  const auto mask = makeVariable<bool>(Dimensions{Dim::Y, ly},
                                       Values(make_bools(ly, {false, true})));

  Dataset a;
  {
    a.setCoord(Dim::X,
               makeVariable<double>(Dims{Dim::X}, Shape{lx}, Values(coordX)));
    a.setCoord(Dim::Y,
               makeVariable<double>(Dims{Dim::Y}, Shape{ly}, Values(coordY)));
    a.setCoord(Dim("t"), labelT);
    a.setData("data_a",
              makeVariable<double>(Dimensions{Dim::X, lx}, Values(rand(lx))));
    a["data_a"].masks().set("mask", makeVariable<bool>(Values{false}));
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
    b.setData("data_a",
              makeVariable<double>(Dimensions{Dim::Y, ly}, Values(rand(ly))));
    b["data_a"].masks().set("mask", mask);
  }

  return std::make_tuple(a, b);
}

TYPED_TEST_SUITE(DataArrayViewBinaryEqualsOpTest, BinaryEquals);
TYPED_TEST_SUITE(DatasetBinaryEqualsOpTest, BinaryEquals);
TYPED_TEST_SUITE(DatasetViewBinaryEqualsOpTest, BinaryEquals);

TYPED_TEST(DataArrayViewBinaryEqualsOpTest, other_data_unchanged) {
  const auto dataset_b = make_no_variances(datasetFactory());

  for (const auto &item : dataset_b) {
    auto dataset_a = datasetFactory().make();
    const auto original_a = copy(dataset_a);
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
    auto target = copy(dataset_a["data_zyx"]);
    auto data_array = copy(target);

    auto reference = copy(target.data());

    if (item.has_variances() && target.dims() != item.dims()) {
      ASSERT_THROW(TestFixture::op(target, item), except::VariancesError);
      continue;
    }

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
    auto target = copy(dataset_a["data_xyz"]);
    auto data_array = copy(target);

    if (item.has_variances()) {
      ASSERT_ANY_THROW(TestFixture::op(target, item));
    } else {
      auto reference = copy(target.data());
      TestFixture::op(reference, item.data());

      ASSERT_NO_THROW(target = TestFixture::op(target, item));
      EXPECT_EQ(target.data(), reference);
      EXPECT_FALSE(target.has_variances());
      EXPECT_EQ(TestFixture::op(data_array, item), target);
    }
  }
}

TYPED_TEST(DataArrayViewBinaryEqualsOpTest, slice_lhs_with_variance) {
  const auto dataset_b = datasetFactory().make();

  for (const auto &item : dataset_b) {
    const bool randomMasks = true;
    auto dataset_a = datasetFactory().make(randomMasks);
    auto target = copy(dataset_a["data_zyx"]);
    const auto &dims = item.dims();

    for (const Dim dim : dims.labels()) {
      auto reference = copy(target.data());

      auto slice = copy(item.slice({dim, 2}));
      if (item.has_variances()) {
        ASSERT_THROW(TestFixture::op(target, slice.data()),
                     except::VariancesError);
        slice.setData(values(slice.data()));
      }

      TestFixture::op(reference, slice.data());

      // Fails if any *other* multi-dimensional coord also depends on the
      // slicing dimension, since it will have mismatching values. Note that
      // this behavior is intended and important. It is crucial for preventing
      // operations between misaligned data in case a coordinate is
      // multi-dimensional.
      const auto coords = item.coords();
      if (std::all_of(coords.begin(), coords.end(), [&](const auto &coord) {
            return coords.dim_of(coord.first) == dim ||
                   !coord.second.dims().contains(dim);
          })) {
        ASSERT_NO_THROW(TestFixture::op(target, slice));
        EXPECT_EQ(target.data(), reference);
      } else {
        ASSERT_ANY_THROW(TestFixture::op(target, slice));
      }
    }
  }
}

// DataArrayViewBinaryEqualsOpTest ensures correctness of operations between
// DataArrayView with itself, so we can rely on that for building the reference.
TYPED_TEST(DatasetBinaryEqualsOpTest, return_value) {
  auto a = make_no_variances(datasetFactory());
  auto b = make_no_variances(datasetFactory());

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
  auto original = copy(dataset);
  auto reference = copy(dataset);

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
  auto original = copy(dataset);
  auto reference = copy(dataset);

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
  auto dataset = make_no_variances(datasetFactory());
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
  auto dataset = datasetFactory().make()["data_xyz"];

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

TYPED_TEST(DatasetBinaryEqualsOpTest, masks_propagate) {
  auto a = datasetFactory().make();
  auto b = datasetFactory().make();
  const auto expectedMask =
      makeVariable<bool>(Dimensions{Dim::X, datasetFactory().lx},
                         Values(make_bools(datasetFactory().lx, true)));

  for (const auto &item :
       {"values_x", "data_x", "data_xy", "data_zyx", "data_xyz"})
    b[item].masks().set("masks_x", expectedMask);

  TestFixture::op(a, b);

  for (const auto &item :
       {"values_x", "data_x", "data_xy", "data_zyx", "data_xyz"})
    EXPECT_EQ(a[item].masks()["masks_x"], expectedMask);
}

TYPED_TEST(DatasetBinaryEqualsOpTest, masks_not_shared) {
  // See test of same name in DatasetBinaryOpTest
  auto a = datasetFactory().make();
  auto b = datasetFactory().make();

  const auto maskA = makeVariable<bool>(Dimensions{Dim::X, datasetFactory().lx},
                                        Values{false, false, true, false});
  const auto maskB = makeVariable<bool>(Dimensions{Dim::X, datasetFactory().lx},
                                        Values{false, false, false, true});
  const auto maskC = makeVariable<bool>(Dimensions{Dim::X, datasetFactory().lx},
                                        Values{false, true, false, false});

  for (const auto &item :
       {"values_x", "data_x", "data_xy", "data_zyx", "data_xyz"}) {
    a[item].masks().set("mask_ab", copy(maskA));
    b[item].masks().set("mask_ab", copy(maskB));
    b[item].masks().set("mask_b", copy(maskB));
  }

  TestFixture::op(a, b);

  for (const auto &item :
       {"values_x", "data_x", "data_xy", "data_zyx", "data_xyz"}) {
    a[item].masks()["mask_b"] |= maskC;
    a[item].masks()["mask_ab"] |= maskC;
    EXPECT_EQ(b[item].masks()["mask_ab"], maskB);
    EXPECT_EQ(b[item].masks()["mask_b"], maskB);
  }
}

TYPED_TEST_SUITE(DatasetMaskSlicingBinaryOpTest, Binary);

TYPED_TEST(DatasetMaskSlicingBinaryOpTest, binary_op_on_sliced_masks) {
  auto a = make_1d_masked();

  const auto expectedMask =
      makeVariable<bool>(Dimensions{Dim::X, 3}, Values(make_bools(3, true)));

  // these are conveniently 0 1 0 and 1 0 1
  const auto slice1 = a.slice({Dim::X, 0, 3});
  const auto slice2 = a.slice({Dim::X, 3, 6});

  const auto slice3 = TestFixture::op(slice1, slice2);

  EXPECT_EQ(slice3["data_x"].masks()["masks_x"], expectedMask);
}

TYPED_TEST(DatasetViewBinaryEqualsOpTest,
           rhs_DataArray_prevented_modification_of_lower_dimensional_items) {
  auto dataset = datasetFactory().make();
  auto reference = copy(dataset);
  for (scipp::index z = 0; z < dataset.coords()[Dim::Z].dims()[Dim::Z]; ++z) {
    ASSERT_THROW(
        TestFixture::op(dataset.slice({Dim::Z, z}), dataset["data_scalar"]),
        except::VariableError);
  }
  EXPECT_EQ(dataset, reference);
}

TYPED_TEST(DatasetViewBinaryEqualsOpTest, rhs_DataArray_self_overlap) {
  auto dataset = datasetFactory().make();
  for (const auto &name : {"values_x", "data_x", "data_xy", "data_scalar"})
    dataset.erase(name);
  // Unless we take the last slice this will not work
  const auto slice = dataset["data_xyz"].slice({Dim::Z, 5});
  auto reference = copy(dataset);

  for (scipp::index z = 0; z < dataset.coords()[Dim::Z].dims()[Dim::Z]; ++z)
    ASSERT_NO_THROW(TestFixture::op(dataset.slice({Dim::Z, z}), slice));
  const auto rhs = copy(reference["data_xyz"].slice({Dim::Z, 5}));
  for (const auto &item : dataset) {
    EXPECT_EQ(item, TestFixture::op(reference[item.name()], rhs));
  }
}

TYPED_TEST(DatasetViewBinaryEqualsOpTest, rhs_Dataset_coord_mismatch) {
  DatasetFactory3D otherCoordsFactory;
  auto a = otherCoordsFactory.make();
  auto b = datasetFactory().make();

  ASSERT_THROW(TestFixture::op(copy(a), b), except::CoordMismatchError);
}

TYPED_TEST(DatasetViewBinaryEqualsOpTest, rhs_Dataset_with_missing_items) {
  auto a = datasetFactory().make();
  a.setData("extra", makeVariable<double>(Values{double{}}));
  auto b = datasetFactory().make();
  auto reference(a);

  ASSERT_NO_THROW(TestFixture::op(copy(a), b));
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

  ASSERT_ANY_THROW(TestFixture::op(copy(a), b));
}

TYPED_TEST(DatasetViewBinaryEqualsOpTest,
           rhs_Dataset_prevented_modification_of_lower_dimensional_items) {
  auto dataset = datasetFactory().make();
  const auto slice = dataset.slice({Dim::Z, 3});
  auto reference = copy(dataset);
  ASSERT_THROW(TestFixture::op(dataset.slice({Dim::Z, 0, 3}), slice),
               except::VariableError);
  ASSERT_THROW(TestFixture::op(dataset.slice({Dim::Z, 3, 6}), slice),
               except::VariableError);
  EXPECT_EQ(dataset, reference);
}

TYPED_TEST(DatasetViewBinaryEqualsOpTest, rhs_DatasetView_self_overlap) {
  auto dataset = make_no_variances(datasetFactory());
  for (const auto &name : {"values_x", "data_x", "data_xy", "data_scalar"})
    dataset.erase(name);
  const auto slice = dataset.slice({Dim::Z, 3});
  auto reference = copy(dataset);

  ASSERT_NO_THROW(TestFixture::op(dataset.slice({Dim::Z, 0, 3}), slice));
  ASSERT_NO_THROW(TestFixture::op(dataset.slice({Dim::Z, 3, 6}), slice));
  for (const auto &item : dataset) {
    EXPECT_EQ(item, TestFixture::op(reference[item.name()],
                                    reference[item.name()].slice({Dim::Z, 3})));
  }
}

TYPED_TEST(DatasetViewBinaryEqualsOpTest,
           rhs_DatasetView_self_overlap_undetectable) {
  auto dataset = make_no_variances(datasetFactory());
  for (const auto &name : {"values_x", "data_x", "data_xy", "data_scalar"})
    dataset.erase(name);
  const auto slice = dataset.slice({Dim::Z, 3});
  auto reference = copy(dataset);

  // Same as `rhs_DatasetView_self_overlap` above, but reverse slice order.
  // The second line will see the updated slice 3, and there is no way to
  // detect and prevent this.
  ASSERT_NO_THROW(TestFixture::op(dataset.slice({Dim::Z, 3, 6}), slice));
  ASSERT_NO_THROW(TestFixture::op(dataset.slice({Dim::Z, 0, 3}), slice));
  for (const auto &item : dataset) {
    // Items independent of Z are removed when creating `slice`.
    EXPECT_NE(item, TestFixture::op(reference[item.name()],
                                    reference[item.name()].slice({Dim::Z, 3})));
  }
}

TYPED_TEST(DatasetViewBinaryEqualsOpTest, rhs_slice_coord_mismatch) {
  auto dataset = datasetFactory().make()["data_xyz"];

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
  for (const auto &item : res) {
    EXPECT_EQ(item.masks(), dataset_b[item.name()].masks());
  }
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

TYPED_TEST(DatasetBinaryOpTest, datasetconstview_lhs_dataset_rhs) {
  const auto dataset_a = datasetFactory().make();
  const auto dataset_b = datasetFactory().make().slice({Dim::X, 1});

  Dataset dataset_a_view = dataset_a.slice({Dim::X, 1});
  const auto res = TestFixture::op(dataset_a_view, dataset_b);

  const auto reference = TestFixture::op(dataset_a_view, dataset_b);
  EXPECT_EQ(res, reference);
}

TYPED_TEST(DatasetBinaryOpTest, dataset_lhs_dataarray_rhs) {
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

  const auto expectedMask =
      makeVariable<bool>(Dimensions{Dim::X, datasetFactory().lx},
                         Values(make_bools(datasetFactory().lx, true)));

  for (const auto &item :
       {"values_x", "data_x", "data_xy", "data_zyx", "data_xyz"})
    b[item].masks().set("masks_x", expectedMask);

  const auto res = TestFixture::op(a, b);

  for (const auto &item :
       {"values_x", "data_x", "data_xy", "data_zyx", "data_xyz"})
    EXPECT_EQ(res[item].masks()["masks_x"], expectedMask);
}

TYPED_TEST(DatasetBinaryOpTest, masks_not_shared) {
  // There are two cases being tested here:
  // 1. Mask in both operands get ORed, ensure the implementation is not using
  //    the input variable without copy.
  // 2. The mask is in one of the operands and gets copied to the output. The
  //    most sensibly behavior appears to be to *copy* it, i.e., masks behave
  //    like data. Otherwise future operations with the output will break
  //    unrelated operands.
  auto a = datasetFactory().make();
  auto b = datasetFactory().make();

  const auto maskA = makeVariable<bool>(Dimensions{Dim::X, datasetFactory().lx},
                                        Values{false, false, true, false});
  const auto maskB = makeVariable<bool>(Dimensions{Dim::X, datasetFactory().lx},
                                        Values{false, false, false, true});
  const auto maskC = makeVariable<bool>(Dimensions{Dim::X, datasetFactory().lx},
                                        Values{false, true, false, false});

  for (const auto &item :
       {"values_x", "data_x", "data_xy", "data_zyx", "data_xyz"}) {
    a[item].masks().set("mask_ab", copy(maskA));
    a[item].masks().set("mask_a", copy(maskA));
    b[item].masks().set("mask_ab", copy(maskB));
    b[item].masks().set("mask_b", copy(maskB));
  }

  const auto res = TestFixture::op(a, b);

  for (const auto &item :
       {"values_x", "data_x", "data_xy", "data_zyx", "data_xyz"}) {
    res[item].masks()["mask_a"] |= maskC;
    res[item].masks()["mask_b"] |= maskC;
    res[item].masks()["mask_ab"] |= maskC;
    EXPECT_EQ(a[item].masks()["mask_ab"], maskA);
    EXPECT_EQ(a[item].masks()["mask_a"], maskA);
    EXPECT_EQ(b[item].masks()["mask_ab"], maskB);
    EXPECT_EQ(b[item].masks()["mask_b"], maskB);
  }
}

TEST(DatasetInPlaceStrongExceptionGuarantee, events) {
  Variable indicesGood = makeVariable<std::pair<scipp::index, scipp::index>>(
      Dims{Dim::X}, Shape{2}, Values{std::pair{0, 2}, std::pair{2, 3}});
  Variable indicesBad = makeVariable<std::pair<scipp::index, scipp::index>>(
      Dims{Dim::X}, Shape{2}, Values{std::pair{0, 2}, std::pair{2, 4}});
  Variable table =
      makeVariable<double>(Dims{Dim::Event}, Shape{4}, units::m,
                           Values{1, 2, 3, 4}, Variances{5, 6, 7, 8});
  Variable good = make_bins(indicesGood, Dim::Event, table);
  Variable bad = make_bins(indicesBad, Dim::Event, table);
  DataArray good_array(good, {}, {});
  Dataset good_dataset;
  good_dataset.setData("a", good);
  good_dataset.setData("b", good);

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

      ASSERT_ANY_THROW(d += good_dataset);
      ASSERT_EQ(d, original);
      // Note that we should not use an item of d in this test, since then
      // operation is delayed and we may end up bypassing the problem that the
      // "dry run" fixes.
      ASSERT_ANY_THROW(d += good_array);
      ASSERT_EQ(d, original);
    }
  }
}

TEST(DataArrayMasks, can_contain_any_type_but_only_OR_bools) {
  DataArray a(makeVariable<double>(Values{1}));
  a.masks().set("double", makeVariable<double>(units::none, Values{1}));
  ASSERT_THROW(a += a, except::TypeError);
  ASSERT_THROW_DISCARD(a + a, except::TypeError);
  a.masks().set("bool", makeVariable<bool>(Values{false}));
  ASSERT_THROW(a += a, except::TypeError);
  ASSERT_THROW_DISCARD(a + a, except::TypeError);
  a.masks().erase("double");
  ASSERT_NO_THROW(a += a);
  ASSERT_NO_THROW_DISCARD(a + a);
}
