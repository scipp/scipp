// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <functional>
#include <string>

#include "scipp/dataset/data_array.h"
#include "scipp/dataset/sized_dict.h"
#include "scipp/variable/variable.h"

namespace scipp::dataset {

namespace detail {

/// Helper for creating iterators of Dataset.
template <class D> struct with_coords {
  const D *dataset;
  template <class T> auto operator()(T &&item) const {
    return item.second.view_with_coords(dataset->coords(), item.first,
                                        dataset->is_readonly());
  }
};
template <class D> with_coords(const D *) -> with_coords<D>;

template <class D> struct item_with_coords {
  const D *dataset;
  template <class T> auto operator()(T &&item) const {
    return std::pair{item.first, with_coords{dataset}(item)};
  }
};

template <class D> item_with_coords(const D *) -> item_with_coords<D>;

// Use to disambiguate constructors.
struct init_from_data_arrays_t {};
static constexpr auto init_from_data_arrays = init_from_data_arrays_t{};
} // namespace detail

/// Collection of data arrays.
class SCIPP_DATASET_EXPORT Dataset {
public:
  using key_type = std::string;
  using mapped_type = DataArray;
  using value_type = std::pair<const std::string &, DataArray>;

  Dataset();
  Dataset(const Dataset &other);
  Dataset(Dataset &&other) = default;
  explicit Dataset(const DataArray &data);

  // The constructor with the DataMap template also works with Variables.
  // But the compiler cannot deduce the type when called with initializer lists.
  template <class CoordMap = core::Dict<Dim, Variable>>
  explicit Dataset(core::Dict<std::string, Variable> data,
                   CoordMap coords = core::Dict<Dim, Variable>{})
      : Dataset(std::move(data), std::move(coords),
                detail::init_from_data_arrays) {}

  template <class DataMap = core::Dict<std::string, DataArray>,
            class CoordMap = core::Dict<Dim, Variable>>
  explicit Dataset(
      DataMap data, CoordMap coords = core::Dict<Dim, Variable>{},
      const detail::init_from_data_arrays_t = detail::init_from_data_arrays) {
    if (data.empty()) {
      if constexpr (std::is_same_v<std::decay_t<CoordMap>, Coords>)
        m_coords = std::move(coords);
      else
        m_coords = Coords(AutoSizeTag{}, std::move(coords));
    } else {
      // Set the sizes based on data in order to handle bin-edge coords.
      // The coords are then individually checked against those sizes.
      m_coords = Coords(data.begin()->second.dims(), {});
      for (auto &&[name, item] : coords) {
        setCoord(name, std::move(item));
      }

      if constexpr (std::is_base_of_v<Dataset, std::decay_t<DataMap>>)
        for (auto &&item : data) {
          setData(item.name(), item);
        }
      else
        for (auto &&[name, item] : data) {
          setData(std::string(name), std::move(item));
        }
    }
  }

  Dataset &operator=(const Dataset &other);
  Dataset &operator=(Dataset &&other);

  void setCoords(Coords other);

  /// Return the number of data items in the dataset.
  ///
  /// This does not include coordinates and masks, but only all named
  /// entities (which can consist of various combinations of values, variances,
  /// and events coordinates).
  index size() const noexcept { return scipp::size(m_data); }
  /// Return true if there are 0 data items in the dataset.
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }
  /// Return the number of elements that space is currently allocated for.
  [[nodiscard]] index capacity() const noexcept { return m_data.capacity(); }

  void clear();

  const Coords &coords() const noexcept;
  Coords &coords() noexcept;

  Dataset drop_coords(const std::span<const Dim> coord_names) const;

  Dataset drop_masks(const std::span<const std::string> mask_names) const;

  bool contains(const std::string &name) const noexcept;

  void erase(const std::string &name);
  [[nodiscard]] DataArray extract(const std::string &name);

  auto find() const && = delete;
  auto find() && = delete;
  auto find(const std::string &name) & noexcept {
    return m_data.find(name).transform(detail::with_coords{this});
  }
  auto find(const std::string &name) const & noexcept {
    return m_data.find(name).transform(detail::with_coords{this});
  }

