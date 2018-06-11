#ifndef VARIABLE_H
#define VARIABLE_H

#include <vector>

#include "index.h"

struct Variable {
  // struct DetectorPosition {};
  // struct SpectrumPosition {};
  struct Tof {
    static const uint32_t type_id = 0;
  };
  struct TofBin {
    static const uint32_t type_id = 1;
  };
  struct Value {
    static const uint32_t type_id = 2;
  };
  struct Error {
    static const uint32_t type_id = 3;
  };
  struct Int {
    static const uint32_t type_id = 4;
  };
  struct DimensionSize {
    static const uint32_t type_id = 5;
  };
};

class Bin {
public:
  Bin(const double left, const double right) : m_left(left), m_right(right) {}

  double center() const { return 0.5 * (m_left + m_right); }
  double width() const { return m_right - m_left; }
  double left() const { return m_left; }
  double right() const { return m_right; }

private:
  double m_left;
  double m_right;
};

template <class T> struct Bins { using value_type = T; };

template <class Tag> struct variable_type;
template <class Tag> struct element_reference_type;

template <> struct variable_type<Variable::Tof> {
  using type = std::vector<double>;
};
template <> struct variable_type<const Variable::Tof> {
  using type = const std::vector<double>;
};

template <> struct variable_type<Bins<Variable::Tof>> {
  using type = std::vector<double>;
};

// template <> struct variable_type<Variable::TofBin> {
//  using type = Bins<Tof>;
//};
// template <> struct variable_type<const Variable::TofBin> {
//  using type = const Bins<Tof>;
//};

template <> struct variable_type<Variable::Value> {
  using type = std::vector<double>;
};
template <> struct variable_type<const Variable::Value> {
  using type = const std::vector<double>;
};

template <> struct variable_type<Variable::Error> {
  using type = std::vector<double>;
};
template <> struct variable_type<const Variable::Error> {
  using type = const std::vector<double>;
};

template <> struct variable_type<Variable::Int> {
  using type = std::vector<int64_t>;
};
template <> struct variable_type<const Variable::Int> {
  using type = const std::vector<int64_t>;
};

template <> struct variable_type<Variable::DimensionSize> {
  using type = std::vector<gsl::index>;
};
template <> struct variable_type<const Variable::DimensionSize> {
  using type = const std::vector<gsl::index>;
};

template <> struct element_reference_type<Variable::Tof> {
  using type = double &;
};
template <> struct element_reference_type<const Variable::Tof> {
  using type = const double &;
};

template <> struct element_reference_type<Bins<Variable::Tof>> {
  // Note: No reference.
  using type = Bin;
};
template <> struct element_reference_type<Bins<const Variable::Tof>> {
  // Note: No reference.
  using type = Bin;
};

template <> struct element_reference_type<Variable::Value> {
  using type = double &;
};
template <> struct element_reference_type<const Variable::Value> {
  using type = const double &;
};

template <> struct element_reference_type<Variable::Error> {
  using type = double &;
};
template <> struct element_reference_type<const Variable::Error> {
  using type = const double &;
};

template <> struct element_reference_type<Variable::Int> {
  using type = int64_t &;
};
template <> struct element_reference_type<const Variable::Int> {
  using type = const int64_t &;
};

template <> struct element_reference_type<Variable::DimensionSize> {
  using type = gsl::index &;
};
template <> struct element_reference_type<const Variable::DimensionSize> {
  using type = const gsl::index &;
};

template <class Tag> using variable_type_t = typename variable_type<Tag>::type;
template <class Tag>
using element_reference_type_t = typename element_reference_type<Tag>::type;

#endif // VARIABLE_H
