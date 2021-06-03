// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen
#pragma once

#include <unordered_map>

#include "scipp/core/dtype.h"

namespace scipp::dyn {
namespace detail {
template <class V, class... Args>
scipp::DType get_dtype(V &&v, [[maybe_unused]] Args &&... args) {
  return std::forward<V>(v).dtype();
}

template <class, class, bool> struct VirtualTraitBase;

template <class Tag, bool Noexcept, class R, class... Args>
struct VirtualTraitBase<Tag, R(Args...), Noexcept> {
  static auto &vtable() {
    static std::unordered_map<scipp::core::DType,
                              R (*)(Args...) noexcept(Noexcept)>
        table;
    return table;
  }

  static const auto &default_impl(R (*f)(Args...)) {
    static R (*impl)(Args...);
    if (f != nullptr) {
      impl = f;
    }
    return impl;
  }

  template <typename Impl> static void add(const DType &key, Impl &&impl) {
    vtable().template emplace(key, std::forward<Impl>(impl));
  }

  template <typename Impl> static void add_default(Impl &&impl) {
    default_impl(std::forward<Impl>(impl));
  }

  R operator()(Args... args) const {
    if (const auto dtype = get_dtype(std::forward<Args>(args)...);
        vtable().find(dtype) != vtable().end()) {
      return vtable().at(dtype)(std::forward<Args>(args)...);
    } else if (const auto &f = default_impl(nullptr); f != nullptr) {
      return f(std::forward<Args>(args)...);
    }
    assert(false);
  }
};
} // namespace detail

template <class, class> struct VirtualTrait;

template <class Tag, class R, class... Args>
struct VirtualTrait<Tag, R(Args...)>
    : detail::VirtualTraitBase<Tag, R(Args...), false> {};

template <class Tag, class R, class... Args>
struct VirtualTrait<Tag, R(Args...) noexcept>
    : detail::VirtualTraitBase<Tag, R(Args...), true> {};

struct Default;

template <class ElementType> struct implement_trait_for {
  template <class Trait, class Impl>
  implement_trait_for(Trait &trait, Impl &&impl) {
    if constexpr (std::is_same_v<ElementType, Default>) {
      trait.add_default(std::forward<Impl>(impl));
    } else {
      trait.add(core::dtype<ElementType>, std::forward<Impl>(impl));
    }
  }
};
} // namespace scipp::dyn