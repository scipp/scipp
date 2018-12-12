/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <variant>

#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "dataset.h"
#include "except.h"

namespace py = pybind11;

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

template <class T>
void declare_VariableView(py::module &m, const std::string &suffix) {
  py::class_<VariableView<T>> view(
      m, (std::string("VariableView_") + suffix).c_str());
  view.def("__getitem__", &VariableView<T>::operator[])
      .def("__len__", &VariableView<T>::size)
      .def("__iter__", [](const VariableView<T> &self) {
        return py::make_iterator(self.begin(), self.end());
      });
}

namespace detail {
template <class Tag>
Variable makeVariable(const Tag, const std::vector<Dim> &labels,
                      py::array_t<typename Tag::type> &data) {
  const py::buffer_info info = data.request();
  if (info.ndim != labels.size())
    throw std::runtime_error(
        "Number of dimensions tags does not match shape of data.");
  Dimensions dims;
  for (gsl::index i = labels.size() - 1; i >= 0; --i)
    dims.add(labels[i], info.shape[i]);

  auto *ptr = (typename Tag::type *)info.ptr;
  return makeVariable<const Tag>(dims, ptr, ptr + dims.volume());
}

template <class Tag>
Variable makeVariableDefaultInit(const Tag, const std::vector<Dim> &labels,
                                 const py::tuple &shape) {
  if (shape.size() != labels.size())
    throw std::runtime_error(
        "Number of dimensions tags does not match shape of data.");
  Dimensions dims;
  for (gsl::index i = labels.size() - 1; i >= 0; --i)
    dims.add(labels[i], shape[i].cast<size_t>());
  return makeVariable<const Tag>(dims);
}

std::string format(const Dimensions &dims) {
  std::string out = "Dimensions = " + dataset::to_string(dims);
  return out;
}

template <class Tag>
void insertCoord(Dataset &self, const Tag,
                 const std::tuple<const std::vector<Dim> &,
                                  py::array_t<typename Tag::type> &> &data) {
  const auto &labels = std::get<0>(data);
  const py::buffer_info info = std::get<1>(data).request();
  if (info.ndim != labels.size())
    throw std::runtime_error(
        "Number of dimensions tags does not match shape of data.");
  Dimensions dims;
  for (gsl::index i = labels.size() - 1; i >= 0; --i)
    dims.add(labels[i], info.shape[i]);

  auto *ptr = (typename Tag::type *)info.ptr;
  self.insert<const Tag>(dims, ptr, ptr + dims.volume());
}

template <class Tag>
void insertCoord1D(Dataset &self, const Tag,
                   const std::tuple<const std::vector<Dim> &,
                                    std::vector<typename Tag::type> &> &data) {
  const auto &labels = std::get<0>(data);
  const auto &values = std::get<1>(data);
  if (labels.size() != 1)
    throw std::runtime_error(
        "Number of dimensions tags does not match shape of data.");
  Dimensions dims{labels[0], static_cast<gsl::index>(values.size())};

  self.insert<const Tag>(dims, values);
}

template <class Tag>
void insert(Dataset &self, const std::pair<Tag, const std::string &> &key,
            const std::tuple<const std::vector<Dim> &,
                             py::array_t<typename Tag::type> &> &data) {
  const auto &labels = std::get<0>(data);
  const py::buffer_info info = std::get<1>(data).request();
  if (info.ndim != labels.size())
    throw std::runtime_error(
        "Number of dimensions tags does not match shape of data.");
  Dimensions dims;
  for (gsl::index i = labels.size() - 1; i >= 0; --i)
    dims.add(labels[i], info.shape[i]);

  auto *ptr = (typename Tag::type *)info.ptr;
  const auto &name = std::get<const std::string &>(key);
  self.insert<const Tag>(name, dims, ptr, ptr + dims.volume());
}

template <class Tag, class Var>
void insert(Dataset &self, const std::pair<Tag, const std::string &> &key,
            const Var &var) {
  const auto &tag = std::get<Tag>(key);
  const auto &name = std::get<const std::string &>(key);
  if (self.contains(tag, name))
    if (self[self.find(tag, name)] == var)
      return;
  const auto &data = var.template get<const Tag>();
  self.insert<Tag>(name, var.dimensions(), data.begin(), data.end());
}

template <class Tag>
void insertDefaultInit(
    Dataset &self, const std::pair<Tag, const std::string &> &key,
    const std::tuple<const std::vector<Dim> &, py::tuple> &data) {
  const auto &labels = std::get<0>(data);
  const auto &shape = std::get<1>(data);
  if (shape.size() != labels.size())
    throw std::runtime_error(
        "Number of dimensions tags does not match shape of data.");
  Dimensions dims;
  for (gsl::index i = labels.size() - 1; i >= 0; --i)
    dims.add(labels[i], shape[i].cast<size_t>());

  const auto &name = std::get<const std::string &>(key);
  self.insert<const Tag>(name, dims);
}

// Add size factor.
template <class Tag>
std::vector<gsl::index> numpy_strides(const std::vector<gsl::index> &s) {
  std::vector<gsl::index> strides(s.size());
  gsl::index elemSize = sizeof(typename Tag::type);
  for (gsl::index i = 0; i < strides.size(); ++i) {
    strides[i] = elemSize * s[i];
  }
  return strides;
}

template <class Tag, class T>
void setData(T &self, const std::pair<const Tag, const std::string> &key,
             py::array_t<typename Tag::type> &data) {
  const gsl::index index = find(self, key.first, key.second);
  const auto &dims = self[index].dimensions();
  py::buffer_info info = data.request();
  const auto &shape = dims.shape();
  if (!std::equal(info.shape.begin(), info.shape.end(), shape.begin(),
                  shape.end()))
    throw std::runtime_error(
        "Shape mismatch when setting data from numpy array.");

  auto buf = detail::makeAccess(self)[index].template get<Tag>();
  double *ptr = (double *)info.ptr;
  std::copy(ptr, ptr + dims.volume(), buf.begin());
}
} // namespace detail

