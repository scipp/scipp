/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef ZIP_VIEW_H
#define ZIP_VIEW_H

#include "range/v3/view/zip.hpp"

#include "dataset.h"

template <class... Tags> struct AccessHelper {
  static void push_back(std::array<Dimensions *, sizeof...(Tags)> &dimensions,
                        std::tuple<Vector<typename Tags::type> *...> &data,
                        const std::tuple<typename Tags::type...> &value);
};

template <class Tag1> struct AccessHelper<Tag1> {
  static void push_back(std::array<Dimensions *, 1> &dimensions,
                        std::tuple<Vector<typename Tag1::type> *> &data,
                        const std::tuple<typename Tag1::type> &value) {
    std::get<0>(data)->push_back(std::get<0>(value));
    dimensions[0]->resize(0, dimensions[0]->size(0) + 1);
  }
};

template <class Tag1, class Tag2> struct AccessHelper<Tag1, Tag2> {
  static void push_back(
      std::array<Dimensions *, 2> &dimensions,
      std::tuple<Vector<typename Tag1::type> *, Vector<typename Tag2::type> *>
          &data,
      const std::tuple<typename Tag1::type, typename Tag2::type> &value) {
    std::get<0>(data)->push_back(std::get<0>(value));
    std::get<1>(data)->push_back(std::get<1>(value));
    dimensions[0]->resize(0, dimensions[0]->size(0) + 1);
    dimensions[1]->resize(0, dimensions[1]->size(0) + 1);
  }
};

// TODO Should also have a const version of this, and support names, similar to
// zipMD. Note that this is simpler to do in this case since const-ness does not
// matter --- creation with mismatching dimensions is anyway not possible. On
// the other hand, this view exists mainly to support length changes, zipMD can
// be used if that is not required, i.e., maybe we do *not* need `ConstZipView`
// (if so, only for consistency?)?
template <class... Tags> class ZipView {
public:
  using value_type = std::tuple<typename Tags::type...>;

  ZipView(Dataset &dataset) {
    // As long as we do not support passing names, duplicate tags are not
    // supported, so this check should be enough.
    if (sizeof...(Tags) != dataset.size())
      throw std::runtime_error("ZipView must be constructed based on "
                               "*all* variables in a dataset.");
    // TODO Probably we can also support 0-dimensional variables that are not
    // touched?
    for (const auto &var : dataset)
      if (var.dimensions().count() != 1)
        throw std::runtime_error("ZipView supports only datasets where "
                                 "all variables are 1-dimensional.");
    if (dataset.dimensions().count() != 1)
      throw std::runtime_error("ZipView supports only 1-dimensional datasets.");

    m_dimensions = {&dataset(Tags{}).m_mutableVariable->mutableDimensions()...};
    m_data = std::make_tuple(
        &dataset(Tags{})
             .m_mutableVariable->template cast<typename Tags::type>()...);
  }

  template <size_t... Is> auto makeView(std::index_sequence<Is...>) {
    return ranges::view::zip(*std::get<Is>(m_data)...);
  }

  auto begin() {
    return makeView(std::make_index_sequence<sizeof...(Tags)>{}).begin();
  }
  auto end() {
    return makeView(std::make_index_sequence<sizeof...(Tags)>{}).end();
  }

  void push_back(const std::tuple<typename Tags::type...> &value) {
    AccessHelper<Tags...>::push_back(m_dimensions, m_data, value);
  }

private:
  std::array<Dimensions *, sizeof...(Tags)> m_dimensions;
  std::tuple<Vector<typename Tags::type> *...> m_data;
};

template <class... Tags>
void swap(typename ZipView<Tags...>::Item &a,
          typename ZipView<Tags...>::Item &b) noexcept {
  a.swap(b);
}

// TODO The item type (event type) is a tuple of references, which is not
// convenient for clients. For common cases we should have a wrapper with named
// getters. We can wrap this in `begin()` and `end()` using
// boost::make_transform_iterator.
template <class... Fields> class ConstEventListProxy {
public:
  ConstEventListProxy(const Fields &... fields) : m_fields(&fields...) {
    if (((std::get<0>(m_fields)->size() != fields.size()) || ...))
      throw std::runtime_error("Cannot zip data with mismatching length.");
  }

  template <size_t... Is> auto makeView(std::index_sequence<Is...>) const {
    return ranges::view::zip(*std::get<Is>(m_fields)...);
  }

  auto begin() const {
    return makeView(std::make_index_sequence<sizeof...(Fields)>{}).begin();
  }
  auto end() const {
    return makeView(std::make_index_sequence<sizeof...(Fields)>{}).end();
  }

private:
  std::tuple<const Fields *...> m_fields;
};

