// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <functional>

#include "scipp/variable/variable.h"

namespace scipp::variable {

/// Abstract base class for "variable makers", used by VariableFactory to
/// dynamically create variables with given type.
class SCIPP_VARIABLE_EXPORT AbstractVariableMaker {
protected:
  auto unreachable() const {
    return std::logic_error("This code path should never be reached");
  }

public:
  using parent_list = std::vector<std::reference_wrapper<const Variable>>;

  virtual ~AbstractVariableMaker() = default;
  virtual bool is_bins() const = 0;
  virtual Variable create(const DType elem_dtype, const Dimensions &dims,
                          const units::Unit &unit, const bool variances,
                          const parent_list &parents) const = 0;
  virtual Dim elem_dim(const Variable &var) const = 0;
  virtual DType elem_dtype(const Variable &var) const = 0;
  virtual units::Unit elem_unit(const Variable &var) const = 0;
  virtual void expect_can_set_elem_unit(const Variable &var,
                                        const units::Unit &u) const = 0;
  virtual void set_elem_unit(Variable &var, const units::Unit &u) const = 0;
  virtual bool has_masks(const Variable &) const { return false; }
  virtual bool has_variances(const Variable &var) const = 0;
  virtual const Variable &data(const Variable &) const { throw unreachable(); }
  virtual Variable data(Variable &) const { throw unreachable(); }
  virtual core::ElementArrayViewParams array_params(const Variable &) const {
    throw unreachable();
  }
  virtual Variable empty_like(const Variable &prototype,
                              const std::optional<Dimensions> &shape,
                              const Variable &sizes) const = 0;
};

SCIPP_VARIABLE_EXPORT bool is_bins(const Variable &var);

/// Dynamic factory for variables.
///
/// The factory can be used for creating variables with a dtype that is not
/// known in the current module, e.g., dtype<bucket<Dataset>> can be used in
/// scipp::variable. The main purpose of this is the implementation of
/// `transform`.
class SCIPP_VARIABLE_EXPORT VariableFactory {
private:
  using parent_list = typename AbstractVariableMaker::parent_list;
  DType bin_dtype(const parent_list &vars) const noexcept;

public:
  VariableFactory() = default;
  VariableFactory(const VariableFactory &) = delete;
  VariableFactory &operator=(const VariableFactory &) = delete;
  void emplace(const DType key, std::unique_ptr<AbstractVariableMaker> makes);
  bool contains(const DType key) const noexcept;
  bool is_bins(const Variable &var) const;
  template <class... Parents>
  Variable create(const DType elem_dtype, const Dimensions &dims,
                  const units::Unit &unit, const bool with_variances,
                  const Parents &... parents) const {
    const auto parents_ = parent_list{parents...};
    const auto key = bin_dtype(parents_);
    return m_makers.at(key == dtype<void> ? elem_dtype : key)
        ->create(elem_dtype, dims, unit, with_variances, parents_);
  }
  Dim elem_dim(const Variable &var) const;
  DType elem_dtype(const Variable &var) const;
  units::Unit elem_unit(const Variable &var) const;
  void expect_can_set_elem_unit(const Variable &var,
                                const units::Unit &u) const;
  void set_elem_unit(Variable &var, const units::Unit &u) const;
  bool has_masks(const Variable &var) const;
  bool has_variances(const Variable &var) const;
  template <class T, class Var> auto values(Var &&var) const {
    if (!is_bins(var))
      return var.template values<T>();
    const auto &maker = *m_makers.at(var.dtype());
    auto &&data = maker.data(var);
    return ElementArrayView(maker.array_params(var),
                            data.template values<T>().data());
  }
  template <class T, class Var> auto variances(Var &&var) const {
    if (!is_bins(var))
      return var.template variances<T>();
    const auto &maker = *m_makers.at(var.dtype());
    auto &&data = maker.data(var);
    return ElementArrayView(maker.array_params(var),
                            data.template variances<T>().data());
  }
  Variable empty_like(const Variable &prototype,
                      const std::optional<Dimensions> &shape,
                      const Variable &sizes = {});

private:
  std::map<DType, std::unique_ptr<AbstractVariableMaker>> m_makers;
};

/// Return the global variable factory instance
SCIPP_VARIABLE_EXPORT VariableFactory &variableFactory();

} // namespace scipp::variable
