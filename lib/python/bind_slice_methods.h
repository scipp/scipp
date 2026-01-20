// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "numpy.h"
#include "pybind11.h"
#include "scipp/core/dtype.h"
#include "scipp/core/slice.h"
#include "scipp/core/tag_util.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/extract.h"
#include "scipp/dataset/shape.h"
#include "scipp/dataset/slice.h"
#include "scipp/dataset/util.h"
#include "scipp/variable/slice.h"
#include "scipp/variable/variable.h"
#include "slice_utils.h"

namespace py = pybind11;
using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

template <class T> auto to_dim_type(std::tuple<std::string, T> &&t) {
  return std::tuple{Dim{std::move(std::get<0>(t))}, std::move(std::get<1>(t))};
}

template <class T> auto dim_extent(const T &object, const Dim dim) {
  if constexpr (std::is_same_v<T, Dataset>) {
    scipp::index extent = -1;
    if (object.sizes().contains(dim))
      extent = object.sizes().at(dim);
    return extent;
  } else {
    return object.dims()[dim];
  }
}

template <class T>
auto from_py_slice(const T &source,
                   const std::tuple<Dim, const py::slice> &index) {
  const auto &[dim, indices] = index;
  size_t start, stop, step, slicelength;
  const auto size = dim_extent(source, dim);
  if (!indices.compute(size, &start, &stop, &step, &slicelength))
    throw py::error_already_set();
  if (slicelength == 0) {
    stop = start; // Propagate vanishing slice length downstream.
  }
  return Slice(dim, start, stop, step);
}

template <class View> struct SetData {
  template <class T> struct Impl {
    static void apply(View &slice, const py::object &obj) {
      if (slice.has_variances())
        throw std::runtime_error("Data object contains variances, to set data "
                                 "values use the `values` property or provide "
                                 "a tuple of values and variances.");

      copy_array_into_view(cast_to_array_like<T>(obj, slice.unit()),
                           slice.template values<T>(), slice.dims());
    }
  };
};

inline void throw_index_error(const scipp::index i, const scipp::index size) {
  throw std::out_of_range("The requested index " + std::to_string(i) +
                          " is out of range. Dimension size is " +
                          std::to_string(size) + " and the allowed range is [" +
                          std::to_string(-size) + ":" +
                          std::to_string(size - 1) + "].");
}

template <class T>
auto get_slice(T &self, const std::tuple<Dim, scipp::index> &index) {
  auto [dim, i] = index;
  auto sz = dim_extent(self, dim);
  if (i < -sz || i >= sz) // index is out of range
    throw_index_error(i, sz);
  if (i < 0)
    i = sz + i;
  return Slice(dim, i);
}

template <class T>
auto get_slice_range(T &self, const std::tuple<Dim, const py::slice> &index) {
  auto [dim, py_slice] = index;
  if constexpr (std::is_same_v<T, DataArray> || std::is_same_v<T, Dataset>) {
    auto start = py::getattr(py_slice, "start");
    auto stop = py::getattr(py_slice, "stop");
    if (!start.is_none() || !stop.is_none()) { // Means default slice : is
                                               // treated as index slice
      try {
        auto [start_var, stop_var] = label_bounds_from_pyslice(py_slice);
        return std::make_from_tuple<Slice>(
            get_slice_params(self, dim, start_var, stop_var));
      } catch (const py::cast_error &) {
      }
    }
  }
  return from_py_slice(self, index);
}

template <class T>
auto getitem(T &self, const std::tuple<Dim, scipp::index> &index) {
  return self.slice(get_slice(self, index));
}

template <class T>
auto getitem(T &self, const std::tuple<Dim, py::slice> &index) {
  return self.slice(get_slice_range(self, index));
}

template <class T> auto getitem(T &self, const py::ellipsis &) {
  return self.slice({});
}

