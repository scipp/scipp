#include "scipp/dataset/special_values.h"
#include "scipp/variable/special_values.h"
#include "scipp/variable/util.h"

namespace scipp::dataset {
using variable::values;

DataArray isfinite(const DataArrayConstView &a) {
  return DataArray(isfinite(a.data()), a.coords(), a.masks(), a.attrs());
}

} // namespace scipp::dataset