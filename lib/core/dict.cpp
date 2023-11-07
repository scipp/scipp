// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen

#include "scipp/core/dict.h"

#include <sstream>

namespace scipp::core {
template <class It>
std::string dict_keys_to_string(It it, const It end,
                                const std::string_view &dict_name) {
  std::ostringstream ss;
  ss << "<" << dict_name << " {";
  bool first = true;
  for (; it != end; ++it) {
    if (!first) {
      ss << ", ";
    } else {
      first = false;
    }
    ss << *it;
  }
  ss << "}>";
  return ss.str();
}

// The value type does not matter here, so set it to something simple.
using dim_dict_key_iterator = Dict<Dim, int>::const_key_iterator;
using str_dict_key_iterator = Dict<std::string, int>::const_key_iterator;

template SCIPP_CORE_EXPORT std::string
dict_keys_to_string(dim_dict_key_iterator, dim_dict_key_iterator,
                    const std::string_view &);
template SCIPP_CORE_EXPORT std::string
dict_keys_to_string(str_dict_key_iterator, str_dict_key_iterator,
                    const std::string_view &);
} // namespace scipp::core
