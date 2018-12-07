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
struct PythonData {
  struct Value : public Tag {
    using type = Data::Value;
    Value() : Tag(::tag<type>) {}
  };
  struct Variance : public Tag {
    using type = Data::Variance;
    Variance() : Tag(::tag<type>) {}
  };
};
struct PythonCoord {
  struct Tof : public Tag {
    using type = Coord::Tof;
    Tof() : Tag(::tag<type>) {}
  };
  struct Mask : public Tag {
    using type = Coord::Mask;
    Mask() : Tag(::tag<type>) {}
  };
  struct X : public Tag {
    using type = Coord::X;
    X() : Tag(::tag<type>) {}
  };
  struct Y : public Tag {
    using type = Coord::Y;
    Y() : Tag(::tag<type>) {}
  };
  struct Z : public Tag {
    using type = Coord::Z;
    Z() : Tag(::tag<type>) {}
  };
  struct RowLabel : public Tag {
    using type = Coord::RowLabel;
    RowLabel() : Tag(::tag<type>) {}
  };
  struct SpectrumNumber : public Tag {
    using type = Coord::SpectrumNumber;
    SpectrumNumber() : Tag(::tag<type>) {}
  };
};

template <class Tag>
Variable makeVariable(const Tag, const std::vector<Dim> &labels,
                      py::array_t<typename Tag::type::type> &data) {
  const py::buffer_info info = data.request();
  if (info.ndim != labels.size())
    throw std::runtime_error(
        "Number of dimensions tags does not match shape of data.");
  Dimensions dims;
  for (gsl::index i = labels.size() - 1; i >= 0; --i)
    dims.add(labels[i], info.shape[i]);

  auto *ptr = (typename Tag::type::type *)info.ptr;
  return makeVariable<const typename Tag::type>(dims, ptr, ptr + dims.volume());
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
  return makeVariable<const typename Tag::type>(dims);
}

std::string format(const Dimensions &dims) {
  std::string out = "Dimensions = " + dataset::to_string(dims);
  return out;
}

template <class Tag, class T> bool containsUnnamed(const T &self, const Tag) {
  return self.contains(tag<Tag>);
}

template <class Tag, class T>
bool contains(const T &self,
              const std::tuple<const Tag, const std::string &> &key) {
  return self.contains(tag<Tag>, std::get<const std::string &>(key));
}

template <class Tag> void eraseUnnamed(Dataset &self, const Tag) {
  self.erase(tag<Tag>);
}

template <class Tag>
void erase(Dataset &self,
           const std::tuple<const Tag, const std::string &> &key) {
  self.erase(tag<Tag>, std::get<const std::string &>(key));
}

template <class Tag>
void insertCoord(
    Dataset &self, const Tag,
    const std::tuple<const std::vector<Dim> &,
                     py::array_t<typename Tag::type::type> &> &data) {
  const auto &labels = std::get<0>(data);
  const py::buffer_info info = std::get<1>(data).request();
  if (info.ndim != labels.size())
    throw std::runtime_error(
        "Number of dimensions tags does not match shape of data.");
  Dimensions dims;
  for (gsl::index i = labels.size() - 1; i >= 0; --i)
    dims.add(labels[i], info.shape[i]);

  auto *ptr = (typename Tag::type::type *)info.ptr;
  self.insert<const typename Tag::type>(dims, ptr, ptr + dims.volume());
}

template <class Tag>
void insertCoord1D(
    Dataset &self, const Tag,
    const std::tuple<const std::vector<Dim> &,
                     std::vector<typename Tag::type::type> &> &data) {
  const auto &labels = std::get<0>(data);
  const auto &values = std::get<1>(data);
  if (labels.size() != 1)
    throw std::runtime_error(
        "Number of dimensions tags does not match shape of data.");
  Dimensions dims{labels[0], static_cast<gsl::index>(values.size())};

  self.insert<const typename Tag::type>(dims, values);
}

