#ifndef UNITS_H
#define UNITS_H

#include <boost/units/systems/si/dimensionless.hpp>
#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/area.hpp>

enum class UnitId { Dimensionless, Length, Area };

UnitId makeUnitId(const boost::units::si::dimensionless &u) {
  return UnitId::Dimensionless;
}
UnitId makeUnitId(const boost::units::si::length &u) { return UnitId::Length; }
UnitId makeUnitId(const boost::units::si::area &u) { return UnitId::Area; }
template <class T> UnitId makeUnitId(const T &u) {
  throw std::runtime_error("Unsupported unit combination");
}

UnitId operator+(const UnitId a, const UnitId b) {
  if (a != b)
    throw std::runtime_error("Cannot add different units");
  return a;
}

template <class A> UnitId multiply(const A &a, const UnitId b) {
  if (b == UnitId::Dimensionless)
    return makeUnitId(a * boost::units::si::dimensionless{});
  if (b == UnitId::Length)
    return makeUnitId(a * boost::units::si::length{});
  if (b == UnitId::Area)
    return makeUnitId(a * boost::units::si::area{});
  throw std::runtime_error("Unsupported unit on RHS");
}

UnitId operator*(const UnitId a, const UnitId b) {
  if (a == UnitId::Dimensionless)
    return multiply(boost::units::si::dimensionless{}, b);
  if (a == UnitId::Length)
    return multiply(boost::units::si::length{}, b);
  if (a == UnitId::Area)
    return multiply(boost::units::si::area{}, b);
}

#endif // UNITS_H
