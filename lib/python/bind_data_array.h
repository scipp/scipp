// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/dataset/dataset.h"
#include "scipp/variable/variable_factory.h"

#include "bind_operators.h"
#include "pybind11.h"
#include "view.h"

namespace py = pybind11;
using namespace scipp;

template <template <class> class View, class T>
void bind_helper_view(py::module &m, const std::string &name) {
  std::string suffix;
  if (std::is_same_v<View<T>, items_view<T>> ||
      std::is_same_v<View<T>, str_items_view<T>>)
    suffix = "_items_view";
  if (std::is_same_v<View<T>, values_view<T>>)
    suffix = "_values_view";
  if (std::is_same_v<View<T>, keys_view<T>> ||
      std::is_same_v<View<T>, str_keys_view<T>>)
    suffix = "_keys_view";
  py::class_<View<T>>(m, (name + suffix).c_str())
      .def(py::init([](T &obj) { return View{obj}; }))
      .def("__len__", &View<T>::size)
      .def(
          "__iter__",
          [](const View<T> &self) {
            return py::make_iterator(self.begin(), self.end(),
                                     py::return_value_policy::move);
          },
          py::return_value_policy::move, py::keep_alive<0, 1>());
}

template <class Other, class T, class... Ignored>
void bind_common_mutable_view_operators(pybind11::class_<T, Ignored...> &view) {
  view.def("__len__", &T::size)
      .def(
          "__getitem__",
          [](const T &self, const typename T::key_type &key) {
            return self[key];
          },
          py::return_value_policy::copy)
      .def("__setitem__", [](T &self, const typename T::key_type key,
                             const Variable &var) { self.set(key, var); })
      .def("__delitem__", &T::erase, py::call_guard<py::gil_scoped_release>())
      .def(
          "values", [](T &self) { return values_view(self); },
          py::keep_alive<0, 1>(), R"(view on self's values)")
      .def("__contains__", &T::contains);
}

template <class T, class... Ignored>
void bind_pop(pybind11::class_<T, Ignored...> &view) {
  view.def(
      "_pop",
      [](T &self, const typename T::key_type &key) {
        return py::cast(self.extract(key));
      },
      py::arg("k"));
}

template <class T>
void bind_mutable_view(py::module &m, const std::string &name) {
  py::class_<T> view(m, name.c_str());
  bind_common_mutable_view_operators<T>(view);
  bind_inequality_to_operator<T>(view);
  bind_pop(view);
  view.def(
          "__iter__",
          [](const T &self) {
            return py::make_iterator(self.keys_begin(), self.keys_end(),
                                     py::return_value_policy::move);
          },
          py::keep_alive<0, 1>())
      .def(
          "keys", [](T &self) { return keys_view(self); },
          py::keep_alive<0, 1>(), R"(view on self's keys)")
      .def(
          "items", [](T &self) { return items_view(self); },
          py::return_value_policy::move, py::keep_alive<0, 1>(),
          R"(view on self's items)")
      .def("_ipython_key_completions_", [](const T &self) {
        py::list out;
        const auto end = self.keys_end();
        for (auto it = self.keys_begin(); it != end; ++it) {
          out.append(*it);
        }
        return out;
      });
}

template <class T>
void bind_mutable_view_no_dim(py::module &m, const std::string &name) {
  py::class_<T> view(m, name.c_str());
  bind_common_mutable_view_operators<T>(view);
  bind_inequality_to_operator<T>(view);
  bind_pop(view);
  view.def(
          "__iter__",
          [](T &self) {
            auto keys_view = str_keys_view(self);
            return py::make_iterator(keys_view.begin(), keys_view.end(),
                                     py::return_value_policy::move);
          },
          py::keep_alive<0, 1>())
      .def(
          "keys", [](T &self) { return str_keys_view(self); },
          py::keep_alive<0, 1>(), R"(view on self's keys)")
      .def(
          "items", [](T &self) { return str_items_view(self); },
          py::return_value_policy::move, py::keep_alive<0, 1>(),
          R"(view on self's items)")
      .def("_ipython_key_completions_", [](const T &self) {
        py::list out;
        const auto end = self.keys_end();
        for (auto it = self.keys_begin(); it != end; ++it) {
          out.append(it->name());
        }
        return out;
      });
}

template <class T, class... Ignored>
void bind_data_array_properties(py::class_<T, Ignored...> &c) {
  if constexpr (std::is_same_v<T, DataArray>)
    c.def_property("name", &T::name, &T::setName,
                   R"(The name of the held data.)");
  else
    c.def_property_readonly("name", &T::name, R"(The name of the held data.)");
  c.def_property(
      "data",
      py::cpp_function([](T &self) { return self.data(); },
                       py::return_value_policy::copy),
      [](T &self, const Variable &data) { self.setData(data); },
      R"(Underlying data item.)");
  c.def_property_readonly(
      "coords", [](T &self) -> decltype(auto) { return self.coords(); },
      R"(Dict of aligned coords.)");
  c.def_property_readonly(
      "meta", [](T &self) -> decltype(auto) { return self.meta(); },
      R"(Dict of coords and attrs.)");
  c.def_property_readonly(
      "attrs", [](T &self) -> decltype(auto) { return self.attrs(); },
      R"(Dict of attrs.)");
  c.def_property_readonly(
      "masks", [](T &self) -> decltype(auto) { return self.masks(); },
      R"(Dict of masks.)");
}
