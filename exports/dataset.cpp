/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <variant>

#include <pybind11/eigen.h>
#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "convert.h"
#include "dataset.h"
#include "events.h"
#include "except.h"
#include "tag_util.h"
#include "tags.h"
#include "unit.h"
#include "zip_view.h"

namespace py = pybind11;

template <typename Collection>
auto getItemBySingleIndex(Collection &self,
                          const std::tuple<Dim, gsl::index> &index) {
  gsl::index idx{std::get<gsl::index>(index)};
  auto &dim = std::get<Dim>(index);
  auto sz = self.dimensions()[dim];
  if (idx <= -sz || idx >= sz) // index is out of range
    throw std::runtime_error("Dimension size is " +
                             std::to_string(self.dimensions()[dim]) +
                             ", can't treat " + std::to_string(idx));
  if (idx < 0)
    idx = sz + idx;
  return self(std::get<Dim>(index), idx);
}

template <class T> struct mutable_span_methods {
  static void add(py::class_<gsl::span<T>> &span) {
    span.def("__setitem__", [](gsl::span<T> &self, const gsl::index i,
                               const T value) { self[i] = value; });
  }
};
template <class T> struct mutable_span_methods<const T> {
  static void add(py::class_<gsl::span<const T>> &) {}
};

template <class T> std::string element_to_string(const T &item) {
  using dataset::to_string;
  using std::to_string;
  if constexpr (std::is_same_v<T, std::string>)
    return {'"' + item + "\", "};
  else if constexpr (std::is_same_v<T, Eigen::Vector3d>)
    return {"(Eigen::Vector3d), "};
  else if constexpr (std::is_same_v<T,
                                    boost::container::small_vector<double, 8>>)
    return {"(vector), "};
  else
    return to_string(item) + ", ";
}

template <class T> std::string array_to_string(const T &arr) {
  const gsl::index size = arr.size();
  if (size == 0)
   return std::string("[]");
  std::string s = "[";
  for (gsl::index i = 0; i < arr.size(); ++i) {
   if (i == 4 && size > 8) {
     s += "..., ";
     i = size - 4;
   }
   s += element_to_string(arr[i]);
  }
  s.resize(s.size() - 2);
  s += "]";
  return s;
}

template <class T> void declare_span(py::module &m, const std::string &suffix) {
  py::class_<gsl::span<T>> span(m, (std::string("span_") + suffix).c_str());
  span.def("__getitem__", &gsl::span<T>::operator[],
           py::return_value_policy::reference)
      .def("size", &gsl::span<T>::size)
      .def("__len__", &gsl::span<T>::size)
      .def("__iter__", [](const gsl::span<T> &self) {
        return py::make_iterator(self.begin(), self.end());
      })
      .def("__repr__",
       [](const gsl::span<T> &self) { return array_to_string(self); });
  mutable_span_methods<T>::add(span);
}

template <class... Keys>
void declare_VariableZipProxy(py::module &m, const std::string &suffix,
                              const Keys &... keys) {
  using Proxy = decltype(zip(std::declval<Dataset &>(), keys...));
  py::class_<Proxy> proxy(m,
                          (std::string("VariableZipProxy_") + suffix).c_str());
  proxy.def("__len__", &Proxy::size)
      .def("__getitem__", &Proxy::operator[])
      .def("__iter__",
           [](const Proxy &self) {
             return py::make_iterator(self.begin(), self.end());
           },
           // WARNING The py::keep_alive is really important. It prevents
           // deletion of the VariableZipProxy when its iterators are still in
           // use. This is necessary due to the underlying implementation, which
           // used ranges::view::zip based on temporary gsl::range.
           py::keep_alive<0, 1>());
}

template <class... Fields>
void declare_ItemZipProxy(py::module &m, const std::string &suffix) {
  using Proxy = ItemZipProxy<Fields...>;
  py::class_<Proxy> proxy(m, (std::string("ItemZipProxy_") + suffix).c_str());
  proxy.def("__len__", &Proxy::size)
      .def("__iter__",
           [](const Proxy &self) {
             return py::make_iterator(self.begin(), self.end());
           })
      .def("append",
           [](const Proxy &self,
              const std::tuple<typename Fields::value_type...> &item) {
             self.push_back(item);
           });
}

template <class... Fields>
void declare_ranges_pair(py::module &m, const std::string &suffix) {
  using Proxy = ranges::v3::common_pair<Fields &...>;
  py::class_<Proxy> proxy(
      m, (std::string("ranges_v3_common_pair_") + suffix).c_str());
  proxy.def("first", [](const Proxy &self) { return std::get<0>(self); })
      .def("second", [](const Proxy &self) { return std::get<1>(self); });
}