template <class Tag>
void insert(Dataset &self, const std::pair<Tag, const std::string &> &key,
            const std::tuple<const std::vector<Dim> &,
                             py::array_t<typename Tag::type::type> &> &data) {
  const auto &labels = std::get<0>(data);
  const py::buffer_info info = std::get<1>(data).request();
  if (info.ndim != labels.size())
    throw std::runtime_error(
        "Number of dimensions tags does not match shape of data.");
  Dimensions dims;
  for (gsl::index i = labels.size() - 1; i >= 0; --i)
    dims.add(labels[i], info.shape[i]);

  auto *ptr = (typename Tag::type::type *)info.ptr;
  const auto &name = std::get<const std::string &>(key);
  self.insert<const typename Tag::type>(name, dims, ptr, ptr + dims.volume());
}

template <class Tag, class Var>
void insert(Dataset &self, const std::pair<Tag, const std::string &> &key,
            const Var &var) {
  const auto &tag = std::get<Tag>(key);
  const auto &name = std::get<const std::string &>(key);
  if (self.contains(tag, name))
    if (self[self.find(tag.value(), name)] == var)
      return;
  const auto &data = var.template get<const typename Tag::type>();
  self.insert<typename Tag::type>(name, var.dimensions(), data.begin(),
                                  data.end());
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
  self.insert<const typename Tag::type>(name, dims);
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

// The way the Python exports are written we require non-const references to
// variables. Note that setting breaking attributes is not exported, so we
// should be safe.
template <class Tag, class T>
VariableSlice<Variable> getCoord(T &self, const Tag) {
  return VariableSlice<Variable>(
      detail::makeAccess(self)[find(self, tag_id<typename Tag::type>, "")]);
}

template <class Tag, class T>
VariableSlice<Variable>
getData(T &self, const std::pair<const Tag, const std::string> &key) {
  return VariableSlice<Variable>(detail::makeAccess(
      self)[find(self, tag_id<typename Tag::type>, key.second)]);
}

template <class Tag, class T>
void setData(T &self, const std::pair<const Tag, const std::string> &key,
             py::array_t<typename Tag::type::type> &data) {
  const gsl::index index = find(self, tag_id<typename Tag::type>, key.second);
  const auto &dims = self[index].dimensions();
  py::buffer_info info = data.request();
  const auto &shape = dims.shape();
  if (!std::equal(info.shape.begin(), info.shape.end(), shape.begin(),
                  shape.end()))
    throw std::runtime_error(
        "Shape mismatch when setting data from numpy array.");

  auto buf = detail::makeAccess(self)[index].template get<typename Tag::type>();
  double *ptr = (double *)info.ptr;
  std::copy(ptr, ptr + dims.volume(), buf.begin());
}
} // namespace detail

