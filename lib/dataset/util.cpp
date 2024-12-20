// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Matthew Andrew

#include "scipp/dataset/util.h"
#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/dataset/sized_dict_forward.h"
#include "scipp/variable/accumulate.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/structure_array_model.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable.h"
#include "scipp/variable/variable_concept.h"

#include "dataset_operations_common.h"

using namespace scipp::variable;
namespace scipp {

namespace {
scipp::index
size_of_elements(const Variable &view, SizeofTag tag,
                 const std::optional<std::pair<Dim, double>> &scale_in_dim);

scipp::index
size_of_impl(const Variable &var, const SizeofTag tag,
             const std::optional<std::pair<Dim, double>> &scale_in_dim) {
  const auto object_size =
      static_cast<scipp::index>(sizeof(Variable)) + var.data().object_size();
  return size_of_elements(var, tag, scale_in_dim) + object_size;
}

/// Return the size in memory of a DataArray object. The aligned coord is
/// optional because for a DataArray owned by a dataset aligned coords are
/// assumed to be owned by the dataset as they can apply to multiple arrays.
scipp::index
size_of_impl(const DataArray &da, const SizeofTag tag,
             const std::optional<std::pair<Dim, double>> &scale_in_dim,
             bool include_coords = true) {
  auto size = static_cast<scipp::index>(
      sizeof(DataArray) + sizeof(dataset::Coords) + sizeof(dataset::Masks) +
      da.coords().capacity() + da.masks().capacity());
  size += size_of_impl(da.data(), tag, scale_in_dim);
  for (const auto &mask : da.masks()) {
    size += size_of_impl(mask.second, tag, scale_in_dim);
  }
  if (include_coords) {
    for (const auto &coord : da.coords()) {
      size += size_of_impl(coord.second, tag, scale_in_dim);
    }
  }
  return size;
}

scipp::index
size_of_impl(const Dataset &ds, const SizeofTag tag,
             const std::optional<std::pair<Dim, double>> &scale_in_dim) {
  auto size = static_cast<scipp::index>(sizeof(Dataset) + ds.capacity());
  for (const auto &data : ds) {
    size += size_of_impl(data, tag, scale_in_dim, false);
  }
  for (const auto &coord : ds.coords()) {
    size += size_of_impl(coord.second, tag, scale_in_dim);
  }
  return size;
}

struct SizeOfKernel {
  using types = std::tuple<std::tuple<scipp::index, std::string>,
                           std::tuple<scipp::index, scipp::index>>;

  void operator()(scipp::index &out, const std::string &str) {
    if (const auto str_address =
            reinterpret_cast<const char *>(std::addressof(str));
        str.data() > str_address &&
        str.data() + str.size() < str_address + sizeof(std::string)) {
      // Small string optimization: The characters are located
      // in the string's internal buffer.
      out += sizeof(std::string);
    } else {
      // A long string: The characters are in a separate
      // array on the heap.
      out += sizeof(std::string) + str.size();
    }
  }
  void operator()(scipp::index &out, const scipp::index &s) { out += s; }
};

struct SizeOfContainerKernel {
  using types = std::tuple<std::tuple<scipp::index, Variable>,
                           std::tuple<scipp::index, DataArray>,
                           std::tuple<scipp::index, Dataset>>;

  SizeofTag tag;

  template <class T> void operator()(scipp::index &out, const T &x) const {
    out += size_of(x, tag);
  }
};

template <class T>
scipp::index size_of_bins(const Variable &view, const SizeofTag tag) {
  const auto &[indices, dim, buffer] = view.constituents<T>();
  std::pair<Dim, double> scale_in_dim{Dim::Invalid, 1.0};
  if (tag == SizeofTag::ViewOnly) {
    const auto &[begin, end] = unzip(indices);
    const auto sizes = sum(end - begin).template value<scipp::index>();
    // avoid division by zero
    scale_in_dim.second =
        sizes == 0 ? 0.0 : sizes / static_cast<double>(buffer.dims()[dim]);
    scale_in_dim.first = dim;
  }
  const auto indices_volume = tag == SizeofTag::Underlying
                                  ? indices.data().size()
                                  : indices.dims().volume();
  return indices_volume * sizeof(scipp::index_pair) +
         sizeof(
             variable::StructureArrayModel<scipp::index_pair, scipp::index>) +
         size_of_impl(buffer, tag, scale_in_dim);
}

template <class Op>
auto accumulate_size_of(const Variable &view, const SizeofTag tag,
                        const Op &op) {
  auto size = makeVariable<scipp::index>(Shape{}, Values{0});
  if (tag == SizeofTag::Underlying) {
    const Variable full(core::Dimensions{Dim::X, view.data().size()},
                        view.data_handle());
    accumulate_in_place(size, full, op, "size_of");
  } else {
    accumulate_in_place(size, view, op, "size_of");
  }
  return size.value<scipp::index>();
}

scipp::index
size_of_elements(const Variable &view, const SizeofTag tag,
                 const std::optional<std::pair<Dim, double>> &scale_in_dim) {
  if (view.dtype() == dtype<bucket<Variable>>) {
    return size_of_bins<Variable>(view, tag);
  }
  if (view.dtype() == dtype<bucket<DataArray>>) {
    return size_of_bins<DataArray>(view, tag);
  }
  if (view.dtype() == dtype<bucket<Dataset>>) {
    return size_of_bins<Dataset>(view, tag);
  }
  if (view.dtype() == dtype<std::string>) {
    return accumulate_size_of(view, tag, SizeOfKernel{});
  }
  if (view.dtype() == dtype<Variable> || view.dtype() == dtype<DataArray> ||
      view.dtype() == dtype<Dataset>) {
    return accumulate_size_of(view, tag, SizeOfContainerKernel{tag});
  }

  const auto value_size = view.data().dtype_size();
  const auto variance_scale = view.has_variances() ? 2 : 1;
  const auto data_size =
      tag == SizeofTag::Underlying ? view.data().size() : view.dims().volume();
  const auto extra_scale =
      scale_in_dim.has_value() && view.dims().contains(scale_in_dim->first)
          ? scale_in_dim->second
          : 1.0;
  return static_cast<scipp::index>(
      static_cast<double>(data_size * value_size * variance_scale) *
      extra_scale);
}
} // namespace

scipp::index size_of(const Variable &var, const SizeofTag tag) {
  return size_of_impl(var, tag, std::nullopt);
}
scipp::index size_of(const DataArray &da, const SizeofTag tag,
                     const bool include_aligned_coords) {
  return size_of_impl(da, tag, std::nullopt, include_aligned_coords);
}
scipp::index size_of(const Dataset &ds, const SizeofTag tag) {
  return size_of_impl(ds, tag, std::nullopt);
}
} // namespace scipp

namespace scipp::dataset {
DataArray strip_edges_along(const DataArray &da, const Dim dim) {
  auto out = da;
  for (const auto &[name, var] : da.coords())
    if (core::is_edges(da.dims(), var.dims(), dim))
      out.coords().erase(name);
  for (const auto &[name, var] : da.masks())
    if (core::is_edges(da.dims(), var.dims(), dim))
      out.masks().erase(name);
  return out;
}

Dataset strip_edges_along(const Dataset &ds, const Dim dim) {
  auto out = apply_to_items(
      ds, [](auto &&...args) { return strip_edges_along(args...); }, dim);
  for (const auto &[name, var] : ds.coords())
    if (!core::is_edges(ds.sizes(), var.dims(), dim))
      out.setCoord(name, var);
  return out;
}

} // namespace scipp::dataset
