// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

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
  virtual ~AbstractVariableMaker() = default;
  virtual bool is_bins() const = 0;
  virtual Variable
  create(const DType elem_dtype, const Dimensions &dims,
         const units::Unit &unit, const bool variances,
         const std::vector<VariableConstView> &parents) const = 0;
  virtual Dim elem_dim(const VariableConstView &var) const = 0;
  virtual DType elem_dtype(const VariableConstView &var) const = 0;
  virtual units::Unit elem_unit(const VariableConstView &var) const = 0;
  virtual void expect_can_set_elem_unit(const VariableView &var,
                                        const units::Unit &u) const = 0;
  virtual void set_elem_unit(const VariableView &var,
                             const units::Unit &u) const = 0;
  virtual bool hasVariances(const VariableConstView &var) const = 0;
  virtual VariableConstView data(const VariableConstView &) const {
    throw unreachable();
  }
  virtual VariableView data(const VariableView &) const { throw unreachable(); }
  virtual core::ElementArrayViewParams
  array_params(const VariableConstView &) const {
    throw unreachable();
  }
  virtual Variable empty_like(const VariableConstView &prototype,
                              const std::optional<Dimensions> &shape,
                              const VariableConstView &sizes) const = 0;
};

template <class T> class VariableMaker : public AbstractVariableMaker {
  using AbstractVariableMaker::create;
  bool is_bins() const override { return false; }
  Variable create(const DType, const Dimensions &dims, const units::Unit &unit,
                  const bool variances,
                  const std::vector<VariableConstView> &) const override {
    const auto volume = dims.volume();
    if (variances)
      return makeVariable<T>(dims, unit,
                             Values(volume, core::default_init_elements),
                             Variances(volume, core::default_init_elements));
    else
      return makeVariable<T>(dims, unit,
                             Values(volume, core::default_init_elements));
  }
  Dim elem_dim(const VariableConstView &) const override {
    return Dim::Invalid;
  }
  DType elem_dtype(const VariableConstView &var) const override {
    return var.dtype();
  }
  units::Unit elem_unit(const VariableConstView &var) const override {
    return var.unit();
  }
  void expect_can_set_elem_unit(const VariableView &var,
                                const units::Unit &u) const override {
    var.expectCanSetUnit(u);
  }
  void set_elem_unit(const VariableView &var,
                     const units::Unit &u) const override {
    var.setUnit(u);
  }
  bool hasVariances(const VariableConstView &var) const override {
    return var.hasVariances();
  }
  Variable empty_like(const VariableConstView &prototype,
                      const std::optional<Dimensions> &shape,
                      const VariableConstView &sizes) const override {
    if (sizes)
      throw except::TypeError(
          "Cannot specify sizes in `empty_like` for non-bin prototype.");
    return create(prototype.dtype(), shape ? *shape : prototype.dims(),
                  prototype.unit(), prototype.hasVariances(), {});
  }
};

SCIPP_VARIABLE_EXPORT bool is_bins(const VariableConstView &var);

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
    return is_bins(var) ? var.dtype() : dtype<void>;
  }
  template <class T, class... Ts>
  auto bucket_dtype(const T &var, const Ts &... vars) const noexcept {
    return is_bins(var) ? var.dtype() : bucket_dtype(vars...);
  }

public:
  void emplace(const DType key, std::unique_ptr<AbstractVariableMaker> makes);
  bool contains(const DType key) const noexcept;
  bool is_bins(const VariableConstView &var) const;
  template <class... Parents>
  Variable create(const DType elem_dtype, const Dimensions &dims,
                  const units::Unit &unit, const bool variances,
                  const Parents &... parents) const {
    const auto key = bucket_dtype(parents...);
    return m_makers.at(key == dtype<void> ? elem_dtype : key)
        ->create(elem_dtype, dims, unit, variances,
                 std::vector<VariableConstView>{parents...});
  }
  Dim elem_dim(const VariableConstView &var) const;
  DType elem_dtype(const VariableConstView &var) const;
  units::Unit elem_unit(const VariableConstView &var) const;
  void expect_can_set_elem_unit(const VariableView &var,
                                const units::Unit &u) const;
  void set_elem_unit(const VariableView &var, const units::Unit &u) const;
  bool hasVariances(const VariableConstView &var) const;
  template <class T, class Var> auto values(Var &&var) const {
    if (!is_bins(var))
      return var.template values<T>();
    const auto &maker = *m_makers.at(var.dtype());
    auto &&data = maker.data(view(var));
    return ElementArrayView(maker.array_params(var),
                            data.template values<T>().data());
  }
  template <class T, class Var> auto variances(Var &&var) const {
    if (!is_bins(var))
      return var.template variances<T>();
    const auto &maker = *m_makers.at(var.dtype());
    auto &&data = maker.data(view(var));
    return ElementArrayView(maker.array_params(var),
                            data.template variances<T>().data());
  }
  Variable empty_like(const VariableConstView &prototype,
                      const std::optional<Dimensions> &shape,
                      const VariableConstView &sizes = {});

private:
  VariableConstView view(const VariableConstView &var) const { return var; }
  VariableView view(Variable &var) const { return var; }
  VariableView view(const VariableView &var) const { return var; }
  std::map<DType, std::unique_ptr<AbstractVariableMaker>> m_makers;
};

/// Return the global variable factory instance
SCIPP_VARIABLE_EXPORT VariableFactory &variableFactory();

} // namespace scipp::variable
