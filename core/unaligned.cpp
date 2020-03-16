// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/unaligned.h"

namespace scipp::core::unaligned {

Dim unaligned_dim(const VariableConstView &unaligned) {
  if (is_events(unaligned))
    return Dim::Invalid;
  if (unaligned.dims().ndim() != 1)
    throw except::UnalignedError("Coordinate used for alignment must be 1-D.");
  return unaligned.dims().inner();
}
} // namespace scipp::core::unaligned
