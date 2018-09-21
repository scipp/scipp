/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "dataset.h"

namespace py = pybind11;

void setItem(gsl::span<double> &self, gsl::index i, double value) {
  self[i] = value;
}

template <class T> struct mutable_span_methods {
  static void add(py::class_<gsl::span<T>> &span) {
    span.def("__setitem__", [](gsl::span<T> &self, const gsl::index i,
                               const T value) { self[i] = value; });
  }
};
template <class T> struct mutable_span_methods<const T> {
  static void add(py::class_<gsl::span<const T>> &span) {}
};

template <class T> void declare_span(py::module &m, const std::string &suffix) {
  py::class_<gsl::span<T>> span(m, (std::string("span_") + suffix).c_str());
  span.def("__getitem__", &gsl::span<T>::operator[])
      .def("size", &gsl::span<T>::size)
      .def("__len__", &gsl::span<T>::size)
      .def("__iter__", [](const gsl::span<T> &self) {
        return py::make_iterator(self.begin(), self.end());
      });
  mutable_span_methods<T>::add(span);
}

PYBIND11_MODULE(dataset, m) {
  py::enum_<Dimension>(m, "Dimension")
      .value("X", Dimension::X)
      .value("Y", Dimension::Y)
      .value("Z", Dimension::Z);

  declare_span<double>(m, "double");
  declare_span<const double>(m, "double_const");

  py::class_<Coord> coord(m, "Coord");
  py::class_<Coord::X>(coord, "X");
  py::class_<Coord::Y>(coord, "Y");
  py::class_<Coord::Z>(coord, "Z");

  py::class_<Dimensions>(m, "Dimensions")
      .def(py::init<>())
      .def("add", &Dimensions::add)
      .def("size",
           py::overload_cast<const Dimension>(&Dimensions::size, py::const_));

  py::class_<Dataset>(m, "Dataset")
      .def(py::init<>())
      .def("__getstate__",
           [](const Dataset &self) {
             py::list vars;
             for (const auto &var : self) {
               const auto &dimensions = var.dimensions();
               py::list dims;
               for (const auto &dim : dimensions)
                 dims.append(py::make_tuple(static_cast<int32_t>(dim.first),
                                            dim.second));
               auto data = var.get<const Data::Value>();
               std::vector<double> buffer(data.begin(), data.end());
               vars.append(py::make_tuple(dims, var.type(),
                                          static_cast<int32_t>(var.unit().id()),
                                          var.name(), buffer));
             }
             return vars;
           })
      .def("__setstate__",
           [](Dataset &self, py::list vars) {
             new (&self) Dataset();
             for (const auto &var : vars) {
               const auto tuple = var.cast<py::tuple>();
               const auto dims = tuple[0].cast<py::list>();
               Dimensions dimensions;
               for (const auto &dim : dims) {
                 const auto d = dim.cast<py::tuple>();
                 dimensions.add(static_cast<Dimension>(d[0].cast<int32_t>()),
                                d[1].cast<gsl::index>());
               }
               const auto type = tuple[1].cast<uint16_t>();
               const auto unit = tuple[2].cast<int32_t>();
               const auto name = tuple[3].cast<std::string>();
               const auto data = tuple[4].cast<std::vector<double>>();
               self.insert<Data::Value>(name, dimensions, data);
             }
           })
      .def("insertCoordX",
           py::overload_cast<Dimensions, const std::vector<double> &>(
               &Dataset::insert<Coord::X, const std::vector<double> &>))
      .def("insertCoordY",
           py::overload_cast<Dimensions, const std::vector<double> &>(
               &Dataset::insert<Coord::Y, const std::vector<double> &>))
      .def("insertCoordZ",
           py::overload_cast<Dimensions, const std::vector<double> &>(
               &Dataset::insert<Coord::Z, const std::vector<double> &>))
      .def("insertDataValue",
           py::overload_cast<const std::string &, Dimensions,
                             const std::vector<double> &>(
               &Dataset::insert<Data::Value, const std::vector<double> &>))
      .def("getDataValueConst", (gsl::span<const double>(Dataset::*)())(
                                    &Dataset::get<const Data::Value>))
      .def("getDataValue",
           [](Dataset &self) { return self.get<Data::Value>(); })
      .def(py::self == py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self += py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self + py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self - py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self * py::self, py::call_guard<py::gil_scoped_release>())
      .def("dimensions", [](const Dataset &self) { return self.dimensions(); })
      .def(
          "slice",
          py::overload_cast<const Dataset &, const Dimension, const gsl::index>(
              &slice),
          py::call_guard<py::gil_scoped_release>())
      .def("size", &Dataset::size);
  m.def("concatenate",
        py::overload_cast<const Dimension, const Dataset &, const Dataset &>(
            &concatenate),
        py::call_guard<py::gil_scoped_release>());
}