  DataArray operator[](const std::string &name) const;

  auto begin() const && = delete;
  auto begin() && = delete;
  /// Return const iterator to the beginning of all data items.
  auto begin() const & noexcept {
    return m_data.begin().transform(detail::with_coords{this});
  }
  /// Return iterator to the beginning of all data items.
  auto begin() & noexcept {
    return m_data.begin().transform(detail::with_coords{this});
  }
  auto end() const && = delete;
  auto end() && = delete;
  /// Return const iterator to the end of all data items.
  auto end() const & noexcept {
    return m_data.end().transform(detail::with_coords{this});
  }

  /// Return iterator to the end of all data items.
  auto end() & noexcept {
    return m_data.end().transform(detail::with_coords{this});
  }

  auto items_begin() const && = delete;
  auto items_begin() && = delete;
  auto items_begin() const & noexcept {
    return m_data.begin().transform(detail::item_with_coords{this});
  }
  auto items_begin() & noexcept {
    return m_data.begin().transform(detail::item_with_coords{this});
  }
  auto items_end() const && = delete;
  auto items_end() && = delete;
  auto items_end() const & noexcept {
    return m_data.end().transform(detail::item_with_coords{this});
  }

  auto items_end() & noexcept {
    return m_data.end().transform(detail::item_with_coords{this});
  }

  auto keys_begin() const && = delete;
  auto keys_begin() && = delete;
  auto keys_begin() const & noexcept { return m_data.keys_begin(); }
  auto keys_begin() & noexcept { return m_data.keys_begin(); }
  auto keys_end() const && = delete;
  auto keys_end() && = delete;
  auto keys_end() const & noexcept { return m_data.keys_end(); }

  auto keys_end() & noexcept { return m_data.keys_end(); }

  void setCoord(const Dim dim, Variable coord);
  void setData(const std::string &name, Variable data);
  void setData(const std::string &name, const DataArray &data);
  void setDataInit(const std::string &name, Variable data);
  void setDataInit(const std::string &name, const DataArray &data);

  Dataset slice(const Slice &s) const;
  [[maybe_unused]] Dataset &setSlice(const Slice &s, const Dataset &dataset);
  [[maybe_unused]] Dataset &setSlice(const Slice &s, const DataArray &array);
  [[maybe_unused]] Dataset &setSlice(const Slice &s, const Variable &var);

  [[nodiscard]] Dataset
  rename_dims(const std::vector<std::pair<Dim, Dim>> &names) const;

  bool operator==(const Dataset &other) const;
  bool operator!=(const Dataset &other) const;

  Dataset &operator+=(const DataArray &other);
  Dataset &operator-=(const DataArray &other);
  Dataset &operator*=(const DataArray &other);
  Dataset &operator/=(const DataArray &other);
  Dataset &operator+=(const Variable &other);
  Dataset &operator-=(const Variable &other);
  Dataset &operator*=(const Variable &other);
  Dataset &operator/=(const Variable &other);
  Dataset &operator+=(const Dataset &other);
  Dataset &operator-=(const Dataset &other);
  Dataset &operator*=(const Dataset &other);
  Dataset &operator/=(const Dataset &other);

  const Sizes &sizes() const;
  const Sizes &dims() const;
  Dim dim() const;
  [[nodiscard]] scipp::index ndim() const;

  bool is_readonly() const noexcept;
  bool is_valid() const noexcept;

  [[nodiscard]] Dataset or_empty() && {
    if (is_valid())
      return std::move(*this);
    return Dataset({}, {});
  }

private:
  // Declared friend so gtest recognizes it
  friend SCIPP_DATASET_EXPORT std::ostream &operator<<(std::ostream &,
                                                       const Dataset &);

  Coords m_coords;
  core::Dict<std::string, DataArray> m_data;
  bool m_readonly{false};
  /// See documentation of setDataInit.
  /// Invalid datasets are for internal use only.
  bool m_valid{true};
};

[[nodiscard]] SCIPP_DATASET_EXPORT Dataset copy(const Dataset &dataset);

