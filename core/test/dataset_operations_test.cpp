// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include "scipp/core/dataset.h"
#include "scipp/core/dimensions.h"

#include "dataset_test_common.h"
#include <initializer_list>

using namespace scipp;
using namespace scipp::core;

DatasetFactory3D datasetFactory;

// We need the decltype(auto) since we are using these operators for both
// proxies and non-proxies. The former return by reference, the latter by value.
struct plus_equals {
  template <class A, class B>
  decltype(auto) operator()(A &&a, const B &b) const {
    return a += b;
  }
};
struct plus {
  template <class A, class B> decltype(auto) operator()(A &&a, B &&b) const {
    return std::forward<A>(a) + std::forward<B>(b);
  }
};
struct minus_equals {
  template <class A, class B>
  decltype(auto) operator()(A &&a, const B &b) const {
    return a -= b;
  }
};
struct minus {
  template <class A, class B> decltype(auto) operator()(A &&a, B &&b) const {
    return std::forward<A>(a) - std::forward<B>(b);
  }
};
struct times_equals {
  template <class A, class B>
  decltype(auto) operator()(A &&a, const B &b) const {
    return a *= b;
  }
};
struct times {
  template <class A, class B> decltype(auto) operator()(A &&a, B &&b) const {
    return std::forward<A>(a) * std::forward<B>(b);
  }
};
struct divide_equals {
  template <class A, class B>
  decltype(auto) operator()(A &&a, const B &b) const {
    return a /= b;
  }
};
struct divide {
  template <class A, class B> decltype(auto) operator()(A &&a, B &&b) const {
    return std::forward<A>(a) / std::forward<B>(b);
  }
};

using Binary = ::testing::Types<plus, minus, times, divide>;
using BinaryEquals =
    ::testing::Types<plus_equals, minus_equals, times_equals, divide_equals>;

template <class Op>
class DataProxyBinaryEqualsOpTest : public ::testing::Test,
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
class DatasetProxyBinaryEqualsOpTest
    : public ::testing::Test,
      public ::testing::WithParamInterface<Op> {
protected:
  Op op;
};

TYPED_TEST_SUITE(DataProxyBinaryEqualsOpTest, BinaryEquals);
TYPED_TEST_SUITE(DatasetBinaryEqualsOpTest, BinaryEquals);
TYPED_TEST_SUITE(DatasetProxyBinaryEqualsOpTest, BinaryEquals);

TYPED_TEST(DataProxyBinaryEqualsOpTest, other_data_unchanged) {
  const auto dataset_b = datasetFactory.make();

  for (const auto &item : dataset_b) {
    auto dataset_a = datasetFactory.make();
    const auto original_a(dataset_a);
    auto target = dataset_a["data_zyx"];

    ASSERT_NO_THROW(target = TestFixture::op(target, item.second));

    for (const auto & [ name, data ] : dataset_a) {
      if (name != "data_zyx") {
        EXPECT_EQ(data, original_a[name]);
      }
    }
  }
}

TYPED_TEST(DataProxyBinaryEqualsOpTest, lhs_with_variance) {
  const auto dataset_b = datasetFactory.make();

  for (const auto &item : dataset_b) {
    auto dataset_a = datasetFactory.make();
    auto target = dataset_a["data_zyx"];

    auto reference(target.data());
    reference = TestFixture::op(target.data(), item.second.data());

    ASSERT_NO_THROW(target = TestFixture::op(target, item.second));
    EXPECT_EQ(target.data(), reference);
  }
}

TYPED_TEST(DataProxyBinaryEqualsOpTest, lhs_without_variance) {
  const auto dataset_b = datasetFactory.make();

  for (const auto &item : dataset_b) {
    auto dataset_a = datasetFactory.make();
    auto target = dataset_a["data_xyz"];

    if (item.second.hasVariances()) {
      ASSERT_ANY_THROW(TestFixture::op(target, item.second));
    } else {
      auto reference(target.data());
      reference = TestFixture::op(target.data(), item.second.data());

      ASSERT_NO_THROW(target = TestFixture::op(target, item.second));
      EXPECT_EQ(target.data(), reference);
      EXPECT_FALSE(target.hasVariances());
    }
  }
}