VariableSlice pySlice(VariableSlice &view,
                      const std::tuple<Dim, const py::slice> &index) {
  const Dim dim = std::get<Dim>(index);
  const auto indices = std::get<const py::slice>(index);
  size_t start, stop, step, slicelength;
  const auto size = view.dimensions().size(dim);
  if (!indices.compute(size, &start, &stop, &step, &slicelength))
    throw py::error_already_set();
  if (step != 1)
    throw std::runtime_error("Step must be 1");
  return view(dim, start, stop);
}

template <class Tag>
py::buffer_info make_py_buffer_info_t(VariableSlice &view) {
  // Note: Currently this always triggers copy-on-write ---
  // py::buffer_info does currently not support the `readonly` flag of
  // the Python buffer protocol. We can probably get this fixed
  // upstream, see discussion and sample implementation here:
  // https://github.com/pybind/pybind11/issues/863.
  return py::buffer_info(
      view.template get<Tag>().data(), /* Pointer to buffer */
      sizeof(typename Tag::type),      /* Size of one scalar */
      py::format_descriptor<typename Tag::type>::format(), /* Python
                                                 struct-style format
                                                 descriptor */
      view.dimensions().count(), /* Number of dimensions */
      view.dimensions().shape(), /* Buffer dimensions */
      detail::numpy_strides<Tag>(
          view.strides()) /* Strides (in bytes) for each index */
  );
}

py::buffer_info make_py_buffer_info(VariableSlice &view) {
  switch (view.tag().value()) {
  case Coord::X{}.value():
    return make_py_buffer_info_t<Coord::X>(view);
  case Coord::Y{}.value():
    return make_py_buffer_info_t<Coord::Y>(view);
  case Coord::Z{}.value():
    return make_py_buffer_info_t<Coord::Z>(view);
  case Coord::Tof{}.value():
    return make_py_buffer_info_t<Coord::Tof>(view);
  case Coord::Mask{}.value():
    return make_py_buffer_info_t<Coord::Mask>(view);
  case Coord::SpectrumNumber{}.value():
    return make_py_buffer_info_t<Coord::SpectrumNumber>(view);
  case Data::Value{}.value():
    return make_py_buffer_info_t<Data::Value>(view);
  case Data::Variance{}.value():
    return make_py_buffer_info_t<Data::Variance>(view);
  default:
    throw std::runtime_error("non implemented for this type.");
  }
}

