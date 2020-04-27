// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#include "pybind11.h"


namespace py = pybind11;


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

using strpair = const std::pair<const std::string, const std::string>;


struct Docstring
{
  const std::string description, raises, seealso, returns, rtype;
};

// Construct docstring
std::string make_docstring(const std::string description,
  const std::string raises,
  const std::string seealso, const std::string returns, const std::string rtype,
  const std::vector<std::pair<const std::string, const std::string>> &params) {

  std::string docstring = description + "\n";
  for (auto &p : params)
    docstring += ":param " + p.first + ": " + p.second +"\n";
  // docstring += ":param " + param2.first + ": " + param2.second +"\n";
  docstring += ":raises: " + raises + "\n";
  docstring += ":seealso: " + seealso + "\n";
  docstring += ":return: " + returns + "\n";
  docstring += ":rtype: " + rtype;
  return docstring;
}

template<class T, class T1>
void bind_free_function(T (*func)(T1), const std::string fname, py::module &m,
  const strpair param1,
  const std::string description,
  const std::string raises,
  const std::string seealso,
  const std::string returns,
  const std::string rtype) {
  // Bind function
  m.def(fname.c_str(),
      [func](T1 a1){return func(a1);},
        py::call_guard<py::gil_scoped_release>(),
        make_docstring(description, raises, seealso, returns, rtype, {param1}).c_str(),
        py::arg(param1.first.c_str()));
}

// template<class T, class V, class T1>
// void bind_free_function_with_out(T (*func)(T1), std::string fname, py::module &m,
//   strpair param1,
//   const std::string description,
//   const std::string raises,
//   const std::string seealso,
//   const std::string returns,
//   const std::string rtype) {
//   bind_free_function<T, T1>(func, fname, m, param1, description, raises, seealso, returns, rtype);
//   bind_free_function<T, T1, V>(func, fname, m, param1, "out", description, raises, seealso, returns, rtype + "(View)");


// }


template<class T, class T1, class T2>
void bind_free_function(T (*func)(T1, T2), const std::string fname, py::module &m,
  const strpair param1, const strpair param2,
  const std::string description,
  const std::string raises,
  const std::string seealso,
  const std::string returns,
  const std::string rtype) {
  // Bind function
  m.def(fname.c_str(),
      [func](T1 a1, T2 a2){return func(a1, a2);},
        py::call_guard<py::gil_scoped_release>(),
        make_docstring(description, raises, seealso, returns, rtype, {param1, param2}).c_str(),
        PYARGS(param1.first, param2.first));
}

template<class T, class T1, class T2, class T3>
void bind_free_function(T (*func)(T1, T2, T3), const std::string fname, py::module &m,
  const strpair param1, const strpair param2, const strpair param3,
  const std::string description,
  const std::string raises,
  const std::string seealso,
  const std::string returns,
  const std::string rtype) {
  // Bind function
  m.def(fname.c_str(),
      [func](T1 a1, T2 a2, T3 a3){return func(a1, a2, a3);},
        py::call_guard<py::gil_scoped_release>(),
        make_docstring(description, raises, seealso, returns, rtype, {param1, param2, param3}).c_str(),
        PYARGS(param1.first, param2.first, param3.first));
}



// template<class T, class V, class T1>
// void bind_free_function_out_arg(T (*func)(T1), V (*func_out)(T1, const V&), std::string fname, py::module &m,
//   strpair param1,
//   const std::string description,
//   const std::string raises,
//   const std::string seealso,
//   const std::string returns,
//   const std::string rtype) {
//   bind_free_function<T, T1>(func, fname, m, param1, description + " (in-place)", raises, seealso, returns, rtype);
//   bind_free_function<V, T1, const V&>(func_out, fname, m, param1, {"out", "Output buffer"}, description, raises, seealso, returns, rtype + " (View)");
// }