template <class T>
void declare_VariableView(py::module &m, const std::string &suffix) {
  py::class_<VariableView<T>> view(
      m, (std::string("VariableView_") + suffix).c_str());
  view.def("__repr__",
           [](const VariableView<T> &self) { return array_to_string(self); })
      .def("__getitem__", &VariableView<T>::operator[],
           py::return_value_policy::reference)
      .def("__setitem__", [](VariableView<T> &self, const gsl::index i,
                             const T value) { self[i] = value; })
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
  if (type.is(py::dtype::of<bool>()))
    return dtype<bool>;
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
    return ::makeVariable<T, ssize_t>(tag, dims, info.strides, ptr);
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
  return CallDType<double, float, int64_t, int32_t, char,
                   bool>::apply<detail::MakeVariable>(dtypeTag, tag, labels,
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
  return CallDType<double, float, int64_t, int32_t, char, bool, Dataset,
                   typename Data::EventTofs_t::type, Eigen::Vector3d>::
      apply<detail::MakeVariableDefaultInit>(dtypeTag, tag, labels, shape);
}

namespace Key {
using Tag = Tag;
using TagName = std::pair<Tag, const std::string &>;
auto get(const Key::Tag &key) {
  static const std::string empty;
  return std::tuple(key, empty);
}
auto get(const Key::TagName &key) { return key; }
} // namespace Key

template <class K>
void insertDefaultInit(
    Dataset &self, const K &key,
    const std::tuple<const std::vector<Dim> &, py::tuple> &data) {
  const auto & [ tag, name ] = Key::get(key);
  const auto & [ labels, array ] = data;
  auto var = makeVariableDefaultInit(tag, labels, array);
  if (!name.empty())
    var.setName(name);
  self.insert(std::move(var));
}

template <class K>
void insert_ndarray(
    Dataset &self, const K &key,
    const std::tuple<const std::vector<Dim> &, py::array &> &data) {
  const auto & [ tag, name ] = Key::get(key);
  const auto & [ labels, array ] = data;
  const auto dtypeTag = convertDType(array.dtype());
  auto var = CallDType<double, float, int64_t, int32_t, char,
                       bool>::apply<detail::MakeVariable>(dtypeTag, tag, labels,
                                                          array);
  if (!name.empty())
    var.setName(name);
  self.insert(std::move(var));
}

// Note the concretely typed py::array_t. If we use py::array it will not match
// plain Python arrays.
template <class T, class K>
void insert_conv(
    Dataset &self, const K &key,
    const std::tuple<const std::vector<Dim> &, py::array_t<T> &> &data) {
  const auto & [ tag, name ] = Key::get(key);
  const auto & [ labels, array ] = data;
  // TODO This is converting back and forth between py::array and py::array_t,
  // can we do this in a better way?
  auto var = detail::MakeVariable<T>::apply(tag, labels, array);
  if (!name.empty())
    var.setName(name);
  self.insert(std::move(var));
}

template <class T, class K>
void insert_0D(Dataset &self, const K &key,
               const std::tuple<const std::vector<Dim> &, T &> &data) {
  const auto & [ tag, name ] = Key::get(key);
  const auto & [ labels, value ] = data;
  if (!labels.empty())
    throw std::runtime_error(
        "Got 0-D data, but nonzero number of dimension labels.");
  auto var = ::makeVariable<T>(tag, {}, {value});
  if (!name.empty())
    var.setName(name);
  self.insert(std::move(var));
}

template <class T, class K>
void insert_1D(
    Dataset &self, const K &key,
    const std::tuple<const std::vector<Dim> &, std::vector<T> &> &data) {
  const auto & [ tag, name ] = Key::get(key);
  const auto & [ labels, array ] = data;
  Dimensions dims{labels, {static_cast<gsl::index>(array.size())}};
  auto var = ::makeVariable<T>(tag, dims, array);
  if (!name.empty())
    var.setName(name);
  self.insert(std::move(var));
}

template <class Var, class K>
void insert(Dataset &self, const K &key, const Var &var) {
  const auto & [ tag, name ] = Key::get(key);
  if (self.contains(tag, name))
    if (self(tag, name) == var)
      return;
  Variable copy(var);
  copy.setTag(tag);
  if (!name.empty())
    copy.setName(name);
  self.insert(std::move(copy));
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

template <class T> struct SetData {
  static void apply(const VariableSlice &slice, const py::array &data) {
    // Pybind11 converts py::array to py::array_t for us, with all sorts of
    // automatic conversions such as integer to double, if required.
    py::array_t<T> dataT(data);

    const auto &dims = slice.dimensions();
    const py::buffer_info info = dataT.request();
    const auto &shape = dims.shape();
    if (!std::equal(info.shape.begin(), info.shape.end(), shape.begin(),
                    shape.end()))
      throw std::runtime_error(
          "Shape mismatch when setting data from numpy array.");

    auto buf = slice.span<T>();
    auto *ptr = (T *)info.ptr;
    std::copy(ptr, ptr + slice.size(), buf.begin());
  }
};

template <class T, class K>
void setData(T &self, const K &key, const py::array &data) {
  const auto & [ tag, name ] = Key::get(key);
  const auto slice = self(tag, name);
  CallDType<double, float, int64_t, int32_t, char,
            bool>::apply<detail::SetData>(slice.dtype(), slice, data);
}

template <class T>
auto pySlice(T &source, const std::tuple<Dim, const py::slice> &index)
    -> decltype(source(Dim::Invalid, 0)) {
  const auto & [ dim, indices ] = index;
  size_t start, stop, step, slicelength;
  const auto size = source.dimensions()[dim];
  if (!indices.compute(size, &start, &stop, &step, &slicelength))
    throw py::error_already_set();
  if (step != 1)
    throw std::runtime_error("Step must be 1");
  return source(dim, start, stop);
}

void setVariableSlice(VariableSlice &self,
                      const std::tuple<Dim, gsl::index> &index,
                      const py::array &data) {
  auto slice = self(std::get<Dim>(index), std::get<gsl::index>(index));
  CallDType<double, float, int64_t, int32_t, char,
            bool>::apply<detail::SetData>(slice.dtype(), slice, data);
}

void setVariableSliceRange(VariableSlice &self,
                           const std::tuple<Dim, const py::slice> &index,
                           const py::array &data) {
  auto slice = pySlice(self, index);
  CallDType<double, float, int64_t, int32_t, char,
            bool>::apply<detail::SetData>(slice.dtype(), slice, data);
}
} // namespace detail

template <class T> struct MakePyBufferInfoT {
  static py::buffer_info apply(VariableSlice &view) {
    return py::buffer_info(
        view.template span<T>().data(), /* Pointer to buffer */
        sizeof(T),                      /* Size of one scalar */
        py::format_descriptor<
            std::conditional_t<std::is_same_v<T, bool>, bool, T>>::
            format(),              /* Python struct-style format descriptor */
        view.dimensions().count(), /* Number of dimensions */
        view.dimensions().shape(), /* Buffer dimensions */
        detail::numpy_strides<T>(
            view.strides()) /* Strides (in bytes) for each index */
    );
  }
};

py::buffer_info make_py_buffer_info(VariableSlice &view) {
  return CallDType<double, float, int64_t, int32_t, char,
                   bool>::apply<MakePyBufferInfoT>(view.dtype(), view);
}

template <class T, class Var> auto as_py_array_t(py::object &obj, Var &view) {
  // TODO Should `Variable` also have a `strides` method?
  const auto strides = VariableSlice(view).strides();
  using py_T = std::conditional_t<std::is_same_v<T, bool>, bool, T>;
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
  case dtype<bool>:
    return {as_py_array_t<bool>(obj, view)};
  default:
    throw std::runtime_error("not implemented for this type.");
  }
}

