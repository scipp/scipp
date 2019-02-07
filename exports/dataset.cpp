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
#include "tag_util.h"

namespace py = pybind11;

template <class T> struct mutable_span_methods {
  static void add(py::class_<gsl::span<T>> &span) {
    span.def("__setitem__", [](gsl::span<T> &self, const gsl::index i,
                               const T value) { self[i] = value; });
  }
};
template <class T> struct mutable_span_methods<const T> {
  static void add(py::class_<gsl::span<const T>> &) {}
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

/// Helper to pass "default" dtype.
struct Empty {
  char dummy;
};

DType convertDType(const py::dtype type) {
  if (type.is(py::dtype::of<double>()))
    return dtype<double>;
  if (type.is(py::dtype::of<float>()))
    return dtype<float>;
  // See https://github.com/pybind/pybind11/pull/1329, int64_t not matching
  // numpy.int64 correctly.
  if (type.is(py::dtype::of<std::int64_t>()) ||
      (type.kind() == 'i' && type.itemsize() == 8))
    return dtype<int64_t>;
  if (type.is(py::dtype::of<int32_t>()))
    return dtype<int32_t>;
  // TODO We should introduce our own Bool, instead of relying on char.
  if (type.is(py::dtype::of<bool>()))
    return dtype<char>;
  throw std::runtime_error("unsupported dtype");
}

namespace detail {
template <class T> struct MakeVariable {
  static Variable apply(const Tag tag, const std::vector<Dim> &labels,
                        py::array data) {
    // Pybind11 converts py::array to py::array_t for us, with all sorts of
    // automatic conversions such as integer to double, if required.
    py::array_t<T> dataT(data);
    const py::buffer_info info = dataT.request();
    Dimensions dims(labels, info.shape);
    auto *ptr = (T *)info.ptr;
    return ::makeVariable<T>(tag, dims, ptr, ptr + dims.volume());
  }
};

template <class T> struct MakeVariableDefaultInit {
  static Variable apply(const Tag tag, const std::vector<Dim> &labels,
                        const py::tuple &shape) {
    Dimensions dims(labels, shape.cast<std::vector<gsl::index>>());
    return ::makeVariable<T>(tag, dims);
  }
};

Variable makeVariable(const Tag tag, const std::vector<Dim> &labels,
                      py::array &data,
                      py::dtype dtype = py::dtype::of<Empty>()) {
  const py::buffer_info info = data.request();
  // Use custom dtype, otherwise dtype of data.
  const auto dtypeTag = dtype.is(py::dtype::of<Empty>())
                            ? convertDType(data.dtype())
                            : convertDType(dtype);
  return CallDType<double, float, int64_t, int32_t,
                   char>::apply<detail::MakeVariable>(dtypeTag, tag, labels,
                                                      data);
}

Variable makeVariableDefaultInit(const Tag tag, const std::vector<Dim> &labels,
                                 const py::tuple &shape,
                                 py::dtype dtype = py::dtype::of<Empty>()) {
  // TODO Numpy does not support strings, how can we specify std::string as a
  // dtype? The same goes for other, more complex item types we need for
  // variables. Do we need an overload with a dtype arg that does not use
  // py::dtype?
  const auto dtypeTag = dtype.is(py::dtype::of<Empty>()) ? defaultDType(tag)
                                                         : convertDType(dtype);
  return CallDType<double, float, int64_t, int32_t,
                   char>::apply<detail::MakeVariableDefaultInit>(dtypeTag, tag,
                                                                 labels, shape);
}

namespace Key {
using Tag = Tag;
using TagAndName = std::pair<Tag, const std::string &>;
auto get(const Key::Tag &key) {
  static const std::string empty;
  return std::tuple(key, empty);
}
auto get(const Key::TagAndName &key) { return key; }
} // namespace Key

template <class K>
void insert_array(
    Dataset &self, const K &key,
    const std::tuple<const std::vector<Dim> &, py::array &> &data) {
  const auto & [ tag, name ] = Key::get(key);
  const auto & [ labels, array ] = data;
  const auto dtypeTag = convertDType(array.dtype());
  auto var = CallDType<double, float, int64_t, int32_t,
                       char>::apply<detail::MakeVariable>(dtypeTag, tag, labels,
                                                          array);
  if (!name.empty())
    var.setName(name);
  self.insert(std::move(var));
}

// Note the concretely typed py::array_t. If we use py::array it will not match
// plain Python arrays.
template <class T>
void insertNamed(
    Dataset &self, const std::pair<Tag, const std::string &> &key,
    const std::tuple<const std::vector<Dim> &, py::array_t<T> &> &data) {
  const auto & [ tag, name ] = key;
  const auto & [ labels, array ] = data;
  // TODO This is converting back and forth between py::array and py::array_t,
  // can we do this in a better way?
  auto var = detail::MakeVariable<T>::apply(tag, labels, array);
  var.setName(name);
  self.insert(std::move(var));
}

template <class T>
void insertCoordT(
    Dataset &self, const Tag tag,
    const std::tuple<const std::vector<Dim> &, py::array_t<T> &> &data) {
  const auto & [ labels, array ] = data;
  // TODO This is converting back and forth between py::array and py::array_t,
  // can we do this in a better way?
  auto var = detail::MakeVariable<T>::apply(tag, labels, array);
  self.insert(std::move(var));
}

template <class Tag>
void insertCoord1D(Dataset &self, const Tag,
                   const std::tuple<const std::vector<Dim> &,
                                    std::vector<typename Tag::type> &> &data) {
  const auto &labels = std::get<0>(data);
  const auto &values = std::get<1>(data);
  Dimensions dims{labels, {static_cast<gsl::index>(values.size())}};
  self.insert(Tag{}, dims, values);
}

template <class Var>
void insert(Dataset &self, const std::pair<Tag, const std::string &> &key,
            const Var &var) {
  const auto & [ tag, name ] = key;
  if (self.contains(tag, name))
    if (self(tag, name) == var)
      return;
  Variable copy(var);
  copy.setTag(tag);
  copy.setName(name);
  self.insert(std::move(copy));
}

void insertDefaultInit(
    Dataset &self, const std::pair<Tag, const std::string &> &key,
    const std::tuple<const std::vector<Dim> &, py::tuple> &data) {
  auto var = makeVariableDefaultInit(std::get<Tag>(key), std::get<0>(data),
                                     std::get<1>(data));
  var.setName(std::get<const std::string &>(key));
  self.insert(std::move(var));
}

// Add size factor.
template <class T>
std::vector<gsl::index> numpy_strides(const std::vector<gsl::index> &s) {
  std::vector<gsl::index> strides(s.size());
  gsl::index elemSize = sizeof(T);
  for (size_t i = 0; i < strides.size(); ++i) {
    strides[i] = elemSize * s[i];
  }
  return strides;
}

// TODO This needs to be refactored to support custom dtype.
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

  auto buf = self[index].get(Tag{});
  double *ptr = (double *)info.ptr;
  std::copy(ptr, ptr + dims.volume(), buf.begin());
}
} // namespace detail

