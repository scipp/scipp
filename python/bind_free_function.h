// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#pragma once

#include "docstring.h"
#include "pybind11.h"


namespace py = pybind11;

// Helper to parse py::args.
// The goal here is to take in a list of strings as arguments, e.g. ("x", "y")
// and generate py::arg("x"), py::arg("y")

// Make a FOREACH macro: see https://www.reddit.com/r/C_Programming/comments/3dfu5w/is_it_possible_to_modify_each_argument_in_a/
#define FE_1(WHAT, X) WHAT(X)
#define FE_2(WHAT, X, ...) WHAT(X),FE_1(WHAT, __VA_ARGS__)
#define FE_3(WHAT, X, ...) WHAT(X),FE_2(WHAT, __VA_ARGS__)
#define FE_4(WHAT, X, ...) WHAT(X),FE_3(WHAT, __VA_ARGS__)
#define FE_5(WHAT, X, ...) WHAT(X),FE_4(WHAT, __VA_ARGS__)
//... repeat as needed
#define GET_MACRO(_1,_2,_3,_4,_5,NAME,...) NAME
#define FOR_EACH(action,...) \
  GET_MACRO(__VA_ARGS__,FE_5,FE_4,FE_3,FE_2,FE_1)(action,__VA_ARGS__)

#define PYARG_FORMAT(IN) py::arg(IN.c_str())
#define PYARGS(...) FOR_EACH(PYARG_FORMAT, __VA_ARGS__)

namespace scipp::python {

template<class T, class T1>
void bind_free_function(T (*func)(T1), const std::string fname, py::module &m,
  const Docstring docs) {
  m.def(fname.c_str(),
      [func](T1 a1){return func(a1);},
        py::call_guard<py::gil_scoped_release>(),
        docs.to_string().c_str(),
        py::arg(docs.param(0).first.c_str()));
}

template<class T, class T1>
void bind_free_function(T (*func)(T1), const std::string fname, py::module &m,
  const strpair param1,
  const std::string description,
  const std::string raises,
  const std::string seealso,
  const std::string returns,
  const std::string rtype) {
  bind_free_function<T, T1>(func, fname, m, Docstring(description, raises, seealso, returns, rtype, {param1}));
}

template<class T, class T1, class T2>
void bind_free_function(T (*func)(T1, T2), const std::string fname, py::module &m,
  const Docstring docs) {
  m.def(fname.c_str(),
      [func](T1 a1, T2 a2){return func(a1, a2);},
        py::call_guard<py::gil_scoped_release>(),
        docs.to_string().c_str(),
        PYARGS(docs.param(0).first, docs.param(1).first));
}

template<class T, class T1, class T2>
void bind_free_function(T (*func)(T1, T2), const std::string fname, py::module &m,
  const strpair param1, const strpair param2,
  const std::string description,
  const std::string raises,
  const std::string seealso,
  const std::string returns,
  const std::string rtype) {
  bind_free_function<T, T1, T2>(func, fname, m, Docstring(description, raises, seealso, returns, rtype, {param1, param2}));
}

template<class T, class T1, class T2, class T3>
void bind_free_function(T (*func)(T1, T2, T3), const std::string fname, py::module &m,
  const Docstring docs) {
  m.def(fname.c_str(),
      [func](T1 a1, T2 a2, T3 a3){return func(a1, a2, a3);},
        py::call_guard<py::gil_scoped_release>(),
        docs.to_string().c_str(),
        PYARGS(docs.param(0).first, docs.param(1).first, docs.param(2).first));
}

template<class T, class T1, class T2, class T3>
void bind_free_function(T (*func)(T1, T2, T3), const std::string fname, py::module &m,
  const strpair param1, const strpair param2, const strpair param3,
  const std::string description,
  const std::string raises,
  const std::string seealso,
  const std::string returns,
  const std::string rtype) {
  bind_free_function<T, T1, T2, T3>(func, fname, m, Docstring(description, raises, seealso, returns, rtype, {param1, param2, param3}));
}

template<class T, class T1, class T2, class T3, class T4>
void bind_free_function(T (*func)(T1, T2, T3, T4), const std::string fname, py::module &m,
  const Docstring docs) {
  m.def(fname.c_str(),
      [func](T1 a1, T2 a2, T3 a3, T4 a4){return func(a1, a2, a3, a4);},
        py::call_guard<py::gil_scoped_release>(),
        docs.to_string().c_str(),
        PYARGS(docs.param(0).first, docs.param(1).first, docs.param(2).first, docs.param(3).first));
}

template<class T, class T1, class T2, class T3, class T4>
void bind_free_function(T (*func)(T1, T2, T3, T4), const std::string fname, py::module &m,
  const strpair param1, const strpair param2, const strpair param3, const strpair param4,
  const std::string description,
  const std::string raises,
  const std::string seealso,
  const std::string returns,
  const std::string rtype) {
  bind_free_function<T, T1, T2, T3, T4>(func, fname, m, Docstring(description, raises, seealso, returns, rtype, {param1, param2, param3, param4}));
}



} // namespace scipp::python
