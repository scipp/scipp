#ifndef DATASET_H
#define DATASET_H

#include <array>
#include <memory>
#include <vector>

#include "dimension.h"
#include "variable.h"

namespace gsl {
using index = ptrdiff_t;
}

using Doubles = std::vector<double>;
using Ints = std::vector<int>;

enum class ColumnType { Ints, Doubles };

template <class T> ColumnType getColumnType() {
  throw std::runtime_error(std::string("Type not registered in ADS ") +
                           typeid(T).name());
}

template <> inline ColumnType getColumnType<Ints>() { return ColumnType::Ints; }
template <> inline ColumnType getColumnType<Doubles>() {
  return ColumnType::Doubles;
}

class ColumnConcept {
public:
  virtual ~ColumnConcept() = default;
  virtual std::unique_ptr<ColumnConcept> clone() const = 0;
  virtual gsl::index size() const = 0;
  virtual void resize(const gsl::index) = 0;
};

template <class T> class ColumnModel : public ColumnConcept {
public:
  ColumnModel(T const &model) : m_model(model) {}
  std::unique_ptr<ColumnConcept> clone() const override {
    return std::make_unique<ColumnModel<T>>(m_model);
  }
  gsl::index size() const override { return m_model.size(); }
  void resize(const gsl::index size) override { m_model.resize(size); }

  T m_model;
};

class ColumnHandle {
public:
  template <class T>
  ColumnHandle(uint32_t id, T object)
      : m_type(id),
        m_object(std::make_unique<ColumnModel<T>>(std::move(object))) {}
  ColumnHandle(const ColumnHandle &other)
      : m_type(other.m_type), m_object(other.m_object->clone()) {}

  gsl::index size() const { return m_object->size(); }
  void resize(const gsl::index size) { m_object->resize(size); }

  template <class T> const T &cast() const {
    return dynamic_cast<ColumnModel<T> &>(*m_object).m_model;
  }

  template <class T> T &cast() {
    return dynamic_cast<ColumnModel<T> &>(*m_object).m_model;
  }

  uint32_t type() const { return m_type; }

private:
  const uint32_t m_type;
  std::unique_ptr<ColumnConcept> m_object;
};

template <class... Ts> class DatasetIterator;

class Dataset {
public:
  template <class Tag> void add(std::string name) {
    // TODO prevent duplicate names
    m_data.emplace_back(name, std::vector<Dimension>{},
                        ColumnHandle(Tag::type_id, variable_type_t<Tag>(1)));
  }

  // need:
  // - dimensions
  // - axis lengths
  // - list of applicable dimensions for all Ts
  void addDimension(const Dimension id, const gsl::index size) {
    // TODO throw if exists
    m_dimensions[id] = size;
  }

  gsl::index size() const { return m_data.size(); }

  template <class Tag> void extendAlongDimension(const Dimension id) {
    for (auto &item : m_data) {
      if (std::get<ColumnHandle>(item).type() == Tag::type_id) {
        std::get<std::vector<Dimension>>(item).push_back(id);
        std::get<ColumnHandle>(item)
            .resize(std::get<ColumnHandle>(item).size() * m_dimensions.at(id));
        // TODO duplicate from slice 0 to all others.
        return;
      }
    }
    throw std::runtime_error("Dataset does not contain such a column");
  }

  // TODO need (helper) types for values and errors (instead of
  // std::vector<double>, which
  // would be duplicate). This is also the reason for T being the column type,
  // not the element type.
  template <class Tag> variable_type_t<Tag> &get() {
    for (auto &item : m_data) {
      // TODO check for duplicate column types (can use get based on name in
      // that case).
      if (std::get<ColumnHandle>(item).type() == Tag::type_id)
        return std::get<ColumnHandle>(item)
            .cast<std::remove_const_t<variable_type_t<Tag>>>();
    }
    throw std::runtime_error("Dataset does not contain such a column");
  }

  const std::map<Dimension, gsl::index> &dimensions() const {
    return m_dimensions;
  }

  template <class Tag> const std::vector<Dimension> &dimensions() const {
    for (auto &item : m_data) {
      if (std::get<ColumnHandle>(item).type() == Tag::type_id)
        return std::get<std::vector<Dimension>>(item);
    }
    throw std::runtime_error("Dataset does not contain such a column");
  }

  template <class... Ts> friend class DatasetIterator;

private:
  std::map<Dimension, gsl::index> m_dimensions;
  std::vector<std::tuple<std::string, std::vector<Dimension>, ColumnHandle>>
      m_data;
};

#endif // DATASET_H