VariableSlice pySlice(VariableSlice &view,
                      const std::tuple<Dim, const py::slice> &index) {
  const Dim dim = std::get<Dim>(index);
  const auto indices = std::get<const py::slice>(index);
  size_t start, stop, step, slicelength;
  const auto size = view.dimensions()[dim];
  if (!indices.compute(size, &start, &stop, &step, &slicelength))
    throw py::error_already_set();
  if (step != 1)
    throw std::runtime_error("Step must be 1");
  return view(dim, start, stop);
}

template <class T> struct MakePyBufferInfoT {
  static py::buffer_info apply(VariableSlice &view) {
    // TODO We should introduce our own Bool, instead of relying on char.
    return py::buffer_info(
        view.template span<T>().data(), /* Pointer to buffer */
        sizeof(T),                      /* Size of one scalar */
        py::format_descriptor<
            std::conditional_t<std::is_same_v<T, char>, bool, T>>::
            format(),              /* Python struct-style format descriptor */
        view.dimensions().count(), /* Number of dimensions */
        view.dimensions().shape(), /* Buffer dimensions */
        detail::numpy_strides<T>(
            view.strides()) /* Strides (in bytes) for each index */
    );
  }
};

py::buffer_info make_py_buffer_info(VariableSlice &view) {
  return CallDType<double, float, int64_t, int32_t,
                   char>::apply<MakePyBufferInfoT>(view.dtype(), view);
}

