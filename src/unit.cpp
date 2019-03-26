/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <optional>
#include <stdexcept>

#include <boost/lexical_cast.hpp>
#include <boost/units/cmath.hpp>
#include <boost/units/io.hpp>

#include "except.h"
#include "unit.h"

using namespace units;

// Helper to check whether type is a member of a given std::variant
template <typename T, typename VARIANT_T> struct isVariantMember;
template <typename T, typename... ALL_T>
struct isVariantMember<T, std::variant<ALL_T...>>
    : public std::disjunction<std::is_same<T, ALL_T>...> {};
// Helper to make checking for allowed units more compact
template <class T> constexpr bool isKnownUnit(const T &) {
  return isVariantMember<T, Unit::unit_t>::value;
}

namespace units {
template <class T> std::string to_string(const T &unit) {
  return boost::lexical_cast<std::string>(unit);
}
} // namespace units

std::string Unit::name() const {
  return std::visit([](auto &&unit) { return units::to_string(unit); }, m_unit);
}

bool operator==(const Unit &a, const Unit &b) { return a() == b(); }
bool operator!=(const Unit &a, const Unit &b) { return !(a == b); }

Unit operator+(const Unit &a, const Unit &b) {
  if (a == b)
    return a;
  throw std::runtime_error("Cannot add " + a.name() + " and " + b.name() + ".");
}

Unit operator-(const Unit &a, const Unit &b) {
  if (a == b)
    return a;
  throw std::runtime_error("Cannot subtract " + a.name() + " and " + b.name() +
                           ".");
}

template <class F, class Variant, size_t... N>
decltype(auto) invoke_active(F &&f, Variant &&v, std::index_sequence<N...>) {
  using Ret = decltype(
      std::invoke(std::forward<F>(f), std::get<0>(std::forward<Variant>(v))));

  if constexpr (!std::is_same_v<void, Ret>) {
    Ret ret;
    if (!((v.index() == N
               ? (ret = std::invoke(std::forward<F>(f),
                                    std::get<N>(std::forward<Variant>(v))),
                  true)
               : false) ||
          ...))
      throw std::bad_variant_access{};

    return ret;
  } else {
    if (!((v.index() == N ? (std::invoke(std::forward<F>(f),
                                         std::get<N>(std::forward<Variant>(v))),
                             true)
                          : false) ||
          ...))
      throw std::bad_variant_access{};
  }
}
template <class F, class Variant> decltype(auto) myvisit(F &&f, Variant &&var) {
  return invoke_active(
      std::forward<F>(f), std::forward<Variant>(var),
      std::make_index_sequence<
          std::variant_size_v<std::remove_reference_t<Variant>>>{});
}
template <class F, class Variant, class Indices> decltype(auto) myvisit(F &&f, Variant &&var, Indices indices) {
  return invoke_active(std::forward<F>(f), std::forward<Variant>(var), indices);
}

template <class T1, class T2> struct multiplies {
  constexpr auto operator()(const T1 &lhs, const T2 &rhs) const {
    return lhs * rhs;
  }
};

template <class T1, class T2> struct divides {
  constexpr auto operator()(const T1 &lhs, const T2 &rhs) const {
    return lhs / rhs;
  }
};

// all indices of alternatives that have a valid product
template <template <class, class> class Op, class T1, class... Out>
constexpr auto product_valid_impl(std::tuple<Out...> out) {
  return out;
}
template <template <class, class> class Op, class T1, class T2, class... T2s,
          class... Out>
constexpr auto product_valid_impl(std::tuple<Out...>) {
  constexpr T1 x;
  constexpr T2 y;
  if constexpr (isKnownUnit(Op<T1, T2>()(x, y)))
    return product_valid_impl<Op, T1, T2s...>(
        std::tuple<Out..., std::pair<T1, T2>>{});
  else
    return product_valid_impl<Op, T1, T2s...>(std::tuple<Out...>{});
}
template <template <class, class> class Op, class T1, class... T2>
constexpr auto product_valid(std::tuple<T2...>) {
  std::tuple<> out{};
  return product_valid_impl<Op, T1, T2...>(out);
}

template <template <class, class> class Op, class... T1, class... T2>
constexpr auto expand(std::variant<T1...>, std::variant<T2...>) {
  return std::tuple_cat(product_valid<Op, T1>(std::tuple<T2...>{})...);
}