template <class... Ts> struct as_VariableViewImpl {
  template <class Var>
  static std::variant<std::conditional_t<
      std::is_same_v<Var, Variable>, gsl::span<underlying_type_t<Ts>>,
      VariableView<underlying_type_t<Ts>>>...>
  get(Var &view) {
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
    case dtype<bool>:
      return {view.template span<bool>()};
    case dtype<std::string>:
      return {view.template span<std::string>()};
    case dtype<boost::container::small_vector<double, 8>>:
      return {view.template span<boost::container::small_vector<double, 8>>()};
    case dtype<Dataset>:
      return {view.template span<Dataset>()};
    case dtype<Eigen::Vector3d>:
      return {view.template span<Eigen::Vector3d>()};
    default:
      throw std::runtime_error("not implemented for this type.");
    }
  }

  // Return a scalar value from a variable, implicitly requiring that the
  // variable is 0-dimensional and thus has only a single item.
  template <class Var> static py::object scalar(Var &view) {
    dataset::expect::equals(Dimensions(), view.dimensions());
    return std::visit(
        [](const auto &data) {
          return py::cast(data[0], py::return_value_policy::reference);
        },
        get(view));
  }
  // Set a scalar value in a variable, implicitly requiring that the
  // variable is 0-dimensional and thus has only a single item.
  template <class Var> static void set_scalar(Var &view, const py::object &o) {
    dataset::expect::equals(Dimensions(), view.dimensions());
    std::visit(
        [&o](const auto &data) {
          data[0] = o.cast<typename std::decay_t<decltype(data)>::value_type>();
        },
        get(view));
  }
};

using as_VariableView =
    as_VariableViewImpl<double, float, int64_t, int32_t, char, bool,
                        std::string, boost::container::small_vector<double, 8>,
                        Dataset, Eigen::Vector3d>;

class SubsetHelper {
public:
  template <class D> SubsetHelper(D &d) : m_data(d) {}
  auto subset(const std::string &name) const { return m_data.subset(name); }
  auto subset(const Tag tag, const std::string &name) const {
    return m_data.subset(tag, name);
  }
  template <class D> void insert(const std::string &name, D &subset) {
    m_data.insert(name, subset);
  }

private:
  DatasetSlice m_data;
};

using small_vector = boost::container::small_vector<double, 8>;
PYBIND11_MAKE_OPAQUE(small_vector);