template <class Tag>
void doSetVariableSlice(VariableSlice &slice,
                        py::array_t<typename Tag::type> &data) {
  const auto &dims = slice.dimensions();
  py::buffer_info info = data.request();
  const auto &shape = dims.shape();
  if (!std::equal(info.shape.begin(), info.shape.end(), shape.begin(),
                  shape.end()))
    throw std::runtime_error(
        "Shape mismatch when setting data from numpy array.");

  auto buf = slice.get(Tag{});
  double *ptr = (double *)info.ptr;
  std::copy(ptr, ptr + dims.volume(), buf.begin());
}

template <class Tag>
void setVariableSlice(VariableSlice &self,
                      const std::tuple<Dim, gsl::index> &index,
                      py::array_t<typename Tag::type> &data) {
  auto slice = self(std::get<Dim>(index), std::get<gsl::index>(index));
  doSetVariableSlice<Tag>(slice, data);
}

template <class Tag>
void setVariableSliceRange(VariableSlice &self,
                           const std::tuple<Dim, const py::slice> &index,
                           py::array_t<typename Tag::type> &data) {
  auto slice = pySlice(self, index);
  doSetVariableSlice<Tag>(slice, data);
}

template <class T, class Var> auto as_py_array_t(py::object &obj, Var &view) {
  // TODO Should `Variable` also have a `strides` method?
  const auto strides = VariableSlice(view).strides();
  // TODO We should introduce our own Bool, instead of relying on char.
  using py_T = std::conditional_t<std::is_same_v<T, char>, bool, T>;
  return py::array_t<py_T>{view.dimensions().shape(),
                           detail::numpy_strides<T>(strides),
                           (py_T *)view.template span<T>().data(), obj};
}

template <class Var, class... Ts>
std::variant<py::array_t<Ts>...> as_py_array_t_variant(py::object &obj) {
  auto &view = obj.cast<Var &>();
  switch (view.dtype()) {
  case dtype<double>:
    return {as_py_array_t<double>(obj, view)};
  case dtype<float>:
    return {as_py_array_t<float>(obj, view)};
  case dtype<int64_t>:
    return {as_py_array_t<int64_t>(obj, view)};
  case dtype<int32_t>:
    return {as_py_array_t<int32_t>(obj, view)};
  case dtype<char>:
    return {as_py_array_t<char>(obj, view)};
  default:
    throw std::runtime_error("not implemented for this type.");
  }
}

template <class Var, class... Ts>
std::variant<std::conditional_t<std::is_same_v<Var, Variable>, gsl::span<Ts>,
                                VariableView<Ts>>...>
as_VariableView_variant(Var &view) {
  switch (view.dtype()) {
  case dtype<double>:
    return {view.template span<double>()};
  case dtype<float>:
    return {view.template span<float>()};
  case dtype<int64_t>:
    return {view.template span<int64_t>()};
  case dtype<int32_t>:
    return {view.template span<int32_t>()};
  case dtype<char>:
    return {view.template span<char>()};
  case dtype<std::string>:
    return {view.template span<std::string>()};
  default:
    throw std::runtime_error("not implemented for this type.");
  }
}