template <class... Fields>
class EventListProxy : public ConstEventListProxy<Fields...> {
public:
  EventListProxy(const bool mayResize, Fields &... fields)
      : ConstEventListProxy<Fields...>(fields...), m_mayResize(mayResize),
        m_fields(&fields...) {}

  template <size_t... Is> auto makeView(std::index_sequence<Is...>) const {
    return ranges::view::zip(*std::get<Is>(m_fields)...);
  }

  auto begin() const {
    return makeView(std::make_index_sequence<sizeof...(Fields)>{}).begin();
  }
  auto end() const {
    return makeView(std::make_index_sequence<sizeof...(Fields)>{}).end();
  }

  template <class... Ts> void push_back(const Ts &... values) const {
    static_assert(sizeof...(Fields) == sizeof...(Ts),
                  "Wrong number of fields in push_back.");
    requireResizable();
    doPushBack<Ts...>(values..., std::make_index_sequence<sizeof...(Ts)>{});
  }
  template <class... Ts>
  void push_back(const ranges::v3::common_pair<Ts &...> &values) const {
    static_assert(sizeof...(Fields) == sizeof...(Ts),
                  "Wrong number of fields in push_back.");
    requireResizable();
    doPushBack<Ts...>(values, std::make_index_sequence<sizeof...(Ts)>{});
  }
  template <class... Ts>
  void push_back(const ranges::v3::common_tuple<Ts &...> &values) const {
    static_assert(sizeof...(Fields) == sizeof...(Ts),
                  "Wrong number of fields in push_back.");
    requireResizable();
    doPushBack<Ts...>(values, std::make_index_sequence<sizeof...(Ts)>{});
  }

private:
  template <class... Ts, size_t... Is>
  void doPushBack(const Ts &... values, std::index_sequence<Is...>) const {
    (std::get<Is>(m_fields)->push_back(values), ...);
  }
  template <class... Ts, size_t... Is>
  void doPushBack(const ranges::v3::common_pair<Ts &...> &values,
                  std::index_sequence<Is...>) const {
    (std::get<Is>(m_fields)->push_back(std::get<Is>(values)), ...);
  }
  template <class... Ts, size_t... Is>
  void doPushBack(const ranges::v3::common_tuple<Ts &...> &values,
                  std::index_sequence<Is...>) const {
    (std::get<Is>(m_fields)->push_back(std::get<Is>(values)), ...);
  }

  void requireResizable() const {
    if (!m_mayResize)
      throw std::runtime_error(
          "Event list cannot be resized via an incomplete proxy.");
  }

  bool m_mayResize;
  std::tuple<Fields *...> m_fields;
};

namespace Access {
template <class T> struct Key {
  Key(const Tag tag, const std::string &name = "") : tag(tag), name(name) {}
  using type = T;
  const Tag tag;
  const std::string name;
};
template <class TagT>
Key(const TagT tag, const std::string &name = "")->Key<typename TagT::type>;

template <class T>
static auto Read(const Tag tag, const std::string &name = "") {
  return Key<const T>{tag, name};
}
template <class T>
static auto Write(const Tag tag, const std::string &name = "") {
  return Key<T>{tag, name};
}
}; // namespace Access

template <class T, size_t... Is>
constexpr auto doMakeEventListProxy(const bool mayResize, const T &item,
                                    std::index_sequence<Is...>) noexcept {
  return EventListProxy(mayResize, std::get<Is>(item)...);
}

template <class T, class... Keys> struct ItemProxy {
  static constexpr auto get(const T &item) noexcept { return item; }
  static constexpr auto getResizable(const T &item) noexcept { return item; }
};

template <class T, class Key> struct ItemProxy<T, Key> {
  static constexpr auto &get(const T &item) noexcept {
    return std::get<0>(item);
  }
  static constexpr auto &getResizable(const T &item) noexcept {
    return std::get<0>(item);
  }
};

// TODO Joint version for any iterable.
template <class T, class Item>
struct ItemProxy<T, Access::Key<std::vector<Item>>> {
  static constexpr auto get(const T &item) noexcept {
    return doMakeEventListProxy(false, item,
                                std::make_index_sequence<1>{});
  }
  static constexpr auto getResizable(const T &item) noexcept {
    return doMakeEventListProxy(true, item,
                                std::make_index_sequence<1>{});
  }
};

