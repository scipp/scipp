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
#include "except.h"

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

std::vector<gsl::index> numpy_shape(const Dimensions &dims) {
  std::vector<gsl::index> shape(dims.count());
  for (gsl::index i = 0; i < shape.size(); ++i)
    shape[i] = dims.size(shape.size() - 1 - i);
  return shape;
}

template <class Tag>
std::vector<gsl::index> numpy_strides(const Dimensions &dims) {
  std::vector<gsl::index> strides(dims.count());
  gsl::index stride = sizeof(typename Tag::type);
  for (gsl::index i = strides.size() - 1; i >= 0; --i) {
    strides[i] = stride;
    stride *= dims.size(strides.size() - 1 - i);
  }
  return strides;
}

template <class Tag> class VariableView {
public:
  // Note: Not const since this is created from a Python object.
  VariableView(Variable &variable)
      : m_variable(&variable), m_start(0),
        m_shape(numpy_shape(variable.dimensions())),
        m_strides(numpy_strides<Tag>(variable.dimensions())),
        m_dimensions(variable.dimensions()) {}

  const Dimensions &dimensions() const { return m_dimensions; }

  VariableView slice(const Dimension dim, const gsl::index index) const {
    const auto dimIndex = m_strides.size() - 1 - dimensions().index(dim);

    VariableView sliced(*this);
    sliced.m_start += index * m_strides[dimIndex] / sizeof(typename Tag::type);
    // Eliminate dimension.
    sliced.m_shape.erase(sliced.m_shape.begin() + dimIndex);
    sliced.m_strides.erase(sliced.m_strides.begin() + dimIndex);
    sliced.m_dimensions.erase(dim);
    return sliced;
  }

  VariableView slice(const Dimension dim, const py::slice indices) const {
    const auto dimIndex = m_strides.size() - 1 - dimensions().index(dim);

    size_t start, stop, step, slicelength;
    if (!indices.compute(m_shape[dimIndex], &start, &stop, &step, &slicelength))
      throw py::error_already_set();
    if (step != 1)
      throw std::runtime_error("Step must be 1");

    VariableView sliced(*this);
    sliced.m_start += start * m_strides[dimIndex] / sizeof(typename Tag::type);
    sliced.m_shape[dimIndex] = slicelength;
    sliced.m_dimensions.resize(dim, slicelength);
    return sliced;
  }

  Variable *m_variable;
  gsl::index m_start;
  std::vector<gsl::index> m_shape;
  std::vector<gsl::index> m_strides;

private:
  Dimensions m_dimensions;
};

template <class Tag> VariableView<Tag> getCoord(Dataset &self, const Tag) {
  return {self.get(self.find(tag_id<Tag>, ""))};
}

template <class Tag>
VariableView<Tag> getData(Dataset &self,
                          const std::pair<const Tag, const std::string> &key) {
  return {self.get(self.find(tag_id<Tag>, key.second))};
}

std::string format(const Dimensions &dims) {
  // TODO reverse order to match numpy convention.
  std::string out = "Dimensions = " + dataset::to_string(dims);
  return out;
}
} // namespace detail

