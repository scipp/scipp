#ifndef VARIABLE_H
#define VARIABLE_H

struct Variable {
  // struct DetectorPosition {};
  // struct SpectrumPosition {};
  struct TofEdge {
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
};

template <class Tag> struct variable_type;
template <class Tag> struct element_reference_type;

template <> struct variable_type<Variable::TofEdge> {
  using type = std::vector<double>;
};
template <> struct variable_type<const Variable::TofEdge> {
  using type = const std::vector<double>;
};

// template <> struct variable_type<Variable::TofBin> {
//  using type = Bins<TofEdge>;
//};
// template <> struct variable_type<const Variable::TofBin> {
//  using type = const Bins<TofEdge>;
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

template <class Tag> using variable_type_t = typename variable_type<Tag>::type;
template <class Tag>
using element_reference_type_t = typename element_reference_type<Tag>::type;

#endif // VARIABLE_H
