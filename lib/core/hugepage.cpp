// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/hugepage.h"

#ifdef __linux__
#include <cstdint>
#include <cstdlib>
#include <string_view>
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace scipp::core {

#if defined(__linux__) && defined(MADV_HUGEPAGE)

namespace {
/// Smallest range for which huge pages are requested. Matches what NumPy uses.
constexpr size_t min_size = 4 * 1024 * 1024;

bool env_enabled() {
  const char *const env = std::getenv("SCIPP_MADVISE_HUGEPAGE");
  return env == nullptr || std::string_view{env} != "0";
}
} // namespace

void madvise_hugepage(void *ptr, const size_t size) noexcept {
  static const bool enabled = env_enabled();
  if (!enabled || size < min_size)
    return;
  const auto page_size = static_cast<size_t>(::sysconf(_SC_PAGESIZE));
  const auto address = reinterpret_cast<std::uintptr_t>(ptr);
  // Shrink to whole pages so we never advise memory outside the allocation.
  const auto begin = (address + page_size - 1) & ~(page_size - 1);
  const auto end = (address + size) & ~(page_size - 1);
  if (begin < end)
    // Ignore failures: without kernel support for huge pages, or if the kernel
    // declines for other reasons, we simply keep using normal pages.
    static_cast<void>(
        ::madvise(reinterpret_cast<void *>(begin), end - begin, MADV_HUGEPAGE));
}

#else

void madvise_hugepage(void *, size_t) noexcept {}

#endif

} // namespace scipp::core
