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
};

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

template <class Tag> struct VariableView {
  const VariableSlice<Variable> &operator()() const { return m_view; }
  VariableSlice<Variable> &operator()() { return m_view; }
  VariableView operator()(const std::tuple<Dim, const py::slice> &index) const {
    auto slice(*this);
    const Dim dim = std::get<Dim>(index);
    const auto indices = std::get<const py::slice>(index);
    size_t start, stop, step, slicelength;
    const auto size = slice().dimensions().size(dim);
    if (!indices.compute(size, &start, &stop, &step, &slicelength))
      throw py::error_already_set();
    if (step != 1)
      throw std::runtime_error("Step must be 1");
    slice() = slice()(dim, start, stop);
    return slice;
  }

  VariableSlice<Variable> m_view;
};

// The way the Python exports are written we require non-const references to
// variables. Note that setting breaking attributes is not exported, so we
// should be safe.
template <class Tag, class T>
VariableView<typename Tag::type> getCoord(T &self, const Tag) {
  return {VariableSlice<Variable>(
      detail::makeAccess(self)[find(self, tag_id<typename Tag::type>, "")])};
}

template <class Tag, class T>
VariableView<typename Tag::type>
getData(T &self, const std::pair<const Tag, const std::string> &key) {
  return {VariableSlice<Variable>(detail::makeAccess(
      self)[find(self, tag_id<typename Tag::type>, key.second)])};
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

template <class Tag>
void setVariableSlice(const VariableView<Tag> &self,
                      const std::tuple<Dim, gsl::index> &index,
                      py::array_t<typename Tag::type> &data) {
  auto slice(self);
  slice() = slice()(std::get<Dim>(index), std::get<gsl::index>(index));

  const auto &dims = slice().dimensions();
  py::buffer_info info = data.request();
  const auto &shape = dims.shape();
  if (!std::equal(info.shape.begin(), info.shape.end(), shape.begin(),
                  shape.end()))
    throw std::runtime_error(
        "Shape mismatch when setting data from numpy array.");

  auto buf = slice().template get<Tag>();
  double *ptr = (double *)info.ptr;
  std::copy(ptr, ptr + dims.volume(), buf.begin());
}

template <class Tag>
void setVariableSliceRange(const VariableView<Tag> &self,
                           const std::tuple<Dim, const py::slice> &index,
                           py::array_t<typename Tag::type> &data) {
  auto slice = self(index);

  const auto &dims = slice().dimensions();
  py::buffer_info info = data.request();
  const auto &shape = dims.shape();
  if (!std::equal(info.shape.begin(), info.shape.end(), shape.begin(),
                  shape.end()))
    throw std::runtime_error(
        "Shape mismatch when setting data from numpy array.");

  auto buf = slice().template get<Tag>();
  double *ptr = (double *)info.ptr;
  std::copy(ptr, ptr + dims.volume(), buf.begin());
}
} // namespace detail

template <class Tag>
void declare_VariableView(py::module &m, const std::string &suffix) {
  py::class_<detail::VariableView<Tag>>(
      m, (std::string("VariableView_") + suffix).c_str(), py::buffer_protocol())
      .def_buffer([](detail::VariableView<Tag> &self) {
        // Note: Currently this always triggers copy-on-write ---
        // py::buffer_info does currently not support the `readonly` flag of the
        // Python buffer protocol. We can probably get this fixed upstream, see
        // discussion and sample implementation here:
        // https://github.com/pybind/pybind11/issues/863.
        return py::buffer_info(
            self().template get<Tag>().data(), /* Pointer to buffer */
            sizeof(typename Tag::type),        /* Size of one scalar */
            py::format_descriptor<typename Tag::type>::format(), /* Python
                                                       struct-style format
                                                       descriptor */
            self().dimensions().count(), /* Number of dimensions */
            self().dimensions().shape(), /* Buffer dimensions */
            detail::numpy_strides<Tag>(
                self().strides()) /* Strides (in bytes) for each index */
        );
      })
      // Careful: Do not expose setName, setDimensions, and assignment,
      // otherwise we can break the dataset.
      .def_property_readonly("dimensions",
                             [](const detail::VariableView<Tag> &self) {
                               return self().dimensions();
                             })
      .def_property_readonly("is_coord",
                             [](const detail::VariableView<Tag> &self) {
                               return self().isCoord();
                             })
      .def_property_readonly(
          "type",
          [](const detail::VariableView<Tag> &self) { return self().type(); })
      .def_property_readonly(
          "name",
          [](const detail::VariableView<Tag> &self) { return self().name(); })
      .def("__getitem__",
           [](const detail::VariableView<Tag> &self,
              const std::tuple<Dim, gsl::index> &index) {
             auto slice(self);
             slice() =
                 slice()(std::get<Dim>(index), std::get<gsl::index>(index));
             return slice;
           })
      .def("__getitem__",
           [](const detail::VariableView<Tag> &self,
              const std::tuple<Dim, const py::slice> &index) {
             return self(index);
           })
      .def("__getitem__",
           [](const detail::VariableView<Tag> &self,
              const std::map<Dimension, const gsl::index> d) {
             auto slice(self);
             for (auto item : d)
               slice() = slice()(item.first, item.second);
             return slice;
           })
      .def("__getitem__",
           [](const detail::VariableView<Tag> &self,
              const std::map<Dimension, const py::slice> d) {
             auto slice(self);
             for (auto item : d)
               slice = slice(item);
             return slice;
           })
      .def("__setitem__", &detail::setVariableSlice<Tag>)
      .def("__setitem__", &detail::setVariableSliceRange<Tag>)
      .def_property(
          "numpy",
          [](py::object &obj) {
            auto &view = obj.cast<detail::VariableView<Tag> &>();
            auto array = py::array_t<typename Tag::type>{
                view().dimensions().shape(),
                detail::numpy_strides<Tag>(view().strides()),
                view().template get<const Tag>().data(), obj};
            // See https://github.com/pybind/pybind11/issues/481.
            reinterpret_cast<py::detail::PyArray_Proxy *>(array.ptr())->flags &=
                ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
            return array;
          },
          [](py::object &obj, const py::array_t<typename Tag::type> &data) {
            auto &view = obj.cast<detail::VariableView<Tag> &>();
            py::array_t<typename Tag::type>{
                view().dimensions().shape(),
                detail::numpy_strides<Tag>(view().strides()),
                view().template get<Tag>().data(), obj} = data;
          })
      .def_property(
          "numpy_mutable",
          [](py::object &obj) {
            auto &view = obj.cast<detail::VariableView<Tag> &>();
            return py::array_t<typename Tag::type>{
                view().dimensions().shape(),
                detail::numpy_strides<Tag>(view().strides()),
                view().template get<Tag>().data(), obj};
          },
          [](py::object &obj, const py::array_t<typename Tag::type> &data) {
            auto &view = obj.cast<detail::VariableView<Tag> &>();
            py::array_t<typename Tag::type>{
                view().dimensions().shape(),
                detail::numpy_strides<Tag>(view().strides()),
                view().template get<Tag>().data(), obj} = data;
          });
}