TYPED_TEST(DataProxyBinaryEqualsOpTest, slice_lhs_with_variance) {
  const auto dataset_b = datasetFactory.make();

  for (const auto &item : dataset_b) {
    auto dataset_a = datasetFactory.make();
    auto target = dataset_a["data_zyx"];
    const auto &dims = item.second.dims();

    for (const Dim dim : dims.labels()) {
      auto reference(target.data());
      reference = TestFixture::op(target.data(), item.second.data());

      // Fails if any *other* multi-dimensional coord/label also depends on the
      // slicing dimension, since it will have mismatching values. Note that
      // this behavior is intended and important. It is crucial for preventing
      // operations between misaligned data in case a coordinate is
      // multi-dimensional.
      const auto coords = item.second.coords();
      const auto labels = item.second.labels();
      if (std::all_of(coords.begin(), coords.end(),
                      [dim](const auto &coord) {
                        return coord.first == dim ||
                               !coord.second.dims().contains(dim);
                      }) &&
          std::all_of(labels.begin(), labels.end(), [dim](const auto &labels_) {
            return labels_.second.dims().inner() == dim ||
                   !labels_.second.dims().contains(dim);
          })) {
        ASSERT_NO_THROW(
            target = TestFixture::op(target, item.second.slice({dim, 2})));
        EXPECT_EQ(target.data(), reference);
      } else {
        ASSERT_ANY_THROW(
            target = TestFixture::op(target, item.second.slice({dim, 2})));
      }
    }
  }
}

// DataProxyBinaryEqualsOpTest ensures correctness of operations between
// DataProxy with itself, so we can rely on that for building the reference.
TYPED_TEST(DatasetBinaryEqualsOpTest, return_value) {
  auto a = datasetFactory.make();
  auto b = datasetFactory.make();

  ASSERT_TRUE((std::is_same_v<decltype(TestFixture::op(a, b["data_scalar"])),
                              Dataset &>));
  const auto &result1 = TestFixture::op(a, b["data_scalar"]);
  ASSERT_EQ(&result1, &a);

  ASSERT_TRUE((std::is_same_v<decltype(TestFixture::op(a, b)), Dataset &>));
  const auto &result2 = TestFixture::op(a, b);
  ASSERT_EQ(&result2, &a);

  ASSERT_TRUE(
      (std::is_same_v<decltype(TestFixture::op(a, b.slice({Dim::Z, 3}))),
                      Dataset &>));
  const auto &result3 = TestFixture::op(a, b.slice({Dim::Z, 3}));
  ASSERT_EQ(&result3, &a);
}

TYPED_TEST(DatasetBinaryEqualsOpTest, rhs_DataProxy_self_overlap) {
  auto dataset = datasetFactory.make();
  auto original(dataset);
  auto reference(dataset);

  ASSERT_NO_THROW(TestFixture::op(dataset, dataset["data_scalar"]));
  for (const auto[name, item] : dataset) {
    EXPECT_EQ(item, TestFixture::op(reference[name], original["data_scalar"]));
  }
}

TYPED_TEST(DatasetBinaryEqualsOpTest, rhs_DataProxy_self_overlap_slice) {
  auto dataset = datasetFactory.make();
  auto original(dataset);
  auto reference(dataset);

  ASSERT_NO_THROW(
      TestFixture::op(dataset, dataset["values_x"].slice({Dim::X, 1})));
  for (const auto[name, item] : dataset) {
    EXPECT_EQ(item, TestFixture::op(reference[name],
                                    original["values_x"].slice({Dim::X, 1})));
  }
}

TYPED_TEST(DatasetBinaryEqualsOpTest, rhs_Dataset) {
  auto a = datasetFactory.make();
  auto b = datasetFactory.make();
  auto reference(a);

  ASSERT_NO_THROW(TestFixture::op(a, b));
  for (const auto[name, item] : a) {
    EXPECT_EQ(item, TestFixture::op(reference[name], b[name]));
  }
}

TYPED_TEST(DatasetBinaryEqualsOpTest, rhs_Dataset_coord_mismatch) {
  auto a = datasetFactory.make();
  DatasetFactory3D otherCoordsFactory;
  auto b = otherCoordsFactory.make();

  ASSERT_THROW(TestFixture::op(a, b), except::CoordMismatchError);
}

TYPED_TEST(DatasetBinaryEqualsOpTest, rhs_Dataset_with_missing_items) {
  auto a = datasetFactory.make();
  a.setData("extra", makeVariable<double>({}));
  auto b = datasetFactory.make();
  auto reference(a);

  ASSERT_NO_THROW(TestFixture::op(a, b));
  for (const auto[name, item] : a) {
    if (name == "extra") {
      EXPECT_EQ(item, reference[name]);
    } else {
      EXPECT_EQ(item, TestFixture::op(reference[name], b[name]));
    }
  }
}

