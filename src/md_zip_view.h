/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef MD_ZIP_VIEW_H
#define MD_ZIP_VIEW_H

#include <algorithm>
#include <cmath>
#include <set>
#include <tuple>
#include <type_traits>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/sort.hpp>
#include <boost/mpl/vector_c.hpp>

#include "dataset.h"
#include "multi_index.h"
#include "traits.h"

namespace detail {
template <class T> struct value_type { using type = T; };
template <class T> struct value_type<Bin<T>> { using type = const T; };
template <class T> using value_type_t = typename value_type<T>::type;

template <class T> struct is_bins : public std::false_type {};
template <class T> struct is_bins<Bin<T>> : public std::true_type {};
template <class T> using is_bins_t = typename is_bins<T>::type;

template <class Tag> struct unit { using type = Unit; };
template <class... Tags> struct unit<MDZipViewImpl<Tags...>> {
  using type = std::tuple<typename unit<Tags>::type...>;
};
template <class... Tags> using unit_t = typename unit<Tags...>::type;
} // namespace detail

template <class Base, class T> struct GetterMixin {};

// TODO Check const correctness here.
#define GETTER_MIXIN(Tag, name)                                                \
  template <class Base> struct GetterMixin<Base, Tag> {                        \
    element_return_type_t<Tag> name() const {                                  \
      return static_cast<const Base *>(this)->template get<Tag>();             \
    }                                                                          \
  };                                                                           \
  template <class Base> struct GetterMixin<Base, const Tag> {                  \
    element_return_type_t<const Tag> name() const {                            \
      return static_cast<const Base *>(this)->template get<Tag>();             \
    }                                                                          \
  };

GETTER_MIXIN(Coord::Tof, tof)
GETTER_MIXIN(Data::Tof, tof)
GETTER_MIXIN(Data::Value, value)
GETTER_MIXIN(Data::Variance, variance)

template <class Base, class T> struct GetterMixin<Base, Bin<T>> {
  // Lift the getters of Bin into the iterator.
  double left() const {
    return static_cast<const Base *>(this)->template get<Bin<T>>().left();
  }
  double right() const {
    return static_cast<const Base *>(this)->template get<Bin<T>>().right();
  }
};

template <class Tag> struct ref_type {
  using type = gsl::span<std::conditional_t<
      std::is_const<Tag>::value, const typename detail::value_type_t<Tag>::type,
      typename detail::value_type_t<Tag>::type>>;
};
template <class Tag> struct ref_type<Bin<Tag>> {
  // First is the offset to the next edge.
  using type =
      std::pair<gsl::index,
                gsl::span<const typename detail::value_type_t<Bin<Tag>>::type>>;
};
template <> struct ref_type<const Coord::Position> {
  using type =
      std::pair<gsl::span<const typename Coord::Position::type>,
                gsl::span<const typename Coord::DetectorGrouping::type>>;
};
template <> struct ref_type<Data::StdDev> {
  using type = typename ref_type<const Data::Variance>::type;
};
template <class... Tags> struct ref_type<MDZipViewImpl<Tags...>> {
  using type = std::tuple<const MultiIndex, const MDZipViewImpl<Tags...>,
                          std::tuple<typename ref_type<Tags>::type...>>;
};
template <class T> using ref_type_t = typename ref_type<T>::type;

template <class Tag> struct SubdataHelper {
  static auto get(const ref_type_t<Tag> &data, const gsl::index offset) {
    return data.subspan(offset);
  }
};

template <class Tag> struct SubdataHelper<Bin<Tag>> {
  static auto get(const ref_type_t<Bin<Tag>> &data, const gsl::index offset) {
    return ref_type_t<Bin<Tag>>{data.first, data.second.subspan(offset)};
  }
};

/// Class with overloads used to handle "virtual" variables such as
/// Coord::Position.
template <class Tag> struct ItemHelper {
  static element_return_type_t<Tag> get(const ref_type_t<Tag> &data,
                                        gsl::index index) {
    return data[index];
  }
};

// Note: Special case! Coord::Position can be either derived based on detectors,
// or stored directly.
template <> struct ItemHelper<const Coord::Position> {
  static element_return_type_t<const Coord::Position>
  get(const ref_type_t<const Coord::Position> &data, gsl::index index) {
    if (data.second.empty())
      return data.first[index];
    if (data.second[index].empty())
      throw std::runtime_error(
          "Spectrum has no detectors, cannot get position.");
    Eigen::Vector3d position{0.0, 0.0, 0.0};
    for (const auto det : data.second[index])
      position += data.first[det];
    return position /= static_cast<double>(data.second[index].size());
  }
};