PYBIND11_MODULE(dataset, m) {
  py::bind_vector<boost::container::small_vector<double, 8>>(
      m, "SmallVectorDouble8");

  py::enum_<Dim>(m, "Dim")
      .value("Component", Dim::Component)
      .value("DeltaE", Dim::DeltaE)
      .value("Detector", Dim::Detector)
      .value("DetectorScan", Dim::DetectorScan)
      .value("Energy", Dim::Energy)
      .value("Event", Dim::Event)
      .value("Invalid", Dim::Invalid)
      .value("Monitor", Dim::Monitor)
      .value("Polarization", Dim::Polarization)
      .value("Position", Dim::Position)
      .value("Q", Dim::Q)
      .value("Row", Dim::Row)
      .value("Run", Dim::Run)
      .value("Spectrum", Dim::Spectrum)
      .value("Temperature", Dim::Temperature)
      .value("Time", Dim::Time)
      .value("Tof", Dim::Tof)
      .value("Qx", Dim::Qx)
      .value("Qy", Dim::Qy)
      .value("Qz", Dim::Qz)
      .value("X", Dim::X)
      .value("Y", Dim::Y)
      .value("Z", Dim::Z);

  py::class_<Tag>(m, "Tag")
      .def(py::self == py::self)
      .def("__repr__",
           [](const Tag &self) { return dataset::to_string(self, "."); });

  // Runtime tags are sufficient in Python, not exporting Tag child classes.
  auto coord_tags = m.def_submodule("Coord");
  coord_tags.attr("Monitor") = Tag(Coord::Monitor);
  coord_tags.attr("DetectorInfo") = Tag(Coord::DetectorInfo);
  coord_tags.attr("ComponentInfo") = Tag(Coord::ComponentInfo);
  coord_tags.attr("Qx") = Tag(Coord::Qx);
  coord_tags.attr("Qy") = Tag(Coord::Qy);
  coord_tags.attr("Qz") = Tag(Coord::Qz);
  coord_tags.attr("X") = Tag(Coord::X);
  coord_tags.attr("Y") = Tag(Coord::Y);
  coord_tags.attr("Z") = Tag(Coord::Z);
  coord_tags.attr("Tof") = Tag(Coord::Tof);
  coord_tags.attr("Energy") = Tag(Coord::Energy);
  coord_tags.attr("DeltaE") = Tag(Coord::DeltaE);
  coord_tags.attr("Ei") = Tag(Coord::Ei);
  coord_tags.attr("Ef") = Tag(Coord::Ef);
  coord_tags.attr("DetectorId") = Tag(Coord::DetectorId);
  coord_tags.attr("SpectrumNumber") = Tag(Coord::SpectrumNumber);
  coord_tags.attr("DetectorGrouping") = Tag(Coord::DetectorGrouping);
  coord_tags.attr("Row") = Tag(Coord::Row);
  coord_tags.attr("Run") = Tag(Coord::Run);
  coord_tags.attr("Polarization") = Tag(Coord::Polarization);
  coord_tags.attr("Temperature") = Tag(Coord::Temperature);
  coord_tags.attr("FuzzyTemperature") = Tag(Coord::FuzzyTemperature);
  coord_tags.attr("Time") = Tag(Coord::Time);
  coord_tags.attr("TimeInterval") = Tag(Coord::TimeInterval);
  coord_tags.attr("Mask") = Tag(Coord::Mask);
  coord_tags.attr("Position") = Tag(Coord::Position);

  auto data_tags = m.def_submodule("Data");
  data_tags.attr("Tof") = Tag(Data::Tof);
  data_tags.attr("PulseTime") = Tag(Data::PulseTime);
  data_tags.attr("Value") = Tag(Data::Value);
  data_tags.attr("Variance") = Tag(Data::Variance);
  data_tags.attr("StdDev") = Tag(Data::StdDev);
  data_tags.attr("Events") = Tag(Data::Events);
  data_tags.attr("EventTofs") = Tag(Data::EventTofs);
  data_tags.attr("EventPulseTimes") = Tag(Data::EventPulseTimes);

  auto attr_tags = m.def_submodule("Attr");
  attr_tags.attr("ExperimentLog") = Tag(Attr::ExperimentLog);
  attr_tags.attr("Monitor") = Tag(Attr::Monitor);

  declare_span<double>(m, "double");
  declare_span<float>(m, "float");
  declare_span<Bool>(m, "bool");
  declare_span<const double>(m, "double_const");
  declare_span<const long>(m, "long_const");
  declare_span<long>(m, "long");
  declare_span<const std::string>(m, "string_const");
  declare_span<std::string>(m, "string");
  declare_span<const Dim>(m, "Dim_const");
  declare_span<Dataset>(m, "Dataset");
  declare_span<Eigen::Vector3d>(m, "Eigen_Vector3d");

  declare_VariableView<double>(m, "double");
  declare_VariableView<float>(m, "float");
  declare_VariableView<int64_t>(m, "int64");
  declare_VariableView<int32_t>(m, "int32");
  declare_VariableView<std::string>(m, "string");
  declare_VariableView<char>(m, "char");
  declare_VariableView<Bool>(m, "bool");
  declare_VariableView<boost::container::small_vector<double, 8>>(
      m, "SmallVectorDouble8");
  declare_VariableView<Dataset>(m, "Dataset");
  declare_VariableView<Eigen::Vector3d>(m, "Eigen_Vector3d");

  py::class_<Unit>(m, "Unit")
      .def(py::init())
      .def("__repr__", [](const Unit &u) -> std::string { return u.name(); })
      .def_property_readonly("name", &Unit::name)
      .def(py::self + py::self)
      .def(py::self - py::self)
      .def(py::self * py::self)
      .def(py::self / py::self)
      .def(py::self == py::self)
      .def(py::self != py::self);

  auto units = m.def_submodule("units");
  units.attr("dimensionless") = Unit(units::dimensionless);
  units.attr("m") = Unit(units::m);
  units.attr("counts") = Unit(units::counts);
  units.attr("s") = Unit(units::s);
  units.attr("kg") = Unit(units::kg);
  units.attr("K") = Unit(units::K);
  units.attr("angstrom") = Unit(units::angstrom);
  units.attr("meV") = Unit(units::meV);
  units.attr("us") = Unit(units::us);

  declare_VariableZipProxy(m, "", Access::Key(Data::EventTofs),
                           Access::Key(Data::EventPulseTimes));
  declare_ItemZipProxy<small_vector, small_vector>(m, "");
  declare_ranges_pair<double, double>(m, "double_double");

  py::class_<Dimensions>(m, "Dimensions")
      .def(py::init<>())
      .def("__repr__",
           [](const Dimensions &self) {
             std::string out = "Dimensions = " + dataset::to_string(self, ".");
             return out;
           })
      .def("__len__", &Dimensions::count)
      .def("__contains__", [](const Dimensions &self,
                              const Dim dim) { return self.contains(dim); })
      .def("__getitem__",
           py::overload_cast<const Dim>(&Dimensions::operator[], py::const_))
      .def_property_readonly("labels", &Dimensions::labels)
      .def_property_readonly("shape", &Dimensions::shape)
      .def("add", &Dimensions::add)
      .def("size",
           py::overload_cast<const Dim>(&Dimensions::operator[], py::const_))
      .def(py::self == py::self)
      .def(py::self != py::self);

  PYBIND11_NUMPY_DTYPE(Empty, dummy);

  py::class_<Variable>(m, "Variable")
      .def(py::init(&detail::makeVariableDefaultInit), py::arg("tag"),
           py::arg("labels"), py::arg("shape"),
           py::arg("dtype") = py::dtype::of<Empty>())
      .def(py::init([](const double data, const Unit &unit) {
             Variable var(Data::Value, {}, {data});
             var.setUnit(unit);
             return var;
           }),
           py::arg("data"), py::arg("unit") = Unit(units::dimensionless))
      // TODO Need to add overload for std::vector<std::string>, etc., see
      // Dataset.__setitem__
      .def(py::init(&detail::makeVariable), py::arg("tag"), py::arg("labels"),
           py::arg("data"), py::arg("dtype") = py::dtype::of<Empty>())
      .def(py::init<const VariableSlice &>())
      .def("__getitem__", detail::pySlice<Variable>, py::keep_alive<0, 1>())
      .def("__setitem__",
           [](Variable &self, const std::tuple<Dim, py::slice> &index,
              const VariableSlice &other) {
             detail::pySlice(self, index).assign(other);
           })
      .def("__setitem__",
           [](Variable &self, const std::tuple<Dim, gsl::index> &index,
              const VariableSlice &other) {
             const auto & [ dim, i ] = index;
             self(dim, i).assign(other);
           })
      .def("copy", [](const Variable &self) { return self; })
      .def("__copy__", [](Variable &self) { return Variable(self); })
      .def("__deepcopy__",
           [](Variable &self, py::dict) { return Variable(self); })
      .def_property_readonly("tag", &Variable::tag)
      .def_property("name", [](const Variable &self) { return self.name(); },
                    &Variable::setName)
      .def_property("unit", &Variable::unit, &Variable::setUnit)
      .def_property_readonly("is_coord", &Variable::isCoord)
      .def_property_readonly("is_data", &Variable::isData)
      .def_property_readonly("is_attr", &Variable::isAttr)
      .def_property_readonly(
          "dimensions", [](const Variable &self) { return self.dimensions(); })
      .def_property_readonly(
          "numpy", &as_py_array_t_variant<Variable, double, float, int64_t,
                                          int32_t, char, bool>)
      .def_property_readonly("data", &as_VariableView::get<Variable>)
      .def_property("scalar", &as_VariableView::scalar<Variable>,
                    &as_VariableView::set_scalar<Variable>,
                    "The only data point for a 0-dimensional "
                    "variable. Raises an exception if the variable is "
                    "not 0-dimensional.")
      .def(py::self == py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self + py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self + double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self - py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self - double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self * py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self * double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self / py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self / double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self += py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self += double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self -= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self -= double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self *= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self *= double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self /= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self /= double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self == py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self != py::self, py::call_guard<py::gil_scoped_release>())
      .def("__eq__", [](Variable &a, VariableSlice &b) { return a == b; },
           py::is_operator())
      .def("__ne__", [](Variable &a, VariableSlice &b) { return a != b; },
           py::is_operator())
      .def("__add__", [](Variable &a, VariableSlice &b) { return a + b; },
           py::is_operator())
      .def("__sub__", [](Variable &a, VariableSlice &b) { return a - b; },
           py::is_operator())
      .def("__mul__", [](Variable &a, VariableSlice &b) { return a * b; },
           py::is_operator())
      .def("__truediv__", [](Variable &a, VariableSlice &b) { return a / b; },
           py::is_operator())
      .def("__iadd__", [](Variable &a, VariableSlice &b) { return a += b; },
           py::is_operator())
      .def("__isub__", [](Variable &a, VariableSlice &b) { return a -= b; },
           py::is_operator())
      .def("__imul__", [](Variable &a, VariableSlice &b) { return a *= b; },
           py::is_operator())
      .def("__itruediv__", [](Variable &a, VariableSlice &b) { return a /= b; },
           py::is_operator())
      .def("__radd__", [](Variable &a, double &b) { return a + b; },
           py::is_operator())
      .def("__rsub__", [](Variable &a, double &b) { return b - a; },
           py::is_operator())
      .def("__rmul__", [](Variable &a, double &b) { return a * b; },
           py::is_operator())
      .def("__len__", &Variable::size)
      .def("__repr__",
           [](const Variable &self) { return dataset::to_string(self, "."); });

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
      .def_property_readonly("is_data", &VariableSlice::isData)
      .def_property_readonly("is_attr", &VariableSlice::isAttr)
      .def_property_readonly("tag", &VariableSlice::tag)
      .def_property_readonly("name", &VariableSlice::name)
      .def_property("unit", &VariableSlice::unit, &VariableSlice::setUnit)
      .def("__getitem__",
           [](VariableSlice &self, const std::tuple<Dim, gsl::index> &index) {
             return getItemBySingleIndex(self, index);
           },
           py::keep_alive<0, 1>())
      .def("__getitem__", &detail::pySlice<VariableSlice>,
           py::keep_alive<0, 1>())
      .def("__getitem__",
           [](VariableSlice &self, const std::map<Dim, const gsl::index> d) {
             auto slice(self);
             for (auto item : d)
               slice = slice(item.first, item.second);
             return slice;
           },
           py::keep_alive<0, 1>())
      .def("__setitem__",
           [](VariableSlice &self, const std::tuple<Dim, py::slice> &index,
              const VariableSlice &other) {
             detail::pySlice(self, index).assign(other);
           })
      .def("__setitem__",
           [](VariableSlice &self, const std::tuple<Dim, gsl::index> &index,
              const VariableSlice &other) {
             const auto & [ dim, i ] = index;
             self(dim, i).assign(other);
           })
      .def("__setitem__", &detail::setVariableSlice)
      .def("__setitem__", &detail::setVariableSliceRange)
      .def("copy", [](const VariableSlice &self) { return Variable(self); })
      .def("__copy__", [](VariableSlice &self) { return Variable(self); })
      .def("__deepcopy__",
           [](VariableSlice &self, py::dict) { return Variable(self); })
      .def_property_readonly(
          "numpy", &as_py_array_t_variant<VariableSlice, double, float, int64_t,
                                          int32_t, char, bool>)
      .def_property_readonly("data", &as_VariableView::get<VariableSlice>)
      .def_property("scalar", &as_VariableView::scalar<VariableSlice>,
                    &as_VariableView::set_scalar<VariableSlice>,
                    "The only data point for a 0-dimensional "
                    "variable. Raises an exception of the variable is "
                    "not 0-dimensional.")
      .def(py::self += py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self -= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self *= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self /= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self + py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self - py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self * py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self / py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self == py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self != py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self += double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self -= double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self *= double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self /= double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self + double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self - double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self * double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self / double(), py::call_guard<py::gil_scoped_release>())
      .def("__eq__", [](VariableSlice &a, Variable &b) { return a == b; },
           py::is_operator())
      .def("__ne__", [](VariableSlice &a, Variable &b) { return a != b; },
           py::is_operator())
      .def("__add__", [](VariableSlice &a, Variable &b) { return a + b; },
           py::is_operator())
      .def("__sub__", [](VariableSlice &a, Variable &b) { return a - b; },
           py::is_operator())
      .def("__mul__", [](VariableSlice &a, Variable &b) { return a * b; },
           py::is_operator())
      .def("__truediv__", [](VariableSlice &a, Variable &b) { return a / b; },
           py::is_operator())
      .def("__iadd__", [](VariableSlice &a, Variable &b) { return a += b; },
           py::is_operator())
      .def("__isub__", [](VariableSlice &a, Variable &b) { return a -= b; },
           py::is_operator())
      .def("__imul__", [](VariableSlice &a, Variable &b) { return a *= b; },
           py::is_operator())
      .def("__itruediv__", [](VariableSlice &a, Variable &b) { return a / b; },
           py::is_operator())
      .def("__radd__", [](VariableSlice &a, double &b) { return a + b; },
           py::is_operator())
      .def("__rsub__", [](VariableSlice &a, double &b) { return b - a; },
           py::is_operator())
      .def("__rmul__", [](VariableSlice &a, double &b) { return a * b; },
           py::is_operator())
      .def("__repr__", [](const VariableSlice &self) {
        return dataset::to_string(self, ".");
      });

  // Implicit conversion VariableSlice -> Variable. Reduces need for excessive
  // operator overload definitions
  py::implicitly_convertible<VariableSlice, Variable>();

  py::class_<SubsetHelper>(m, "SubsetHelper")
      .def("__getitem__",
           [](SubsetHelper &self, const std::string &name) {
             return self.subset(name);
           },
           py::keep_alive<0, 1>())
      .def("__getitem__",
           [](SubsetHelper &self,
              const std::tuple<const Tag, const std::string &> &index) {
             const auto & [ tag, name ] = index;
             return self.subset(tag, name);
           },
           py::keep_alive<0, 1>())
      .def("__setitem__",
           [](SubsetHelper &self, const std::string &name,
              const DatasetSlice &data) { self.insert(name, data); })
      .def("__setitem__", [](SubsetHelper &self, const std::string &name,
                             const Dataset &data) { self.insert(name, data); });
  py::class_<DatasetSlice>(m, "DatasetSlice")
      .def(py::init<Dataset &>())
      .def_property_readonly(
          "dimensions",
          [](const DatasetSlice &self) { return self.dimensions(); })
      .def("__len__", &DatasetSlice::size)
      .def("__iter__",
           [](DatasetSlice &self) {
             return py::make_iterator(self.begin(), self.end());
           },
           py::keep_alive<0, 1>())
      .def("__contains__", &DatasetSlice::contains, py::arg("tag"),
           py::arg("name") = "")
      .def("__contains__",
           [](const DatasetSlice &self,
              const std::tuple<const Tag, const std::string> &key) {
             return self.contains(std::get<0>(key), std::get<1>(key));
           })
      .def("__getitem__",
           [](DatasetSlice &self, const std::tuple<Dim, gsl::index> &index) {
             return getItemBySingleIndex(self, index);
           },
           py::keep_alive<0, 1>())
      .def("__getitem__", &detail::pySlice<DatasetSlice>,
           py::keep_alive<0, 1>())
      .def("__getitem__",
           [](DatasetSlice &self, const Tag &tag) { return self(tag); },
           py::keep_alive<0, 1>())
      .def(
          "__getitem__",
          [](DatasetSlice &self, const std::pair<Tag, const std::string> &key) {
            return self(key.first, key.second);
          },
          py::keep_alive<0, 1>())
      .def("copy", [](const DatasetSlice &self) { return Dataset(self); })
      .def("__copy__", [](DatasetSlice &self) { return Dataset(self); })
      .def("__deepcopy__",
           [](DatasetSlice &self, py::dict) { return Dataset(self); })
      .def_property_readonly(
          "subset", [](DatasetSlice &self) { return SubsetHelper(self); })
      .def("__setitem__",
           [](DatasetSlice &self, const std::tuple<Dim, py::slice> &index,
              const DatasetSlice &other) {
             detail::pySlice(self, index).assign(other);
           })
      .def("__setitem__",
           [](DatasetSlice &self, const std::tuple<Dim, gsl::index> &index,
              const DatasetSlice &other) {
             const auto & [ dim, i ] = index;
             self(dim, i).assign(other);
           })
      .def("__setitem__", detail::setData<DatasetSlice, detail::Key::Tag>)
      .def("__setitem__", detail::setData<DatasetSlice, detail::Key::TagName>)
      .def(py::self + py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self - py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self += py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self -= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self *= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self == py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self != py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self + double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self - double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self * double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self / double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self += double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self -= double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self *= double(), py::call_guard<py::gil_scoped_release>())
      .def("__eq__",
           [](const DatasetSlice &self, const Dataset &other) {
             return self == other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__ne__",
           [](const DatasetSlice &self, const Dataset &other) {
             return self != other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__add__",
           [](const DatasetSlice &self, const Dataset &other) {
             return self + other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__sub__",
           [](const DatasetSlice &self, const Dataset &other) {
             return self - other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__mul__",
           [](const DatasetSlice &self, const Dataset &other) {
             return self * other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__iadd__",
           [](const DatasetSlice &self, const Dataset &other) {
             return self += other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__isub__",
           [](const DatasetSlice &self, const Dataset &other) {
             return self -= other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__imul__",
           [](const DatasetSlice &self, const Dataset &other) {
             return self *= other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__radd__",
           [](const DatasetSlice &self, double &other) { return self + other; },
           py::is_operator())
      .def("__rsub__",
           [](const DatasetSlice &self, double &other) { return other - self; },
           py::is_operator())
      .def("__rmul__",
           [](const DatasetSlice &self, double &other) { return self * other; },
           py::is_operator())
      .def("__repr__", [](const DatasetSlice &self) {
        return dataset::to_string(self, ".");
      });

  py::class_<Dataset>(m, "Dataset")
      .def(py::init<>())
      .def(py::init<const DatasetSlice &>())
      .def_property_readonly(
          "dimensions", [](const Dataset &self) { return self.dimensions(); })
      .def("__len__", &Dataset::size)
      .def("__repr__",
           [](const Dataset &self) { return dataset::to_string(self, "."); })
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
      // TODO: This __getitem__ is here only to prevent unhandled errors when
      // trying to get a dataset slice by supplying only a Dimension, e.g.
      // dataset[Dim.X]. By default, an implicit conversion between Dim and
      // gsl::index is attempted and the __getitem__ then crashes when
      // self[index] is performed below. This fix covers only one case and we
      // need to find a better way of protecting all unsopprted cases. This
      // should ideally fail with a TypeError, in the same way as if only a
      // string is supplied, e.g. dataset["a"].
      .def("__getitem__",
           [](Dataset &, const Dim &) {
             throw std::runtime_error("Syntax error: cannot get slice with "
                                      "just a Dim, please use dataset[Dim.X, "
                                      ":]");
           })
      .def("__getitem__",
           [](Dataset &self, const gsl::index index) { return self[index]; },
           py::keep_alive<0, 1>())
      .def("__getitem__",
           [](Dataset &self, const std::tuple<Dim, gsl::index> &index) {
             return getItemBySingleIndex(self, index);
           },
           py::keep_alive<0, 1>())
      .def("__getitem__", &detail::pySlice<Dataset>, py::keep_alive<0, 1>())
      .def("__getitem__",
           [](Dataset &self, const Tag &tag) { return self(tag); })
      .def("__getitem__",
           [](Dataset &self, const std::pair<Tag, const std::string> &key) {
             return self(key.first, key.second);
           },
           py::keep_alive<0, 1>())
      .def("copy", [](const Dataset &self) { return self; })
      .def("__copy__", [](Dataset &self) { return Dataset(self); })
      .def("__deepcopy__",
           [](Dataset &self, py::dict) { return Dataset(self); })
      .def_property_readonly("subset",
                             [](Dataset &self) { return SubsetHelper(self); })
      // Careful: The order of overloads is really important here, otherwise
      // DatasetSlice matches the overload below for py::array_t. I have not
      // understood all details of this yet though. See also
      // https://pybind11.readthedocs.io/en/stable/advanced/functions.html#overload-resolution-order.
      .def("__setitem__",
           [](Dataset &self, const std::tuple<Dim, py::slice> &index,
              const DatasetSlice &other) {
             detail::pySlice(self, index).assign(other);
           })
      .def("__setitem__",
           [](Dataset &self, const std::tuple<Dim, gsl::index> &index,
              const DatasetSlice &other) {
             const auto & [ dim, i ] = index;
             self(dim, i).assign(other);
           })

      // A) No dimensions argument, this will set data of existing item.
      .def("__setitem__", detail::setData<Dataset, detail::Key::Tag>)
      .def("__setitem__", detail::setData<Dataset, detail::Key::TagName>)

      // B) Variants with dimensions, inserting new item.
      // 0. Insertion from Variable or Variable slice.
      .def("__setitem__", detail::insert<Variable, detail::Key::Tag>)
      .def("__setitem__", detail::insert<Variable, detail::Key::TagName>)
      .def("__setitem__", detail::insert<VariableSlice, detail::Key::Tag>)
      .def("__setitem__", detail::insert<VariableSlice, detail::Key::TagName>)
      // 1. Insertion with default init. TODO Should this be removed?
      .def("__setitem__", detail::insertDefaultInit<detail::Key::Tag>)
      .def("__setitem__", detail::insertDefaultInit<detail::Key::TagName>)
      // 2. Insertion from numpy.ndarray
      .def("__setitem__", detail::insert_ndarray<detail::Key::Tag>)
      .def("__setitem__", detail::insert_ndarray<detail::Key::TagName>)
      // 2. Handle integers before case 3. below, which would convert to double.
      .def("__setitem__", detail::insert_0D<int64_t, detail::Key::Tag>)
      .def("__setitem__", detail::insert_0D<int64_t, detail::Key::TagName>)
      .def("__setitem__", detail::insert_0D<std::string, detail::Key::Tag>)
      .def("__setitem__", detail::insert_0D<std::string, detail::Key::TagName>)
      .def("__setitem__", detail::insert_0D<Dataset, detail::Key::Tag>)
      .def("__setitem__", detail::insert_0D<Dataset, detail::Key::TagName>)
      // 3. Handle integers before case 4. below, which would convert to double.
      .def("__setitem__", detail::insert_1D<int64_t, detail::Key::Tag>)
      .def("__setitem__", detail::insert_1D<int64_t, detail::Key::TagName>)
      .def("__setitem__", detail::insert_1D<Eigen::Vector3d, detail::Key::Tag>)
      .def("__setitem__",
           detail::insert_1D<Eigen::Vector3d, detail::Key::TagName>)
      // 4. Insertion attempting forced conversion to array of double. This
      //    is handled by automatic conversion by pybind11 when using
      //    py::array_t. Handles also scalar data. See also the
      //    py::array::forcecast argument, we need to minimize implicit (and
      //    potentially expensive conversion). If we wanted to avoid some
      //    conversion we need to provide explicit variants for specific types,
      //    same as or similar to insert_1D in case 5. below.
      .def("__setitem__", detail::insert_conv<double, detail::Key::Tag>)
      .def("__setitem__", detail::insert_conv<double, detail::Key::TagName>)
      // 5. Insertion of numpy-incompatible data. py::array_t does not support
      //    non-POD types like std::string, so we need to handle them
      //    separately.
      .def("__setitem__", detail::insert_1D<std::string, detail::Key::Tag>)
      .def("__setitem__", detail::insert_1D<std::string, detail::Key::TagName>)
      .def("__setitem__", detail::insert_1D<Dataset, detail::Key::Tag>)
      .def("__setitem__", detail::insert_1D<Dataset, detail::Key::TagName>)

      // TODO Make sure we have all overloads covered to avoid implicit
      // conversion of DatasetSlice to Dataset.
      .def(py::self == py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self != py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self += py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self -= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self *= py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self + py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self - py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self * py::self, py::call_guard<py::gil_scoped_release>())
      .def(py::self + double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self - double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self * double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self / double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self += double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self -= double(), py::call_guard<py::gil_scoped_release>())
      .def(py::self *= double(), py::call_guard<py::gil_scoped_release>())
      .def("__eq__",
           [](const Dataset &self, const DatasetSlice &other) {
             return self == other;
           },
           py::call_guard<py::gil_scoped_release>())
      .def("__ne__",
           [](const Dataset &self, const DatasetSlice &other) {
             return self != other;
           },
           py::call_guard<py::gil_scoped_release>())
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
      .def("__radd__",
           [](const Dataset &self, double &other) { return self + other; },
           py::is_operator())
      .def("__rsub__",
           [](const Dataset &self, double &other) { return other - self; },
           py::is_operator())
      .def("__rmul__",
           [](const Dataset &self, double &other) { return self * other; },
           py::is_operator())
      .def("merge", &Dataset::merge)
      // TODO For now this is just for testing. We need to decide on an API for
      // specifying the keys.
      .def("zip", [](Dataset &self) {
        return zip(self, Access::Key(Data::EventTofs),
                   Access::Key(Data::EventPulseTimes));
      });

  // Implicit conversion DatasetSlice -> Dataset. Reduces need for excessive
  // operator overload definitions
  py::implicitly_convertible<DatasetSlice, Dataset>();

  //-----------------------dataset free functions-------------------------------
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
  m.def("histogram",
        py::overload_cast<const Dataset &, const Variable &>(&histogram),
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
        py::call_guard<py::gil_scoped_release>(), py::arg("dataset"),
        py::arg("dim"),
        "Returns a new dataset containing the mean of the data along the "
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

  //-----------------------variable free functions------------------------------
  m.def("split",
        py::overload_cast<const Variable &, const Dim,
                          const std::vector<gsl::index> &>(&split),
        py::call_guard<py::gil_scoped_release>());
  m.def("concatenate",
        py::overload_cast<const Variable &, const Variable &, const Dim>(
            &concatenate),
        py::call_guard<py::gil_scoped_release>());
  m.def("rebin",
        py::overload_cast<const Variable &, const Variable &, const Variable &>(
            &rebin),
        py::call_guard<py::gil_scoped_release>());
  m.def("filter",
        py::overload_cast<const Variable &, const Variable &>(&filter),
        py::call_guard<py::gil_scoped_release>());
  m.def("sum", py::overload_cast<const Variable &, const Dim>(&sum),
        py::call_guard<py::gil_scoped_release>());
  m.def("mean", py::overload_cast<const Variable &, const Dim>(&mean),
        py::call_guard<py::gil_scoped_release>());
  m.def("norm", py::overload_cast<const Variable &>(&norm),
        py::call_guard<py::gil_scoped_release>());
  // find out why py::overload_cast is not working correctly here
  m.def("sqrt", [](const Variable &self) { return sqrt(self); },
        py::call_guard<py::gil_scoped_release>());

  //-----------------------dimensions free functions----------------------------
  m.def("dimensionCoord", &dimensionCoord);
  m.def("coordDimension",
        [](const Tag t) { return coordDimension[t.value()]; });

  auto events = m.def_submodule("events");
  events.def("sort_by_tof", &events::sortByTof);
}
