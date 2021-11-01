// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Owen Arnold
#pragma once
#include <Eigen/Core>
namespace scipp {
template <class T> struct default_init {
  static T value() { return T{}; }
};
// Eigen does not zero-initialize matrices (vectors), which is a recurrent
// source of bugs. Variable does zero-init instead.
template <class T, int Rows, int Cols>
struct default_init<Eigen::Matrix<T, Rows, Cols>> {
  static Eigen::Matrix<T, Rows, Cols> value() {
    return Eigen::Matrix<T, Rows, Cols>::Zero();
  }
};

template <class T> struct zero_init {
  static T value() { return T{0}; }
};

template <class T, int Rows, int Cols>
struct zero_init<Eigen::Matrix<T, Rows, Cols>>
    : default_init<Eigen::Matrix<T, Rows, Cols>> {};
} // namespace scipp
