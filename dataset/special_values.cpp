#include "scipp/dataset/special_values.h"
#include "scipp/variable/special_values.h"

namespace scipp::dataset {

DataArray isfinite(const DataArrayConstView &a) {
  return DataArray(isfinite(a.data()), a.coords(), a.masks(), a.attrs());
}

} // namespace scipp::dataset