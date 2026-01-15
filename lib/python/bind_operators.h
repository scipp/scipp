// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/dataset/arithmetic.h"
#include "scipp/dataset/astype.h"
#include "scipp/dataset/generated_comparison.h"
#include "scipp/dataset/generated_logical.h"
#include "scipp/dataset/generated_math.h"
#include "scipp/dataset/to_unit.h"
#include "scipp/dataset/util.h"
#include "scipp/units/except.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/astype.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/logical.h"
#include "scipp/variable/pow.h"
#include "scipp/variable/to_unit.h"

#include "dtype.h"
#include "format.h"
#include "pybind11.h"

namespace py = pybind11;

template <class T, class... Ignored>
void bind_common_operators(pybind11::class_<T, Ignored...> &c) {
  c.def("__abs__", [](const T &self) { return abs(self); });
  c.def("__repr__", [](const T &self) { return to_string(self); });
  c.def("__bool__", [](const T &self) {
    if constexpr (std::is_same_v<T, scipp::Variable>) {
      if (self.unit() != scipp::sc_units::none)
        throw scipp::except::UnitError(
            "The truth value of a variable with unit is undefined.");
      return self.template value<bool>() == true;
    }
    throw std::runtime_error("The truth value of a variable, data array, or "
                             "dataset is ambiguous. Use any() or all().");
  });
  c.def(
      "copy",
      [](const T &self, const bool deep) { return deep ? copy(self) : self; },
      py::arg("deep") = true, py::call_guard<py::gil_scoped_release>(),
      R"(
      Return a (by default deep) copy.

      If `deep=True` (the default), a deep copy is made. Otherwise, a shallow
      copy is made, and the returned data (and meta data) values are new views
      of the data and meta data values of this object.

      Examples
      --------

        >>> import scipp as sc
        >>> var = sc.array(dims=['x'], values=[1.0, 2.0, 3.0], unit='m')
        >>> var_copy = var.copy()
        >>> var_copy.values[0] = 999.0
        >>> var  # Original unchanged
        <scipp.Variable> (x: 3)    float64              [m]  [1, 2, 3]
)");
  c.def(
      "__copy__", [](const T &self) { return self; },
      py::call_guard<py::gil_scoped_release>(), "Return a (shallow) copy.");
  c.def(
       "__deepcopy__",
       [](const T &self, const py::typing::Dict<py::object, py::object> &) {
         return copy(self);
       },
       py::call_guard<py::gil_scoped_release>(), "Return a (deep) copy.")
      .def(
          "__sizeof__",
          [](const T &self) {
            return size_of(self, scipp::SizeofTag::ViewOnly);
          },
          R"doc(Return the size of the object in bytes.

The size includes the object itself and all arrays contained in it.
But arrays may be counted multiple times if components share buffers,
e.g. multiple coordinates referencing the same memory.
Conversely, the size may be underestimated. Especially, but not only,
with dtype=PyObject.

This function only includes memory of the current slice. Use
``underlying_size`` to get the full memory size of the underlying structure.)doc")
      .def(
          "underlying_size",
          [](const T &self) {
            return size_of(self, scipp::SizeofTag::Underlying);
          },
          R"doc(Return the size of the object in bytes.

The size includes the object itself and all arrays contained in it.
But arrays may be counted multiple times if components share buffers,
e.g. multiple coordinates referencing the same memory.
Conversely, the size may be underestimated. Especially, but not only,
with dtype=PyObject.

This function includes all memory of the underlying buffers. Use
``__sizeof__`` to get the size of the current slice only.)doc");
}

