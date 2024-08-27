// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/eigen.h"
#include "scipp/core/spatial_transforms.h"
#include "scipp/variable/structure_array_variable.tcc"
#include "scipp/variable/structures.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

// TODO For now we use hard-coded field names and offsets. The intention is to
// generalize StructureArrayModel to support more general structures. Field
// names and sizes/offsets would then be stored as part of the model, and would
// be initialized dynamically at runtime.
template <>
constexpr auto structure_element_offset<Eigen::Vector3d> =
    [](const std::string &key) {
      static std::map<std::string, scipp::index> offsets{
          {"x", 0}, {"y", 1}, {"z", 2}};
      return offsets.at(key);
    };

template <>
constexpr auto structure_element_offset<Eigen::Matrix3d> =
    [](const std::string &key) {
      static std::map<std::string, scipp::index> offsets{
          {"xx", 0}, {"xy", 3}, {"xz", 6}, {"yx", 1}, {"yy", 4},
          {"yz", 7}, {"zx", 2}, {"zy", 5}, {"zz", 8}};
      return offsets.at(key);
    };

template <>
constexpr auto structure_element_offset<Eigen::Affine3d> =
    []([[maybe_unused]] const std::string &key) -> scipp::index {
  throw except::TypeError("Not supported for Affine3d types");
};

template <>
constexpr auto structure_element_offset<scipp::core::Quaternion> =
    []([[maybe_unused]] const std::string &key) -> scipp::index {
  throw except::TypeError("Not supported for Affine3d types");
};

template <>
constexpr auto structure_element_offset<scipp::core::Translation> =
    []([[maybe_unused]] const std::string &key) -> scipp::index {
  throw except::TypeError("Not supported for Affine3d types");
};

template <>
constexpr auto structure_element_offset<scipp::index_pair> =
    [](const std::string &key) {
      static std::map<std::string, scipp::index> offsets{{"begin", 0},
                                                         {"end", 1}};
      return offsets.at(key);
    };

std::vector<std::string> element_keys(const Variable &var) {
  if (variableFactory().elem_dtype(var) == dtype<Eigen::Vector3d>)
    return {"x", "y", "z"};
  if (variableFactory().elem_dtype(var) == dtype<Eigen::Matrix3d>)
    return {"xx", "xy", "xz", "yx", "yy", "yz", "zx", "zy", "zz"};
  if (variableFactory().elem_dtype(var) == dtype<Eigen::Affine3d>)
    throw except::TypeError("Not supported for Affine3d types");
  if (variableFactory().elem_dtype(var) == dtype<scipp::index_pair>)
    return {"begin", "end"};
  throw except::TypeError("dtype is not structured");
}

// Defining the destructor here ensures that all users of StructureArrayModel
// use the same visibility of that class. Otherwise, instantiating the
// StructureArrayModel template creates a class without visibility attribute.
// But INSTANTIATE_STRUCTURE_ARRAY_VARIABLE below gives it SCIPP_EXPORT
// visibility which would cause a mismatch.
// With this explicit definition, instantiation is deferred.
// This was observed with Apple Clang 13 and using
// std::make_shared<StructureArrayModel<T, Elem>> in structures.cpp
template <class T_, class Elem_>
StructureArrayModel<T_, Elem_>::~StructureArrayModel() = default;

INSTANTIATE_STRUCTURE_ARRAY_VARIABLE(vector3, Eigen::Vector3d, double)
INSTANTIATE_STRUCTURE_ARRAY_VARIABLE(linear_transform3, Eigen::Matrix3d, double)
INSTANTIATE_STRUCTURE_ARRAY_VARIABLE(affine_transform3, Eigen::Affine3d, double)
INSTANTIATE_STRUCTURE_ARRAY_VARIABLE(rotation3, scipp::core::Quaternion, double)
INSTANTIATE_STRUCTURE_ARRAY_VARIABLE(translation3, scipp::core::Translation,
                                     double)
INSTANTIATE_STRUCTURE_ARRAY_VARIABLE(index_pair, scipp::index_pair,
                                     scipp::index)

} // namespace scipp::variable
