#include "scipp/dataset/slice.h"
#include "scipp/dataset/dataset.h"
#include "scipp/units/dim.h"
#include "scipp/variable/variable.h"

namespace scipp::dataset {
DataArray slice(const DataArrayConstView &to_slice, const Dim dim,
                const VariableConstView begin, const VariableConstView *end) {
  auto coords = to_slice.coords();
  if (!coords.contains(dim))
    throw except::DimensionNotFoundError(to_slice.dims(), dim);
  auto coordIt = *coords.find(dim);
  if (coordIt.second.dims().ndim() != 1) {
    throw except::SizeError(
        "multi-dimensional coordinates not supported in slice");
  }

  return DataArray{};
}
} // namespace scipp::dataset
