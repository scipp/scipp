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

template <class T> auto make_no_variances(T &&factory) {
  auto ds = factory.make("data");
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

  Dataset a(
      {{"data_a", makeVariable<double>(Dimensions{{Dim::X, lx}, {Dim::Y, ly}},
                                       Values(rand(lx * ly)))},
       {"data_b", makeVariable<double>(Dimensions{{Dim::Y, ly}, {Dim::X, lx}},
                                       Values(rand(lx * ly)))}},
      {{Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{lx}, Values(coordX))},
       {Dim::Y, makeVariable<double>(Dims{Dim::Y}, Shape{ly}, Values(coordY))},
       {Dim("t"), labelT}});
  a["data_a"].masks().set("mask", makeVariable<bool>(Values{false}));

  Dataset b(
      {{"data_a", makeVariable<double>(Dimensions{{Dim::X, lx}, {Dim::Y, ly}},
                                       Values(rand(lx * ly)))}},
      {{Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{lx}, Values(coordX))},
       {Dim::Y, makeVariable<double>(Dims{Dim::Y}, Shape{ly}, Values(coordY))},
       {Dim("t"), labelT}});
  b["data_a"].masks().set("mask", mask);

  return std::make_tuple(a, b);
}

TYPED_TEST_SUITE(DataArrayViewBinaryEqualsOpTest, BinaryEquals);
TYPED_TEST_SUITE(DatasetBinaryEqualsOpTest, BinaryEquals);
TYPED_TEST_SUITE(DatasetViewBinaryEqualsOpTest, BinaryEquals);

TYPED_TEST(DataArrayViewBinaryEqualsOpTest, other_data_unchanged) {
  const auto dataset_b = make_no_variances(DatasetFactory{});

  for (const auto &item : dataset_b) {
    auto dataset_a = DatasetFactory{}.make();
    const auto original_a = copy(dataset_a);
    auto target = dataset_a["data"];

    ASSERT_NO_THROW(TestFixture::op(target, item));

    for (const auto &data : dataset_a) {
      if (data.name() != "data") {
        EXPECT_EQ(data, original_a[data.name()]);
      } else {
        EXPECT_NE(data, original_a[data.name()]);
      }
    }
  }
}

