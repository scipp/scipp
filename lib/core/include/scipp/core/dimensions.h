// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <vector>

#include "scipp-core_export.h"
#include "scipp/common/index.h"
#include "scipp/core/sizes.h"
#include "scipp/units/dim.h"

namespace scipp::core {

/// Dimensions are accessed very frequently, so packing everything into a single
/// (64 Byte) cacheline should be advantageous.
/// We follow the numpy convention: First dimension is outer dimension, last
/// dimension is inner dimension.
class SCIPP_CORE_EXPORT Dimensions : public Sizes {
public:
  constexpr Dimensions() noexcept = default;
  Dimensions(const Dim dim, const scipp::index size)
      : Dimensions({{dim, size}}) {}
  Dimensions(scipp::span<const Dim> labels,
             scipp::span<const scipp::index> shape);
  Dimensions(const std::initializer_list<Dim> labels,
             const std::initializer_list<scipp::index> shape)
      : Dimensions({labels.begin(), labels.end()},
                   {shape.begin(), shape.end()}) {}
  Dimensions(const std::initializer_list<std::pair<Dim, scipp::index>> dims) {
    for (const auto &[dim, len] : dims)
      addInner(dim, len);
  }

  constexpr bool operator==(const Dimensions &other) const noexcept {
    if (ndim() != other.ndim())
      return false;
    for (int32_t i = 0; i < ndim(); ++i) {
      if (shape()[i] != other.shape()[i])
        return false;
      if (labels()[i] != other.labels()[i])
        return false;
    }
    return true;
  }
  constexpr bool operator!=(const Dimensions &other) const noexcept {
    return !(*this == other);
  }

  /// Return the shape of the space defined by *this.
  [[nodiscard]] constexpr scipp::span<const scipp::index>
  shape() const &noexcept {
    return sizes();
  }

  /// Return the volume of the space defined by *this.
  [[nodiscard]] constexpr scipp::index volume() const noexcept {
    scipp::index volume{1};
    for (const auto &length : shape())
      volume *= length;
    return volume;
  }

  /// Return number of dims
  [[nodiscard]] constexpr scipp::index ndim() const noexcept {
    return Sizes::size();
  }

  [[nodiscard]] Dim inner() const noexcept;

  [[nodiscard]] Dim label(const scipp::index i) const;
  [[nodiscard]] scipp::index size(const scipp::index i) const;
  [[nodiscard]] scipp::index offset(const Dim label) const;

  // TODO Better names required.
  void add(const Dim label, const scipp::index size);
  void addInner(const Dim label, const scipp::index size);
};

[[nodiscard]] SCIPP_CORE_EXPORT constexpr Dimensions
merge(const Dimensions &a) noexcept {
  return a;
}

[[nodiscard]] SCIPP_CORE_EXPORT Dimensions merge(const Dimensions &a,
                                                 const Dimensions &b);

template <class... Ts>
Dimensions merge(const Dimensions &a, const Dimensions &b,
                 const Ts &... other) {
  return merge(merge(a, b), other...);
}

[[nodiscard]] SCIPP_CORE_EXPORT Dimensions intersection(const Dimensions &a,
                                                        const Dimensions &b);

[[nodiscard]] SCIPP_CORE_EXPORT Dimensions
transpose(const Dimensions &dims, scipp::span<const Dim> labels = {});

[[nodiscard]] SCIPP_CORE_EXPORT Dimensions fold(const Dimensions &old_dims,
                                                const Dim from_dim,
                                                const Dimensions &to_dims);

} // namespace scipp::core

namespace scipp {
using core::Dimensions;
}