VariableSlice<Variable> pySlice(VariableSlice<Variable> &view,
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
py::buffer_info make_py_buffer_info_t(VariableSlice<Variable> &view) {
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

py::buffer_info make_py_buffer_info(VariableSlice<Variable> &view) {
  switch (view.type()) {
  case tag<Coord::X>.value():
    return make_py_buffer_info_t<Coord::X>(view);
  case tag<Coord::Y>.value():
    return make_py_buffer_info_t<Coord::Y>(view);
  case tag<Coord::Z>.value():
    return make_py_buffer_info_t<Coord::Z>(view);
  case tag<Coord::Tof>.value():
    return make_py_buffer_info_t<Coord::Tof>(view);
  case tag<Coord::Mask>.value():
    return make_py_buffer_info_t<Coord::Mask>(view);
  case tag<Coord::SpectrumNumber>.value():
    return make_py_buffer_info_t<Coord::SpectrumNumber>(view);
  case tag<Data::Value>.value():
    return make_py_buffer_info_t<Data::Value>(view);
  case tag<Data::Variance>.value():
    return make_py_buffer_info_t<Data::Variance>(view);
  default:
    throw std::runtime_error("non implemented for this type.");
  }
}

template <class Tag>
void setVariableSlice(VariableSlice<Variable> &self,
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
void setVariableSliceRange(VariableSlice<Variable> &self,
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

template <class Tag>
auto as_py_array_t(py::object &obj, VariableSlice<Variable> &view) {
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
  auto &view = obj.cast<VariableSlice<Variable> &>();
  switch (view.type()) {
  case tag<Coord::X>.value():
    return {as_py_array_t<Coord::X>(obj, view)};
  case tag<Coord::Y>.value():
    return {as_py_array_t<Coord::Y>(obj, view)};
  case tag<Coord::Z>.value():
    return {as_py_array_t<Coord::Z>(obj, view)};
  case tag<Coord::Tof>.value():
    return {as_py_array_t<Coord::Tof>(obj, view)};
  case tag<Coord::Mask>.value():
    return {as_py_array_t<Coord::Mask>(obj, view)};
  case tag<Coord::SpectrumNumber>.value():
    return {as_py_array_t<Coord::SpectrumNumber>(obj, view)};
  case tag<Data::Value>.value():
    return {as_py_array_t<Data::Value>(obj, view)};
  case tag<Data::Variance>.value():
    return {as_py_array_t<Data::Variance>(obj, view)};
  default:
    throw std::runtime_error("non implemented for this type.");
  }
}

template <class... Ts>
std::variant<VariableView<Ts>...>
as_VariableView_variant(VariableSlice<Variable> &view) {
  switch (view.type()) {
  case tag<Coord::X>.value():
    return {view.get<Coord::X>()};
  case tag<Coord::Y>.value():
    return {view.get<Coord::Y>()};
  case tag<Coord::Z>.value():
    return {view.get<Coord::Z>()};
  case tag<Coord::Tof>.value():
    return {view.get<Coord::Tof>()};
  case tag<Coord::Mask>.value():
    return {view.get<Coord::Mask>()};
  case tag<Coord::RowLabel>.value():
    return {view.get<Coord::RowLabel>()};
  case tag<Coord::SpectrumNumber>.value():
    return {view.get<Coord::SpectrumNumber>()};
  case tag<Data::Value>.value():
    return {view.get<Data::Value>()};
  case tag<Data::Variance>.value():
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

  py::class_<Tag>(m, "Tag");

  auto data_tags = m.def_submodule("Data");
  py::class_<detail::PythonData::Value, Tag>(data_tags, "_Value");
  py::class_<detail::PythonData::Variance, Tag>(data_tags, "_Variance");
  data_tags.attr("Value") = detail::PythonData::Value{};
  data_tags.attr("Variance") = detail::PythonData::Variance{};

  auto coord_tags = m.def_submodule("Coord");
  py::class_<detail::PythonCoord::Mask, Tag>(coord_tags, "_Mask");
  py::class_<detail::PythonCoord::X, Tag>(coord_tags, "_X");
  py::class_<detail::PythonCoord::Y, Tag>(coord_tags, "_Y");
  py::class_<detail::PythonCoord::Z, Tag>(coord_tags, "_Z");
  py::class_<detail::PythonCoord::Tof, Tag>(coord_tags, "_Tof");
  py::class_<detail::PythonCoord::RowLabel, Tag>(coord_tags, "_RowLabel");
  py::class_<detail::PythonCoord::SpectrumNumber, Tag>(coord_tags,
                                                       "_SpectrumNumber");
  coord_tags.attr("Mask") = detail::PythonCoord::Mask{};
  coord_tags.attr("X") = detail::PythonCoord::X{};
  coord_tags.attr("Y") = detail::PythonCoord::Y{};
  coord_tags.attr("Z") = detail::PythonCoord::Z{};
  coord_tags.attr("Tof") = detail::PythonCoord::Tof{};
  coord_tags.attr("RowLabel") = detail::PythonCoord::RowLabel{};
  coord_tags.attr("SpectrumNumber") = detail::PythonCoord::SpectrumNumber{};

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
      .def(py::init(&detail::makeVariable<detail::PythonCoord::Mask>))
      .def(py::init(&detail::makeVariable<detail::PythonCoord::X>))
      .def(py::init(&detail::makeVariable<detail::PythonCoord::Y>))
      .def(py::init(&detail::makeVariable<detail::PythonCoord::Z>))
      .def(py::init(&detail::makeVariable<detail::PythonData::Value>))
      .def(py::init(&detail::makeVariable<detail::PythonData::Variance>))
      .def(
          py::init(&detail::makeVariableDefaultInit<detail::PythonCoord::Mask>))
      .def(py::init(&detail::makeVariableDefaultInit<detail::PythonCoord::X>))
      .def(py::init(&detail::makeVariableDefaultInit<detail::PythonCoord::Y>))
      .def(py::init(&detail::makeVariableDefaultInit<detail::PythonCoord::Z>))
      .def(
          py::init(&detail::makeVariableDefaultInit<detail::PythonData::Value>))
      .def(py::init(
          &detail::makeVariableDefaultInit<detail::PythonData::Variance>))
      .def(py::init<const VariableSlice<Variable> &>())
      .def_property_readonly("type", &Variable::type)
      .def_property("name", &Variable::name, &Variable::setName)
      .def_property_readonly("is_coord", &Variable::isCoord)
      .def_property_readonly("dimensions", &Variable::dimensions)
      .def(py::self += py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self -= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self *= py::self, py::call_guard<py::gil_scoped_release>());

  py::class_<VariableSlice<Variable>> view(m, "VariableSlice",
                                           py::buffer_protocol());
  view.def_buffer(&make_py_buffer_info);
  view.def_property_readonly(
          "dimensions",
          [](const VariableSlice<Variable> &self) { return self.dimensions(); },
          py::return_value_policy::copy)
      .def("__len__",
           [](const VariableSlice<Variable> &self) {
             const auto &dims = self.dimensions();
             if (dims.count() == 0)
               throw std::runtime_error("len() of unsized object.");
             return dims.shape()[0];
           })
      .def_property_readonly("is_coord", &VariableSlice<Variable>::isCoord)
      .def_property_readonly("type", &VariableSlice<Variable>::type)
      .def_property_readonly("name", &VariableSlice<Variable>::name)
      .def("__getitem__",
           [](VariableSlice<Variable> &self,
              const std::tuple<Dim, gsl::index> &index) {
             return self(std::get<Dim>(index), std::get<gsl::index>(index));
           })
      .def("__getitem__", &pySlice)
      .def("__getitem__",
           [](VariableSlice<Variable> &self,
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
      .def("__iadd__",
           [](VariableSlice<Variable> &a, Variable &b) { return a += b; },
           py::is_operator())
      .def("__isub__",
           [](VariableSlice<Variable> &a, Variable &b) { return a -= b; },
           py::is_operator())
      .def("__imul__",
           [](VariableSlice<Variable> &a, Variable &b) { return a *= b; },
           py::is_operator());

  py::class_<Slice<Dataset>>(m, "DatasetView")
      .def(py::init<Dataset &>())
      .def("__len__", &Slice<Dataset>::size)
      .def("__iter__",
           [](Slice<Dataset> &self) {
             return py::make_iterator(detail::makeAccess(self).begin(),
                                      detail::makeAccess(self).end());
           })
      .def("__contains__", &Slice<Dataset>::contains, py::arg("tag"),
           py::arg("name") = "")
      .def("__contains__",
           [](const Slice<Dataset> &self,
              const std::tuple<const Tag, const std::string> &key) {
             return self.contains(std::get<0>(key), std::get<1>(key));
           })
      .def("__getitem__",
           [](Slice<Dataset> &self, const std::tuple<Dim, gsl::index> &index) {
             return self(std::get<Dim>(index), std::get<gsl::index>(index));
           })
      .def("__getitem__",
           [](Slice<Dataset> &self,
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
           detail::getCoord<detail::PythonCoord::X, Slice<Dataset>>)
      .def("__getitem__",
           detail::getCoord<detail::PythonCoord::Y, Slice<Dataset>>)
      .def("__getitem__",
           detail::getCoord<detail::PythonCoord::Z, Slice<Dataset>>)
      .def("__getitem__",
           detail::getCoord<detail::PythonCoord::Tof, Slice<Dataset>>)
      .def("__getitem__",
           detail::getCoord<detail::PythonCoord::RowLabel, Slice<Dataset>>)
      .def(
          "__getitem__",
          detail::getCoord<detail::PythonCoord::SpectrumNumber, Slice<Dataset>>)
      .def("__getitem__",
           detail::getData<detail::PythonData::Value, Slice<Dataset>>)
      .def("__getitem__",
           detail::getData<detail::PythonData::Variance, Slice<Dataset>>)
      .def("__setitem__",
           detail::setData<detail::PythonData::Value, Slice<Dataset>>)
      .def("__setitem__",
           detail::setData<detail::PythonData::Variance, Slice<Dataset>>)
      .def(py::self += py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self -= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self *= py::self, py::call_guard<py::gil_scoped_release>());

  py::class_<Dataset>(m, "Dataset")
      .def(py::init<>())
      .def(py::init<const Slice<Dataset> &>())
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
      .def("__getitem__", detail::getCoord<detail::PythonCoord::X, Dataset>)
      .def("__getitem__", detail::getCoord<detail::PythonCoord::Y, Dataset>)
      .def("__getitem__", detail::getCoord<detail::PythonCoord::Z, Dataset>)
      .def("__getitem__", detail::getCoord<detail::PythonCoord::Tof, Dataset>)
      .def("__getitem__", detail::getCoord<detail::PythonCoord::Mask, Dataset>)
      .def("__getitem__",
           detail::getCoord<detail::PythonCoord::RowLabel, Dataset>)
      .def("__getitem__",
           detail::getCoord<detail::PythonCoord::SpectrumNumber, Dataset>)
      .def("__getitem__", detail::getData<detail::PythonData::Value, Dataset>)
      .def("__getitem__",
           detail::getData<detail::PythonData::Variance, Dataset>)
      .def("__getitem__",
           [](Dataset &self, const std::string &name) { return self[name]; })
      // Careful: The order of overloads is really important here, otherwise
      // Slice<Dataset> matches the overload below for py::array_t. I have not
      // understood all details of this yet though. See also
      // https://pybind11.readthedocs.io/en/stable/advanced/functions.html#overload-resolution-order.
      .def("__setitem__",
           [](Dataset &self, const std::tuple<Dim, gsl::index> &index,
              const Slice<Dataset> &other) {
             auto slice =
                 self(std::get<Dim>(index), std::get<gsl::index>(index));
             if (slice == other)
               return;
             throw std::runtime_error("Non-self-assignment of Dataset slices "
                                      "is not implemented yet.\n");
           })
      .def("__setitem__", detail::setData<detail::PythonData::Value, Dataset>)
      .def("__setitem__",
           detail::setData<detail::PythonData::Variance, Dataset>)
      .def("__setitem__", detail::insertCoord<detail::PythonCoord::X>)
      .def("__setitem__", detail::insertCoord<detail::PythonCoord::Y>)
      .def("__setitem__", detail::insertCoord<detail::PythonCoord::Z>)
      .def("__setitem__", detail::insertCoord<detail::PythonCoord::Tof>)
      .def("__setitem__", detail::insertCoord<detail::PythonCoord::Mask>)
      .def("__setitem__", detail::insertCoord1D<detail::PythonCoord::RowLabel>)
      .def("__setitem__",
           detail::insertCoord<detail::PythonCoord::SpectrumNumber>)
      .def("__setitem__", detail::insert<detail::PythonData::Value>)
      .def("__setitem__", detail::insert<detail::PythonData::Variance>)
      .def("__setitem__", detail::insert<detail::PythonData::Value, Variable>)
      .def("__setitem__",
           detail::insert<detail::PythonData::Variance, Variable>)
      .def("__setitem__",
           detail::insert<detail::PythonData::Value, VariableSlice<Variable>>)
      .def(
          "__setitem__",
          detail::insert<detail::PythonData::Variance, VariableSlice<Variable>>)
      .def("__setitem__", detail::insertDefaultInit<detail::PythonData::Value>)
      .def("__setitem__",
           detail::insertDefaultInit<detail::PythonData::Variance>)
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

  py::implicitly_convertible<Slice<Dataset>, Dataset>();

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