// Helpers wrapped in struct to avoid unresolvable overloads.
template <class T> struct slicer {
  static auto get_by_value(T &self,
                           const std::tuple<std::string, Variable> &value) {
    auto &[dim, val] = value;
    return slice(self, Dim{dim}, val);
  }

  static void set_from_numpy(T &&slice, const py::object &obj) {
    core::CallDType<double, float, int64_t, int32_t,
                    bool>::apply<SetData<T>::template Impl>(slice.dtype(),
                                                            slice, obj);
  }

  template <class Other>
  static void set_from_view(T &self, const std::tuple<Dim, scipp::index> &index,
                            const Other &data) {
    self.setSlice(get_slice(self, index), data);
  }

  template <class Other>
  static void set_from_view(T &self,
                            const std::tuple<Dim, const py::slice> &index,
                            const Other &data) {
    self.setSlice(get_slice_range(self, index), data);
  }

  template <class Other>
  static void set_from_view(T &self, const py::ellipsis &, const Other &data) {
    self.setSlice(Slice{}, data);
  }

  template <class Other>
  static void set_by_value(T &self,
                           const std::tuple<std::string, Variable> &value,
                           const Other &data) {
    auto &[dim, val] = value;
    self.setSlice(
        std::make_from_tuple<Slice>(get_slice_params(self, Dim(dim), val)),
        data);
  }

  // Manually dispatch based on the object we are assigning from in order to
  // cast it correctly to a scipp view, numpy array or fallback std::vector.
  // This needs to happen partly based on the dtype which cannot be encoded
  // in the Python bindings directly.
  template <class IndexOrRange>
  static void set(T &self, const IndexOrRange &index, const py::object &data) {
    if constexpr (std::is_same_v<T, Dataset>)
      if (py::isinstance<Dataset>(data))
        return set_from_view(self, index, py::cast<Dataset>(data));

    if constexpr (!std::is_same_v<T, Variable>)
      if (py::isinstance<DataArray>(data))
        return set_from_view(self, index, py::cast<DataArray>(data));

    if (py::isinstance<Variable>(data))
      return set_from_view(self, index, py::cast<Variable>(data));

    if constexpr (!std::is_same_v<T, Dataset>)
      return set_from_numpy(getitem(self, index), data);

    std::ostringstream oss;
    oss << "Cannot to assign a " << py::str(py::type::of(data))
        << " to a slice of a " << py::type_id<T>();
    throw py::type_error(oss.str());
  }
};

namespace {
void expect_implicit_dimension(const Sizes &dims) {
  using std::to_string;
  if (dims.size() == 0)
    throw except::DimensionError("Slicing a scalar object is not possible.");
  if (dims.size() > 1) {
    std::string msg("Slicing with implicit dimension label is only possible "
                    "for 1-D objects. Got " +
                    to_string(dims) + " with ndim=" + to_string(dims.size()) +
                    ". Provide an explicit dimension label, e.g., var['" +
                    to_string(*dims.begin()) + "', 0] instead of var[0].");
    throw except::DimensionError(msg);
  }
}

void expect_positional_index(const py::slice &py_slice) {
  for (const auto &key : {"start", "stop"}) {
    if (const auto index = py::getattr(py_slice, key); !index.is_none()) {
      try {
        static_cast<void>(index.cast<Variable>());
        throw except::DimensionError(
            "Dimension must be specified when indexing with a label.");
      } catch (const py::cast_error &) {
      }
    }
  }
}

template <class T>
T slice_by_list(const T &obj,
                const std::tuple<Dim, std::vector<scipp::index>> &index) {
  const auto make_slice = [](scipp::index p, const scipp::index s) {
    const auto positive_p = p < 0 ? s + p : p;
    if (positive_p < 0 || positive_p >= s)
      throw std::out_of_range("The requested index " + std::to_string(p) +
                              " is out of range for dimension of length " +
                              std::to_string(s));
    return std::tuple{positive_p, positive_p + 1};
  };
  const auto &[dim, indices] = index;
  const auto size = obj.dims()[dim];
  if (!indices.empty()) {
    const auto [min, max] = std::minmax_element(indices.begin(), indices.end());
    if (*min < -size || *max >= size) {
      const auto bad = *min < -size ? *min : *max;
      throw_index_error(bad, size);
    }
  }
  std::vector<scipp::index_pair> ranges;
  ranges.reserve(indices.size());
  for (const auto &pos : indices) {
    const auto [start, stop] = make_slice(pos, size);
    ranges.emplace_back(static_cast<scipp::index>(start),
                        static_cast<scipp::index>(stop));
  }
  return extract_ranges(makeVariable<scipp::index_pair>(
                            Dims{dim}, Shape{ranges.size()}, Values(ranges)),
                        obj, dim);
}
} // namespace