template <class Tag>
void setVariableSlice(VariableSlice &self,
                      const std::tuple<Dim, gsl::index> &index,
                      py::array_t<typename Tag::type> &data) {
  auto slice = self(std::get<Dim>(index), std::get<gsl::index>(index));

  const auto &dims = slice.dimensions();
  py::buffer_info info = data.request();
  const auto &shape = dims.shape();
  if (!std::equal(info.shape.begin(), info.shape.end(), shape.begin(),
                  shape.end()))
    throw std::runtime_error(
        "Shape mismatch when setting data from numpy array.");

  auto buf = slice.template get<Tag>();
  double *ptr = (double *)info.ptr;
  std::copy(ptr, ptr + dims.volume(), buf.begin());
}

template <class Tag>
void setVariableSliceRange(VariableSlice &self,
                           const std::tuple<Dim, const py::slice> &index,
                           py::array_t<typename Tag::type> &data) {
  auto slice = pySlice(self, index);

  const auto &dims = slice.dimensions();
  py::buffer_info info = data.request();
  const auto &shape = dims.shape();
  if (!std::equal(info.shape.begin(), info.shape.end(), shape.begin(),
                  shape.end()))
    throw std::runtime_error(
        "Shape mismatch when setting data from numpy array.");

  auto buf = slice.template get<Tag>();
  double *ptr = (double *)info.ptr;
  std::copy(ptr, ptr + dims.volume(), buf.begin());
}

template <class Tag> auto as_py_array_t(py::object &obj, VariableSlice &view) {
  auto array = py::array_t<typename Tag::type>{
      view.dimensions().shape(), detail::numpy_strides<Tag>(view.strides()),
      view.template get<const Tag>().data(), obj};
  // See https://github.com/pybind/pybind11/issues/481.
  reinterpret_cast<py::detail::PyArray_Proxy *>(array.ptr())->flags &=
      ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
  return array;
}

template <class... Ts>
std::variant<py::array_t<Ts>...> as_py_array_t_variant(py::object &obj) {
  auto &view = obj.cast<VariableSlice &>();
  switch (view.tag().value()) {
  case Coord::X{}.value():
    return {as_py_array_t<Coord::X>(obj, view)};
  case Coord::Y{}.value():
    return {as_py_array_t<Coord::Y>(obj, view)};
  case Coord::Z{}.value():
    return {as_py_array_t<Coord::Z>(obj, view)};
  case Coord::Tof{}.value():
    return {as_py_array_t<Coord::Tof>(obj, view)};
  case Coord::Mask{}.value():
    return {as_py_array_t<Coord::Mask>(obj, view)};
  case Coord::SpectrumNumber{}.value():
    return {as_py_array_t<Coord::SpectrumNumber>(obj, view)};
  case Data::Value{}.value():
    return {as_py_array_t<Data::Value>(obj, view)};
  case Data::Variance{}.value():
    return {as_py_array_t<Data::Variance>(obj, view)};
  default:
    throw std::runtime_error("non implemented for this type.");
  }
}

template <class... Ts>
std::variant<VariableView<Ts>...> as_VariableView_variant(VariableSlice &view) {
  switch (view.tag().value()) {
  case Coord::X{}.value():
    return {view.get<Coord::X>()};
  case Coord::Y{}.value():
    return {view.get<Coord::Y>()};
  case Coord::Z{}.value():
    return {view.get<Coord::Z>()};
  case Coord::Tof{}.value():
    return {view.get<Coord::Tof>()};
  case Coord::Mask{}.value():
    return {view.get<Coord::Mask>()};
  case Coord::RowLabel{}.value():
    return {view.get<Coord::RowLabel>()};
  case Coord::SpectrumNumber{}.value():
    return {view.get<Coord::SpectrumNumber>()};
  case Data::Value{}.value():
    return {view.get<Data::Value>()};
  case Data::Variance{}.value():
    return {view.get<Data::Variance>()};
  default:
    throw std::runtime_error("non implemented for this type.");
  }
}