template <class T, class... Ignored>
void bind_astype(py::class_<T, Ignored...> &c) {
  c.def(
      "astype",
      [](const T &self, const py::object &type, const bool copy) {
        const auto [scipp_dtype, dtype_unit] =
            cast_dtype_and_unit(type, DefaultUnit{});
        if (dtype_unit.has_value() &&
            (dtype_unit != scipp::sc_units::one && dtype_unit != self.unit())) {
          throw scipp::except::UnitError(scipp::python::format(
              "Conversion of units via the dtype is not allowed. Occurred when "
              "trying to change dtype from ",
              self.dtype(), " to ", type,
              ". Use to_unit in combination with astype."));
        }
        [[maybe_unused]] py::gil_scoped_release release;
        return astype(self, scipp_dtype,
                      copy ? scipp::CopyPolicy::Always
                           : scipp::CopyPolicy::TryAvoid);
      },
      py::arg("type"), py::kw_only(), py::arg("copy") = true,
      R"(
        Converts a Variable or DataArray to a different dtype.

        If the dtype is unchanged and ``copy`` is `False`, the object
        is returned without making a deep copy.

        :param type: Target dtype.
        :param copy: If `False`, return the input object if possible.
                     If `True`, the function always returns a new object.
        :raises: If the data cannot be converted to the requested dtype.
        :return: New variable or data array with specified dtype.
        :rtype: Union[scipp.Variable, scipp.DataArray]

        Examples
        --------

        Convert integers to floating-point:

          >>> import scipp as sc
          >>> var = sc.array(dims=['x'], values=[1, 2, 3])
          >>> var.dtype
          DType('int64')
          >>> var.astype('float64')
          <scipp.Variable> (x: 3)    float64  [dimensionless]  [1, 2, 3]

        Convert floats to integers (truncates toward zero):

          >>> sc.array(dims=['x'], values=[1.7, 2.3, -0.8]).astype('int64')
          <scipp.Variable> (x: 3)      int64  [dimensionless]  [1, 2, 0]

        With ``copy=False``, avoid unnecessary copies when dtype is unchanged:

          >>> var_f64 = sc.array(dims=['x'], values=[1.0, 2.0], dtype='float64')
          >>> var_f64.astype('float64', copy=False).dtype
          DType('float64')
)");
}

template <class Other, class T, class... Ignored>
void bind_inequality_to_operator(pybind11::class_<T, Ignored...> &c) {
  c.def(
      "__eq__", [](const T &a, const Other &b) { return a == b; },
      py::is_operator(), py::call_guard<py::gil_scoped_release>());
  c.def(
      "__ne__", [](const T &a, const Other &b) { return a != b; },
      py::is_operator(), py::call_guard<py::gil_scoped_release>());
}

struct Identity {
  template <class T> const T &operator()(const T &x) const noexcept {
    return x;
  }
};
struct ScalarToVariable {
  template <class T> scipp::Variable operator()(const T &x) const noexcept {
    return x * scipp::sc_units::one;
  }
};

