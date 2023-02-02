#include <gtest/gtest.h>

#include "scipp/core/format.h"

#include "scipp/variable/format.h"
#include "scipp/variable/variable.h"

using namespace scipp;
using namespace scipp::variable;

TEST(FormatTest, var) {
  const auto &formatters = core::FormatRegistry::instance();
  core::FormatSpec spec;
  auto var = makeVariable<double>(Dims{Dim::X}, Shape{7}, units::m,
                                  Values{1, 2, 3, 4, 5, 6, 7});
  std::cout << formatters.format(var, spec) << std::endl;
  auto var2 = makeVariable<double>(
      Dims{Dim::X}, Shape{7}, units::m, Values{1, 2, 3, 4, 5, 6, 7},
      Variances{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7});
  std::cout << formatters.format(var2, spec) << std::endl;
  std::cout << formatters.format(1l, spec) << std::endl;
  std::cout << formatters.format(1.2, spec) << std::endl;
}