template <class T, class... Ignored>
void bind_slice_methods(pybind11::class_<T, Ignored...> &c) {
  // Slice with implicit dim possible only if there is exactly one dimension. We
  // do *not* use the numpy/xarray mechanism which slices the outer dimension in
  // this case, since we consider it dangerous, leading to hard to find bugs.
  c.def("__getitem__", [](T &self, const scipp::index &index) {
    expect_implicit_dimension(self.dims());
    return getitem(self, {self.dim(), index});
  });
  c.def("__getitem__", [](T &self, const py::slice &index) {
    expect_implicit_dimension(self.dims());
    expect_positional_index(index);
    return getitem(self, {self.dim(), index});
  });
  // Note the order of overloads: For some reason pybind11(?) calls `len()` on
  // __getitem__ arguments when there is an overload accepting std::tuple. This
  // fails for scalar variables, so we place this before those overloads.
  c.def(
      "__getitem__",
      [](T &self, const Variable &condition) {
        return extract(self, condition);
      },
      py::call_guard<py::gil_scoped_release>());
  // These overloads must also be placed before the ones accepting std::tuple.
  // Otherwise, pybind11 would call `int()` on the 2nd tuple element
  // because of `Variable.__int__`.
  //
  // Note that label-based indexing with an implicit unique dim is not supported
  // for now. Labels can also be taken from any other coord, so if the coord
  // label is omitted we would have to "default" to using the dimension coord.
  // This may be too confusing, so we do not implement this for now. Note that
  // the objection to this is not absolute (unlike in the case of slicing outer
  // dimension above).
  if constexpr (std::is_same_v<T, DataArray>) {
    c.def("__getitem__", &slicer<T>::get_by_value);
    c.def("__setitem__", &slicer<T>::template set_by_value<Variable>);
    c.def("__setitem__", &slicer<T>::template set_by_value<DataArray>);
  }
  if constexpr (std::is_same_v<T, Dataset>) {
    c.def("__getitem__", &slicer<T>::get_by_value);
    c.def("__setitem__", &slicer<T>::template set_by_value<Dataset>);
  } else {
    c.def("__len__", [](const T &self) {
      if (self.dims().ndim() == 0)
        throw except::TypeError("len() of scalar object");
      return self.dims().size(0);
    });
    c.def("_ipython_key_completions_", [](T &self) {
      py::typing::List<py::str> out;
      for (const auto &dim : self.dims()) {
        out.append(dim.name());
      }
      return out;
    });
  }
  c.def("__getitem__",
        [](T &self, std::tuple<std::string, scipp::index> index) {
          return getitem(self, to_dim_type(std::move(index)));
        });
  c.def("__getitem__", [](T &self, std::tuple<std::string, py::slice> index) {
    return getitem(self, to_dim_type(std::move(index)));
  });
  c.def("__getitem__", [](T &self, const py::ellipsis &index) {
    return getitem(self, index);
  });
  c.def("__setitem__",
        [](T &self, const scipp::index &index, const py::object &data) {
          expect_implicit_dimension(self.dims());
          slicer<T>::template set<std::tuple<Dim, scipp::index>>(
              self, {self.dim(), index}, data);
        });
  c.def("__setitem__",
        [](T &self, const py::slice &index, const py::object &data) {
          expect_implicit_dimension(self.dims());
          expect_positional_index(index);
          slicer<T>::template set<std::tuple<Dim, py::slice>>(
              self, {self.dim(), index}, data);
        });
  c.def("__setitem__", [](T &self, std::tuple<std::string, scipp::index> index,
                          const py::object &data) {
    slicer<T>::template set<std::tuple<Dim, scipp::index>>(
        self, to_dim_type(std::move(index)), data);
  });
  c.def("__setitem__", [](T &self, std::tuple<std::string, py::slice> index,
                          const py::object &data) {
    slicer<T>::template set<std::tuple<Dim, py::slice>>(
        self, to_dim_type(std::move(index)), data);
  });
  c.def("__setitem__", &slicer<T>::template set<py::ellipsis>);
  c.def(
      "__getitem__",
      [](T &self, const std::vector<scipp::index> &indices) {
        expect_implicit_dimension(self.dims());
        return slice_by_list(self, {self.dim(), indices});
      },
      py::call_guard<py::gil_scoped_release>());
  c.def(
      "__getitem__",
      [](T &self, std::tuple<std::string, std::vector<scipp::index>> indices) {
        return slice_by_list<T>(self, to_dim_type(std::move(indices)));
      },
      py::call_guard<py::gil_scoped_release>());
}