[[maybe_unused]] SCIPP_DATASET_EXPORT DataArray &copy(const DataArray &array,
                                                      DataArray &out);
[[maybe_unused]] SCIPP_DATASET_EXPORT DataArray copy(const DataArray &array,
                                                     DataArray &&out);
[[maybe_unused]] SCIPP_DATASET_EXPORT Dataset &copy(const Dataset &dataset,
                                                    Dataset &out);
[[maybe_unused]] SCIPP_DATASET_EXPORT Dataset copy(const Dataset &dataset,
                                                   Dataset &&out);

SCIPP_DATASET_EXPORT Dataset operator+(const Dataset &lhs, const Dataset &rhs);
SCIPP_DATASET_EXPORT Dataset operator+(const Dataset &lhs,
                                       const DataArray &rhs);
SCIPP_DATASET_EXPORT Dataset operator+(const DataArray &lhs,
                                       const Dataset &rhs);
SCIPP_DATASET_EXPORT Dataset operator+(const Dataset &lhs, const Variable &rhs);
SCIPP_DATASET_EXPORT Dataset operator+(const Variable &lhs, const Dataset &rhs);

SCIPP_DATASET_EXPORT Dataset operator-(const Dataset &lhs, const Dataset &rhs);
SCIPP_DATASET_EXPORT Dataset operator-(const Dataset &lhs,
                                       const DataArray &rhs);
SCIPP_DATASET_EXPORT Dataset operator-(const DataArray &lhs,
                                       const Dataset &rhs);
SCIPP_DATASET_EXPORT Dataset operator-(const Dataset &lhs, const Variable &rhs);
SCIPP_DATASET_EXPORT Dataset operator-(const Variable &lhs, const Dataset &rhs);

SCIPP_DATASET_EXPORT Dataset operator*(const Dataset &lhs, const Dataset &rhs);
SCIPP_DATASET_EXPORT Dataset operator*(const Dataset &lhs,
                                       const DataArray &rhs);
SCIPP_DATASET_EXPORT Dataset operator*(const DataArray &lhs,
                                       const Dataset &rhs);
SCIPP_DATASET_EXPORT Dataset operator*(const Dataset &lhs, const Variable &rhs);
SCIPP_DATASET_EXPORT Dataset operator*(const Variable &lhs, const Dataset &rhs);

SCIPP_DATASET_EXPORT Dataset operator/(const Dataset &lhs, const Dataset &rhs);
SCIPP_DATASET_EXPORT Dataset operator/(const Dataset &lhs,
                                       const DataArray &rhs);
SCIPP_DATASET_EXPORT Dataset operator/(const DataArray &lhs,
                                       const Dataset &rhs);
SCIPP_DATASET_EXPORT Dataset operator/(const Dataset &lhs, const Variable &rhs);
SCIPP_DATASET_EXPORT Dataset operator/(const Variable &lhs, const Dataset &rhs);

/// Union the masks of the two proxies.
/// If any of the masks repeat they are OR'ed.
/// The result is stored in a new map
SCIPP_DATASET_EXPORT
typename Masks::holder_type union_or(const Masks &currentMasks,
                                     const Masks &otherMasks);

/// Union the masks of the two proxies.
/// If any of the masks repeat they are OR'ed.
/// The result is stored in the first view.
SCIPP_DATASET_EXPORT void union_or_in_place(Masks &masks,
                                            const Masks &otherMasks);

SCIPP_DATASET_EXPORT Dataset merge(const Dataset &a, const Dataset &b);

[[nodiscard]] SCIPP_DATASET_EXPORT bool equals_nan(const Dataset &a,
                                                   const Dataset &b);

} // namespace scipp::dataset

namespace scipp::core {
template <> inline constexpr DType dtype<dataset::DataArray>{2000};
template <> inline constexpr DType dtype<dataset::Dataset>{2001};
template <> inline constexpr DType dtype<bucket<dataset::DataArray>>{2002};
template <> inline constexpr DType dtype<bucket<dataset::Dataset>>{2003};
} // namespace scipp::core

namespace scipp {
using dataset::Dataset;
} // namespace scipp

#include "scipp/dataset/arithmetic.h"