template <> struct ItemHelper<Data::StdDev> {
  static element_return_type_t<Data::StdDev>
  get(const ref_type_t<Data::StdDev> &data, gsl::index index) {
    return std::sqrt(ItemHelper<const Data::Variance>::get(data, index));
  }
};

template <class Tag> struct ItemHelper<Bin<Tag>> {
  static element_return_type_t<Bin<Tag>> get(const ref_type_t<Bin<Tag>> &data,
                                             gsl::index index) {
    auto offset = data.first;
    return DataBin(data.second[index], data.second[index + offset]);
  }
};

template <class... Tags> struct ItemHelper<MDZipViewImpl<Tags...>> {
  template <class Tag>
  static constexpr auto subindex =
      detail::index<Tag, std::tuple<Tags...>>::value;

  static element_return_type_t<MDZipViewImpl<Tags...>>
  get(const ref_type_t<MDZipViewImpl<Tags...>> &data, gsl::index index) {
    // Add offset to each span passed to the nested MDZipView.
    MultiIndex nestedIndex = std::get<0>(data);
    nestedIndex.setIndex(index);
    auto subdata = std::make_tuple(
        SubdataHelper<Tags>::get(std::get<subindex<Tags>>(std::get<2>(data)),
                                 nestedIndex.get<subindex<Tags>>())...);
    return MDZipViewImpl<Tags...>(std::get<1>(data), subdata);
  }
};

template <class... Ts> class MDZipViewImpl {
  static_assert(sizeof...(Ts),
                "MDZipView requires at least one variable for iteration");

private:
  using tags = std::tuple<std::remove_const_t<Ts>...>;
  // Note: Not removing const from Tag, we want it to fail if const is passed.
  // TODO detail::index is from tags.h, put it somewhere else and rename.
  template <class Tag>
  static constexpr size_t tag_index = detail::index<Tag, tags>::value;
  template <class Tag>
  using maybe_const = std::tuple_element_t<tag_index<Tag>, std::tuple<Ts...>>;

  Dimensions relevantDimensions(
      const Dataset &dataset,
      boost::container::small_vector<Dimensions, 4> variableDimensions,
      const std::set<Dim> &fixedDimensions) const;

public:
  class iterator;
  class Item : public GetterMixin<Item, Ts>... {
  public:
    Item(const gsl::index index, const MultiIndex &multiIndex,
         const std::tuple<ref_type_t<Ts>...> &variables)
        : m_index(multiIndex), m_variables(&variables) {
      setIndex(index);
    }

    template <class Tag> element_return_type_t<maybe_const<Tag>> get() const {
      // Should we allow passing const?
      static_assert(!std::is_const<Tag>::value, "Do not use `const` qualifier "
                                                "for tags when accessing "
                                                "MDZipView::iterator.");
      constexpr auto variableIndex = tag_index<Tag>;
      return ItemHelper<maybe_const<Tag>>::get(
          std::get<variableIndex>(*m_variables), m_index.get<variableIndex>());
    }

  private:
    friend class iterator;
    // Private such that iterator can be copied but clients cannot extract Item
    // (access only by reference).
    Item(const Item &) = default;
    Item &operator=(const Item &) = default;
    void setIndex(const gsl::index index) { m_index.setIndex(index); }

    bool operator==(const Item &other) const {
      return m_index == other.m_index;
    }

    MultiIndex m_index;
    const std::tuple<ref_type_t<Ts>...> *m_variables;
  };

  class iterator
      : public boost::iterator_facade<iterator, const Item,
                                      boost::random_access_traversal_tag> {
  public:
    iterator(const gsl::index index, const MultiIndex &multiIndex,
             const std::tuple<ref_type_t<Ts>...> &variables)
        : m_item(index, multiIndex, variables) {}

  private:
    friend class boost::iterator_core_access;

    bool equal(const iterator &other) const { return m_item == other.m_item; }
    void increment() { m_item.m_index.increment(); }
    const Item &dereference() const { return m_item; }
    void decrement() { m_item.setIndex(m_item.m_index.index() - 1); }

    void advance(int64_t delta) {
      if (delta == 1)
        increment();
      else
        m_item.setIndex(m_item.m_index.index() + delta);
    }

    int64_t distance_to(const iterator &other) const {
      return static_cast<int64_t>(other.m_item.m_index.index()) -
             static_cast<int64_t>(m_item.m_index.index());
    }

    Item m_item;
  };

  MDZipViewImpl(detail::MaybeConstDataset<Ts...> &dataset,
                const std::string &name,
                const std::set<Dim> &fixedDimensions = {});
  MDZipViewImpl(detail::MaybeConstDataset<Ts...> &dataset,
                const std::set<Dim> &fixedDimensions = {});
  MDZipViewImpl(detail::MaybeConstDataset<Ts...> &dataset,
                const std::initializer_list<Dim> &fixedDimensions);

  MDZipViewImpl(const MDZipViewImpl &other,
                const std::tuple<ref_type_t<Ts>...> &data);

  gsl::index size() const { return std::get<0>(m_variables); }
  iterator begin() const {
    return {0, std::get<1>(m_variables), std::get<2>(m_variables)};
  }
  iterator end() const {
    return {std::get<0>(m_variables), std::get<1>(m_variables),
            std::get<2>(m_variables)};
  }

private:
  std::tuple<const gsl::index, const MultiIndex,
             const std::tuple<ref_type_t<Ts>...>>
  makeVariables(detail::MaybeConstDataset<Ts...> &dataset,
                const std::set<Dim> &fixedDimensions,
                const std::string &name = std::string{}) const;

  const std::tuple<detail::unit_t<Ts>...> m_units;
  const std::tuple<const gsl::index, const MultiIndex,
                   const std::tuple<ref_type_t<Ts>...>>
      m_variables;
};

