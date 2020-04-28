// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#pragma once

#include <tuple>


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

using strpair = const std::pair<const std::string, const std::string>;


class Docstring {

public:

  Docstring() = default;
  Docstring(const std::string description, const std::string raises, const std::string seealso, const std::string returns, const std::string rtype);
  Docstring(const std::string description, const std::string raises, const std::string seealso, const std::string returns, const std::string rtype, const std::vector<std::pair<std::string, std::string>> &params);
  const std::string description() {return m_description; };
  const std::string raises() {return m_raises; };
  const std::string seealso() {return m_seealso; };
  const std::string returns() {return m_returns; };
  const std::string rtype() {return m_rtype; };

private:
  std::string m_description, m_raises, m_seealso, m_returns, m_rtype;
  std::vector<std::pair<std::string, std::string>> params;

};

void docstring_with_out_arg(const Docstring &docs);

std::string make_docstring(const Docstring docs,
  const std::vector<std::pair<const std::string, const std::string>> &params);

template<class T, class T1>
void bind_free_function(T (*func)(T1), const std::string fname, py::module &m,
  const strpair param1,
  const std::string description,
  const std::string raises,
  const std::string seealso,
  const std::string returns,
  const std::string rtype) {
  m.def(fname.c_str(),
      [func](T1 a1){return func(a1);},
        py::call_guard<py::gil_scoped_release>(),
        make_docstring(description, raises, seealso, returns, rtype, {param1}).c_str(),
        py::arg(param1.first.c_str()));
}

template<class T, class T1>
void bind_free_function(T (*func)(T1), const std::string fname, py::module &m,
  const strpair param1,
  const Docstring docs) {
  bind_free_function<T, T1>(func, fname, m, param1, docs.description, docs.raises, docs.seealso, docs.returns, docs.rtype);
}


template<class T, class T1, class T2>
void bind_free_function(T (*func)(T1, T2), const std::string fname, py::module &m,
  const strpair param1, const strpair param2,
  const std::string description,
  const std::string raises,
  const std::string seealso,
  const std::string returns,
  const std::string rtype) {
  m.def(fname.c_str(),
      [func](T1 a1, T2 a2){return func(a1, a2);},
        py::call_guard<py::gil_scoped_release>(),
        make_docstring(description, raises, seealso, returns, rtype, {param1, param2}).c_str(),
        PYARGS(param1.first, param2.first));
}

template<class T, class T1, class T2>
void bind_free_function(T (*func)(T1, T2), const std::string fname, py::module &m,
  const strpair param1,
  const strpair param2,
  const Docstring docs) {
  bind_free_function<T, T1, T2>(func, fname, m, param1, param2, docs.description, docs.raises, docs.seealso, docs.returns, docs.rtype);
}

template<class T, class T1, class T2, class T3>
void bind_free_function(T (*func)(T1, T2, T3), const std::string fname, py::module &m,
  const strpair param1, const strpair param2, const strpair param3,
  const std::string description,
  const std::string raises,
  const std::string seealso,
  const std::string returns,
  const std::string rtype) {
  m.def(fname.c_str(),
      [func](T1 a1, T2 a2, T3 a3){return func(a1, a2, a3);},
        py::call_guard<py::gil_scoped_release>(),
        make_docstring(description, raises, seealso, returns, rtype, {param1, param2, param3}).c_str(),
        PYARGS(param1.first, param2.first, param3.first));
}

template<class T, class T1, class T2, class T3>
void bind_free_function(T (*func)(T1, T2, T3), const std::string fname, py::module &m,
  const strpair param1,
  const strpair param2,
  const strpair param3,
  const Docstring docs) {
  bind_free_function<T, T1, T2, T3>(func, fname, m, param1, param2, param3, docs.description, docs.raises, docs.seealso, docs.returns, docs.rtype);
}

template<class T, class T1, class T2, class T3, class T4>
void bind_free_function(T (*func)(T1, T2, T3, T4), const std::string fname, py::module &m,
  const strpair param1, const strpair param2, const strpair param3, const strpair param4,
  const std::string description,
  const std::string raises,
  const std::string seealso,
  const std::string returns,
  const std::string rtype) {
  m.def(fname.c_str(),
      [func](T1 a1, T2 a2, T3 a3, T4 a4){return func(a1, a2, a3, a4);},
        py::call_guard<py::gil_scoped_release>(),
        make_docstring(description, raises, seealso, returns, rtype, {param1, param2, param3, param4}).c_str(),
        PYARGS(param1.first, param2.first, param3.first, param4.first));
}

template<class T, class T1, class T2, class T3, class T4>
void bind_free_function(T (*func)(T1, T2, T3, T4), const std::string fname, py::module &m,
  const strpair param1,
  const strpair param2,
  const strpair param3,
  const strpair param4,
  const Docstring docs) {
  bind_free_function<T, T1, T2, T3, T4>(func, fname, m, param1, param2, param3, param4, docs.description, docs.raises, docs.seealso, docs.returns, docs.rtype);
}


// template<class... Ts>
// void bind_free_function(T (*func)(T1), const std::string fname, py::module &m,



//   const strpair param1,
//   const std::string description,
//   const std::string raises,
//   const std::string seealso,
//   const std::string returns,
//   const std::string rtype) {
//   m.def(fname.c_str(),
//       [func](T1 a1){return func(a1);},
//         py::call_guard<py::gil_scoped_release>(),
//         make_docstring(description, raises, seealso, returns, rtype, {param1}).c_str(),
//         py::arg(param1.first.c_str()));
// }







// template <Objs..., Ts...>
// void bind_func(m) {
//   // (f(args), ...);
//   (bind_free_function<T, Ts>(
//     // Operation function pointer
//     dot,
//     // Operation python name
//     "dot",
//     // py::module
//     m,
//     // Input parameters
//     {"x", "Left operand Variable, DataArray, or Dataset."},
//     {"y", "Right operand Variable, DataArray, or Dataset."},
//     // Description
//     "Element-wise dot-product.",
//     // Raises
//     "If the dtype is not a vector such as :py:class:`scipp.dtype.vector_3_double.`",
//     // See also
//     "",
//     // Returns
//     "Variable, data array, or dataset with scalar elements based on the two inputs.",
//     // Return type
//     "Variable, DataArray, or Dataset.");
// }

// int main(int, char* [])
// {
//     function( std::tuple{ First<4>{}, First<3>{} },
//           std::tuple{ Second<1>{}, Second<4>{} });
// }


// // template<class T>
// // X bind_forward(T const& t){ X x; f(x, t); return x; }

// // template<class... Args>
// // void bind_functions(Args... args){
// //   { fw(args)... };
// //   // g(xs, sizeof...(Args));
// // }

} // namespace scipp::python