PYBIND11_MODULE(dataset, m) {
  py::enum_<Dimension>(m, "Dim")
      .value("Row", Dim::Row)
      .value("Spectrum", Dim::Spectrum)
      .value("Tof", Dim::Tof)
      .value("X", Dim::X)
      .value("Y", Dim::Y)
      .value("Z", Dim::Z);

  py::class_<Tag>(m, "Tag").def(py::self == py::self);

  auto data_tags = m.def_submodule("Data");
  py::class_<Data::Value, Tag>(data_tags, "_Value");
  py::class_<Data::Variance, Tag>(data_tags, "_Variance");
  data_tags.attr("Value") = Data::Value{};
  data_tags.attr("Variance") = Data::Variance{};

  auto coord_tags = m.def_submodule("Coord");
  py::class_<Coord::Mask, Tag>(coord_tags, "_Mask");
  py::class_<Coord::X, Tag>(coord_tags, "_X");
  py::class_<Coord::Y, Tag>(coord_tags, "_Y");
  py::class_<Coord::Z, Tag>(coord_tags, "_Z");
  py::class_<Coord::Tof, Tag>(coord_tags, "_Tof");
  py::class_<Coord::RowLabel, Tag>(coord_tags, "_RowLabel");
  py::class_<Coord::SpectrumNumber, Tag>(coord_tags, "_SpectrumNumber");
  coord_tags.attr("Mask") = Coord::Mask{};
  coord_tags.attr("X") = Coord::X{};
  coord_tags.attr("Y") = Coord::Y{};
  coord_tags.attr("Z") = Coord::Z{};
  coord_tags.attr("Tof") = Coord::Tof{};
  coord_tags.attr("RowLabel") = Coord::RowLabel{};
  coord_tags.attr("SpectrumNumber") = Coord::SpectrumNumber{};

  declare_span<double>(m, "double");
  declare_span<const double>(m, "double_const");
  declare_span<const std::string>(m, "string_const");
  declare_span<const Dim>(m, "Dim_const");

  declare_VariableView<double>(m, "double");
  declare_VariableView<int32_t>(m, "int32");
  declare_VariableView<std::string>(m, "string");
  declare_VariableView<char>(m, "char");

  py::class_<Dimensions>(m, "Dimensions")
      .def(py::init<>())
      .def("__repr__", &detail::format)
      .def("__len__", &Dimensions::count)
      .def("__contains__", [](const Dimensions &self,
                              const Dim dim) { return self.contains(dim); })
      .def_property_readonly("labels", &Dimensions::labels)
      .def("add", &Dimensions::add)
      .def("size",
           py::overload_cast<const Dimension>(&Dimensions::size, py::const_));

  py::class_<Variable>(m, "Variable")
      .def(py::init(&detail::makeVariable<Coord::Mask>))
      .def(py::init(&detail::makeVariable<Coord::X>))
      .def(py::init(&detail::makeVariable<Coord::Y>))
      .def(py::init(&detail::makeVariable<Coord::Z>))
      .def(py::init(&detail::makeVariable<Data::Value>))
      .def(py::init(&detail::makeVariable<Data::Variance>))
      .def(py::init(&detail::makeVariableDefaultInit<Coord::Mask>))
      .def(py::init(&detail::makeVariableDefaultInit<Coord::X>))
      .def(py::init(&detail::makeVariableDefaultInit<Coord::Y>))
      .def(py::init(&detail::makeVariableDefaultInit<Coord::Z>))
      .def(py::init(&detail::makeVariableDefaultInit<Data::Value>))
      .def(py::init(&detail::makeVariableDefaultInit<Data::Variance>))
      .def(py::init<const VariableSlice &>())
      .def_property_readonly("tag", &Variable::tag)
      .def_property("name", &Variable::name, &Variable::setName)
      .def_property_readonly("is_coord", &Variable::isCoord)
      .def_property_readonly("dimensions", &Variable::dimensions)
      .def(py::self += py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self -= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self *= py::self, py::call_guard<py::gil_scoped_release>());

  py::class_<VariableSlice> view(m, "VariableSlice", py::buffer_protocol());
  view.def_buffer(&make_py_buffer_info);
  view.def_property_readonly(
          "dimensions",
          [](const VariableSlice &self) { return self.dimensions(); },
          py::return_value_policy::copy)
      .def("__len__",
           [](const VariableSlice &self) {
             const auto &dims = self.dimensions();
             if (dims.count() == 0)
               throw std::runtime_error("len() of unsized object.");
             return dims.shape()[0];
           })
      .def_property_readonly("is_coord", &VariableSlice::isCoord)
      .def_property_readonly("tag", &VariableSlice::tag)
      .def_property_readonly("name", &VariableSlice::name)
      .def("__getitem__",
           [](VariableSlice &self, const std::tuple<Dim, gsl::index> &index) {
             return self(std::get<Dim>(index), std::get<gsl::index>(index));
           })
      .def("__getitem__", &pySlice)
      .def("__getitem__",
           [](VariableSlice &self,
              const std::map<Dimension, const gsl::index> d) {
             auto slice(self);
             for (auto item : d)
               slice = slice(item.first, item.second);
             return slice;
           })
      // TODO Make these using py::array instead of py::array_t, then cast based
      // on tag?
      .def("__setitem__", &setVariableSlice<Data::Value>)
      .def("__setitem__", &setVariableSliceRange<Data::Value>)
      .def_property_readonly(
          "numpy", &as_py_array_t_variant<double, int64_t, int32_t, char>)
      .def_property_readonly(
          "data", &as_VariableView_variant<double, int32_t, std::string, char>)
      .def(py::self += py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self -= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self *= py::self, py::call_guard<py::gil_scoped_release>())
      .def("__iadd__", [](VariableSlice &a, Variable &b) { return a += b; },
           py::is_operator())
      .def("__isub__", [](VariableSlice &a, Variable &b) { return a -= b; },
           py::is_operator())
      .def("__imul__", [](VariableSlice &a, Variable &b) { return a *= b; },
           py::is_operator());

  py::class_<DatasetSlice>(m, "DatasetView")
      .def(py::init<Dataset &>())
      .def("__len__", &DatasetSlice::size)
      .def("__iter__",
           [](DatasetSlice &self) {
             return py::make_iterator(detail::makeAccess(self).begin(),
                                      detail::makeAccess(self).end());
           })
      .def("__contains__", &DatasetSlice::contains, py::arg("tag"),
           py::arg("name") = "")
      .def("__contains__",
           [](const DatasetSlice &self,
              const std::tuple<const Tag, const std::string> &key) {
             return self.contains(std::get<0>(key), std::get<1>(key));
           })
      .def("__getitem__",
           [](DatasetSlice &self, const std::tuple<Dim, gsl::index> &index) {
             return self(std::get<Dim>(index), std::get<gsl::index>(index));
           })
      .def("__getitem__",
           [](DatasetSlice &self,
              const std::tuple<Dim, const py::slice> &index) {
             const Dim dim = std::get<Dim>(index);
             const auto indices = std::get<const py::slice>(index);
             size_t start, stop, step, slicelength;
             gsl::index size = -1;
             for (const auto &dimSize : self.dimensions())
               if (std::get<Dim>(dimSize) == dim)
                 size = std::get<gsl::index>(dimSize);
             if (size == -1)
               throw std::runtime_error(
                   "Dataset does not contain this dimension.");
             if (!indices.compute(size, &start, &stop, &step, &slicelength))
               throw py::error_already_set();
             if (step != 1)
               throw std::runtime_error("Step must be 1");
             return self(dim, start, stop);
           })
      .def("__getitem__",
           [](DatasetSlice &self, const Tag &tag) { return self(tag); })
      .def(
          "__getitem__",
          [](DatasetSlice &self, const std::pair<Tag, const std::string> &key) {
            return self(key.first, key.second);
          })
      .def("__setitem__", detail::setData<Data::Value, DatasetSlice>)
      .def("__setitem__", detail::setData<Data::Variance, DatasetSlice>)
      .def(py::self += py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self -= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self *= py::self, py::call_guard<py::gil_scoped_release>());

  py::class_<Dataset>(m, "Dataset")
      .def(py::init<>())
      .def(py::init<const DatasetSlice &>())
      .def("__len__", &Dataset::size)
      .def("__iter__",
           [](const Dataset &self) {
             return py::make_iterator(self.begin(), self.end());
           })
      .def("__contains__", &Dataset::contains, py::arg("tag"),
           py::arg("name") = "")
      .def("__contains__",
           [](const Dataset &self,
              const std::tuple<const Tag, const std::string> &key) {
             return self.contains(std::get<0>(key), std::get<1>(key));
           })
      .def("__delitem__",
           py::overload_cast<const Tag, const std::string &>(&Dataset::erase),
           py::arg("tag"), py::arg("name") = "")
      .def("__delitem__",
           [](Dataset &self,
              const std::tuple<const Tag, const std::string &> &key) {
             self.erase(std::get<0>(key), std::get<1>(key));
           })
      .def("__getitem__",
           [](Dataset &self, const std::tuple<Dim, gsl::index> &index) {
             return self(std::get<Dim>(index), std::get<gsl::index>(index));
           })
      .def("__getitem__",
           [](Dataset &self, const std::tuple<Dim, const py::slice> &index) {
             const Dim dim = std::get<Dim>(index);
             const auto indices = std::get<const py::slice>(index);
             size_t start, stop, step, slicelength;
             const auto size = self.dimensions().size(dim);
             if (!indices.compute(size, &start, &stop, &step, &slicelength))
               throw py::error_already_set();
             if (step != 1)
               throw std::runtime_error("Step must be 1");
             return self(dim, start, stop);
           })
      .def("__getitem__",
           [](Dataset &self, const Tag &tag) { return self(tag); })
      .def("__getitem__",
           [](Dataset &self, const std::pair<Tag, const std::string> &key) {
             return self(key.first, key.second);
           })
      .def("__getitem__",
           [](Dataset &self, const std::string &name) { return self[name]; })
      // Careful: The order of overloads is really important here, otherwise
      // DatasetSlice matches the overload below for py::array_t. I have not
      // understood all details of this yet though. See also
      // https://pybind11.readthedocs.io/en/stable/advanced/functions.html#overload-resolution-order.
      .def("__setitem__",
           [](Dataset &self, const std::tuple<Dim, gsl::index> &index,
              const DatasetSlice &other) {
             auto slice =
                 self(std::get<Dim>(index), std::get<gsl::index>(index));
             if (slice == other)
               return;
             throw std::runtime_error("Non-self-assignment of Dataset slices "
                                      "is not implemented yet.\n");
           })
      .def("__setitem__", detail::setData<Data::Value, Dataset>)
      .def("__setitem__", detail::setData<Data::Variance, Dataset>)
      .def("__setitem__", detail::insertCoord<Coord::X>)
      .def("__setitem__", detail::insertCoord<Coord::Y>)
      .def("__setitem__", detail::insertCoord<Coord::Z>)
      .def("__setitem__", detail::insertCoord<Coord::Tof>)
      .def("__setitem__", detail::insertCoord<Coord::Mask>)
      .def("__setitem__", detail::insertCoord1D<Coord::RowLabel>)
      .def("__setitem__", detail::insertCoord<Coord::SpectrumNumber>)
      .def("__setitem__", detail::insert<Data::Value>)
      .def("__setitem__", detail::insert<Data::Variance>)
      .def("__setitem__", detail::insert<Data::Value, Variable>)
      .def("__setitem__", detail::insert<Data::Variance, Variable>)
      .def("__setitem__", detail::insert<Data::Value, VariableSlice>)
      .def("__setitem__", detail::insert<Data::Variance, VariableSlice>)
      .def("__setitem__", detail::insertDefaultInit<Data::Value>)
      .def("__setitem__", detail::insertDefaultInit<Data::Variance>)
      // Note: As it is this will always implicitly convert a RHS view into a
      // Dataset, i.e., makes a copy. Need to expose the operator overloads for
      // views as well.
      .def(py::self == py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self += py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self -= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self *= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self + py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self - py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self * py::self, py::call_guard<py::gil_scoped_release>())
      .def("dimensions", [](const Dataset &self) { return self.dimensions(); })
      .def(
          "slice",
          py::overload_cast<const Dataset &, const Dimension, const gsl::index>(
              &slice),
          py::call_guard<py::gil_scoped_release>());

  py::implicitly_convertible<DatasetSlice, Dataset>();

  m.def("split",
        py::overload_cast<const Dataset &, const Dimension,
                          const std::vector<gsl::index> &>(&split),
        py::call_guard<py::gil_scoped_release>());
  m.def("concatenate",
        py::overload_cast<const Dataset &, const Dataset &, const Dimension>(
            &concatenate),
        py::call_guard<py::gil_scoped_release>());
  m.def("rebin", py::overload_cast<const Dataset &, const Variable &>(&rebin),
        py::call_guard<py::gil_scoped_release>());
  m.def(
      "sort",
      py::overload_cast<const Dataset &, const Tag, const std::string &>(&sort),
      py::arg("dataset"), py::arg("tag"), py::arg("name") = "",
      py::call_guard<py::gil_scoped_release>());
  m.def("filter", py::overload_cast<const Dataset &, const Variable &>(&filter),
        py::call_guard<py::gil_scoped_release>());
  m.def("sum", py::overload_cast<const Dataset &, const Dim>(&sum),
        py::call_guard<py::gil_scoped_release>());
  m.def("mean", py::overload_cast<const Dataset &, const Dim>(&mean),
        py::call_guard<py::gil_scoped_release>());
}
