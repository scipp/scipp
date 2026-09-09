// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/element_array.h"

#ifdef __linux__

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

using scipp::core::element_array;
using scipp::core::init_for_overwrite;

namespace {
/// Does the mapping containing ptr have the VM_HUGEPAGE flag, i.e., did someone
/// call madvise(MADV_HUGEPAGE) on it?
///
/// The flag is a property of the mapping, not of an individual allocation, so
/// its *absence* cannot be tested reliably: a small allocation may well be
/// placed in a mapping that a preceding large allocation had advised.
bool advises_hugepages(const void *ptr) {
  const auto address = reinterpret_cast<std::uintptr_t>(ptr);
  std::ifstream smaps("/proc/self/smaps");
  bool in_mapping = false;
  for (std::string line; std::getline(smaps, line);) {
    unsigned long long begin = 0, end = 0;
    if (std::sscanf(line.c_str(), "%llx-%llx", &begin, &end) == 2) {
      in_mapping = begin <= address && address < end;
    } else if (in_mapping && line.rfind("VmFlags:", 0) == 0) {
      std::istringstream flags(line.substr(8));
      for (std::string flag; flags >> flag;)
        if (flag == "hg")
          return true;
      return false;
    }
  }
  return false;
}

bool hugepages_testable() {
  const char *const env = std::getenv("SCIPP_MADVISE_HUGEPAGE");
  return std::filesystem::exists("/sys/kernel/mm/transparent_hugepage") &&
         (env == nullptr || std::string_view{env} != "0");
}
} // namespace

TEST(HugepageTest, large_allocation_advises_hugepages) {
  if (!hugepages_testable())
    GTEST_SKIP() << "kernel without huge page support, or disabled via "
                    "SCIPP_MADVISE_HUGEPAGE";
  const element_array<double> array(2 * 1024 * 1024, init_for_overwrite);
  // Not array.data(): the leading partial page is not advised, so the kernel
  // splits the mapping and the flag is set only from the next page boundary.
  EXPECT_TRUE(advises_hugepages(array.data() + array.size() / 2));
}

#endif
