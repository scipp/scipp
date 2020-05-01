// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#pragma once

#include <vector>
#include <string>

#include "scipp/common/index.h"

namespace scipp::python {

using strpair = const std::pair<const std::string, const std::string>;

class Docstring {

public:

  Docstring() = default;
  Docstring(const std::string description, const std::string raises, const std::string seealso, const std::string returns, const std::string rtype);
  Docstring(const std::string description, const std::string raises, const std::string seealso, const std::string returns, const std::string rtype, const std::vector<std::pair<std::string, std::string>> &params);
  Docstring(const Docstring &) = default;
  Docstring operator~() const;

  const std::string description() const {return m_description; };
  const std::string raises() const {return m_raises; };
  const std::string seealso() const {return m_seealso; };
  const std::string returns() const {return m_returns; };
  const std::string rtype() const {return m_rtype; };
  strpair param(const scipp::index ind) const {return m_params[ind];};
  void set_description(const std::string description) {m_description = description;};
  void set_raises(const std::string raises){m_raises = raises;};
  void set_seealso(const std::string seealso){m_seealso = seealso;};
  void set_returns(const std::string returns){m_returns = returns;};
  void set_rtype(const std::string rtype){m_rtype = rtype;};
  void set_param(const scipp::index ind, strpair param){m_params[ind] = param;};
  void insert_param(const scipp::index ind, strpair param);

  const Docstring with_out_arg();

  const std::string to_string() const;

private:
  std::string m_description, m_raises, m_seealso, m_returns, m_rtype;
  std::vector<std::pair<std::string, std::string>> m_params;

};


} // namespace scipp::python