PYBIND11_MODULE(dataset, m) {
  py::enum_<Dimension>(m, "Dim")
      .value("X", Dimension::X)
      .value("Y", Dimension::Y)
      .value("Z", Dimension::Z);

  py::class_<Tag>(m, "Tag");

  auto data_tags = m.def_submodule("Data");
  py::class_<detail::PythonData::Value, Tag>(data_tags, "_Value");
  py::class_<detail::PythonData::Variance, Tag>(data_tags, "_Variance");
  data_tags.attr("Value") = detail::PythonData::Value{};
  data_tags.attr("Variance") = detail::PythonData::Variance{};

  auto coord_tags = m.def_submodule("Coord");
  py::class_<detail::PythonCoord::X, Tag>(coord_tags, "_X");
  py::class_<detail::PythonCoord::Y, Tag>(coord_tags, "_Y");
  py::class_<detail::PythonCoord::Z, Tag>(coord_tags, "_Z");
  coord_tags.attr("X") = detail::PythonCoord::X{};
  coord_tags.attr("Y") = detail::PythonCoord::Y{};
  coord_tags.attr("Z") = detail::PythonCoord::Z{};

  declare_span<double>(m, "double");
  declare_span<const double>(m, "double_const");
  declare_span<const std::string>(m, "string_const");

  py::class_<Dimensions>(m, "Dimensions")
      .def(py::init<>())
      .def("__repr__", &detail::format)
      .def("__len__", &Dimensions::count)
      .def_property_readonly("labels", &Dimensions::labels)
      .def("add", &Dimensions::add)
      .def("size",
           py::overload_cast<const Dimension>(&Dimensions::size, py::const_));

  declare_VariableView<Data::Value>(m, "DataValue");
  declare_VariableView<Coord::X>(m, "CoordX");
  declare_VariableView<Coord::Y>(m, "CoordY");
  declare_VariableView<Coord::Z>(m, "CoordZ");

  py::class_<Variable>(m, "Variable");
  py::class_<Slice<Dataset>>(m, "DatasetView")
      .def("__len__", &Slice<Dataset>::size)
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
           detail::getData<detail::PythonData::Value, Slice<Dataset>>)
      .def("__setitem__",
           detail::setData<detail::PythonData::Value, Slice<Dataset>>);

  py::class_<Dataset>(m, "Dataset")
      .def(py::init<>())
      .def("__len__", &Dataset::size)
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
              const std::tuple<const Tag, const std::string> &key) {
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
      .def("__getitem__", detail::getData<detail::PythonData::Value, Dataset>)
      .def("__getitem__",
           [](Dataset &self, const std::string &name) { return self[name]; })
      .def("__setitem__", detail::setData<detail::PythonData::Value, Dataset>)
      .def("__setitem__", detail::insertCoord<detail::PythonCoord::X>)
      .def("__setitem__", detail::insertCoord<detail::PythonCoord::Y>)
      .def("__setitem__", detail::insertCoord<detail::PythonCoord::Z>)
      .def("__setitem__", detail::insert<detail::PythonData::Value>)
      .def("__setitem__", detail::insert<detail::PythonData::Variance>)
      .def("__setitem__", detail::insertDefaultInit<detail::PythonData::Value>)
      .def("__setitem__",
           detail::insertDefaultInit<detail::PythonData::Variance>)
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
          py::call_guard<py::gil_scoped_release>());
  m.def("concatenate",
        py::overload_cast<const Dataset &, const Dataset &, const Dimension>(
            &concatenate),
        py::call_guard<py::gil_scoped_release>());
  m.def(
      "sort",
      py::overload_cast<const Dataset &, const Tag, const std::string &>(&sort),
      py::arg("dataset"), py::arg("tag"), py::arg("name") = "",
      py::call_guard<py::gil_scoped_release>());
  m.def("filter", py::overload_cast<const Dataset &, const Variable &>(&filter),
        py::call_guard<py::gil_scoped_release>());
}
