#ifndef DATA_ARRAY_H
#define DATA_ARRAY_H

#include <type_traits>

#include "cow_ptr.h"
#include "dimensions.h"
#include "index.h"
#include "variable.h"

class DataArrayConcept {
public:
  virtual ~DataArrayConcept() = default;
  virtual std::unique_ptr<DataArrayConcept> clone() const = 0;
  virtual gsl::index size() const = 0;
  virtual void resize(const gsl::index) = 0;
};

template <class T> class DataArrayModel : public DataArrayConcept {
public:
  DataArrayModel(T const &model) : m_model(model) {}
  std::unique_ptr<DataArrayConcept> clone() const override {
    return std::make_unique<DataArrayModel<T>>(m_model);
  }
  gsl::index size() const override { return m_model.size(); }
  void resize(const gsl::index size) override { m_model.resize(size); }

  T m_model;
};

class DataArray {
public:
  template <class T>
  DataArray(uint32_t id, Dimensions dimensions, T object)
      : m_type(id), m_dimensions(std::move(dimensions)),
        m_object(std::make_unique<DataArrayModel<T>>(std::move(object))) {
    if (m_dimensions.volume() != m_object->size())
      throw std::runtime_error("Creating DataArray: data size does not match "
                               "volume given by dimension extents");
  }

  gsl::index size() const { return m_object->size(); }
  void resize(const gsl::index size) { m_object.access().resize(size); }

  const Dimensions &dimensions() const { return m_dimensions; }

  template <class Tag>
  bool valueTypeIs() const {
    return Tag::type_id == m_type;
  }

  uint32_t type() const { return m_type; }

  template <class Tag> const variable_type_t<Tag> &get() const {
    return cast<std::remove_const_t<variable_type_t<Tag>>>();
  }

  template <class Tag>
  variable_type_t<Tag> &
  get(std::enable_if_t<std::is_const<Tag>::value> * = nullptr) {
    return const_cast<const DataArray *>(this)->get<Tag>();
  }

  template <class Tag>
  variable_type_t<Tag> &
  get(std::enable_if_t<!std::is_const<Tag>::value> * = nullptr) {
    return cast<std::remove_const_t<variable_type_t<Tag>>>();
  }

private:
  template <class T> const T &cast() const {
    return dynamic_cast<const DataArrayModel<T> &>(*m_object).m_model;
  }

  template <class T> T &cast() {
    return dynamic_cast<DataArrayModel<T> &>(m_object.access()).m_model;
  }

  uint32_t m_type;
  Dimensions m_dimensions;
  cow_ptr<DataArrayConcept> m_object;
};

template <class Tag, class... Args> DataArray makeDataArray(Dimensions dimensions, Args &&... args) {
  return DataArray(Tag::type_id, std::move(dimensions),
                   variable_type_t<Tag>(std::forward<Args>(args)...));
}

template <class Tag, class T>
DataArray makeDataArray(Dimensions dimensions,std::initializer_list<T> values) {
  return DataArray(Tag::type_id, std::move(dimensions),
                   variable_type_t<Tag>(values));
}

#endif // DATA_ARRAY_H
