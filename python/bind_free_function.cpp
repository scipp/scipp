// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#include "bind_free_function.h"

/// Construct docstring
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
