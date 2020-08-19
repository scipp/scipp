// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
#include "scipp/common/deep_ptr.h"
#include "scipp/common/index.h"
#include "scipp/common/span.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/dtype.h"
#include "scipp/core/element_array_view.h"

#include <Eigen/Dense>

#include <memory>
#include <vector>

namespace scipp::variable {

class Variable;
class VariableConstView;
class VariableView;
class VariableConcept;

class SCIPP_VARIABLE_EXPORT VariableConceptHandle
    : public scipp::deep_ptr<VariableConcept> {
public:
  using scipp::deep_ptr<VariableConcept>::deep_ptr;
  template <class T> VariableConceptHandle(T object);
  VariableConceptHandle(VariableConceptHandle &&) = default;
  VariableConceptHandle(const VariableConceptHandle &other);
  VariableConceptHandle &operator=(VariableConceptHandle &&) = default;
  VariableConceptHandle &operator=(const VariableConceptHandle &other);
};

/// Abstract base class for any data that can be held by Variable. This is using
/// so-called concept-based polymorphism, see talks by Sean Parent.
///
/// This is the most generic representation for a multi-dimensional array of
/// data. More operations are supportd by the typed DataModel.
class SCIPP_VARIABLE_EXPORT VariableConcept {
public:
  VariableConcept(const Dimensions &dimensions);
  virtual ~VariableConcept() = default;

  virtual VariableConceptHandle clone() const = 0;
  virtual VariableConceptHandle
  makeDefaultFromParent(const Dimensions &dims) const = 0;

  virtual DType dtype() const noexcept = 0;
  const Dimensions &dims() const { return m_dimensions; }

  virtual bool hasVariances() const noexcept = 0;
  virtual void setVariances(Variable &&variances) = 0;

  virtual bool equals(const VariableConstView &a,
                      const VariableConstView &b) const = 0;
  virtual void copy(const VariableConstView &src,
                    const VariableView &dest) const = 0;
  virtual void assign(const VariableConcept &other) = 0;

  friend class Variable;

private:
  Dimensions m_dimensions;
};

template <class T>
VariableConceptHandle::VariableConceptHandle(T object)
    : VariableConceptHandle(
          std::unique_ptr<VariableConcept>(std::move(object))) {}

} // namespace scipp::variable
