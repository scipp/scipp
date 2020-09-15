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
  virtual DType elem_dtype(const VariableConstView &var) const = 0;
  virtual bool hasVariances(const VariableConstView &var) const = 0;
  virtual VariableConstView data(const VariableConstView &var) const = 0;
  virtual VariableView data(const VariableView &var) const = 0;
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
  DType elem_dtype(const VariableConstView &var) const override {
    return var.dtype();
  }
  bool hasVariances(const VariableConstView &var) const override {
    return var.hasVariances();
  }
  VariableConstView data(const VariableConstView &var) const override {
    return var.underlying();
  }
  VariableView data(const VariableView &var) const override {
    return var.underlying();
  }
};

/// Dynamic factory for variables.
///
/// The factory can be used for creating variables with a dtype that is not
/// known in the current module, e.g., dtype<bucket<Dataset>> can be used in
/// scipp::variable. The main prupose of this is the implementation of
/// `transform`.
class SCIPP_VARIABLE_EXPORT VariableFactory {
private:
  auto bucket_dtype() const noexcept { return dtype<void>; }
  template <class T> auto bucket_dtype(const T &var) const noexcept {
    return var.dtype() == elem_dtype(var) ? dtype<void> : var.dtype();
  }
  template <class T, class... Ts>
  auto bucket_dtype(const T &var, const Ts &... vars) const noexcept {
    return var.dtype() == elem_dtype(var) ? bucket_dtype(vars...) : var.dtype();
  }

public:
  void emplace(const DType key, std::unique_ptr<AbstractVariableMaker> makes);
  bool contains(const DType key) const noexcept;
  template <class... Parents>
  Variable create(const DType elem_dtype, const Dimensions &dims,
                  const bool variances, const Parents &... parents) const {
    const auto key = bucket_dtype(parents...);
    if(key == dtype<void>)
      return m_makers.at(elem_dtype)->create(dims, variances);
    else {
      if constexpr (sizeof...(parents) < 1)
        throw std::runtime_error("No parent for obtaining bucket params");
      else
        return m_makers.at(key)->create_buckets(elem_dtype, dims, variances,
                                                parents...);
    }
  }
  DType elem_dtype(const VariableConstView &var) const;
  bool hasVariances(const VariableConstView &var) const;
  template <class T, class Var> auto values(Var &&var) const {
    auto &&data = m_makers.at(var.dtype())->data(view(var));
    return ElementArrayView(base_view(var), data.template values<T>().data());
  }
  template <class T, class Var> auto variances(Var &&var) const {
    auto &&data = m_makers.at(var.dtype())->data(view(var));
    return ElementArrayView(base_view(var),
                            data.template variances<T>().data());
  }

private:
  VariableConstView view(const VariableConstView &var) const { return var; }
  VariableView view(Variable &var) const { return var; }
  VariableView view(const VariableView &var) const { return var; }
  core::element_array_view base_view(const VariableConstView &var) const {
    return var.base_view();
  }
  std::map<DType, std::unique_ptr<AbstractVariableMaker>> m_makers;
};

/// Return the global variable factory instance
SCIPP_VARIABLE_EXPORT VariableFactory &variableFactory();

} // namespace scipp::variable