TYPED_TEST(DataArrayViewBinaryEqualsOpTest, lhs_with_variance) {
  DatasetFactory factory;
  const auto dataset_b = factory.make();

  for (const auto &item : dataset_b) {
    auto dataset_a = factory.make_with_random_masks("data");
    dataset_a.coords() = dataset_b.coords();
    auto target = copy(dataset_a["data"]);
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
  DatasetFactory factory;
  const auto dataset_b = factory.make("data");

  for (const auto &item : dataset_b) {
    auto dataset_a = factory.make_with_random_masks("data");
    dataset_a.coords() = dataset_b.coords();
    auto target = copy(dataset_a["data"]);
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
  DatasetFactory factory;
  const auto dataset_b = factory.make("data");

  for (const auto &item : dataset_b) {
    auto dataset_a = factory.make_with_random_masks("data");
    dataset_a.coords() = dataset_b.coords();
    auto target = copy(dataset_a["data"]);
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
// DataArray with itself, so we can rely on that for building the reference.
TYPED_TEST(DatasetBinaryEqualsOpTest, return_value) {
  DatasetFactory factory;
  auto a = make_no_variances(factory);
  auto b = make_no_variances(factory);
  b.coords() = a.coords();

  ASSERT_TRUE((std::is_same_v<decltype(TestFixture::op(a, b["data"].data())),
                              Dataset &>));
  {
    const auto &result = TestFixture::op(a, b["data"].data());
    ASSERT_EQ(&result, &a);
  }

  ASSERT_TRUE(
      (std::is_same_v<decltype(TestFixture::op(a, b["data"])), Dataset &>));
  {
    const auto &result = TestFixture::op(a, b["data"]);
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

  ASSERT_TRUE((std::is_same_v<decltype(TestFixture::op(a, 5.0 * sc_units::one)),
                              Dataset &>));
  {
    const auto &result = TestFixture::op(a, 5.0 * sc_units::one);
    ASSERT_EQ(&result, &a);
  }
}

TYPED_TEST(DatasetBinaryEqualsOpTest, rhs_DataArrayView_self_overlap) {
  auto dataset = DatasetFactory{}.make();
  auto original = copy(dataset);
  auto reference = copy(dataset);

  ASSERT_NO_THROW(TestFixture::op(dataset, dataset["data"]));
  for (const auto &item : dataset) {
    EXPECT_EQ(item, TestFixture::op(reference[item.name()], original["data"]));
  }
}

TYPED_TEST(DatasetBinaryEqualsOpTest, rhs_Variable_self_overlap) {
  auto dataset = DatasetFactory{}.make();
  auto original(dataset);
  auto reference(dataset);

  ASSERT_NO_THROW(TestFixture::op(dataset, dataset["data"].data()));
  for (const auto &item : dataset) {
    EXPECT_EQ(item,
              TestFixture::op(reference[item.name()], original["data"].data()));
  }
}

TYPED_TEST(DatasetBinaryEqualsOpTest, rhs_DataArrayView_self_overlap_slice) {
  auto dataset = DatasetFactory{}.make();
  dataset.coords().erase(Dim{"xy"}); // multi-dim coord causes mismatch
  auto original = copy(dataset);
  auto reference = copy(dataset);

  ASSERT_NO_THROW(TestFixture::op(dataset, dataset["data"].slice({Dim::X, 1})));
  for (const auto &item : dataset) {
    EXPECT_EQ(item, TestFixture::op(reference[item.name()],
                                    original["data"].slice({Dim::X, 1})));
  }
}

TYPED_TEST(DatasetBinaryEqualsOpTest, rhs_Dataset) {
  auto a = DatasetFactory{}.make();
  auto b = DatasetFactory{}.make();
  auto reference(a);

  ASSERT_NO_THROW(TestFixture::op(a, b));
  for (const auto &item : a) {
    EXPECT_EQ(item, TestFixture::op(reference[item.name()], b[item.name()]));
  }
}

TYPED_TEST(DatasetBinaryEqualsOpTest, rhs_Dataset_coord_mismatch) {
  DatasetFactory factory_a;
  factory_a.seed(7471);
  DatasetFactory factory_b;
  factory_b.seed(7198);
  auto a = factory_a.make();
  auto b = factory_b.make();

  ASSERT_THROW(TestFixture::op(a, b), except::CoordMismatchError);
}

TYPED_TEST(DatasetBinaryEqualsOpTest, rhs_Dataset_with_missing_items) {
  DatasetFactory factory;
  auto a = factory.make();
  a.setData("extra", makeVariable<double>(a["data"].dims()));
  auto b = factory.make();
  b.coords() = a.coords();
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
  auto a = DatasetFactory{}.make();
  auto b = DatasetFactory{}.make();
  b.setData("extra", b["data"]);

  ASSERT_ANY_THROW(TestFixture::op(a, b));
}

TYPED_TEST(DatasetBinaryEqualsOpTest, rhs_DatasetView_self_overlap) {
  auto dataset = make_no_variances(DatasetFactory{});
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
  DatasetFactory factory{{Dim::X, 4}, {Dim::Y, 5}, {Dim::Z, 4}};
  auto dataset = factory.make()["data"];

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
  DatasetFactory factory;
  auto a = factory.make("data");
  auto b = factory.make("data");
  b.coords() = a.coords();
  const auto expectedMask =
      makeVariable<bool>(Dimensions{Dim::X, factory.dims()[Dim::X]},
                         Values(make_bools(factory.dims()[Dim::X], true)));
  b["data"].masks().set("masks_x", expectedMask);

  TestFixture::op(a, b);

  EXPECT_EQ(a["data"].masks()["masks_x"], expectedMask);
}

TYPED_TEST(DatasetBinaryEqualsOpTest, masks_not_shared) {
  // See test of same name in DatasetBinaryOpTest
  DatasetFactory factory;
  auto a = factory.make("data");
  auto b = factory.make("data");
  b.coords() = a.coords();

  const auto maskA =
      makeVariable<bool>(Dimensions{Dim::X, factory.dims()[Dim::X]},
                         Values{false, false, true, false});
  const auto maskB =
      makeVariable<bool>(Dimensions{Dim::X, factory.dims()[Dim::X]},
                         Values{false, false, false, true});
  const auto maskC =
      makeVariable<bool>(Dimensions{Dim::X, factory.dims()[Dim::X]},
                         Values{false, true, false, false});

  a["data"].masks().set("mask_ab", copy(maskA));
  b["data"].masks().set("mask_ab", copy(maskB));
  b["data"].masks().set("mask_b", copy(maskB));

  TestFixture::op(a, b);

  a["data"].masks()["mask_b"] |= maskC;
  a["data"].masks()["mask_ab"] |= maskC;
  EXPECT_EQ(b["data"].masks()["mask_ab"], maskB);
  EXPECT_EQ(b["data"].masks()["mask_b"], maskB);
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

TYPED_TEST(DatasetViewBinaryEqualsOpTest, rhs_DataArray_self_overlap) {
  auto dataset = DatasetFactory{}.make();
  // Unless we take the last slice this will not work
  const auto slice =
      dataset["data"].slice({Dim::Z, dataset.dims()[Dim::Z] - 1});
  auto reference = copy(dataset);

  for (scipp::index z = 0; z < dataset.coords()[Dim::Z].dims()[Dim::Z]; ++z)
    ASSERT_NO_THROW(TestFixture::op(dataset.slice({Dim::Z, z}), slice));
  const auto rhs =
      copy(reference["data"].slice({Dim::Z, dataset.dims()[Dim::Z] - 1}));
  for (const auto &item : dataset) {
    EXPECT_EQ(item, TestFixture::op(reference[item.name()], rhs));
  }
}

TYPED_TEST(DatasetViewBinaryEqualsOpTest, rhs_Dataset_coord_mismatch) {
  DatasetFactory factory;
  auto a = factory.make();
  auto b = factory.make();

  ASSERT_THROW(TestFixture::op(copy(a), b), except::CoordMismatchError);
}

TYPED_TEST(DatasetViewBinaryEqualsOpTest, rhs_Dataset_with_missing_items) {
  DatasetFactory factory;
  auto a = factory.make("data");
  auto b = a;
  a.setData("extra", a["data"]);
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
  auto a = DatasetFactory{}.make();
  auto b = DatasetFactory{}.make();
  b.setData("extra", b["data"]);

  ASSERT_ANY_THROW(TestFixture::op(copy(a), b));
}

TYPED_TEST(DatasetViewBinaryEqualsOpTest, rhs_DatasetView_self_overlap) {
  DatasetFactory factory{{Dim::Z, 6}};
  auto dataset = make_no_variances(factory);
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
  DatasetFactory factory{{Dim::Z, 6}};
  auto dataset = make_no_variances(factory);
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
  DatasetFactory factory{{Dim::X, 4}, {Dim::Y, 5}, {Dim::Z, 4}};
  auto dataset = factory.make()["data"];

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
  for (const auto &item : res)
    EXPECT_EQ(item.masks(), dataset_b[item.name()].masks());
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

TYPED_TEST(DatasetBinaryOpTest, dataset_lhs_dataset_rhs_empty_operand) {
  auto [dataset_a, dataset_b] = generateBinaryOpTestCase();
  dataset_b.erase("data_a");

  const auto ab = TestFixture::op(dataset_a, dataset_b);
  EXPECT_TRUE(ab.is_valid());
  EXPECT_TRUE(ab.empty());
  EXPECT_EQ(ab.dims(), Dimensions{});

  const auto ba = TestFixture::op(dataset_b, dataset_a);
  EXPECT_TRUE(ba.is_valid());
  EXPECT_TRUE(ba.empty());
  EXPECT_EQ(ba.dims(), Dimensions{});
}

TYPED_TEST(DatasetBinaryOpTest, broadcast) {
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  const auto c = makeVariable<double>(Values{2.0});
  Dataset a({{"data1", x}, {"data2", x}}, {{Dim::X, x}});
  Dataset b({{"data1", c}, {"data2", c + c}});
  const auto res = TestFixture::op(a, b);
  EXPECT_EQ(res["data1"].data(), TestFixture::op(x, c));
  EXPECT_EQ(res["data2"].data(), TestFixture::op(x, c + c));
}

TYPED_TEST(DatasetBinaryOpTest, dataset_lhs_scalar_rhs) {
  const auto dataset = std::get<0>(generateBinaryOpTestCase());
  const auto scalar = 4.5 * sc_units::one;

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
  const auto scalar = 4.5 * sc_units::one;

  const auto res = TestFixture::op(scalar, dataset);

  /* Test that the dataset contains the equivalent of operating on the Variable
   * directly. */
  /* Correctness of results is tested via Variable tests. */
  const auto reference = TestFixture::op(scalar, dataset["data_a"].data());
  EXPECT_EQ(reference, res["data_a"].data());

  /* Expect coordinatesto be copied to the result dataset */
  EXPECT_EQ(res.coords(), dataset.coords());
}

TYPED_TEST(DatasetBinaryOpTest, dataset_lhs_scalar_rhs_empty_operand) {
  auto dataset = std::get<1>(generateBinaryOpTestCase());
  dataset.erase("data_a");
  const auto scalar = 4.5 * sc_units::one;

  const auto res = TestFixture::op(dataset, scalar);
  EXPECT_TRUE(res.is_valid());
  EXPECT_TRUE(res.empty());
  EXPECT_EQ(res.dims(), Dimensions{});
}

TYPED_TEST(DatasetBinaryOpTest, dataset_lhs_dataarray_rhs) {
  const auto [dataset_a, dataset_b] = generateBinaryOpTestCase();

  const auto res = TestFixture::op(dataset_a, dataset_b["data_a"]);

  for (const auto &item : res) {
    const auto reference = TestFixture::op(dataset_a[item.name()].data(),
                                           dataset_b["data_a"].data());
    EXPECT_EQ(reference, item.data());
  }
}

TYPED_TEST(DatasetBinaryOpTest, dataset_lhs_dataarray_rhs_empty_operand) {
  auto [dataset_a, dataset_b] = generateBinaryOpTestCase();
  dataset_b.erase("data_a");

  const auto res = TestFixture::op(dataset_a["data_a"], dataset_b);
  EXPECT_TRUE(res.is_valid());
  EXPECT_TRUE(res.empty());
  EXPECT_EQ(res.dims(), Dimensions{});
}

TYPED_TEST(DatasetBinaryOpTest, masks_propagate) {
  DatasetFactory factory;
  auto a = factory.make("data");
  auto b = factory.make("data");
  b.coords() = a.coords();

  const auto expectedMask =
      makeVariable<bool>(Dimensions{Dim::X, factory.dims()[Dim::X]},
                         Values(make_bools(factory.dims()[Dim::X], true)));

  b["data"].masks().set("masks_x", expectedMask);

  const auto res = TestFixture::op(a, b);

  EXPECT_EQ(res["data"].masks()["masks_x"], expectedMask);
}

TYPED_TEST(DatasetBinaryOpTest, masks_not_shared) {
  // There are two cases being tested here:
  // 1. Mask in both operands get ORed, ensure the implementation is not using
  //    the input variable without copy.
  // 2. The mask is in one of the operands and gets copied to the output. The
  //    most sensibly behavior appears to be to *copy* it, i.e., masks behave
  //    like data. Otherwise future operations with the output will break
  //    unrelated operands.
  DatasetFactory factory;
  auto a = factory.make("data");
  auto b = factory.make("data");
  b.coords() = a.coords();

  const auto maskA =
      makeVariable<bool>(Dimensions{Dim::X, factory.dims()[Dim::X]},
                         Values{false, false, true, false});
  const auto maskB =
      makeVariable<bool>(Dimensions{Dim::X, factory.dims()[Dim::X]},
                         Values{false, false, false, true});
  const auto maskC =
      makeVariable<bool>(Dimensions{Dim::X, factory.dims()[Dim::X]},
                         Values{false, true, false, false});

  a["data"].masks().set("mask_ab", copy(maskA));
  a["data"].masks().set("mask_a", copy(maskA));
  b["data"].masks().set("mask_ab", copy(maskB));
  b["data"].masks().set("mask_b", copy(maskB));

  const auto res = TestFixture::op(a, b);

  res["data"].masks()["mask_a"] |= maskC;  // cppcheck-suppress unreadVariable
  res["data"].masks()["mask_b"] |= maskC;  // cppcheck-suppress unreadVariable
  res["data"].masks()["mask_ab"] |= maskC; // cppcheck-suppress unreadVariable
  EXPECT_EQ(a["data"].masks()["mask_ab"], maskA);
  EXPECT_EQ(a["data"].masks()["mask_a"], maskA);
  EXPECT_EQ(b["data"].masks()["mask_ab"], maskB);
  EXPECT_EQ(b["data"].masks()["mask_b"], maskB);
}

TEST(DatasetInPlaceStrongExceptionGuarantee, events) {
  Variable indicesGood = makeVariable<std::pair<scipp::index, scipp::index>>(
      Dims{Dim::X}, Shape{2}, Values{std::pair{0, 2}, std::pair{2, 3}});
  Variable indicesBad = makeVariable<std::pair<scipp::index, scipp::index>>(
      Dims{Dim::X}, Shape{2}, Values{std::pair{0, 2}, std::pair{2, 4}});
  Variable table =
      makeVariable<double>(Dims{Dim::Event}, Shape{4}, sc_units::m,
                           Values{1, 2, 3, 4}, Variances{5, 6, 7, 8});
  Variable good = make_bins(indicesGood, Dim::Event, table);
  Variable bad = make_bins(indicesBad, Dim::Event, table);
  DataArray good_array(good);
  Dataset good_dataset({{"a", good}, {"b", good}});

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
      Dataset d({{key1, value1}, {key2, value2}});
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
  a.masks().set("double", makeVariable<double>(sc_units::none, Values{1}));
  ASSERT_THROW(a += a, except::TypeError);
  ASSERT_THROW_DISCARD(a + a, except::TypeError);
  a.masks().set("bool", makeVariable<bool>(Values{false}));
  ASSERT_THROW(a += a, except::TypeError);
  ASSERT_THROW_DISCARD(a + a, except::TypeError);
  a.masks().erase("double");
  ASSERT_NO_THROW(a += a);
  ASSERT_NO_THROW_DISCARD(a + a);
}
