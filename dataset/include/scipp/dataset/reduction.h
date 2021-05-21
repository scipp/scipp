// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/dataset/dataset.h"

namespace scipp::dataset {

SCIPP_DATASET_EXPORT DataArray sum(const DataArray &a);
SCIPP_DATASET_EXPORT DataArray sum(const DataArray &a, const Dim dim);
SCIPP_DATASET_EXPORT Dataset sum(const Dataset &d, const Dim dim);
SCIPP_DATASET_EXPORT Dataset sum(const Dataset &d);

SCIPP_DATASET_EXPORT DataArray nansum(const DataArray &a);
SCIPP_DATASET_EXPORT DataArray nansum(const DataArray &a, const Dim dim);
SCIPP_DATASET_EXPORT Dataset nansum(const Dataset &d, const Dim dim);
SCIPP_DATASET_EXPORT Dataset nansum(const Dataset &d);

SCIPP_DATASET_EXPORT DataArray mean(const DataArray &a, const Dim dim);
SCIPP_DATASET_EXPORT DataArray mean(const DataArray &a);
SCIPP_DATASET_EXPORT Dataset mean(const Dataset &d, const Dim dim);
SCIPP_DATASET_EXPORT Dataset mean(const Dataset &d);

SCIPP_DATASET_EXPORT DataArray nanmean(const DataArray &a, const Dim dim);
SCIPP_DATASET_EXPORT DataArray nanmean(const DataArray &a);
SCIPP_DATASET_EXPORT Dataset nanmean(const Dataset &d, const Dim dim);
SCIPP_DATASET_EXPORT Dataset nanmean(const Dataset &d);

} // namespace scipp::dataset
