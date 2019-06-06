// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <variant>

#include <pybind11/eigen.h>
#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "dataset.h"
#include "except.h"

using namespace scipp;
using namespace scipp::core;

namespace py = pybind11;

template <class T, class ConstT>
void bind_mutable_proxy(py::module &m, const std::string &name) {
  py::class_<ConstT>(m, (name + "ConstProxy").c_str());
  py::class_<T, ConstT>(m, (name + "Proxy").c_str())
      .def("__len__", &T::size)
      .def("__getitem__", &T::operator[])
      .def("__iter__",
           [](T &self) { return py::make_iterator(self.begin(), self.end()); });
}

template <class T> void bind_coord_properties(py::class_<T> &c) {
  c.def_property_readonly("coords", [](T &self) { return self.coords(); });
  c.def_property_readonly("labels", [](T &self) { return self.labels(); });
  c.def_property_readonly("attrs", [](T &self) { return self.attrs(); });
}

template <class T> void bind_dataset_proxy_methods(py::class_<T> &c) {
  c.def("__len__", &T::size);
  c.def("__repr__", [](const T &self) { return to_string(self, "."); });
  c.def("__iter__",
        [](T &self) { return py::make_iterator(self.begin(), self.end()); });
  c.def("__getitem__",
        [](T &self, const std::string &name) { return self[name]; },
        py::keep_alive<0, 1>());
}