PYBIND11_MODULE(dataset, m) {
  py::enum_<Dim>(m, "Dim")
      .value("Row", Dim::Row)
      .value("Spectrum", Dim::Spectrum)
      .value("Tof", Dim::Tof)
      .value("X", Dim::X)
      .value("Y", Dim::Y)
      .value("Z", Dim::Z);

  py::class_<Tag>(m, "Tag").def(py::self == py::self);

  auto data_tags = m.def_submodule("Data");
  py::class_<Data::Value_t, Tag>(data_tags, "_Value");
  py::class_<Data::Variance_t, Tag>(data_tags, "_Variance");
  data_tags.attr("Value") = Data::Value;
  data_tags.attr("Variance") = Data::Variance;

  auto coord_tags = m.def_submodule("Coord");
  py::class_<Coord::Mask_t, Tag>(coord_tags, "_Mask");
  py::class_<Coord::X_t, Tag>(coord_tags, "_X");
  py::class_<Coord::Y_t, Tag>(coord_tags, "_Y");
  py::class_<Coord::Z_t, Tag>(coord_tags, "_Z");
  py::class_<Coord::Tof_t, Tag>(coord_tags, "_Tof");
  py::class_<Coord::RowLabel_t, Tag>(coord_tags, "_RowLabel");
  py::class_<Coord::SpectrumNumber_t, Tag>(coord_tags, "_SpectrumNumber");
  coord_tags.attr("Mask") = Coord::Mask;
  coord_tags.attr("X") = Coord::X;
  coord_tags.attr("Y") = Coord::Y;
  coord_tags.attr("Z") = Coord::Z;
  coord_tags.attr("Tof") = Coord::Tof;
  coord_tags.attr("RowLabel") = Coord::RowLabel;
  coord_tags.attr("SpectrumNumber") = Coord::SpectrumNumber;

  declare_span<double>(m, "double");
  declare_span<float>(m, "float");
  declare_span<const double>(m, "double_const");
  declare_span<const std::string>(m, "string_const");
  declare_span<const Dim>(m, "Dim_const");

  declare_VariableView<double>(m, "double");
  declare_VariableView<float>(m, "float");
  declare_VariableView<int64_t>(m, "int64");
  declare_VariableView<int32_t>(m, "int32");
  declare_VariableView<std::string>(m, "string");
  declare_VariableView<char>(m, "char");

  py::class_<Dimensions>(m, "Dimensions")
      .def(py::init<>())
      .def("__repr__",
           [](const Dimensions &self) {
             std::string out = "Dimensions = " + dataset::to_string(self);
             return out;
           })
      .def("__len__", &Dimensions::count)
      .def("__contains__", [](const Dimensions &self,
                              const Dim dim) { return self.contains(dim); })
      .def_property_readonly("labels", &Dimensions::labels)
      .def("add", &Dimensions::add)
      .def("size",
           py::overload_cast<const Dim>(&Dimensions::operator[], py::const_));

  PYBIND11_NUMPY_DTYPE(Empty, dummy);

  py::class_<Variable>(m, "Variable")
      .def(py::init(&detail::makeVariableDefaultInit), py::arg("tag"),
           py::arg("labels"), py::arg("shape"),
           py::arg("dtype") = py::dtype::of<Empty>())
      // TODO Need to add overload for std::vector<std::string>, etc.
      .def(py::init(&detail::makeVariable), py::arg("tag"), py::arg("labels"),
           py::arg("data"), py::arg("dtype") = py::dtype::of<Empty>())
      .def(py::init<const VariableSlice &>())
      .def_property_readonly("tag", &Variable::tag)
      .def_property("name", [](const Variable &self) { return self.name(); },
                    &Variable::setName)
      .def_property_readonly("is_coord", &Variable::isCoord)
      .def_property_readonly(
          "dimensions", [](const Variable &self) { return self.dimensions(); })
      .def_property_readonly("numpy",
                             &as_py_array_t_variant<Variable, double, float,
                                                    int64_t, int32_t, bool>)
      .def_property_readonly(
          "data", &as_VariableView_variant<Variable, double, float, int64_t,
                                           int32_t, char, std::string>)
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
           [](VariableSlice &self, const std::map<Dim, const gsl::index> d) {
             auto slice(self);
             for (auto item : d)
               slice = slice(item.first, item.second);
             return slice;
           })
      // TODO Make these using py::array instead of py::array_t, then cast based
      // on tag?
      .def("__setitem__", &setVariableSlice<Data::Value_t>)
      .def("__setitem__", &setVariableSliceRange<Data::Value_t>)
      .def_property_readonly(
          "numpy", &as_py_array_t_variant<VariableSlice, double, float, int64_t,
                                          int32_t, bool>)
      .def_property_readonly(
          "data", &as_VariableView_variant<VariableSlice, double, float,
                                           int64_t, int32_t, char, std::string>)
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
             return py::make_iterator(self.begin(), self.end());
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
             gsl::index size = self.dimensions()[dim];
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
      .def("__setitem__", detail::setData<Data::Value_t, DatasetSlice>)
      .def("__setitem__", detail::setData<Data::Variance_t, DatasetSlice>)
      .def(py::self += py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self -= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self *= py::self, py::call_guard<py::gil_scoped_release>());

  py::class_<Dataset>(m, "Dataset")
      .def(py::init<>())
      .def(py::init<const DatasetSlice &>())
      .def("__len__", &Dataset::size)
      .def("__iter__",
           [](Dataset &self) {
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
             const auto size = self.dimensions()[dim];
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
      // No dimensions argument, this will set data of existing item.
      .def("__setitem__", detail::setData<Data::Value_t, Dataset>)
      .def("__setitem__", detail::setData<Data::Variance_t, Dataset>)
      // Variants with dimensions, inserting new item.
      .def("__setitem__", detail::insert_array<detail::Key::Tag>)
      .def("__setitem__", detail::insert_array<detail::Key::TagAndName>)
      // TODO: Overloaded to cover non-numpy data such as a scalar value. This
      // is handled by automatic conversion by PYbind11 when using py::array_t.
      // See also the py::array::forcecast argument, we need to minimize
      // implicit (and potentially expensive conversion).
      .def("__setitem__", detail::insertCoordT<double>)
      // py::array_t does not support non-POD types like std::string, so we need
      // to handle them separately.
      .def("__setitem__", detail::insertCoord1D<Coord::RowLabel_t>)
      // TODO There should be overloads with name also for other variants.
      .def("__setitem__", detail::insertNamed<double>)
      .def("__setitem__", detail::insert<Variable>)
      .def("__setitem__", detail::insert<VariableSlice>)
      .def("__setitem__", detail::insertDefaultInit)
      // TODO Make sure we have all overloads covered to avoid implicit
      // conversion of DatasetSlice to Dataset.
      .def(py::self == py::self, py::call_guard<py::gil_scoped_release>())
      .def("__iadd__",
           [](Dataset &self, const DatasetSlice &other) {
             return self += other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__isub__",
           [](Dataset &self, const DatasetSlice &other) {
             return self -= other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__imul__",
           [](Dataset &self, const DatasetSlice &other) {
             return self *= other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__add__",
           [](const Dataset &self, const DatasetSlice &other) {
             return self + other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__sub__",
           [](const Dataset &self, const DatasetSlice &other) {
             return self - other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__mul__",
           [](const Dataset &self, const DatasetSlice &other) {
             return self * other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def(py::self += py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self -= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self *= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self + py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self - py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self * py::self, py::call_guard<py::gil_scoped_release>())
      .def("dimensions", [](const Dataset &self) { return self.dimensions(); });

  py::implicitly_convertible<DatasetSlice, Dataset>();

  m.def("split",
        py::overload_cast<const Dataset &, const Dim,
                          const std::vector<gsl::index> &>(&split),
        py::call_guard<py::gil_scoped_release>());
  m.def("concatenate",
        py::overload_cast<const Dataset &, const Dataset &, const Dim>(
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
