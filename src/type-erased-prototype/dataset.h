#ifndef DATASET_H
#define DATASET_H

#include <vector>

#include <gsl/gsl_util>

#include "dimension.h"
#include "tags.h"
#include "variable.h"

class Dataset {
public:
  void insert(Variable variable);

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
  void insertAsEdge(const Dimension dimension, Variable variable);

  gsl::index size() const { return m_variables.size(); }
  const Variable &operator[](gsl::index i) const { return m_variables[i]; }
  auto begin() const { return m_variables.begin(); }
  auto end() const { return m_variables.end(); }

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

  template <class Tag> const Unit &unit(const std::string &name) const {
    return m_variables[find(tag_id<Tag>, name)].unit();
  }

  gsl::index find(const uint16_t id, const std::string &name) const;

  Dataset &operator+=(const Dataset &other);
  Dataset &operator*=(const Dataset &other);

private:
  gsl::index count(const uint16_t id) const;
  gsl::index count(const uint16_t id, const std::string &name) const;
  gsl::index findUnique(const uint16_t id) const;
  void mergeDimensions(const auto &dims);

  Dimensions m_dimensions;
  std::vector<Variable> m_variables;
};

Dataset concatenate(const Dimension dim, const Dataset &d1, const Dataset &d2);

#endif // DATASET_H
