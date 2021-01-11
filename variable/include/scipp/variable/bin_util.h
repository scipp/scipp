// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/variable/variable.h"

namespace scipp::variable {

//[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable make_bins(Variable indices,
//                                                       const Dim dim,
//                                                       Variable buffer);
//

[[nodiscard]] SCIPP_VARIABLE_EXPORT std::tuple<Variable, Variable>
subbin_offsets(const VariableConstView &start_, const VariableConstView &stop_,
               const VariableConstView &subbin_sizes_, const scipp::index nsrc,
               const scipp::index ndst, const scipp::index nbin);

// instead of this helper class, we could also just use two variables for
// offset and sizes (the latter one a bin-variable), and implement custom sum
// and cumsum ops.
class SCIPP_VARIABLE_EXPORT SubbinSizes {
public:
  SubbinSizes(const scipp::index offset, std::vector<scipp::index> &&sizes);
  const auto &offset() const noexcept { return m_offset; }
  const auto &sizes() const noexcept { return m_sizes; }

private:
  scipp::index m_offset;
  // consider boost::small_vector?
  std::vector<scipp::index> m_sizes; // TODO can we avoid many small vecs?
};

[[nodiscard]] SCIPP_VARIABLE_EXPORT bool operator==(const SubbinSizes &a,
                                                    const SubbinSizes &b);

[[nodiscard]] SCIPP_VARIABLE_EXPORT SubbinSizes operator+(const SubbinSizes &a,
                                                          const SubbinSizes &b);

} // namespace scipp::variable
