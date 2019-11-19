// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "histogram.h"
#include "scipp/common/numeric.h"
#include "scipp/core/dataset.h"
#include "scipp/core/subspan_view.h"
#include "scipp/core/transform.h"

namespace scipp::core {

DataArray::DataArray(const DataConstProxy &proxy) {
  m_holder.setData(proxy.name(), proxy);
}

DataArray::operator DataConstProxy() const { return get(); }
DataArray::operator DataProxy() { return get(); }

void requireValid(const DataArray &a) {
  if (!a)
    throw std::runtime_error("Invalid DataArray.");
}

DataConstProxy DataArray::get() const {
  requireValid(*this);
  return m_holder.begin()->second;
}

DataProxy DataArray::get() {
  requireValid(*this);
  return m_holder.begin()->second;
}

constexpr static auto times = [](const auto &a, const auto &b) {
  return a * b;
};

constexpr static auto divide = [](const auto &a, const auto &b) {
  return a / b;
};

template <class Op>
Variable sparse_dense_op_impl(Op op, const VariableConstProxy &sparseCoord_,
                              const VariableConstProxy &edges_,
                              const VariableConstProxy &weights_) {
  const Dim dim = sparseCoord_.dims().sparseDim();
  return transform<
      std::tuple<std::tuple<sparse_container<double>, span<const double>,
                            span<const double>>,
                 std::tuple<sparse_container<float>, span<const float>,
                            span<const float>>>>(
      sparseCoord_, subspan_view(edges_, dim), subspan_view(weights_, dim),
      overloaded{
          [op](const auto &sparse, const auto &edges, const auto &weights) {
            expect::histogram::sorted_edges(edges);
            using W = std::decay_t<decltype(weights)>;
            constexpr bool have_variance =
                core::detail::is_ValueAndVariance_v<W>;
            using T = sparse_container<
                typename core::detail::element_type_t<W>::value_type>;
            T out_vals;
            T out_vars;
            out_vals.reserve(sparse.size());
            if (have_variance)
              out_vars.reserve(sparse.size());
            if (scipp::numeric::is_linspace(edges)) {
              auto len = scipp::size(sparse) - 1;
              const auto offset = edges.front();
              const auto nbin = static_cast<decltype(offset)>(len);
              const auto scale = nbin / (edges.back() - edges.front());
              for (const auto c : sparse) {
                const auto bin = (c - offset) * scale;
                if (bin >= 0.0 && bin < nbin) {
                  if constexpr (have_variance) {
                    const auto [val, var] =
                        op(1.0, detail::ValueAndVariance{
                                    weights.value[bin], weights.variance[bin]});
                    out_vals.emplace_back(val);
                    out_vars.emplace_back(var);
                  } else {
                    out_vals.emplace_back(op(1.0, weights[bin]));
                  }
                } else {
                  if constexpr (have_variance) {
                    const auto [val, var] =
                        op(1.0, detail::ValueAndVariance{0.0, 0.0});
                    out_vals.emplace_back(val);
                    out_vars.emplace_back(var);
                  } else {
                    out_vals.emplace_back(op(1.0, 0.0));
                  }
                }
              }
            } else {
              throw std::runtime_error("Only equal-sized bins supported.");
            }
            if constexpr (have_variance) {
              return std::pair(std::move(out_vals), std::move(out_vars));
            } else {
              return out_vals;
            }
          },
          transform_flags::expect_no_variance_arg<0>,
          transform_flags::expect_no_variance_arg<1>,
          [op](const units::Unit &sparse, const units::Unit &edges,
               const units::Unit &weights) {
            expect::equals(sparse, edges);
            // Sparse data without values has an implicit value of 1 count.
            return op(units::Unit(units::counts), weights);
          }});
}

DataArray &DataArray::operator+=(const DataConstProxy &other) {
  expect::coordsAndLabelsAreSuperset(*this, other);
  data() += other.data();
  return *this;
}

DataArray &DataArray::operator-=(const DataConstProxy &other) {
  expect::coordsAndLabelsAreSuperset(*this, other);
  data() -= other.data();
  return *this;
}

DataArray &DataArray::operator*=(const DataConstProxy &other) {
  expect::coordsAndLabelsAreSuperset(*this, other);
  data() *= other.data();
  return *this;
}

DataArray &DataArray::operator/=(const DataConstProxy &other) {
  expect::coordsAndLabelsAreSuperset(*this, other);
  data() /= other.data();
  return *this;
}

DataArray &DataArray::operator+=(const VariableConstProxy &other) {
  data() += other;
  return *this;
}

DataArray &DataArray::operator-=(const VariableConstProxy &other) {
  data() -= other;
  return *this;
}

DataArray &DataArray::operator*=(const VariableConstProxy &other) {
  data() *= other;
  return *this;
}

DataArray &DataArray::operator/=(const VariableConstProxy &other) {
  data() /= other;
  return *this;
}

DataArray operator+(const DataConstProxy &a, const DataConstProxy &b) {
  return DataArray(a.data() + b.data(), union_(a.coords(), b.coords()),
                   union_(a.labels(), b.labels()),
                   union_or(a.masks(), b.masks()));
}

DataArray operator-(const DataConstProxy &a, const DataConstProxy &b) {
  return {a.data() - b.data(), union_(a.coords(), b.coords()),
          union_(a.labels(), b.labels()), union_or(a.masks(), b.masks())};
}

bool is_histogram(const DataConstProxy &a, const Dim dim) {
  const auto dims = a.dims();
  const auto coords = a.coords();
  return !dims.sparse() && dims.contains(dim) && coords.contains(dim) &&
         coords[dim].dims()[dim] == dims[dim] + 1;
}

bool is_sparse_and_histogram(const DataConstProxy &a, const DataConstProxy &b) {
  return (a.dims().sparse() && is_histogram(b, a.dims().sparseDim())) ||
         (b.dims().sparse() && is_histogram(a, b.dims().sparseDim()));
}

template <class Op>
auto sparse_dense_op(Op op, const DataConstProxy &a, const DataConstProxy &b) {
  // Note that the current implementation will fail if any of the inputs is
  // sparse data *with weights*, except for the special case of matching sizes.
  if (!is_sparse_and_histogram(a, b))
    return op(a.data(), b.data());
  if (a.dims().sparse() && !a.hasData()) {
    const Dim dim = a.dims().sparseDim();
    return sparse_dense_op_impl(op, a.coords()[dim], b.coords()[dim], b.data());
  }
  // histogram divided by sparse not supported, would typically result in unit
  // 1/counts which is meaningless
  if constexpr (std::is_same_v<Op, std::decay_t<decltype(times)>>)
    return sparse_dense_op(op, b, a);

  throw except::SparseDataError("Unsupported combination of sparse and dense "
                                "data in binary arithmetic operation.");
}

auto sparse_dense_coord_union(const DataConstProxy &a,
                              const DataConstProxy &b) {
  if (!is_sparse_and_histogram(a, b))
    return union_(a.coords(), b.coords());
  // Use slice to remove dense coord, since output will be sparse.
  if (a.dims().sparse())
    return union_(a.coords(), b.slice({a.dims().sparseDim(), 0}).coords());
  else
    return union_(a.slice({b.dims().sparseDim(), 0}).coords(), b.coords());
}

DataArray operator*(const DataConstProxy &a, const DataConstProxy &b) {
  return {sparse_dense_op(times, a, b), sparse_dense_coord_union(a, b),
          union_(a.labels(), b.labels()), union_or(a.masks(), b.masks())};
}

DataArray operator/(const DataConstProxy &a, const DataConstProxy &b) {
  return {sparse_dense_op(divide, a, b), sparse_dense_coord_union(a, b),
          union_(a.labels(), b.labels()), union_or(a.masks(), b.masks())};
}

DataArray operator+(const DataConstProxy &a, const VariableConstProxy &b) {
  return DataArray(a.data() + b, a.coords(), a.labels(), a.masks(), a.attrs());
}

DataArray operator-(const DataConstProxy &a, const VariableConstProxy &b) {
  return DataArray(a.data() - b, a.coords(), a.labels(), a.masks(), a.attrs());
}

DataArray operator*(const DataConstProxy &a, const VariableConstProxy &b) {
  return DataArray(a.data() * b, a.coords(), a.labels(), a.masks(), a.attrs());
}

DataArray operator/(const DataConstProxy &a, const VariableConstProxy &b) {
  return DataArray(a.data() / b, a.coords(), a.labels(), a.masks(), a.attrs());
}

DataArray operator+(const VariableConstProxy &a, const DataConstProxy &b) {
  return DataArray(a + b.data(), b.coords(), b.labels(), b.masks(), b.attrs());
}

DataArray operator-(const VariableConstProxy &a, const DataConstProxy &b) {
  return DataArray(a - b.data(), b.coords(), b.labels(), b.masks(), b.attrs());
}

DataArray operator*(const VariableConstProxy &a, const DataConstProxy &b) {
  return DataArray(a * b.data(), b.coords(), b.labels(), b.masks(), b.attrs());
}

DataArray operator/(const VariableConstProxy &a, const DataConstProxy &b) {
  return DataArray(a / b.data(), b.coords(), b.labels(), b.masks(), b.attrs());
}

} // namespace scipp::core
