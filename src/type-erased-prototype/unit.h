#ifndef UNIT_H
#define UNIT_H

class Unit {
public:
  enum class Id { Dimensionless, Length, Area };
  // TODO should this be explicit?
  Unit(const Unit::Id id) : m_id(id) {}

  const Unit::Id &id() const { return m_id; }

private:
  Id m_id;
  // TODO need to support scale
};

inline bool operator==(const Unit &a, const Unit &b) {
  return a.id() == b.id();
}
inline bool operator!=(const Unit &a, const Unit &b) { return !(a == b); }

Unit operator+(const Unit &a, const Unit &b);
Unit operator*(const Unit &a, const Unit &b);

#endif // UNIT_H
