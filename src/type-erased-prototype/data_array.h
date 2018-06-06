#ifndef DATA_ARRAY_H
#define DATA_ARRAY_H

#include "cow_ptr.h"
#include "index.h"

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

class DataArray{
public:
  template <class T>
  DataArray(uint32_t id, T object)
      : m_type(id),
        m_object(std::make_unique<DataArrayModel<T>>(std::move(object))) {}
  DataArray(const DataArray&other)
      : m_type(other.m_type), m_object(m_object) {}

  gsl::index size() const { return m_object->size(); }
  void resize(const gsl::index size) { m_object.access().resize(size); }

  template <class T> const T &cast() const {
    return dynamic_cast<DataArrayModel<T> &>(*m_object).m_model;
  }

  template <class T> T &cast() {
    return dynamic_cast<DataArrayModel<T> &>(m_object.access()).m_model;
  }

  uint32_t type() const { return m_type; }

private:
  const uint32_t m_type;
  cow_ptr<DataArrayConcept> m_object;
};


#endif // DATA_ARRAY_H
