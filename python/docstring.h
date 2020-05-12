// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#pragma once

#include <map>
#include <string>
#include <vector>

#include "scipp/dataset/dataset.h"
#include "scipp/variable/variable.h"

using namespace scipp::variable;
using namespace scipp::dataset;

template <class T> const std::string type_to_string() {
  if (std::is_same_v<T, Variable>)
    return "Variable";
  if (std::is_same_v<T, VariableView>)
    return "VariableView";
  if (std::is_same_v<T, DataArray>)
    return "DataArray";
  if (std::is_same_v<T, DataArrayView>)
    return "DataArrayView";
  if (std::is_same_v<T, Dataset>)
    return "Dataset";
  if (std::is_same_v<T, DatasetView>)
    return "DatasetView";
  return "";
}

class Docstring {
public:
  Docstring &description(const std::string &s, const bool append = false);
  Docstring &raises(const std::string &s, const bool append = false);
  Docstring &seealso(const std::string &s, const bool append = false);
  Docstring &returns(const std::string &s, const bool append = false);
  Docstring &rtype(const std::string &s, const bool append = false);
  Docstring &param(const std::string &name, const std::string &about,
                   const std::string &type);

  template <class T> Docstring &rtype() { return rtype(type_to_string<T>()); }

  template <class T>
  Docstring &param(const std::string &name, const std::string &about) {
    return param(name, about, type_to_string<T>());
  }

  const char *c_str();

private:
  std::string m_description;
  std::string m_raises;
  std::string m_seealso;
  std::string m_returns;
  std::string m_rtype;
  std::string m_output;
  std::vector<std::string> m_order;
  std::map<std::string, std::pair<std::string, std::string>> m_params;
  void update(std::string &field, const std::string &s,
              const bool append = false);
};
