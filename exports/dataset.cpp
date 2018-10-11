/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <pybind11/numpy.h>
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

template <class Tag> class TypedVariable {
  using value_type = typename Tag::type;

public:
  TypedVariable(Variable &variable)
      : variable(variable), data(variable.get<const Tag>()) {}

  const Dimensions &dimensions() const { return variable.dimensions(); }
  const std::string &name() const { return variable.name(); }
  gsl::index size() const { return data.size(); }
  void setItem(const gsl::index i, const value_type value) {
    // Note that this is not thread-safe but it should not matter for Python
    // exports? Make sure to not release GIL.
    if (!m_mutableData) {
      m_mutableData = variable.get<Tag>().data();
      data = variable.get<const Tag>();
    }
    m_mutableData[i] = value;
  }

  gsl::span<const value_type> data;

  Variable &variable;

private:
  value_type *m_mutableData{nullptr};
};

template <class Tag> TypedVariable<Tag> getCoord(Dataset &self, const Tag) {
  return {self[self.find(tag_id<Tag>, "")]};
}

template <class Tag>
TypedVariable<Tag> getData(Dataset &self,
                           const std::pair<const Tag, const std::string> &key) {
  return {self[self.find(tag_id<Tag>, key.second)]};
}

std::string formatDim(const Dimension dim) {
  switch (dim) {
  case Dimension::X:
    return "Dim.X";
  case Dimension::Y:
    return "Dim.Y";
  case Dimension::Z:
    return "Dim.Z";
  default:
    return "<unknown-dimension-tag>";
  }
}

std::string format(const Dimensions &dims) {
  std::string out = "Dimensions = {";
  // TODO reverse order to match numpy convention.
  for (const auto &item : dims)
    out += formatDim(item.first) + ": " + std::to_string(item.second) + ", ";
  out.resize(out.size() - 2);
  out += '}';
  return out;
}
} // namespace detail

template <class Tag>
void declare_typed_variable(py::module &m, const std::string &suffix) {
  py::class_<detail::TypedVariable<Tag>>(
      m, (std::string("Variable_") + suffix).c_str())
      // This is a linear index. If we decide to keep this, we should probably
      // not support slicing notation. If we change to using multi-dimensional
      // indexing like numpy we can also support slicing.
      .def("__len__", &detail::TypedVariable<Tag>::size)
      // TODO Also support indexing with dimensions names, as in
      // http://xarray.pydata.org/en/stable/indexing.html.
      .def("__getitem__", [](const detail::TypedVariable<Tag> &self,
                             const gsl::index i) { return self.data[i]; })
      .def("__getitem__",
           [](const detail::TypedVariable<Tag> &self,
              const std::map<Dimension, gsl::index> d) {
             if (d.size() != 1)
               throw std::runtime_error(
                   "Currently only 1D slicing is supported.");
             // TODO It is a bit weird that this returns Variable instead of
             // TypedVariable, but we cannot since the latter only holds a
             // reference... the resulting Variable cannot be modified in Python
             // right now, would need an extra cast step.
             // Would this be solved by returning the slice as a view?
             // This highlights a more general problem: The mechanism of
             // returning a typed variable in Dataset.__getitem__ is nice, but
             // we cannot provide the same interface as for a stand-alone
             // Variable?
             // => Always return a Variable in __getitem__, even if 0-dim.
             // => Use explicit syntax for accessing individual items.
             // => *Must* return a view on slicing to be consistent with numpy
             //    and xarray.
             // numpy does some magic to copy data into a view when the parent
             // is deleted! Do they just share the data ownership? How can we
             // combine this with our COW? Have two distinct reference counters,
             // one for sharing views, and one for distinct owners?
             return slice(self.variable, d.begin()->first, d.begin()->second);

           })
      .def("__setitem__", &detail::TypedVariable<Tag>::setItem)
      .def_property_readonly("dimensions",
                             &detail::TypedVariable<Tag>::dimensions)
      .def_property_readonly("name", &detail::TypedVariable<Tag>::name);
}

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
      .def("__repr__", &detail::format)
      .def("add", &Dimensions::add)
      .def("size",
           py::overload_cast<const Dimension>(&Dimensions::size, py::const_));

  declare_typed_variable<Data::Value>(m, "DataValue");
  declare_typed_variable<Coord::X>(m, "CoordX");

  py::class_<Variable>(m, "Variable");

  py::class_<Dataset>(m, "Dataset")
      .def(py::init<>())
      .def("__getitem__", detail::getCoord<Coord::X>)
      .def("__getitem__", detail::getData<Data::Value>)
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
      .def(
          "getNumpy",
          [](py::object &obj) {
            const auto &d = obj.cast<const Dataset &>();
            auto dims = d.dimensions<const Data::Value>();
            std::vector<int64_t> shape(dims.count());
            for (gsl::index i = 0; i < shape.size(); ++i)
              shape[i] = dims.size(shape.size() - 1 - i);
            auto array = py::array_t<double>(
                shape, {}, d.get<const Data::Value>().data(), obj);
            // See https://github.com/pybind/pybind11/issues/481.
            reinterpret_cast<py::detail::PyArray_Proxy *>(array.ptr())->flags &=
                ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
            return array;
          })
      .def("getNumpyMutable",
           [](py::object &obj) {
             const auto &d = obj.cast<const Dataset &>();
             auto dims = d.dimensions<const Data::Value>();
             std::vector<int64_t> shape(dims.count());
             for (gsl::index i = 0; i < shape.size(); ++i)
               shape[i] = dims.size(shape.size() - 1 - i);
             return py::array_t<double>(shape, {},
                                        d.get<const Data::Value>().data(), obj);
           })
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