template <class Tag>
void declare_VariableView(py::module &m, const std::string &suffix) {
  py::class_<detail::VariableView<Tag>>(
      m, (std::string("VariableView_") + suffix).c_str())
      // Careful: Do not expose setName, setDimensions, and assignment,
      // otherwise we can break the dataset.
      .def_property_readonly("dimensions",
                             &detail::VariableView<Tag>::dimensions)
      .def_property_readonly("is_coord",
                             [](const detail::VariableView<Tag> &self) {
                               return self.m_variable->isCoord();
                             })
      .def_property_readonly("type",
                             [](const detail::VariableView<Tag> &self) {
                               return self.m_variable->type();
                             })
      .def_property_readonly("name",
                             [](const detail::VariableView<Tag> &self) {
                               return self.m_variable->name();
                             })
      .def("__getitem__",
           [](const detail::VariableView<Tag> &self,
              const std::map<Dimension, const gsl::index> d) {
             auto slice(self);
             for (auto item : d)
               slice = slice.slice(item.first, item.second);
             return slice;
           })
      .def("__getitem__",
           [](const detail::VariableView<Tag> &self,
              const std::map<Dimension, const py::slice> d) {
             auto slice(self);
             for (auto item : d)
               slice = slice.slice(item.first, item.second);
             return slice;
           })
      .def_property(
          "numpy",
          [](py::object &obj) {
            auto &view = obj.cast<detail::VariableView<Tag> &>();
            auto array = py::array_t<typename Tag::type>{
                view.m_shape, view.m_strides,
                view.m_variable->template get<const Tag>().data() +
                    view.m_start,
                obj};
            // See https://github.com/pybind/pybind11/issues/481.
            reinterpret_cast<py::detail::PyArray_Proxy *>(array.ptr())->flags &=
                ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
            return array;
          },
          [](py::object &obj, const py::array_t<typename Tag::type> &data) {
            auto &view = obj.cast<detail::VariableView<Tag> &>();
            py::array_t<typename Tag::type>{
                view.m_shape, view.m_strides,
                view.m_variable->template get<Tag>().data() + view.m_start,
                obj} = data;
          })
      .def_property(
          "numpy_mutable",
          [](py::object &obj) {
            auto &view = obj.cast<detail::VariableView<Tag> &>();
            return py::array_t<typename Tag::type>{
                view.m_shape, view.m_strides,
                view.m_variable->template get<Tag>().data() + view.m_start,
                obj};
          },
          [](py::object &obj, const py::array_t<typename Tag::type> &data) {
            auto &view = obj.cast<detail::VariableView<Tag> &>();
            py::array_t<typename Tag::type>{
                view.m_shape, view.m_strides,
                view.m_variable->template get<Tag>().data() + view.m_start,
                obj} = data;
          });
}

PYBIND11_MODULE(dataset, m) {
  py::enum_<Dimension>(m, "Dim")
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
      .def_property_readonly("labels", &Dimensions::labels)
      .def("add", &Dimensions::add)
      .def("size",
           py::overload_cast<const Dimension>(&Dimensions::size, py::const_));

  declare_VariableView<Data::Value>(m, "DataValue");
  declare_VariableView<Coord::X>(m, "CoordX");
  declare_VariableView<Coord::Y>(m, "CoordY");
  declare_VariableView<Coord::Z>(m, "CoordZ");

  py::class_<Variable>(m, "Variable");

  py::class_<Dataset>(m, "Dataset")
      .def(py::init<>())
      .def("__getitem__", detail::getCoord<Coord::X>)
      .def("__getitem__", detail::getCoord<Coord::Y>)
      .def("__getitem__", detail::getCoord<Coord::Z>)
      .def("__getitem__", detail::getData<Data::Value>)
      .def("serialize",
           [](const Dataset &self) {
             py::list vars;
             for (const auto &var : self) {
               detail::VariableHeader header;
               const auto &dimensions = var.dimensions();
               header.nDim = dimensions.count();
               throw std::runtime_error(
                   "TODO: Update implementation for new API.");
               // std::copy(dimensions.shape().begin(),
               // dimensions.shape().end(),
               //           header.dimensions.begin());
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
            const auto &dims = d.dimensions<const Data::Value>();
            auto shape = detail::numpy_shape(dims);
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
             const auto &dims = d.dimensions<const Data::Value>();
             auto shape = detail::numpy_shape(dims);
             return py::array_t<double>(shape, {},
                                        d.get<const Data::Value>().data(), obj);
           })
      .def("insertDataValue",
           py::overload_cast<const std::string &, const Dimensions &,
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
        py::overload_cast<const Dataset &, const Dataset &, const Dimension>(
            &concatenate),
        py::call_guard<py::gil_scoped_release>());
}