template <class T, class... Ts>
struct ItemProxy<T, Access::Key<std::vector<Ts>>...> {
  static constexpr auto get(const T &item) noexcept {
    return doMakeEventListProxy(false, item,
                                std::make_index_sequence<sizeof...(Ts)>{});
  }
  static constexpr auto getResizable(const T &item) noexcept {
    return doMakeEventListProxy(true, item,
                                std::make_index_sequence<sizeof...(Ts)>{});
  }
};

template <class T, class Item>
struct ItemProxy<T, Access::Key<boost::container::small_vector<Item, 8>>> {
  static constexpr auto get(const T &item) noexcept {
    return doMakeEventListProxy(false, item,
                                std::make_index_sequence<1>{});
  }
  static constexpr auto getResizable(const T &item) noexcept {
    return doMakeEventListProxy(true, item,
                                std::make_index_sequence<1>{});
  }
};

template <class T, class... Ts>
struct ItemProxy<T, Access::Key<boost::container::small_vector<Ts, 8>>...> {
  static constexpr auto get(const T &item) noexcept {
    return doMakeEventListProxy(false, item,
                                std::make_index_sequence<sizeof...(Ts)>{});
  }
  static constexpr auto getResizable(const T &item) noexcept {
    return doMakeEventListProxy(true, item,
                                std::make_index_sequence<sizeof...(Ts)>{});
  }
};

template <class D, class... Keys> class VariableZipProxy {
private:
  using type = decltype(ranges::view::zip(
      std::declval<D &>().template span<typename Keys::type>(Tag{})...));
  using item_type = decltype(std::declval<type>()[0]);

public:
  VariableZipProxy(D &dataset, const Keys &... keys)
      : m_view(ranges::view::zip(dataset.template span<typename Keys::type>(
            keys.tag, keys.name)...)) {
    // All requested keys must have same dimensions. This restriction could be
    // dropped for const access.
    const auto &key0 = std::get<0>(std::tuple<const Keys &...>(keys...));
    const auto &dims = dataset(key0.tag, key0.name).dimensions();
    if (((dims != dataset(keys.tag, keys.name).dimensions()) || ...))
      throw std::runtime_error("Variables to be zipped have mismatching "
                               "dimensions, use `zipMD()` instead.");
    // If for each key all fields from a group are included, the item proxy will
    // support push_back, in case the item is a vector-like.
    m_mayResizeItems = true;
    const std::array<std::pair<Tag, std::string>, sizeof...(Keys)> keyList{
        std::pair(keys.tag, keys.name)...};
    for (const auto &key : keyList) {
      if (std::count(keyList.begin(), keyList.end(), key) != 1)
        throw std::runtime_error("Duplicate key.");
      const auto &name = std::get<std::string>(key);
      gsl::index count = 0;
      for (const auto &key2 : keyList) {
        if (name == std::get<std::string>(key2))
          ++count;
      }
      gsl::index requiredCount = 0;
      for (const auto &var : dataset) {
        if (name == var.name())
          ++requiredCount;
      }
      m_mayResizeItems &= (count == requiredCount);
    }
  }

  gsl::index size() const { return m_view.size(); }
  auto begin() const {
    if (m_mayResizeItems)
      return boost::make_transform_iterator(
          m_view.begin(), ItemProxy<item_type, Keys...>::getResizable);
    else
      return boost::make_transform_iterator(m_view.begin(),
                                            ItemProxy<item_type, Keys...>::get);
  }
  auto end() const {
    if (m_mayResizeItems)
      return boost::make_transform_iterator(
          m_view.end(), ItemProxy<item_type, Keys...>::getResizable);
    else
      return boost::make_transform_iterator(m_view.end(),
                                            ItemProxy<item_type, Keys...>::get);
  }

private:
  bool m_mayResizeItems;
  type m_view;
};

template <class... Keys> auto zip(Dataset &dataset, const Keys &... keys) {
  return VariableZipProxy<Dataset, Keys...>(dataset, keys...);
}
template <class... Keys>
auto zip(const Dataset &dataset, const Keys &... keys) {
  return VariableZipProxy<const Dataset, Keys...>(dataset, keys...);
}

#endif // ZIP_VIEW_H