template <class RHSSetup> struct OpBinder {
  template <class Other, class T, class... Ignored>
  static void in_place_binary(pybind11::class_<T, Ignored...> &c) {
    using namespace scipp;
    // In-place operators return py::object due to the way in-place operators
    // work in Python (assigning return value to this). This avoids extra
    // copies, and additionally ensures that all references to the object keep
    // referencing the same object after the operation.
    // WARNING: It is crucial to explicitly return 'py::object &' here.
    // Otherwise the py::object is returned by value, which increments the
    // reference count, which is not only suboptimal but also incorrect since
    // we have released the GIL via py::gil_scoped_release.
    c.def(
        "__iadd__",
        [](py::object &a, Other &b) -> py::object & {
          a.cast<T &>() += RHSSetup{}(b);
          return a;
        },
        py::is_operator(), py::call_guard<py::gil_scoped_release>());
    c.def(
        "__isub__",
        [](py::object &a, Other &b) -> py::object & {
          a.cast<T &>() -= RHSSetup{}(b);
          return a;
        },
        py::is_operator(), py::call_guard<py::gil_scoped_release>());
    c.def(
        "__imul__",
        [](py::object &a, Other &b) -> py::object & {
          a.cast<T &>() *= RHSSetup{}(b);
          return a;
        },
        py::is_operator(), py::call_guard<py::gil_scoped_release>());
    c.def(
        "__itruediv__",
        [](py::object &a, Other &b) -> py::object & {
          a.cast<T &>() /= RHSSetup{}(b);
          return a;
        },
        py::is_operator(), py::call_guard<py::gil_scoped_release>());
    if constexpr (!(std::is_same_v<T, Dataset> ||
                    std::is_same_v<Other, Dataset>)) {
      c.def(
          "__imod__",
          [](py::object &a, Other &b) -> py::object & {
            a.cast<T &>() %= RHSSetup{}(b);
            return a;
          },
          py::is_operator(), py::call_guard<py::gil_scoped_release>());
      c.def(
          "__ifloordiv__",
          [](py::object &a, Other &b) -> py::object & {
            floor_divide_equals(a.cast<T &>(), RHSSetup{}(b));
            return a;
          },
          py::is_operator(), py::call_guard<py::gil_scoped_release>());
      if constexpr (!(std::is_same_v<T, DataArray> ||
                      std::is_same_v<Other, DataArray>)) {
        c.def(
            "__ipow__",
            [](T &base, Other &exponent) -> T & {
              return pow(base, RHSSetup{}(exponent), base);
            },
            py::is_operator(), py::call_guard<py::gil_scoped_release>());
      }
    }
  }

  template <class Other, class T, class... Ignored>
  static void binary(pybind11::class_<T, Ignored...> &c) {
    using namespace scipp;
    c.def(
        "__add__", [](const T &a, const Other &b) { return a + RHSSetup{}(b); },
        py::is_operator(), py::call_guard<py::gil_scoped_release>());
    c.def(
        "__sub__", [](const T &a, const Other &b) { return a - RHSSetup{}(b); },
        py::is_operator(), py::call_guard<py::gil_scoped_release>());
    c.def(
        "__mul__", [](const T &a, const Other &b) { return a * RHSSetup{}(b); },
        py::is_operator(), py::call_guard<py::gil_scoped_release>());
    c.def(
        "__truediv__",
        [](const T &a, const Other &b) { return a / RHSSetup{}(b); },
        py::is_operator(), py::call_guard<py::gil_scoped_release>());
    if constexpr (!(std::is_same_v<T, Dataset> ||
                    std::is_same_v<Other, Dataset>)) {
      c.def(
          "__floordiv__",
          [](const T &a, const Other &b) {
            return floor_divide(a, RHSSetup{}(b));
          },
          py::is_operator(), py::call_guard<py::gil_scoped_release>());
      c.def(
          "__mod__",
          [](const T &a, const Other &b) { return a % RHSSetup{}(b); },
          py::is_operator(), py::call_guard<py::gil_scoped_release>());
      c.def(
          "__pow__",
          [](const T &base, const Other &exponent) {
            return pow(base, RHSSetup{}(exponent));
          },
          py::is_operator(), py::call_guard<py::gil_scoped_release>());
    }
  }

  template <class Other, class T, class... Ignored>
  static void reverse_binary(pybind11::class_<T, Ignored...> &c) {
    using namespace scipp;
    c.def(
        "__radd__", [](const T &a, const Other b) { return RHSSetup{}(b) + a; },
        py::is_operator(), py::call_guard<py::gil_scoped_release>());
    c.def(
        "__rsub__", [](const T &a, const Other b) { return RHSSetup{}(b)-a; },
        py::is_operator(), py::call_guard<py::gil_scoped_release>());
    c.def(
        "__rmul__", [](const T &a, const Other b) { return RHSSetup{}(b)*a; },
        py::is_operator(), py::call_guard<py::gil_scoped_release>());
    c.def(
        "__rtruediv__",
        [](const T &a, const Other b) { return RHSSetup{}(b) / a; },
        py::is_operator(), py::call_guard<py::gil_scoped_release>());
    if constexpr (!(std::is_same_v<T, Dataset> ||
                    std::is_same_v<Other, Dataset>)) {
      c.def(
          "__rfloordiv__",
          [](const T &a, const Other &b) {
            return floor_divide(RHSSetup{}(b), a);
          },
          py::is_operator(), py::call_guard<py::gil_scoped_release>());
      c.def(
          "__rmod__",
          [](const T &a, const Other &b) { return RHSSetup{}(b) % a; },
          py::is_operator(), py::call_guard<py::gil_scoped_release>());
      c.def(
          "__rpow__",
          [](const T &exponent, const Other &base) {
            return pow(RHSSetup{}(base), exponent);
          },
          py::is_operator(), py::call_guard<py::gil_scoped_release>());
    }
  }

  template <class Other, class T, class... Ignored>
  static void comparison(pybind11::class_<T, Ignored...> &c) {
    c.def(
        "__eq__", [](const T &a, Other &b) { return equal(a, RHSSetup{}(b)); },
        py::is_operator(), py::call_guard<py::gil_scoped_release>());
    c.def(
        "__ne__",
        [](const T &a, Other &b) { return not_equal(a, RHSSetup{}(b)); },
        py::is_operator(), py::call_guard<py::gil_scoped_release>());
    c.def(
        "__lt__", [](const T &a, Other &b) { return less(a, RHSSetup{}(b)); },
        py::is_operator(), py::call_guard<py::gil_scoped_release>());
    c.def(
        "__gt__",
        [](const T &a, Other &b) { return greater(a, RHSSetup{}(b)); },
        py::is_operator(), py::call_guard<py::gil_scoped_release>());
    c.def(
        "__le__",
        [](const T &a, Other &b) { return less_equal(a, RHSSetup{}(b)); },
        py::is_operator(), py::call_guard<py::gil_scoped_release>());
    c.def(
        "__ge__",
        [](const T &a, Other &b) { return greater_equal(a, RHSSetup{}(b)); },
        py::is_operator(), py::call_guard<py::gil_scoped_release>());
  }
};

