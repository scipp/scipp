#ifndef DATASET_H
#define DATASET_H

#include <array>
#include <vector>

namespace gsl {
using index = ptrdiff_t;
}

using Doubles = std::vector<double>;
using Ints = std::vector<int>;

enum class ColumnType { Ints, Doubles };

template <class T> ColumnType getColumnType() {
  throw std::runtime_error("Type not registered in ADS");
}

template <> inline ColumnType getColumnType<Ints>() { return ColumnType::Ints; }
template <> inline ColumnType getColumnType<Doubles>() { return ColumnType::Doubles; }


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
  ColumnHandle(T object)
      : m_type(getColumnType<T>()),
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

  ColumnType type() const { return m_type; }

private:
  ColumnType m_type;
  std::unique_ptr<ColumnConcept> m_object;
};

enum class Dimension { SpectrumNumber, Run, DetectorId, Tof, Q };

template <class T, class... Ts> class FlatDatasetItem;

class Dataset {
public:
  template <class... Ts>
  Dataset(Ts &&... columns)
      : m_data{std::make_pair(std::vector<std::string>{},
                              ColumnHandle(std::forward<Ts>(columns)))...} {}
  // need:
  // - dimensions
  // - axis lengths
  // - list of applicable dimensions for all Ts
  void addDimension(const std::string &name, const gsl::index size) {
    // TODO throw if exists
    m_dimensions[name] = size;
  }

  gsl::index columns() const { return m_data.size(); }

  void extendAlongDimension(const ColumnType column, const std::string &name) {
    for (auto &item : m_data) {
      if (item.second.type() == column) {
        item.first.push_back(name);
        item.second.resize(item.second.size() * m_dimensions.at(name));
        // TODO duplicate from slice 0 to all others.
        return;
      }
    }
    throw std::runtime_error("Dataset does not contain such a column");
  }

  template <class T> T &get() {
    const auto columnType = getColumnType<T>();
    for (auto &item : m_data) {
      if(item.second.type() == columnType)
        return item.second.cast<T>();
    }
    throw std::runtime_error("Dataset does not contain such a column");
  }

  /*
  // how to get iterator centered to a certain type?
  template <class T> auto at(const gsl::index i) {
    return FlatDatasetItem<T, Ts...>(i, *this);
    // data items fall in three cases:
    // 1. dimensions match those of T => pass reference
    // 2. misses dimension(s) of T => pass const reference
    // 3. has additional dimensions => pass reference to container with stride access
    // Problem: Dimensions known only at runtime
    // - Always pass const reference to container with stride access, except T
    //   which can be non-const?
    // - Implies that all fields in returned item are wrapped in vector-like :(
    // - Implicitly convert vector-like to item if size is 1?
  }

  template <class T> const std::vector<std::string> &dimensions() const {
    return std::get<std::pair<std::vector<std::string>, std::vector<T>>>(m_data)
        .first;
  }

  gsl::index size(const std::string &dimension) const {
    return m_dimensions.at(dimension);
  }
  */

private:
  std::map<std::string, gsl::index> m_dimensions;
  std::vector<std::pair<std::vector<std::string>, ColumnHandle>> m_data;
};

#endif // DATASET_H
