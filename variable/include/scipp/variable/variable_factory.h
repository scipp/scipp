// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/variable/variable.h"

namespace scipp::variable {

/// Abstract base class for "variable makers", used by VariableFactory to
/// dynamically create variables with given type.
class SCIPP_VARIABLE_EXPORT AbstractVariableMaker {
public:
  virtual ~AbstractVariableMaker() = default;
  virtual Variable create(const Dimensions &dims,
                          const bool variances) const = 0;
  virtual Variable create_buckets(const DType, const Dimensions &, const bool,
                                  const VariableConstView &) const {
    throw std::runtime_error("abc");
  }
  virtual Variable create_buckets(const DType, const Dimensions &, const bool,
                                  const VariableConstView &,
                                  const VariableConstView &) const {
    throw std::runtime_error("abc");
  }
  virtual Variable create_buckets(const DType, const Dimensions &, const bool,
                                  const VariableConstView &,
                                  const VariableConstView &,
                                  const VariableConstView &) const {
    throw std::runtime_error("abc");
  }
};

template <class T> class VariableMaker : public AbstractVariableMaker {
  using AbstractVariableMaker::create;
  Variable create(const Dimensions &dims, const bool variances) const override {
    const auto volume = dims.volume();
    if (variances)
      return makeVariable<T>(dims, Values(volume, core::default_init_elements),
                             Variances(volume, core::default_init_elements));
    else
      return makeVariable<T>(dims, Values(volume, core::default_init_elements));
  }
};

/// Dynamic factory for variables.
///
/// The factory can be used for creating variables with a dtype that is not
/// known in the current module, e.g., dtype<bucket<Dataset>> can be used in
/// scipp::variable. The main prupose of this is the implementation of
/// `transform`.
class SCIPP_VARIABLE_EXPORT VariableFactory {
public:
  void emplace(const DType key, std::unique_ptr<AbstractVariableMaker> makes);
  bool contains(const DType key) const noexcept;
  Variable create(const DType key, const Dimensions &dims,
                  const bool variances) const;
  template <class... Parents>
  Variable create_buckets(const DType key, const DType elem_dtype,
                          const Dimensions &dims, const bool variances,
                          const Parents &... parents) const {
    return m_makers.at(key)->create_buckets(elem_dtype, dims, variances,
                                            parents...);
  }

private:
  std::map<DType, std::unique_ptr<AbstractVariableMaker>> m_makers;
};

/// Return the global variable factory instance
SCIPP_VARIABLE_EXPORT VariableFactory &variableFactory();

} // namespace scipp::variable
