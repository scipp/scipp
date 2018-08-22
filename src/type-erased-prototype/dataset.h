#ifndef DATASET_H
#define DATASET_H

#include <vector>

#include <gsl/gsl_util>

#include "dimension.h"
#include "tags.h"
#include "variable.h"

class Dataset {
public:
  void insert(Variable variable) {
    if (variable.isCoord() && count(variable.type()))
      throw std::runtime_error("Attempt to insert duplicate coordinate.");
    if (!variable.isCoord()) {
      if (variable.name().empty())
        throw std::runtime_error(
            "Non-coordinate variable cannot have an empty name.");
      for (const auto &item : m_variables)
        if (item.type() == variable.type() && item.name() == variable.name())
          throw std::runtime_error(
              "Attempt to insert data of same type with duplicate name.");
    }
    // TODO special handling for special variables types like
    // Data::Histogram (either prevent adding, or extract into underlying
    // variables).
    mergeDimensions(variable.dimensions());
    m_variables.push_back(std::move(variable));
  }

  template <class Tag, class... Args>
  void insert(Dimensions dimensions, Args &&... args) {
    static_assert(is_coord<Tag>, "Non-coordinate variable must have a name.");
    auto a =
        makeVariable<Tag>(std::move(dimensions), std::forward<Args>(args)...);
    insert(std::move(a));
  }

  template <class Tag, class... Args>
  void insert(const std::string &name, Dimensions dimensions, Args &&... args) {
    static_assert(!is_coord<Tag>, "Coordinate variable cannot have a name.");
    auto a =
        makeVariable<Tag>(std::move(dimensions), std::forward<Args>(args)...);
    a.setName(name);
    insert(std::move(a));
  }

  template <class Tag, class T>
  void insert(Dimensions dimensions, std::initializer_list<T> values) {
    static_assert(is_coord<Tag>, "Non-coordinate variable must have a name.");
    auto a = makeVariable<Tag>(std::move(dimensions), values);
    insert(std::move(a));
  }

  template <class Tag, class T>
  void insert(const std::string &name, Dimensions dimensions,
              std::initializer_list<T> values) {
    static_assert(!is_coord<Tag>, "Coordinate variable cannot have a name.");
    auto a = makeVariable<Tag>(std::move(dimensions), values);
    a.setName(name);
    insert(std::move(a));
  }

  // Only need this for coordinates... insertEdgeCoord?
  void insertAsEdge(const Dimension dimension, Variable variable) {
    // Edges are by 1 longer than other data, so dimension size check and
    // merging uses modified dimensions.
    auto dims = variable.dimensions();
    dims.resize(dimension, dims.size(dimension) - 1);
    mergeDimensions(dims);
    m_variables.push_back(std::move(variable));
  }

  gsl::index size() const { return m_variables.size(); }
  const Variable &operator[](gsl::index i) const { return m_variables[i]; }

  template <class Tag> auto get() const {
    return m_variables[findUnique(tag_id<Tag>)].template get<Tag>();
  }

  template <class Tag> auto get(const std::string &name) const {
    return m_variables[find(tag_id<Tag>, name)].template get<Tag>();
  }

  template <class Tag> auto get() {
    return m_variables[findUnique(tag_id<Tag>)].template get<Tag>();
  }

  template <class Tag> auto get(const std::string &name) {
    return m_variables[find(tag_id<Tag>, name)].template get<Tag>();
  }

  const Dimensions &dimensions() const { return m_dimensions; }

  template <class Tag> const Dimensions &dimensions() const {
    return m_variables[findUnique(tag_id<Tag>)].dimensions();
  }

  template <class Tag>
  const Dimensions &dimensions(const std::string &name) const {
    return m_variables[find(tag_id<Tag>, name)].dimensions();
  }

  template <class Tag> const Unit &unit() const {
    return m_variables[findUnique(tag_id<Tag>)].unit();
  }

  template <class Tag>
  const Unit &unit(const std::string &name) const {
    return m_variables[find(tag_id<Tag>, name)].unit();
  }

  gsl::index find(const uint16_t id, const std::string &name) const {
    for (gsl::index i = 0; i < size(); ++i)
      if (m_variables[i].type() == id && m_variables[i].name() == name)
        return i;
    throw std::runtime_error("Dataset does not contain such a variable.");
  }

  Dataset &operator+=(const Dataset &other);

private:
  gsl::index count(const uint16_t id) const {
    gsl::index n = 0;
    for (auto &item : m_variables)
      if (item.type() == id)
        ++n;
    return n;
  }

  gsl::index findUnique(const uint16_t id) const {
    gsl::index index = -1;
    for (gsl::index i = 0; i < size(); ++i) {
      if (m_variables[i].type() == id) {
        if (index != -1)
          throw std::runtime_error(
              "Given variable tag is not unique. Must provide a name.");
        index = i;
      }
    }
    if (index == -1)
      throw std::runtime_error("Dataset does not contain such a variable.");
    return index;
  }

  void mergeDimensions(const auto &dims) {
    gsl::index j = 0;
    gsl::index found = 0;
    for (gsl::index i = 0; i < dims.count(); ++i) {
      const auto dim = dims.label(i);
      const auto size = dims.size(i);
      bool found = false;
      for (; j < m_dimensions.count(); ++j) {
        if (m_dimensions.label(j) == dim) {
          if (m_dimensions.size(j) != size) // TODO compare ragged
            throw std::runtime_error(
                "Cannot insert variable into Dataset: Dimensions do not match");
          found = true;
          break;
        }
      }
      if (!found) {
        m_dimensions.add(dim, size);
      }
    }
  }

  Dimensions m_dimensions;
  std::vector<Variable> m_variables;
};

Dataset concatenate(const Dimension dim, const Dataset &d1, const Dataset &d2);

#endif // DATASET_H
