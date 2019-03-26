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

// all indices of alternatives that have a valid product
template <size_t N1, size_t... Out1, size_t... Out2>
constexpr auto product_valid_impl(std::index_sequence<Out1...> out1,
                                  std::index_sequence<Out2...> out2) {
  return std::pair{out1, out2};
}
template <size_t N1, size_t N2, size_t... N2s, size_t... Out1, size_t... Out2>
constexpr auto product_valid_impl(std::index_sequence<Out1...>,
                                  std::index_sequence<Out2...>) {
  constexpr std::variant_alternative_t<N1, Unit::unit_t> x;
  constexpr std::variant_alternative_t<N2, Unit::unit_t> y;
  if constexpr (isKnownUnit(x * y))
    return product_valid_impl<N1, N2s..., Out1..., N1, Out2..., N2>(
        std::index_sequence<Out1..., N1>{}, std::index_sequence<Out2..., N2>{});
  else
    return product_valid_impl<N1, N2s..., Out1..., Out2...>(
        std::index_sequence<Out1...>{}, std::index_sequence<Out2...>{});
}
template <size_t N1, size_t... N2>
constexpr auto product_valid(std::index_sequence<N2...>) {
  std::index_sequence<> out1{};
  std::index_sequence<> out2{};
  return product_valid_impl<N1, N2...>(out1, out2);
}

template <auto... Vals> struct value_sequence {};
template <auto... As, auto... Bs>
constexpr value_sequence<As..., Bs...> operator+(value_sequence<As...>,
                                                 value_sequence<Bs...>) {
  return {};
}

template <size_t N1, size_t... N2>
constexpr auto expand(std::index_sequence<N2...>) {
  return std::tuple{std::pair{N1, N2}...};
}

template <size_t... N1, size_t... N2>
constexpr auto expand(std::index_sequence<N1...> n1, std::index_sequence<N2...> n2) {
  //return std::tuple_cat(expand<N1>(product_valid<N1>(n2))...);
  return std::pair{(product_valid<N1>(n2).first + ...),
                   (product_valid<N1>(n2).second + ...)};
}

template <class F, class Variant, size_t... N1, size_t... N2>
decltype(auto) invoke_active(F &&f, Variant &&v1, Variant &&v2,
                             std::index_sequence<N1...>,
                             std::index_sequence<N2...>) {
  using Ret = decltype(std::invoke(std::forward<F>(f),
                                   std::get<0>(std::forward<Variant>(v1)),
                                   std::get<0>(std::forward<Variant>(v2))));

  if constexpr (!std::is_same_v<void, Ret>) {
    Ret ret;
    if (!(((v1.index() == N1) && (v2.index() == N2)
               ? (ret = std::invoke(std::forward<F>(f),
                                    std::get<N1>(std::forward<Variant>(v1)),
                                    std::get<N2>(std::forward<Variant>(v2))),
                  true)
               : false) ||
          ...))
      throw std::bad_variant_access{};

    return ret;
  } else {
    throw std::runtime_error("???");
  }
}

template <class F, class Variant>
decltype(auto) myvisit(F &&f, Variant &&var1, Variant &&var2) {
  constexpr auto indices = expand(std::make_index_sequence<
                 std::variant_size_v<std::remove_reference_t<Variant>>>{},
             std::make_index_sequence<
                 std::variant_size_v<std::remove_reference_t<Variant>>>{});
  return invoke_active(
      std::forward<F>(f), std::forward<Variant>(var1),
      std::forward<Variant>(var2), indices.first, indices.second);
}

// Mutliplying two units together using std::visit to run through the contents
// of the std::variant
Unit operator*(const Unit &a, const Unit &b) {
  const auto result = myvisit(
      [](auto x, auto y) -> Unit::unit_t {
        // Creation of z needed here because putting x*y inside the call to
        // isKnownUnit(x*y) leads to error: temporary of non-literal type in
        // a constant expression
        if constexpr (isKnownUnit(x*y))
          return x*y;
        //return {};
        //return x;
        throw std::runtime_error(
            "Unsupported unit as result of multiplication: (" +
            units::to_string(x) + ") * (" + units::to_string(y) + ')');
      },
      a(), b());
  return Unit(result);
  //if (result)
  //  return Unit(*result);
  //throw std::runtime_error("Unsupported unit as result of multiplication: (" +
  //                         a.name() + ") * (" + b.name() + ')');
}

Unit operator/(const Unit &a, const Unit &b) {
  return Unit(std::visit(
      [](auto x, auto y) -> Unit::unit_t {
        // It is done here to have the si::dimensionless then the units are
        // the same, but is the si::dimensionless valid for non si types? TODO
        if constexpr (std::is_same_v<decltype(x), decltype(y)>)
          return dimensionless;
        auto z{x / y};
        if constexpr (isKnownUnit(z))
          return z;
        throw std::runtime_error("Unsupported unit as result of division: (" +
                                 units::to_string(x) + ") / (" +
                                 units::to_string(y) + ')');
      },
      a(), b()));
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
