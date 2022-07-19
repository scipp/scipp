// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Matthew Andrew

#include "scipp/dataset/util.h"
#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/variable/accumulate.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable.h"
#include "scipp/variable/variable_concept.h"

using namespace scipp::variable;
namespace scipp {

namespace {
constexpr auto size_of_kernel = overloaded{
    core::element::arg_list<std::tuple<scipp::index, std::string>>,
    [](auto &out, const std::string &str) {
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
    }};

template <class T>
scipp::index size_of_bins(const Variable &view, const SizeofTag tag) {
  const auto &[indices, dim, buffer] = view.constituents<T>();
  double scale = 1;
  if (tag == SizeofTag::ViewOnly) {
    const auto &[begin, end] = unzip(indices);
    const auto sizes = sum(end - begin).template value<scipp::index>();
    // avoid division by zero
    scale = sizes == 0 ? 0.0 : sizes / static_cast<double>(buffer.dims()[dim]);
  }
  return size_of(indices, tag) + size_of(buffer, tag) * scale;
}
} // namespace

scipp::index size_of(const Variable &view, const SizeofTag tag) {
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
    auto size = makeVariable<scipp::index>(Shape{}, Values{0});
    if (tag == SizeofTag::Underlying) {
      const Variable full(core::Dimensions{Dim::X, view.data().size()},
                          view.data_handle());
      accumulate_in_place(size, full, size_of_kernel, "size_of");
    } else {
      accumulate_in_place(size, view, size_of_kernel, "size_of");
    }
    return size.value<scipp::index>();
  }

  const auto value_size = view.data().dtype_size();
  const auto variance_scale = view.has_variances() ? 2 : 1;
  const auto data_size =
      tag == SizeofTag::Underlying ? view.data().size() : view.dims().volume();
  return data_size * value_size * variance_scale;
}

/// Return the size in memory of a DataArray object. The aligned coord is
/// optional because for a DataArray owned by a dataset aligned coords are
/// assumed to be owned by the dataset as they can apply to multiple arrays.
scipp::index size_of(const DataArray &dataarray, const SizeofTag tag,
                     bool include_aligned_coords) {
  scipp::index size = 0;
  size += size_of(dataarray.data(), tag);
  for (const auto &coord : dataarray.attrs()) {
    size += size_of(coord.second, tag);
  }
  for (const auto &mask : dataarray.masks()) {
    size += size_of(mask.second, tag);
  }
  if (include_aligned_coords) {
    for (const auto &coord : dataarray.coords()) {
      size += size_of(coord.second, tag);
    }
  }
  return size;
}

scipp::index size_of(const Dataset &dataset, const SizeofTag tag) {
  scipp::index size = 0;
  for (const auto &data : dataset) {
    size += size_of(data, tag, false);
  }
  for (const auto &coord : dataset.coords()) {
    size += size_of(coord.second, tag);
  }
  return size;
}
} // namespace scipp
