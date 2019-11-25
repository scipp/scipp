// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Dimitar Tasev
#ifndef SCIPP_COMMON_STRING_H
#define SCIPP_COMMON_STRING_H

#include <initializer_list>
#include <sstream>
#include <string>

template <class T> std::string to_string(const std::initializer_list<T> items) {
  std::stringstream ss;

  for (const auto &item : items) {
    ss << to_string(item) << " ";
  }

  return ss.str();
}

#endif // SCIPP_COMMON_STRING_H