void init_dataset(py::module &m) {

  /*
  py::class_<DatasetProxy>(m, "DatasetProxy")
      .def(py::init<Dataset &>())
      .def_property_readonly(
          "dimensions",
          [](const DatasetProxy &self) { return self.dimensions(); },
          "A read-only Dimensions object containing the dimensions of the "
          "DatasetProxy.")
      .def("__len__", &DatasetProxy::size)
      .def("__iter__",
           [](DatasetProxy &self) {
             return py::make_iterator(self.begin(), self.end());
           },
           py::keep_alive<0, 1>())
      .def("__contains__", &DatasetProxy::contains, py::arg("tag"),
           py::arg("name") = "")
      .def("__contains__",
           [](const DatasetProxy &self,
              const std::tuple<const Tag, const std::string> &key) {
             return self.contains(std::get<0>(key), std::get<1>(key));
           })
      .def("__getitem__",
           [](DatasetProxy &self, const std::tuple<Dim, scipp::index> &index) {
             return getItemBySingleIndex(self, index);
           },
           py::keep_alive<0, 1>())
      .def("__getitem__", &pySlice<DatasetProxy>, py::keep_alive<0, 1>())
      .def("__getitem__",
           [](DatasetProxy &self, const Tag &tag) { return self(tag); },
           py::keep_alive<0, 1>())
      .def(
          "__getitem__",
          [](DatasetProxy &self, const std::pair<Tag, const std::string> &key) {
            return self(key.first, key.second);
          },
          py::keep_alive<0, 1>())
      .def("copy", [](const DatasetProxy &self) { return Dataset(self); },
           "Make a copy of a DatasetProxy.")
      .def("__copy__", [](DatasetProxy &self) { return Dataset(self); })
      .def("__deepcopy__",
           [](DatasetProxy &self, py::dict) { return Dataset(self); })
      .def_property_readonly(
          "subset", [](DatasetProxy &self) { return SubsetHelper(self); },
          "Used to extract a read-only subset of the DatasetProxy in two "
          "different ways:\n - one can use just a string, e.g. "
          "d.subset['sample'] to extract the sample and its variance, and all "
          "the relevant coordinates.\n - one can also use a tag/string "
          "combination for a more restrictive extraction, e.g. "
          "d.subset[Data.Value, 'sample'] to only get the Value and associated "
          "coordinates.\nAll other Variables (including attributes) are "
          "dropped.")
      .def(
          "__setitem__",
          [](DatasetProxy &self, const std::tuple<Dim, py::slice> &index,
             const DatasetProxy &other) { pySlice(self, index).assign(other); })
      .def("__setitem__",
           [](DatasetProxy &self, const std::tuple<Dim, scipp::index> &index,
              const DatasetProxy &other) {
             const auto & [ dim, i ] = index;
             self(dim, i).assign(other);
           })
      .def("__setitem__", setData<DatasetProxy, Key::Tag>)
      .def("__setitem__", setData<DatasetProxy, Key::TagName>)
      .def(py::self + py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self - py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self * py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self / py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self += py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self -= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self *= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self /= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self == py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self != py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self + double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self - double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self * double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self / double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self += double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self -= double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self *= double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self /= double(), py::call_guard<py::gil_scoped_release>())
      .def("__eq__",
           [](const DatasetProxy &self, const Dataset &other) {
             return self == other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__ne__",
           [](const DatasetProxy &self, const Dataset &other) {
             return self != other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__add__",
           [](const DatasetProxy &self, const Dataset &other) {
             return self + other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__add__",
           [](const DatasetProxy &self, const Variable &other) {
             return self + other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__sub__",
           [](const DatasetProxy &self, const Dataset &other) {
             return self - other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__sub__",
           [](const DatasetProxy &self, const Variable &other) {
             return self - other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__mul__",
           [](const DatasetProxy &self, const Dataset &other) {
             return self * other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__mul__",
           [](const DatasetProxy &self, const Variable &other) {
             return self * other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__truediv__",
           [](const DatasetProxy &self, const Dataset &other) {
             return self / other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__truediv__",
           [](const DatasetProxy &self, const Variable &other) {
             return self / other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__iadd__",
           [](const DatasetProxy &self, const Dataset &other) {
             return self += other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__iadd__",
           [](const DatasetProxy &self, const Variable &other) {
             return self += other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__isub__",
           [](const DatasetProxy &self, const Dataset &other) {
             return self -= other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__isub__",
           [](const DatasetProxy &self, const Variable &other) {
             return self -= other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__imul__",
           [](const DatasetProxy &self, const Dataset &other) {
             return self *= other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__imul__",
           [](const DatasetProxy &self, const Variable &other) {
             return self *= other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__itruediv__",
           [](const DatasetProxy &self, const Dataset &other) {
             return self /= other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__itruediv__",
           [](const DatasetProxy &self, const Variable &other) {
             return self /= other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__radd__",
           [](const DatasetProxy &self, double &other) { return self + other; },
           py::is_operator())
      .def("__rsub__",
           [](const DatasetProxy &self, double &other) { return other - self; },
           py::is_operator())
      .def("__rmul__",
           [](const DatasetProxy &self, double &other) { return self * other; },
           py::is_operator())
      .def("__repr__",
           [](const DatasetProxy &self) { return to_string(self, "."); });

      */

  bind_mutable_proxy<CoordsProxy, CoordsConstProxy>(m, "Coords");
  bind_mutable_proxy<LabelsProxy, LabelsConstProxy>(m, "Labels");
  bind_mutable_proxy<AttrsProxy, AttrsConstProxy>(m, "Attrs");

  py::class_<DataProxy> dataProxy(m, "DataProxy");
  dataProxy.def_property_readonly("data", &DataProxy::data,
                                  py::keep_alive<0, 1>());

  py::class_<DatasetProxy> datasetProxy(m, "DatasetProxy");

  py::class_<Dataset> dataset(m, "Dataset");
  dataset.def(py::init<>())
      .def("__setitem__", &Dataset::setData)
      .def("set_coord", &Dataset::setCoord);

  bind_dataset_proxy_methods(dataset);
  bind_dataset_proxy_methods(datasetProxy);
  bind_coord_properties(dataset);
  bind_coord_properties(datasetProxy);
  bind_coord_properties(dataProxy);

  /*
  .def_property_readonly(
      "dimensions", [](const Dataset &self) { return self.dimensions(); },
      "A read-only Dimensions object containing the dimensions of the "
      "Dataset.")
  .def(py::init<const DatasetProxy &>())
  // TODO: This __getitem__ is here only to prevent unhandled
  // errors when trying to get a dataset slice by supplying only a
  // Dimension, e.g. dataset[Dim.X]. By default, an implicit
  // conversion between Dim and scipp::index is attempted and the
  // __getitem__ then crashes when self[index] is performed below.
  // This fix covers only one case and we need to find a better way
  // of protecting all unsopprted cases. This should ideally fail
  // with a TypeError, in the same way as if only a string is
  // supplied, e.g. dataset["a"].
  .def("__getitem__",
       [](Dataset &, const Dim &) {
         throw std::runtime_error("Syntax error: cannot get slice with "
                                  "just a Dim, please use dataset[Dim.X, "
                                  ":]");
       })
  .def("__getitem__",
       [](Dataset &self, const scipp::index index) { return self[index]; },
       py::keep_alive<0, 1>())
  .def("__getitem__",
       [](Dataset &self, const std::tuple<Dim, scipp::index> &index) {
         return getItemBySingleIndex(self, index);
       },
       py::keep_alive<0, 1>())
  .def("__getitem__", &pySlice<Dataset>, py::keep_alive<0, 1>())
  .def("__getitem__",
       [](Dataset &self, const Tag &tag) { return self(tag); })
  .def("__getitem__",
       [](Dataset &self, const std::pair<Tag, const std::string> &key) {
         return self(key.first, key.second);
       },
       py::keep_alive<0, 1>())
  .def("copy", [](const Dataset &self) { return self; },
       "Make a copy of a Dataset.")
  .def("__copy__", [](Dataset &self) { return Dataset(self); })
  .def("__deepcopy__",
       [](Dataset &self, py::dict) { return Dataset(self); })
  .def_property_readonly(
      "subset", [](Dataset &self) { return SubsetHelper(self); },
      "Used to extract a read-only subset of the Dataset in two different "
      "ways: \n - one can use just a string, e.g. d.subset['sample'] to "
      "extract the sample and its variance, and all the relevant "
      "coordinates.\n - one can also use a tag/string combination for a "
      "more restrictive extraction, e.g. d.subset[Data.Value, 'sample'] to "
      "only get the Value and associated coordinates.\nAll other Variables "
      "(including attributes) are dropped. Note that a DatasetProxy is "
      "returned, not a new Dataset.")
  // Careful: The order of overloads is really important here,
  // otherwise DatasetProxy matches the overload below for
  // py::array_t. I have not understood all details of this yet
  // though. See also
  //
  https://pybind11.readthedocs.io/en/stable/advanced/functions.html#overload-resolution-order.
  .def(
      "__setitem__",
      [](Dataset &self, const std::tuple<Dim, py::slice> &index,
         const DatasetProxy &other) { pySlice(self, index).assign(other); })
  .def("__setitem__",
       [](Dataset &self, const std::tuple<Dim, scipp::index> &index,
          const DatasetProxy &other) {
         const auto & [ dim, i ] = index;
         self(dim, i).assign(other);
       })

  // A) No dimensions argument, this will set data of existing item.
  .def("__setitem__", setData<Dataset, Key::Tag>)
  .def("__setitem__", setData<Dataset, Key::TagName>)

  // B) Variants with dimensions, inserting new item.
  // 0. Insertion from Variable or Variable slice.
  .def("__setitem__", insert<Variable, Key::Tag>)
  .def("__setitem__", insert<Variable, Key::TagName>)
  .def("__setitem__", insert<VariableSlice, Key::Tag>)
  .def("__setitem__", insert<VariableSlice, Key::TagName>)
  // 1. Insertion with default init. TODO Should this be removed?
  .def("__setitem__", insertDefaultInit<Key::Tag>)
  .def("__setitem__", insertDefaultInit<Key::TagName>)
  // 2. Insertion from numpy.ndarray
  .def("__setitem__", insert_ndarray<Key::Tag>)
  .def("__setitem__", insert_ndarray<Key::TagName>)
  // 2. Handle integers before case 3. below, which would convert to double.
  .def("__setitem__", insert_0D<int64_t, Key::Tag>)
  .def("__setitem__", insert_0D<int64_t, Key::TagName>)
  .def("__setitem__", insert_0D<double, Key::Tag>)
  .def("__setitem__", insert_0D<double, Key::TagName>)
  .def("__setitem__", insert_0D<std::string, Key::Tag>)
  .def("__setitem__", insert_0D<std::string, Key::TagName>)
  .def("__setitem__", insert_0D<Dataset, Key::Tag>)
  .def("__setitem__", insert_0D<Dataset, Key::TagName>)
  // 3. Handle integers before case 4. below, which would convert to double.
  .def("__setitem__", insert_1D<int64_t, Key::Tag>)
  .def("__setitem__", insert_1D<int64_t, Key::TagName>)
  .def("__setitem__", insert_1D<Eigen::Vector3d, Key::Tag>)
  .def("__setitem__", insert_1D<Eigen::Vector3d, Key::TagName>)
  // 4. Insertion attempting forced conversion to array of double. This
  //    is handled by automatic conversion by pybind11 when using
  //    py::array_t. Handles also scalar data. See also the
  //    py::array::forcecast argument, we need to minimize implicit
  //    (and potentially expensive conversion). If we wanted to
  //    avoid some conversion we need to provide explicit variants
  //    for specific types, same as or similar to insert_1D in
  //    case 5. below.
  .def("__setitem__", insert_conv<double, Key::Tag>)
  .def("__setitem__", insert_conv<double, Key::TagName>)
  // 5. Insertion of numpy-incompatible data. py::array_t does not support
  //    non-POD types like std::string, so we need to handle them
  //    separately.
  .def("__setitem__", insert_1D<std::string, Key::Tag>)
  .def("__setitem__", insert_1D<std::string, Key::TagName>)
  .def("__setitem__", insert_1D<Dataset, Key::Tag>)
  .def("__setitem__", insert_1D<Dataset, Key::TagName>)

  // TODO Make sure we have all overloads covered to avoid implicit
  // conversion of DatasetProxy to Dataset.
  .def(py::self == py::self, py::call_guard<py::gil_scoped_release>())
  .def(py::self != py::self, py::call_guard<py::gil_scoped_release>())
  .def(py::self += py::self, py::call_guard<py::gil_scoped_release>())
  .def(py::self -= py::self, py::call_guard<py::gil_scoped_release>())
  .def(py::self *= py::self, py::call_guard<py::gil_scoped_release>())
  .def(py::self /= py::self, py::call_guard<py::gil_scoped_release>())
  .def(py::self + py::self, py::call_guard<py::gil_scoped_release>())
  .def(py::self - py::self, py::call_guard<py::gil_scoped_release>())
  .def(py::self * py::self, py::call_guard<py::gil_scoped_release>())
  .def(py::self / py::self, py::call_guard<py::gil_scoped_release>())
  .def(py::self + double(), py::call_guard<py::gil_scoped_release>())
  .def(py::self - double(), py::call_guard<py::gil_scoped_release>())
  .def(py::self * double(), py::call_guard<py::gil_scoped_release>())
  .def(py::self / double(), py::call_guard<py::gil_scoped_release>())
  .def(py::self += double(), py::call_guard<py::gil_scoped_release>())
  .def(py::self -= double(), py::call_guard<py::gil_scoped_release>())
  .def(py::self *= double(), py::call_guard<py::gil_scoped_release>())
  .def(py::self /= double(), py::call_guard<py::gil_scoped_release>())
  .def("__eq__",
       [](const Dataset &self, const DatasetProxy &other) {
         return self == other;
       },
       py::call_guard<py::gil_scoped_release>())
  .def("__ne__",
       [](const Dataset &self, const DatasetProxy &other) {
         return self != other;
       },
       py::call_guard<py::gil_scoped_release>())
  .def("__iadd__",
       [](Dataset &self, const DatasetProxy &other) {
         return self += other;
       },
       py::call_guard<py::gil_scoped_release>())
  .def("__iadd__",
       [](Dataset &self, const Variable &other) { return self += other; },
       py::call_guard<py::gil_scoped_release>())
  .def("__isub__",
       [](Dataset &self, const DatasetProxy &other) {
         return self -= other;
       },
       py::call_guard<py::gil_scoped_release>())
  .def("__isub__",
       [](Dataset &self, const Variable &other) { return self -= other; },
       py::call_guard<py::gil_scoped_release>())
  .def("__imul__",
       [](Dataset &self, const DatasetProxy &other) {
         return self *= other;
       },
       py::call_guard<py::gil_scoped_release>())
  .def("__imul__",
       [](Dataset &self, const Variable &other) { return self *= other; },
       py::call_guard<py::gil_scoped_release>())
  .def("__itruediv__",
       [](Dataset &self, const DatasetProxy &other) {
         return self /= other;
       },
       py::call_guard<py::gil_scoped_release>())
  .def("__itruediv__",
       [](Dataset &self, const Variable &other) { return self /= other; },
       py::call_guard<py::gil_scoped_release>())
  .def("__add__",
       [](const Dataset &self, const DatasetProxy &other) {
         return self + other;
       },
       py::call_guard<py::gil_scoped_release>())
  .def("__add__",
       [](const Dataset &self, const Variable &other) {
         return self + other;
       },
       py::call_guard<py::gil_scoped_release>())
  .def("__sub__",
       [](const Dataset &self, const DatasetProxy &other) {
         return self - other;
       },
       py::call_guard<py::gil_scoped_release>())
  .def("__sub__",
       [](const Dataset &self, const Variable &other) {
         return self - other;
       },
       py::call_guard<py::gil_scoped_release>())
  .def("__mul__",
       [](const Dataset &self, const DatasetProxy &other) {
         return self * other;
       },
       py::call_guard<py::gil_scoped_release>())
  .def("__mul__",
       [](const Dataset &self, const Variable &other) {
         return self * other;
       },
       py::call_guard<py::gil_scoped_release>())
  .def("__truediv__",
       [](const Dataset &self, const DatasetProxy &other) {
         return self / other;
       },
       py::call_guard<py::gil_scoped_release>())
  .def("__truediv__",
       [](const Dataset &self, const Variable &other) {
         return self / other;
       },
       py::call_guard<py::gil_scoped_release>())
  .def("__radd__",
       [](const Dataset &self, double &other) { return self + other; },
       py::is_operator())
  .def("__rsub__",
       [](const Dataset &self, double &other) { return other - self; },
       py::is_operator())
  .def("__rmul__",
       [](const Dataset &self, double &other) { return self * other; },
       py::is_operator())
  .def("merge", &Dataset::merge,
       "Merge two Datasets together: all the "
       "Variables from the Dataset passed as an argument that do not exist "
       "in the present Dataset are copied. Variables from the argument "
       "Dataset that already exist (i.e. have the same Tag and name) in "
       "the present Dataset, are compared; if the two are identical, it is "
       "simply left alone in the parent Dataset. If they are not, the "
       "merge operation fails.");*/

  // Implicit conversion DatasetProxy -> Dataset. Reduces need for
  // excessive operator overload definitions
  py::implicitly_convertible<DatasetProxy, Dataset>();

  //-----------------------dataset free functions-------------------------------
  /*
  m.def("split",
        py::overload_cast<const Dataset &, const Dim,
                          const std::vector<scipp::index> &>(&split),
        py::call_guard<py::gil_scoped_release>(),
        "Split a Dataset along a given Dimension.");
  m.def("concatenate",
        py::overload_cast<const Dataset &, const Dataset &, const Dim>(
            &concatenate),
        py::call_guard<py::gil_scoped_release>(),
        "Returns a new dataset containing a concatenation of two Datasets "
        "and their underlying Variables along a given Dimension. All the "
        "Variable arrays are concatenated one by one. If there is any "
        "disagreement between the Datasets on Variable names, units, tags or "
        "dimensions, then the concatenation operation fails.");
  m.def("rebin", py::overload_cast<const Dataset &, const Variable &>(&rebin),
        py::call_guard<py::gil_scoped_release>(),
        "Returns a new Dataset whose data is re-gridded/rebinned/resampled to "
        "a new coordinate axis.");
  m.def("histogram",
        py::overload_cast<const Dataset &, const Variable &>(&histogram),
        py::call_guard<py::gil_scoped_release>(),
        "Perform histogramming of events.");
  m.def(
      "sort",
      py::overload_cast<const Dataset &, const Tag, const std::string &>(&sort),
      py::arg("dataset"), py::arg("tag"), py::arg("name") = "",
      py::call_guard<py::gil_scoped_release>());
  m.def("filter", py::overload_cast<const Dataset &, const Variable &>(&filter),
        py::call_guard<py::gil_scoped_release>());
  m.def("sum", py::overload_cast<const Dataset &, const Dim>(&sum),
        py::call_guard<py::gil_scoped_release>(),
        "Returns a new Dataset containing the sum of the data along the "
        "specified dimension.");
  m.def("mean", py::overload_cast<const Dataset &, const Dim>(&mean),
        py::call_guard<py::gil_scoped_release>(), py::arg("dataset"),
        py::arg("dim"),
        "Returns a new Dataset containing the mean of the data along the "
        "specified dimension. Any variances in the input dataset are "
        "transformed into the variance of the mean.");
  m.def("integrate", py::overload_cast<const Dataset &, const Dim>(&integrate),
        py::call_guard<py::gil_scoped_release>());
  m.def("convert",
        py::overload_cast<const Dataset &, const Dim, const Dim>(&convert),
        py::call_guard<py::gil_scoped_release>());
  m.def("convert",
        py::overload_cast<const Dataset &, const std::vector<Dim> &,
                          const Dataset &>(&convert),
        py::call_guard<py::gil_scoped_release>());
        */
}
