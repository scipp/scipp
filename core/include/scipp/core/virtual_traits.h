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

template <class, class, bool> class VirtualTraitBase;

template <class Tag, bool Noexcept, class R, class... Args>
class VirtualTraitBase<Tag, R(Args...), Noexcept> {
public:
  template <typename Impl> void add(const DType &key, Impl &&impl) {
    vtable.emplace(key, std::forward<Impl>(impl));
  }

  template <typename Impl> void add_default(Impl &&impl) {
    default_impl = std::forward<Impl>(impl);
  }

  R operator()(Args... args) const {
    if (const auto dtype = get_dtype(std::forward<Args>(args)...);
        vtable.find(dtype) != vtable.end()) {
      return vtable.at(dtype)(std::forward<Args>(args)...);
    } else if (default_impl != nullptr) {
      return default_impl(std::forward<Args>(args)...);
    }
    assert(false);
  }

private:
  std::unordered_map<scipp::core::DType, R (*)(Args...) noexcept(Noexcept)>
      vtable;
  R (*default_impl)(Args...);
};
} // namespace detail

template <class, class> class VirtualTrait;

template <class Tag, class R, class... Args>
class VirtualTrait<Tag, R(Args...)>
    : public detail::VirtualTraitBase<Tag, R(Args...), false> {};

template <class Tag, class R, class... Args>
class VirtualTrait<Tag, R(Args...) noexcept>
    : public detail::VirtualTraitBase<Tag, R(Args...), true> {};

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