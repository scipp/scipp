// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/dataset/arithmetic.h"
#include "scipp/dataset/astype.h"
#include "scipp/dataset/generated_comparison.h"
#include "scipp/dataset/generated_logical.h"
#include "scipp/dataset/generated_math.h"
#include "scipp/dataset/to_unit.h"
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
  c.def("__abs__", [](T &self) { return abs(self); });
  c.def("__repr__", [](T &self) { return to_string(self); });
  c.def("__bool__", [](T &) {
    throw std::runtime_error("The truth value of a variable, data array, or "
                             "dataset is ambiguous. Use any() or all().");
  });
  c.def(
      "copy", [](T &self, const bool deep) { return deep ? copy(self) : self; },
      py::arg("deep") = true, py::call_guard<py::gil_scoped_release>(),
      R"(
      Return a (by default deep) copy.

      If `deep=True` (the default), a deep copy is made. Otherwise, a shallow
      copy is made, and the returned data (and meta data) values are new views
      of the data and meta data values of this object.)");
  c.def(
      "__copy__", [](const T &self) { return self; },
      py::call_guard<py::gil_scoped_release>(), "Return a (shallow) copy.");
  c.def(
      "__deepcopy__",
      [](const T &self, const py::dict &) { return copy(self); },
      py::call_guard<py::gil_scoped_release>(), "Return a (deep) copy.");
}

template <class T, class... Ignored>
void bind_astype(py::class_<T, Ignored...> &c) {
  c.def(
      "astype",
      [](const T &self, const py::object &type, const bool copy) {
        const auto [scipp_dtype, dtype_unit] =
            cast_dtype_and_unit(type, std::nullopt);
        if (dtype_unit != scipp::units::one && dtype_unit != self.unit()) {
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
        :rtype: Union[scipp.Variable, scipp.DataArray])");
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

template <class Other, class T, class... Ignored>
void bind_comparison(pybind11::class_<T, Ignored...> &c) {
  c.def(
      "__eq__", [](T &a, Other &b) { return equal(a, b); }, py::is_operator(),
      py::call_guard<py::gil_scoped_release>());
  c.def(
      "__ne__", [](T &a, Other &b) { return not_equal(a, b); },
      py::is_operator(), py::call_guard<py::gil_scoped_release>());
  c.def(
      "__lt__", [](T &a, Other &b) { return less(a, b); }, py::is_operator(),
      py::call_guard<py::gil_scoped_release>());
  c.def(
      "__gt__", [](T &a, Other &b) { return greater(a, b); }, py::is_operator(),
      py::call_guard<py::gil_scoped_release>());
  c.def(
      "__le__", [](T &a, Other &b) { return less_equal(a, b); },
      py::is_operator(), py::call_guard<py::gil_scoped_release>());
  c.def(
      "__ge__", [](T &a, Other &b) { return greater_equal(a, b); },
      py::is_operator(), py::call_guard<py::gil_scoped_release>());
}

struct Identity {
  template <class T> const T &operator()(const T &x) const noexcept {
    return x;
  }
};
struct ScalarToVariable {
  template <class T> scipp::Variable operator()(const T &x) const noexcept {
    return x * scipp::units::one;
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
    c.def(
        "__iadd__",
        [](py::object &a, Other &b) {
          a.cast<T &>() += RHSSetup{}(b);
          return a;
        },
        py::is_operator(), py::call_guard<py::gil_scoped_release>());
    c.def(
        "__isub__",
        [](py::object &a, Other &b) {
          a.cast<T &>() -= RHSSetup{}(b);
          return a;
        },
        py::is_operator(), py::call_guard<py::gil_scoped_release>());
    c.def(
        "__imul__",
        [](py::object &a, Other &b) {
          a.cast<T &>() *= RHSSetup{}(b);
          return a;
        },
        py::is_operator(), py::call_guard<py::gil_scoped_release>());
    c.def(
        "__itruediv__",
        [](py::object &a, Other &b) {
          a.cast<T &>() /= RHSSetup{}(b);
          return a;
        },
        py::is_operator(), py::call_guard<py::gil_scoped_release>());
    if constexpr (!(std::is_same_v<T, Dataset> ||
                    std::is_same_v<Other, Dataset>)) {
      c.def(
          "__imod__",
          [](py::object &a, Other &b) {
            a.cast<T &>() %= RHSSetup{}(b);
            return a;
          },
          py::is_operator(), py::call_guard<py::gil_scoped_release>());
      if constexpr (!(std::is_same_v<T, DataArray> ||
                      std::is_same_v<Other, DataArray>)) {
        c.def(
            "__ipow__",
            [](T &base, Other &exponent) {
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
};

template <class Other, class T, class... Ignored>
static void bind_in_place_binary(pybind11::class_<T, Ignored...> &c) {
  OpBinder<Identity>::in_place_binary<Other>(c);
}

template <class Other, class T, class... Ignored>
static void bind_binary(pybind11::class_<T, Ignored...> &c) {
  OpBinder<Identity>::binary<Other>(c);
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
      [](const py::object &a, const T2 &b) {
        a.cast<T &>() |= b;
        return a;
      },
      py::is_operator(), py::call_guard<py::gil_scoped_release>());
  c.def(
      "__ixor__",
      [](const py::object &a, const T2 &b) {
        a.cast<T &>() ^= b;
        return a;
      },
      py::is_operator(), py::call_guard<py::gil_scoped_release>());
  c.def(
      "__iand__",
      [](const py::object &a, const T2 &b) {
        a.cast<T &>() &= b;
        return a;
      },
      py::is_operator(), py::call_guard<py::gil_scoped_release>());
}
