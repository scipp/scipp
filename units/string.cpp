#include "scipp/units/string.h"
namespace scipp::units {

std::string to_string(const units::Unit &unit) { return unit.name(); }

} // namespace scipp::units