namespace detail {
// Helpers used for making MDZipView independent of the order used when
// specifying tags.
template <class T> struct type_to_id {
  static constexpr int32_t value =
      std::is_const<T>::value ? 4 * T{}.value() + 1 : 4 * T{}.value() + 3;
};
template <class T> struct type_to_id<Bin<T>> {
  static constexpr int32_t value =
      std::is_const<T>::value ? 4 * T{}.value() + 2 : 4 * T{}.value() + 4;
};
// Nested MDZipView gets an ID based on the IDs of all child tags.
template <class... Ts> struct type_to_id<MDZipViewImpl<Ts...>> {
  static constexpr std::array<int32_t, sizeof...(Ts)> ids{
      type_to_id<Ts>::value...};
  static constexpr int32_t value =
      200 * ((sizeof...(Ts) == 1)
                 ? ids[0]
                 : (sizeof...(Ts) == 2)
                       ? 200 * ids[1] + ids[0]
                       : 200 * 200 * ids[2] + 200 * ids[1] + ids[0]);
};

template <int32_t N>
using get_elem_type =
    detail::TagImpl<std::tuple_element_t<(N - 1) / 4, detail::Tags>>;

template <int32_t N>
using get_type = std::conditional_t<
    N % 2 == 0,
    std::conditional_t<N % 4 == 0, Bin<get_elem_type<N>>,
                       const Bin<get_elem_type<N>>>,
    std::conditional_t<N % 4 == 3, get_elem_type<N>, const get_elem_type<N>>>;

template <int32_t N> struct id_to_type {
  using type = std::conditional_t<
      (N < 200), get_type<N % 200>,
      std::conditional_t<
          (N < 200 * 200), std::tuple<get_type<(N / 200) % 200>>,
          std::conditional_t<
              (N < 200 * 200 * 200),
              std::tuple<get_type<(N / 200) % 200>,
                         get_type<(N / (200 * 200)) % 200>>,
              std::tuple<get_type<(N / 200) % 200>,
                         get_type<(N / (200 * 200)) % 200>,
                         get_type<(N / (200 * 200 * 200)) % 200>>>>>;
};
template <int32_t N> using id_to_type_t = typename id_to_type<N>::type;

template <class Sorted, size_t... Is>
auto sort_types_impl(std::index_sequence<Is...>) {
  return std::tuple<
      id_to_type_t<boost::mpl::at_c<Sorted, Is>::type::value>...>();
}

template <class... Ts> auto sort_types() {
  using Unsorted = boost::mpl::vector_c<int, type_to_id<Ts>::value...>;
  return sort_types_impl<typename boost::mpl::sort<Unsorted>::type>(
      std::make_index_sequence<sizeof...(Ts)>{});
}

// Helper to return either Tag or translate tuple of tags into MDZipView.
template <class Tag> struct tag { using type = Tag; };
template <class... Tags> struct tag<std::tuple<Tags...>> {
  using type = MDZipViewImpl<Tags...>;
};

// Helper to translate (potentially nested) tuple of Tags into MDZipView.
template <class T> struct dataset_view;
template <class... Ts> struct dataset_view<std::tuple<Ts...>> {
  using type = MDZipViewImpl<typename tag<Ts>::type...>;
};
} // namespace detail

template <class... Ts>
using MDZipView =
    typename detail::dataset_view<decltype(detail::sort_types<Ts...>())>::type;

#endif // MD_ZIP_VIEW_H