template <class Other, class T, class... Ignored>
static void bind_in_place_binary(pybind11::class_<T, Ignored...> &c) {
  OpBinder<Identity>::in_place_binary<Other>(c);
}

template <class Other, class T, class... Ignored>
static void bind_binary(pybind11::class_<T, Ignored...> &c) {
  OpBinder<Identity>::binary<Other>(c);
}

template <class Other, class T, class... Ignored>
static void bind_comparison(pybind11::class_<T, Ignored...> &c) {
  OpBinder<Identity>::comparison<Other>(c);
}

template <class T, class... Ignored>
void bind_in_place_binary_scalars(pybind11::class_<T, Ignored...> &c) {
  OpBinder<ScalarToVariable>::in_place_binary<double>(c);
  OpBinder<ScalarToVariable>::in_place_binary<int64_t>(c);
}

template <class T, class... Ignored>
void bind_binary_scalars(pybind11::class_<T, Ignored...> &c) {
  OpBinder<ScalarToVariable>::binary<double>(c);
  OpBinder<ScalarToVariable>::binary<int64_t>(c);
}

template <class T, class... Ignored>
static void bind_reverse_binary_scalars(pybind11::class_<T, Ignored...> &c) {
  OpBinder<ScalarToVariable>::reverse_binary<double>(c);
  OpBinder<ScalarToVariable>::reverse_binary<int64_t>(c);
}

template <class T, class... Ignored>
void bind_comparison_scalars(pybind11::class_<T, Ignored...> &c) {
  OpBinder<ScalarToVariable>::comparison<double>(c);
  OpBinder<ScalarToVariable>::comparison<int64_t>(c);
}

template <class T, class... Ignored>
void bind_unary(pybind11::class_<T, Ignored...> &c) {
  c.def(
      "__neg__", [](const T &a) { return -a; }, py::is_operator(),
      py::call_guard<py::gil_scoped_release>());
}

template <class T, class... Ignored>
void bind_boolean_unary(pybind11::class_<T, Ignored...> &c) {
  c.def(
      "__invert__", [](const T &a) { return ~a; }, py::is_operator(),
      py::call_guard<py::gil_scoped_release>());
}

template <class Other, class T, class... Ignored>
void bind_logical(pybind11::class_<T, Ignored...> &c) {
  using T1 = const T;
  using T2 = const Other;
  c.def(
      "__or__", [](const T1 &a, const T2 &b) { return a | b; },
      py::is_operator(), py::call_guard<py::gil_scoped_release>());
  c.def(
      "__xor__", [](const T1 &a, const T2 &b) { return a ^ b; },
      py::is_operator(), py::call_guard<py::gil_scoped_release>());
  c.def(
      "__and__", [](const T1 &a, const T2 &b) { return a & b; },
      py::is_operator(), py::call_guard<py::gil_scoped_release>());
  c.def(
      "__ior__",
      [](const py::object &a, const T2 &b) -> const py::object & {
        a.cast<T &>() |= b;
        return a;
      },
      py::is_operator(), py::call_guard<py::gil_scoped_release>());
  c.def(
      "__ixor__",
      [](const py::object &a, const T2 &b) -> const py::object & {
        a.cast<T &>() ^= b;
        return a;
      },
      py::is_operator(), py::call_guard<py::gil_scoped_release>());
  c.def(
      "__iand__",
      [](const py::object &a, const T2 &b) -> const py::object & {
        a.cast<T &>() &= b;
        return a;
      },
      py::is_operator(), py::call_guard<py::gil_scoped_release>());
}
