// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_GROUPBY_H
#define SCIPP_CORE_GROUPBY_H

#include <vector>

#include <scipp/core/dataset.h>

namespace scipp::core {

SCIPP_CORE_EXPORT DataArray groupby(const DatasetConstProxy &dataset,
                                    const std::string &labels);

} // namespace scipp::core

#endif // SCIPP_CORE_GROUPBY_H
