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
std::string format(const Dimensions &dims) {
  // TODO reverse order to match numpy convention.
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

std::vector<gsl::index> numpy_shape(const Dimensions &dims) {
  std::vector<gsl::index> shape(dims.count());
  for (gsl::index i = 0; i < shape.size(); ++i)
    shape[i] = dims.size(shape.size() - 1 - i);
  return shape;
}

// Reverse strides s and add size factor.
template <class Tag>
std::vector<gsl::index> numpy_strides(const std::vector<gsl::index> &s) {
  std::vector<gsl::index> strides(s.size());
  gsl::index elemSize = sizeof(typename Tag::type);
  for (gsl::index i = strides.size() - 1; i >= 0; --i) {
    strides[i] = elemSize * s[strides.size() - 1 - i];
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
template <class Tag, class T> VariableView<Tag> getCoord(T &self, const Tag) {
  return {VariableSlice<Variable>(
      detail::makeAccess(self)[find(self, tag_id<Tag>, "")])};
}

template <class Tag, class T>
VariableView<Tag> getData(T &self,
                          const std::pair<const Tag, const std::string> &key) {
  return {VariableSlice<Variable>(
      detail::makeAccess(self)[find(self, tag_id<Tag>, key.second)])};
}

template <class Tag, class T>
void setData(T &self, const std::pair<const Tag, const std::string> &key,
             py::array_t<typename Tag::type> &data) {
  const gsl::index index = find(self, tag_id<Tag>, key.second);
  const auto &dims = self[index].dimensions();
  py::buffer_info info = data.request();
  if (info.shape != detail::numpy_shape(dims))
    throw std::runtime_error(
        "Shape mismatch when setting data from numpy array.");

  auto buf = detail::makeAccess(self)[index].template get<Tag>();
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
  if (info.shape != detail::numpy_shape(dims))
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
  if (info.shape != detail::numpy_shape(dims))
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
            self().dimensions().count(),              /* Number of dimensions */
            detail::numpy_shape(self().dimensions()), /* Buffer dimensions */
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
                detail::numpy_shape(view().dimensions()),
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
                detail::numpy_shape(view().dimensions()),
                detail::numpy_strides<Tag>(view().strides()),
                view().template get<Tag>().data(), obj} = data;
          })
      .def_property(
          "numpy_mutable",
          [](py::object &obj) {
            auto &view = obj.cast<detail::VariableView<Tag> &>();
            return py::array_t<typename Tag::type>{
                detail::numpy_shape(view().dimensions()),
                detail::numpy_strides<Tag>(view().strides()),
                view().template get<Tag>().data(), obj};
          },
          [](py::object &obj, const py::array_t<typename Tag::type> &data) {
            auto &view = obj.cast<detail::VariableView<Tag> &>();
            py::array_t<typename Tag::type>{
                detail::numpy_shape(view().dimensions()),
                detail::numpy_strides<Tag>(view().strides()),
                view().template get<Tag>().data(), obj} = data;
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
      .def("size", &Slice<Dataset>::size)
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
      .def("__getitem__", detail::getCoord<Coord::X, Slice<Dataset>>)
      .def("__getitem__", detail::getCoord<Coord::Y, Slice<Dataset>>)
      .def("__getitem__", detail::getCoord<Coord::Z, Slice<Dataset>>)
      .def("__getitem__", detail::getData<Data::Value, Slice<Dataset>>)
      .def("__setitem__", detail::setData<Data::Value, Slice<Dataset>>);

  py::class_<Dataset>(m, "Dataset")
      .def(py::init<>())
      .def("__len__", &Dataset::size)
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
      .def("__getitem__", detail::getCoord<Coord::X, Dataset>)
      .def("__getitem__", detail::getCoord<Coord::Y, Dataset>)
      .def("__getitem__", detail::getCoord<Coord::Z, Dataset>)
      .def("__getitem__", detail::getData<Data::Value, Dataset>)
      .def("__getitem__",
           [](Dataset &self, const std::string &name) { return self[name]; })
      .def("__setitem__", detail::setData<Data::Value, Dataset>)
      .def("__setitem__", detail::insertCoord<Coord::X>)
      .def("__setitem__", detail::insertCoord<Coord::Y>)
      .def("__setitem__", detail::insertCoord<Coord::Z>)
      .def("__setitem__", detail::insert<Data::Value>)
      .def("__setitem__", detail::insert<Data::Variance>)
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
