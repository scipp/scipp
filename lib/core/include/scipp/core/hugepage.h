// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <cstddef>

#include "scipp-core_export.h"

namespace scipp::core {

/// Ask the kernel to back a memory range with transparent huge pages.
///
/// Large buffers benefit from huge pages since fewer TLB entries are required
/// to cover them. The Linux kernel uses huge pages for all memory only if
/// /sys/kernel/mm/transparent_hugepage/enabled is set to `always`. With the
/// common default of `madvise` it does so only for ranges that requested them
/// via madvise(MADV_HUGEPAGE), which is what this function does.
///
/// Ranges shorter than an internal threshold are left alone, since the kernel
/// rounds up to the huge page size (typically 2 MByte), which would waste
/// memory for small buffers.
///
/// Has no effect on platforms other than Linux, or if the environment variable
/// SCIPP_MADVISE_HUGEPAGE is set to 0. Disabling can help if the system suffers
/// from memory fragmentation: with the `defrag` setting `madvise` (another
/// common default) the kernel compacts memory synchronously when a page of an
/// advised range is first touched, which may stall the allocating thread.
SCIPP_CORE_EXPORT void madvise_hugepage(void *ptr, size_t size) noexcept;

} // namespace scipp::core
