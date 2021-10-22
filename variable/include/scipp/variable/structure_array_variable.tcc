// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/bins.h"
#include "scipp/variable/element_array_variable.tcc"
#include "scipp/variable/structure_array_model.h"

namespace scipp::variable {

template <class T>
constexpr auto structure_element_offset{
    T::missing_specialization_of_structure_element_offset};

template <class T, class... Is>
Variable Variable::elements_impl(Is... index) const {
  if (dtype() == core::dtype<core::bin<Variable>>) {
    const auto &[idx, dim, buf] = constituents<Variable>();
    return make_bins_no_validate(idx, dim, buf.template elements<T>(index...));
  }
  constexpr auto N = model_t<T>::element_count;
  auto elements(*this);
  elements.m_object = cast<T>(*this).elements();
  // Scale offset and strides (which refer to type T) so they are correct for
  // the *element type* of T.
  elements.m_offset *= N;
  for (scipp::index i = 0; i < dims().ndim(); ++i)
    elements.m_strides[i] = N * strides()[i];
  if constexpr (sizeof...(Is) == 0) {
    // Get all elements by setting up internal dim and stride
    elements.unchecked_dims().addInner(Dim::InternalStructureComponent, N);
    elements.unchecked_strides()[dims().ndim()] = 1;
  } else {
    // Get specific element at offset
    const auto offset = structure_element_offset<T>(index...);
    if (offset < 0 || offset >= N)
      throw std::out_of_range("Bad offset");
    elements.m_offset += offset;
  }
  return elements;
}

template <class T> Variable Variable::elements() const {
  return elements_impl<T>();
}

template <class T> Variable Variable::elements(const std::string &key) const {
  return elements_impl<T>(key);
}

template <class T, class Elem>
void StructureArrayModel<T, Elem>::assign(const VariableConcept &other) {
  *this = requireT<const StructureArrayModel<T, Elem>>(other);
}

template <class T, class Elem>
const T *StructureArrayModel<T, Elem>::get_values() const {
  return reinterpret_cast<const T *>(
      requireT<const ElementArrayModel<Elem>>(*m_elements).values().data());
}

template <class T, class Elem> T *StructureArrayModel<T, Elem>::get_values() {
  return reinterpret_cast<T *>(
      requireT<ElementArrayModel<Elem>>(*m_elements).values().data());
}

template <class T> struct arg_type;
template <class T, class U> struct arg_type<T(U)> { using type = U; };

/// Instantiate Variable for structure dtype with element access.
#define INSTANTIATE_STRUCTURE_ARRAY_VARIABLE(name, T, Elem)                    \
  template <> struct model<arg_type<void(T)>::type> {                          \
    using type = StructureArrayModel<arg_type<void(T)>::type, Elem>;           \
  };                                                                           \
  template SCIPP_EXPORT Variable Variable::elements<arg_type<void(T)>::type>() \
      const;                                                                   \
  template SCIPP_EXPORT Variable Variable::elements<arg_type<void(T)>::type>(  \
      const std::string &) const;                                              \
  template class SCIPP_EXPORT                                                  \
      StructureArrayModel<arg_type<void(T)>::type, Elem>;                      \
  INSTANTIATE_ELEMENT_ARRAY_VARIABLE_BASE(name, arg_type<void(T)>::type)

} // namespace scipp::variable