TYPED_TEST(DatasetBinaryEqualsOpTest, rhs_Dataset_with_extra_items) {
  auto a = datasetFactory.make();
  auto b = datasetFactory.make();
  b.setData("extra", makeVariable<double>({}));

  ASSERT_ANY_THROW(TestFixture::op(a, b));
}

TYPED_TEST(DatasetBinaryEqualsOpTest, rhs_DatasetProxy_self_overlap) {
  auto dataset = datasetFactory.make();
  const auto slice = dataset.slice({Dim::Z, 3});
  auto reference(dataset);

  ASSERT_NO_THROW(TestFixture::op(dataset, slice));
  for (const auto[name, item] : dataset) {
    // Items independent of Z are removed when creating `slice`.
    if (item.dims().contains(Dim::Z)) {
      EXPECT_EQ(item, TestFixture::op(reference[name],
                                      reference[name].slice({Dim::Z, 3})));
    } else {
      EXPECT_EQ(item, reference[name]);
    }
  }
}

TYPED_TEST(DatasetBinaryEqualsOpTest, rhs_DatasetProxy_coord_mismatch) {
  auto dataset = datasetFactory.make();

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

Dataset make_simple_sparse(std::initializer_list<double> values,
                           std::string key = "sparse") {
  Dataset ds;
  auto var = makeVariable<double>({Dim::X, Dimensions::Sparse});
  var.sparseValues<double>()[0] = values;
  ds.setData(key, var);
  return ds;
}

Dataset make_sparse_2d(std::initializer_list<double> values,
                       std::string key = "sparse") {
  Dataset ds;
  auto var = makeVariable<double>({Dim::X, Dim::Y}, {2, Dimensions::Sparse});
  var.sparseValues<double>()[0] = values;
  var.sparseValues<double>()[1] = values;
  ds.setData(key, var);
  return ds;
}

TYPED_TEST(DatasetBinaryEqualsOpTest,
           with_single_var_with_single_sparse_dimensions_sized_same) {
  Dataset a = make_simple_sparse({1.1, 2.2});
  Dataset b = make_simple_sparse({3.3, 4.4});
  Dataset c = TestFixture::op(a, b);
  auto c_data = c["sparse"].data().sparseValues<double>()[0];
  ASSERT_EQ(c_data[0], TestFixture::op(1.1, 3.3));
  ASSERT_EQ(c_data[1], TestFixture::op(2.2, 4.4));
}

TYPED_TEST(DatasetBinaryEqualsOpTest,
           with_single_var_dense_and_sparse_dimension) {
  Dataset a = make_sparse_2d({1.1, 2.2});
  Dataset b = make_sparse_2d({3.3, 4.4});
  Dataset c = TestFixture::op(a, b);
  ASSERT_EQ(c["sparse"].data().sparseValues<double>().size(), 2);
  auto c_data = c["sparse"].data().sparseValues<double>()[0];
  ASSERT_EQ(c_data[0], TestFixture::op(1.1, 3.3));
  ASSERT_EQ(c_data[1], TestFixture::op(2.2, 4.4));
}

TYPED_TEST(DatasetBinaryEqualsOpTest, with_multiple_variables) {
  Dataset a = make_simple_sparse({1.1, 2.2});
  a.setData("sparse2", a["sparse"].data());
  Dataset b = make_simple_sparse({3.3, 4.4});
  b.setData("sparse2", b["sparse"].data());
  Dataset c = TestFixture::op(a, b);
  ASSERT_EQ(c.size(), 2);
  auto c_data = c["sparse"].data().sparseValues<double>()[0];
  ASSERT_EQ(c_data[0], TestFixture::op(1.1, 3.3));
  ASSERT_EQ(c_data[1], TestFixture::op(2.2, 4.4));
  c_data = c["sparse2"].data().sparseValues<double>()[1];
  ASSERT_EQ(c_data[0], TestFixture::op(1.1, 3.3));
  ASSERT_EQ(c_data[1], TestFixture::op(2.2, 4.4));
}

TYPED_TEST(DatasetBinaryEqualsOpTest,
           with_sparse_dimensions_of_different_sizes) {
  Dataset a = make_simple_sparse({1.1, 2.2});
  Dataset b = make_simple_sparse({3.3, 4.4, 5.5});
  ASSERT_THROW(TestFixture::op(a, b), std::runtime_error);
}

TYPED_TEST(DatasetProxyBinaryEqualsOpTest, return_value) {
  auto a = datasetFactory.make();
  auto b = datasetFactory.make();
  DatasetProxy proxy(a);

  ASSERT_TRUE(
      (std::is_same_v<decltype(TestFixture::op(proxy, b["data_scalar"])),
                      DatasetProxy>));
  const auto &result1 = TestFixture::op(proxy, b["data_scalar"]);
  EXPECT_EQ(&result1["data_scalar"].template values<double>()[0],
            &a["data_scalar"].template values<double>()[0]);

  ASSERT_TRUE(
      (std::is_same_v<decltype(TestFixture::op(proxy, b)), DatasetProxy>));
  const auto &result2 = TestFixture::op(proxy, b);
  EXPECT_EQ(&result2["data_scalar"].template values<double>()[0],
            &a["data_scalar"].template values<double>()[0]);

  ASSERT_TRUE(
      (std::is_same_v<decltype(TestFixture::op(proxy, b.slice({Dim::Z, 3}))),
                      DatasetProxy>));
  const auto &result3 = TestFixture::op(proxy, b.slice({Dim::Z, 3}));
  EXPECT_EQ(&result3["data_scalar"].template values<double>()[0],
            &a["data_scalar"].template values<double>()[0]);
}

TYPED_TEST(DatasetProxyBinaryEqualsOpTest, rhs_DataProxy_self_overlap) {
  auto dataset = datasetFactory.make();
  auto reference(dataset);
  TestFixture::op(reference, dataset["data_scalar"]);

  for (scipp::index z = 0; z < dataset.coords()[Dim::Z].dims()[Dim::Z]; ++z) {
    for (const auto & [ name, item ] : dataset)
      if (item.dims().contains(Dim::Z)) {
        EXPECT_NE(item, reference[name]);
      }
    ASSERT_NO_THROW(
        TestFixture::op(dataset.slice({Dim::Z, z}), dataset["data_scalar"]));
  }
  for (const auto & [ name, item ] : dataset)
    if (item.dims().contains(Dim::Z)) {
      EXPECT_EQ(item, reference[name]);
    }
}

TYPED_TEST(DatasetProxyBinaryEqualsOpTest, rhs_DataProxy_self_overlap_slice) {
  auto dataset = datasetFactory.make();
  auto reference(dataset);
  TestFixture::op(reference, dataset["values_x"].slice({Dim::X, 1}));

  for (scipp::index z = 0; z < dataset.coords()[Dim::Z].dims()[Dim::Z]; ++z) {
    for (const auto & [ name, item ] : dataset)
      if (item.dims().contains(Dim::Z)) {
        EXPECT_NE(item, reference[name]);
      }
    ASSERT_NO_THROW(TestFixture::op(dataset.slice({Dim::Z, z}),
                                    dataset["values_x"].slice({Dim::X, 1})));
  }
  for (const auto & [ name, item ] : dataset)
    if (item.dims().contains(Dim::Z)) {
      EXPECT_EQ(item, reference[name]);
    }
}

TYPED_TEST(DatasetProxyBinaryEqualsOpTest, rhs_Dataset_coord_mismatch) {
  DatasetFactory3D otherCoordsFactory;
  auto a = otherCoordsFactory.make();
  auto b = datasetFactory.make();

  ASSERT_THROW(TestFixture::op(DatasetProxy(a), b), except::CoordMismatchError);
}

TYPED_TEST(DatasetProxyBinaryEqualsOpTest, rhs_Dataset_with_missing_items) {
  auto a = datasetFactory.make();
  a.setData("extra", makeVariable<double>({}));
  auto b = datasetFactory.make();
  auto reference(a);

  ASSERT_NO_THROW(TestFixture::op(DatasetProxy(a), b));
  for (const auto[name, item] : a) {
    if (name == "extra") {
      EXPECT_EQ(item, reference[name]);
    } else {
      EXPECT_EQ(item, TestFixture::op(reference[name], b[name]));
    }
  }
}

TYPED_TEST(DatasetProxyBinaryEqualsOpTest, rhs_Dataset_with_extra_items) {
  auto a = datasetFactory.make();
  auto b = datasetFactory.make();
  b.setData("extra", makeVariable<double>({}));

  ASSERT_ANY_THROW(TestFixture::op(DatasetProxy(a), b));
}

TYPED_TEST(DatasetProxyBinaryEqualsOpTest, rhs_DatasetProxy_self_overlap) {
  auto dataset = datasetFactory.make();
  const auto slice = dataset.slice({Dim::Z, 3});
  auto reference(dataset);

  ASSERT_NO_THROW(TestFixture::op(dataset.slice({Dim::Z, 0, 3}), slice));
  ASSERT_NO_THROW(TestFixture::op(dataset.slice({Dim::Z, 3, 6}), slice));
  for (const auto[name, item] : dataset) {
    // Items independent of Z are removed when creating `slice`.
    if (item.dims().contains(Dim::Z)) {
      EXPECT_EQ(item, TestFixture::op(reference[name],
                                      reference[name].slice({Dim::Z, 3})));
    } else {
      EXPECT_EQ(item, reference[name]);
    }
  }
}

TYPED_TEST(DatasetProxyBinaryEqualsOpTest,
           rhs_DatasetProxy_self_overlap_undetectable) {
  auto dataset = datasetFactory.make();
  const auto slice = dataset.slice({Dim::Z, 3});
  auto reference(dataset);

  // Same as `rhs_DatasetProxy_self_overlap` above, but reverse slice order.
  // The second line will see the updated slice 3, and there is no way to
  // detect and prevent this.
  ASSERT_NO_THROW(TestFixture::op(dataset.slice({Dim::Z, 3, 6}), slice));
  ASSERT_NO_THROW(TestFixture::op(dataset.slice({Dim::Z, 0, 3}), slice));
  for (const auto[name, item] : dataset) {
    // Items independent of Z are removed when creating `slice`.
    if (item.dims().contains(Dim::Z)) {
      EXPECT_NE(item, TestFixture::op(reference[name],
                                      reference[name].slice({Dim::Z, 3})));
    } else {
      EXPECT_EQ(item, reference[name]);
    }
  }
}

TYPED_TEST(DatasetProxyBinaryEqualsOpTest, rhs_DatasetProxy_coord_mismatch) {
  auto dataset = datasetFactory.make();
  const DatasetProxy proxy(dataset);

  // Non-range sliced throws for X and Y due to multi-dimensional coords.
  ASSERT_THROW(TestFixture::op(proxy, dataset.slice({Dim::X, 3})),
               except::CoordMismatchError);
  ASSERT_THROW(TestFixture::op(proxy, dataset.slice({Dim::Y, 3})),
               except::CoordMismatchError);

  ASSERT_THROW(TestFixture::op(proxy, dataset.slice({Dim::X, 3, 4})),
               except::CoordMismatchError);
  ASSERT_THROW(TestFixture::op(proxy, dataset.slice({Dim::Y, 3, 4})),
               except::CoordMismatchError);
  ASSERT_THROW(TestFixture::op(proxy, dataset.slice({Dim::Z, 3, 4})),
               except::CoordMismatchError);
}

template <class Op>
class DatasetBinaryOpTest : public ::testing::Test,
                            public ::testing::WithParamInterface<Op> {
protected:
  Op op;
};

TYPED_TEST_SUITE(DatasetBinaryOpTest, Binary);

TYPED_TEST(DatasetBinaryOpTest,
           dataset_const_lvalue_lhs_dataset_const_lvalue_rhs) {
  auto dataset_a = datasetFactory.make();
  auto dataset_b = datasetFactory.make();

  const auto res = TestFixture::op(dataset_a, dataset_b);

  for (const auto & [ name, item ] : res) {
    const auto reference =
        TestFixture::op(dataset_a[name].data(), dataset_b[name].data());
    EXPECT_EQ(reference, item.data());
  }
}

TYPED_TEST(DatasetBinaryOpTest, dataset_rvalue_lhs_dataset_const_lvalue_rhs) {
  const auto dataset_a = datasetFactory.make();
  auto dataset_b = datasetFactory.make();

  auto dataset_a_copy(dataset_a);
  const auto res = TestFixture::op(std::move(dataset_a_copy), dataset_b);

  for (const auto & [ name, item ] : res) {
    const auto reference =
        TestFixture::op(dataset_a[name].data(), dataset_b[name].data());
    EXPECT_EQ(reference, item.data());
  }
}

TYPED_TEST(DatasetBinaryOpTest, dataset_const_lvalue_lhs_dataset_rvalue_rhs) {
  auto dataset_a = datasetFactory.make();
  const auto dataset_b = datasetFactory.make();

  auto dataset_b_copy(dataset_b);
  const auto res = TestFixture::op(dataset_a, std::move(dataset_b_copy));

  for (const auto & [ name, item ] : res) {
    const auto reference =
        TestFixture::op(dataset_a[name].data(), dataset_b[name].data());
    EXPECT_EQ(reference, item.data());
  }
}

TYPED_TEST(DatasetBinaryOpTest, dataset_rvalue_lhs_dataset_rvalue_rhs) {
  const auto dataset_a = datasetFactory.make();
  const auto dataset_b = datasetFactory.make();

  auto dataset_a_copy(dataset_a);
  auto dataset_b_copy(dataset_b);
  const auto res =
      TestFixture::op(std::move(dataset_a_copy), std::move(dataset_b_copy));

  for (const auto & [ name, item ] : res) {
    const auto reference =
        TestFixture::op(dataset_a[name].data(), dataset_b[name].data());
    EXPECT_EQ(reference, item.data());
  }
}

TYPED_TEST(DatasetBinaryOpTest,
           dataset_const_lvalue_lhs_datasetconstproxy_const_lvalue_rhs) {
  auto dataset_a = datasetFactory.make();
  auto dataset_b = datasetFactory.make();

  DatasetConstProxy dataset_b_proxy(dataset_b);
  const auto res = TestFixture::op(dataset_a, dataset_b_proxy);

  for (const auto & [ name, item ] : res) {
    const auto reference =
        TestFixture::op(dataset_a[name].data(), dataset_b[name].data());
    EXPECT_EQ(reference, item.data());
  }
}

TYPED_TEST(DatasetBinaryOpTest,
           datasetconstproxy_const_lvalue_lhs_dataset_const_lvalue_rhs) {
  auto dataset_a = datasetFactory.make();
  auto dataset_b = datasetFactory.make();

  DatasetConstProxy dataset_a_proxy(dataset_a);
  const auto res = TestFixture::op(dataset_a_proxy, dataset_b);

  for (const auto & [ name, item ] : res) {
    const auto reference =
        TestFixture::op(dataset_a[name].data(), dataset_b[name].data());
    EXPECT_EQ(reference, item.data());
  }
}

TYPED_TEST(
    DatasetBinaryOpTest,
    datasetconstproxy_const_lvalue_lhs_datasetconstproxy_const_lvalue_rhs) {
  auto dataset_a = datasetFactory.make();
  auto dataset_b = datasetFactory.make();

  DatasetConstProxy dataset_a_proxy(dataset_a);
  DatasetConstProxy dataset_b_proxy(dataset_b);
  const auto res = TestFixture::op(dataset_a_proxy, dataset_b_proxy);

  for (const auto & [ name, item ] : res) {
    const auto reference =
        TestFixture::op(dataset_a[name].data(), dataset_b[name].data());
    EXPECT_EQ(reference, item.data());
  }
}

TYPED_TEST(DatasetBinaryOpTest,
           dataset_rvalue_lhs_datasetconstproxy_const_lvalue_rhs) {
  const auto dataset_a = datasetFactory.make();
  auto dataset_b = datasetFactory.make();

  auto dataset_a_copy(dataset_a);
  DatasetConstProxy dataset_b_proxy(dataset_b);
  const auto res = TestFixture::op(std::move(dataset_a_copy), dataset_b_proxy);

  for (const auto & [ name, item ] : res) {
    const auto reference =
        TestFixture::op(dataset_a[name].data(), dataset_b[name].data());
    EXPECT_EQ(reference, item.data());
  }
}

TYPED_TEST(DatasetBinaryOpTest, dataset_rvalue_lhs_dataproxy_const_lvalue_rhs) {
  const auto dataset_a = datasetFactory.make();
  auto dataset_b = datasetFactory.make();

  auto dataset_a_copy(dataset_a);
  const auto res =
      TestFixture::op(std::move(dataset_a_copy), dataset_b["data_scalar"]);

  for (const auto & [ name, item ] : res) {
    const auto reference = TestFixture::op(dataset_a[name].data(),
                                           dataset_b["data_scalar"].data());
    EXPECT_EQ(reference, item.data());
  }
}

TYPED_TEST(DatasetBinaryOpTest,
           dataset_const_lvalue_lhs_dataproxy_const_lvalue_rhs) {
  auto dataset_a = datasetFactory.make();
  auto dataset_b = datasetFactory.make();

  const auto res = TestFixture::op(dataset_a, dataset_b["data_scalar"]);

  for (const auto & [ name, item ] : res) {
    const auto reference = TestFixture::op(dataset_a[name].data(),
                                           dataset_b["data_scalar"].data());
    EXPECT_EQ(reference, item.data());
  }
}
