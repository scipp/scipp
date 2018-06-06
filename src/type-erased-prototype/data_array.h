#ifndef DATA_ARRAY_H
#define DATA_ARRAY_H

#include <type_traits>

#include "cow_ptr.h"
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
  DataArray(uint32_t id, T object)
      : m_type(id),
        m_object(std::make_unique<DataArrayModel<T>>(std::move(object))) {}
  DataArray(const DataArray &other)
      : m_type(other.m_type), m_object(other.m_object) {}

  gsl::index size() const { return m_object->size(); }
  void resize(const gsl::index size) { m_object.access().resize(size); }

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

  const uint32_t m_type;
  cow_ptr<DataArrayConcept> m_object;
};

template <class Tag, class... Args> DataArray makeDataArray(Args &&... args) {
  return DataArray(Tag::type_id,
                   variable_type_t<Tag>(std::forward<Args>(args)...));
}

template <class Tag, class T>
DataArray makeDataArray(std::initializer_list<T> values) {
  return DataArray(Tag::type_id, variable_type_t<Tag>(values));
}

#endif // DATA_ARRAY_H
