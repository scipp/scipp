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

namespace detail {
struct VariableHeader {
  gsl::index nDim;
  std::array<std::pair<Dimension, gsl::index>, 4> dimensions;
  uint16_t type;
  Unit::Id unit;
  gsl::index dataSize;
};

template <class Tag>
void insertCoord(Dataset &self, const Tag, const Dimensions &dims,
                 const std::vector<double> &data) {
  self.insert<const Tag>(dims, data);
}

template <class Tag>
void insert(Dataset &self, const Tag, const std::string &name,
            const Dimensions &dims, const std::vector<double> &data) {
  self.insert<const Tag>(name, dims, data);
}

template <class Tag>
auto get(const Dataset &self, const Tag, const std::string &name)
    -> decltype(self.get<const Tag>(name)) {
  return self.get<const Tag>(name);
}
} // namespace detail

PYBIND11_MODULE(dataset, m) {
  py::enum_<Dimension>(m, "Dimension")
      .value("X", Dimension::X)
      .value("Y", Dimension::Y)
      .value("Z", Dimension::Z);

  auto data_tags = m.def_submodule("Data");
  py::class_<Data::Value>(data_tags, "_Value");
  py::class_<Data::Variance>(data_tags, "_Variance");
  py::class_<Data::String>(data_tags, "_String");
  data_tags.attr("Value") = Data::Value{};
  data_tags.attr("Variance") = Data::Variance{};
  data_tags.attr("String") = Data::String{};

  auto coord_tags = m.def_submodule("Coord");
  py::class_<Coord::X>(coord_tags, "_X");
  py::class_<Coord::Y>(coord_tags, "_Y");
  py::class_<Coord::Z>(coord_tags, "_Z");
  coord_tags.attr("X") = Coord::X{};
  coord_tags.attr("Y") = Coord::Y{};
  coord_tags.attr("Z") = Coord::Z{};

  declare_span<double>(m, "double");
  declare_span<const double>(m, "double_const");
  declare_span<const std::string>(m, "string_const");

  py::class_<Dimensions>(m, "Dimensions")
      .def(py::init<>())
      .def("add", &Dimensions::add)
      .def("size",
           py::overload_cast<const Dimension>(&Dimensions::size, py::const_));

  py::class_<Dataset>(m, "Dataset")
      .def(py::init<>())
      .def("serialize",
           [](const Dataset &self) {
             py::list vars;
             for (const auto &var : self) {
               detail::VariableHeader header;
               const auto &dimensions = var.dimensions();
               header.nDim = dimensions.count();
               std::copy(dimensions.begin(), dimensions.end(),
                         header.dimensions.begin());
               header.type = var.type();
               header.unit = var.unit().id();

               gsl::span<const double> data;
               if (var.valueTypeIs<Data::Value>()) {
                 data = var.get<const Data::Value>();
               }
               if (var.valueTypeIs<Coord::X>()) {
                 data = var.get<const Coord::X>();
               }
               if (var.valueTypeIs<Coord::Y>()) {
                 data = var.get<const Coord::Y>();
               }
               if (var.valueTypeIs<Coord::Z>()) {
                 data = var.get<const Coord::Z>();
               }
               header.dataSize = data.size() * sizeof(double);
               gsl::index size =
                   sizeof(detail::VariableHeader) + header.dataSize;
               auto buffer = std::make_unique<char[]>(size);
               memcpy(buffer.get(), &header, sizeof(detail::VariableHeader));
               memcpy(buffer.get() + sizeof(detail::VariableHeader),
                      data.data(), header.dataSize);
               vars.append(py::bytes(buffer.get(), size));
             }
             return vars;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("deserialize",
           [](Dataset &self, py::list vars) {
             for (const auto &var : vars) {
               const auto buffer = var.cast<py::bytes>().cast<std::string>();
               detail::VariableHeader header;
               memcpy(&header, buffer.data(), sizeof(detail::VariableHeader));

               Dimensions dimensions;
               for (gsl::index dim = 0; dim < header.nDim; ++dim)
                 dimensions.add(header.dimensions[dim].first,
                                header.dimensions[dim].second);
               Vector<double> data(header.dataSize / sizeof(double));
               memcpy(data.data(),
                      buffer.data() + sizeof(detail::VariableHeader),
                      header.dataSize);
               switch (header.type) {
               case tag_id<Data::Value>:
                 self.insert<Data::Value>("", dimensions, std::move(data));
                 break;
               case tag_id<Coord::X>:
                 self.insert<Coord::X>(dimensions, std::move(data));
                 break;
               case tag_id<Coord::Y>:
                 self.insert<Coord::Y>(dimensions, std::move(data));
                 break;
               case tag_id<Coord::Z>:
                 self.insert<Coord::Z>(dimensions, std::move(data));
                 break;
               }
             }
           },
           py::call_guard<py::gil_scoped_release>())
      .def("insert", detail::insertCoord<Coord::X>)
      .def("insert", detail::insertCoord<Coord::Y>)
      .def("insert", detail::insertCoord<Coord::Z>)
      .def("insert", detail::insert<Data::Value>)
      .def("insert", detail::insert<Data::Variance>)
      .def("get", detail::get<Data::Value>)
      .def("get", detail::get<Data::Variance>)
      .def("get", detail::get<Data::String>)
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