template <class F, class Variant, class... T1, class... T2>
decltype(auto) invoke_active(F &&f, Variant &&v1, Variant &&v2,
                             std::tuple<std::pair<T1, T2>...>) {
  using Ret = decltype(std::invoke(std::forward<F>(f),
                                   std::get<0>(std::forward<Variant>(v1)),
                                   std::get<0>(std::forward<Variant>(v2))));

  if constexpr (!std::is_same_v<void, Ret>) {
    Ret ret;
    if (!((std::holds_alternative<T1>(v1) && std::holds_alternative<T2>(v2)
               ? (ret = std::invoke(std::forward<F>(f),
                                    std::get<T1>(std::forward<Variant>(v1)),
                                    std::get<T2>(std::forward<Variant>(v2))),
                  true)
               : false) ||
          ...))
      throw std::bad_variant_access{};

    return ret;
  } else {
    throw std::runtime_error("???");
  }
}

template <template <class, class> class Op, class F, class Variant>
decltype(auto) myvisit(F &&f, Variant &&var1, Variant &&var2) {
  auto indices = expand<Op>(var1, var2);
  return invoke_active(std::forward<F>(f), std::forward<Variant>(var1),
                       std::forward<Variant>(var2), indices);
}

template<class T> struct always_false : std::false_type {};

// Mutliplying two units together using std::visit to run through the contents
// of the std::variant
Unit operator*(const Unit &a, const Unit &b) {
  try {
    return Unit(myvisit<multiplies>(
        [](auto x, auto y) -> Unit::unit_t {
          // Creation of z needed here because putting x*y inside the call to
          // isKnownUnit(x*y) leads to error: temporary of non-literal type in
          // a constant expression
          if constexpr (isKnownUnit(x * y))
            return x * y;
          else
            static_assert(always_false<decltype(x)>::value,
                          "Broken myvisit implementation!");
        },
        a(), b()));
  } catch (std::bad_variant_access &) {
    throw std::runtime_error("Unsupported unit as result of multiplication: (" +
                             a.name() + ") * (" + b.name() + ')');
  }
}

Unit operator/(const Unit &a, const Unit &b) {
  try {
    return Unit(myvisit<divides>(
        [](auto x, auto y) -> Unit::unit_t {
          // It is done here to have the si::dimensionless then the units are
          // the same, but is the si::dimensionless valid for non si types? TODO
          if constexpr (std::is_same_v<decltype(x), decltype(y)>)
            return dimensionless;
          auto z{x / y};
          if constexpr (isKnownUnit(z))
            return z;
          else
            static_assert(always_false<decltype(x)>::value,
                          "Broken myvisit implementation!");
        },
        a(), b()));
  } catch (std::bad_variant_access &) {
    throw std::runtime_error("Unsupported unit as result of division: (" +
                             a.name() + ") * (" + b.name() + ')');
  }
}

// all indices of alternatives that have a valid sqrt
template <size_t... Out>
constexpr auto get_valid_indices(std::index_sequence<Out...> out) {
  return out;
}
template <size_t I, size_t... In, size_t... Out>
constexpr auto get_valid_indices(std::index_sequence<Out...>) {
  constexpr std::variant_alternative_t<I, Unit::unit_t> x;
  typename decltype(sqrt(1.0 * x))::unit_type sqrt_x;
  if constexpr (isKnownUnit(sqrt_x))
    return get_valid_indices<In...>(std::index_sequence<Out..., I>{});
  else
    return get_valid_indices<In...>(std::index_sequence<Out...>{});
}
template <size_t... In>
constexpr auto valid_indices(std::index_sequence<In...>) {
  std::index_sequence<> out{};
  return get_valid_indices<In...>(out);
}

Unit sqrt(const Unit &a) {
  constexpr auto indices = valid_indices(
      std::make_index_sequence<std::variant_size_v<Unit::unit_t>>{});
  try {
    return Unit(myvisit(
        [](auto x) -> Unit::unit_t {
          typename decltype(sqrt(1.0 * x))::unit_type sqrt_x;
          return sqrt_x;
        },
        a(), indices));
  } catch (std::bad_variant_access &) {
    throw std::runtime_error("Unsupported unit as result of sqrt: sqrt(" +
                             a.name() + ").");
  }

  //return Unit(std::visit(
  //    [](auto x) -> Unit::unit_t {
  //      typename decltype(sqrt(1.0 * x))::unit_type sqrt_x;
  //      if constexpr (isKnownUnit(sqrt_x))
  //        return sqrt_x;
  //      throw std::runtime_error("Unsupported unit as result of sqrt: sqrt(" +
  //                               units::to_string(x) + ").");
  //    },
  //    a()));
}

namespace units {
bool containsCounts(const Unit &unit) {
  if ((unit == units::counts) || unit == units::counts / units::us)
    return true;
  return false;
}
bool containsCountsVariance(const Unit &unit) {
  if (unit == units::counts * units::counts ||
      unit == (units::counts / units::us) * (units::counts / units::us))
    return true;
  return false;
}
} // namespace units
