#include "dataset.h"

Dataset concatenate(const Dimension dim, const Dataset &d1, const Dataset &d2) {
  // Match type and name, drop missing?
  // What do we have to do to check and compute the resulting dimensions?
  // - If dim is in m_dimensions, *some* of the variables contain it. Those that
  //   do not must then be identical (do not concatenate) or we could
  //   automatically broadcast? Yes!?
  // - If dim is new, concatenate variables if different, copy if same.
  // We will be doing deep comparisons here, it would be nice if we could setup
  // sharing, but d1 and d2 are const, is there a way...? Not without breaking
  // thread safety? Could cache cow_ptr for future sharing setup, done by next
  // non-const op?
  Dataset out;
  for (gsl::index i1 = 0; i1 < d1.size(); ++i1) {
    const auto &var1 = d1[i1];
    const auto &var2 = d2[d2.find(var1.type(), var1.name())];
    // TODO may need to extend things along constant dimensions to match shapes!
    if (var1.dimensions().contains(dim)) {
      out.insert(concatenate(dim, var1, var2));
    } else {
      if (var1 == var2) {
        out.insert(var1);
      } else {
        if (d1.dimensions().contains(dim)) {
          // Variable does not contain dimension but Dataset does, i.e.,
          // Variable is constant. We need to extend it before concatenating.
          throw std::runtime_error("TODO");
        } else {
          // Creating a new dimension
          out.insert(concatenate(dim, var1, var2));
        }
      }
    }
  }
  return out;
}
