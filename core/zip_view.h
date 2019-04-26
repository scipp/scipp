// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef ZIP_VIEW_H
#define ZIP_VIEW_H

#include "range/v3/view/zip.hpp"

#include "dataset.h"

namespace scipp::core {

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
// TODO At this point this is mainly required for the (potentially deprecated)
// EventListProxy. Ideally it should be completely replaced by `zip` provided
// below.
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
      if (std::get<VariableSlice>(var).dimensions().count() != 1)
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
template <class... Fields> class ConstItemZipProxy {
public:
  ConstItemZipProxy(const Fields &... fields) : m_fields(&fields...) {
    if (((std::get<0>(m_fields)->size() != fields.size()) || ...))
      throw std::runtime_error("Cannot zip data with mismatching length.");
  }

  // NOTE `Fields` are not temporary proxy objects, so this does *not* suffer
  // from the same issue as VariableZipProxy below. Here it is ok if the zip
  // view goes out of scope, iterators will stay valid since they reference an
  // object not owned by the proxy.
  template <size_t... Is>
  constexpr auto makeView(std::index_sequence<Is...>) const noexcept {
    return ranges::view::zip(*std::get<Is>(m_fields)...);
  }

  scipp::index size() const { return std::get<0>(m_fields)->size(); }

  auto begin() const noexcept {
    return makeView(std::make_index_sequence<sizeof...(Fields)>{}).begin();
  }
  auto end() const noexcept {
    return makeView(std::make_index_sequence<sizeof...(Fields)>{}).end();
  }

private:
  std::tuple<const Fields *...> m_fields;
};

template <class... Fields>
class ItemZipProxy : public ConstItemZipProxy<Fields...> {
public:
  ItemZipProxy(const bool mayResize, Fields &... fields)
      : ConstItemZipProxy<Fields...>(fields...), m_mayResize(mayResize),
        m_fields(&fields...) {}

  template <size_t... Is>
  constexpr auto makeView(std::index_sequence<Is...>) const noexcept {
    return ranges::view::zip(*std::get<Is>(m_fields)...);
  }

  auto begin() const noexcept {
    return makeView(std::make_index_sequence<sizeof...(Fields)>{}).begin();
  }
  auto end() const noexcept {
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
  template <class... Ts> void push_back(const std::tuple<Ts...> &values) const {
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
  template <class... Ts, size_t... Is>
  void doPushBack(const std::tuple<Ts...> &values,
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

// See https://stackoverflow.com/a/29634934.
namespace detail {
// To allow ADL with custom begin/end
using std::begin;
using std::end;

template <typename T>
auto is_iterable_impl(int) -> decltype(
    begin(std::declval<T &>()) !=
        end(std::declval<T &>()), // begin/end and operator !=
    void(),                       // Handle evil operator ,
    ++std::declval<decltype(begin(std::declval<T &>())) &>(), // operator ++
    void(*begin(std::declval<T &>())),                        // operator*
    std::true_type{});

template <typename T> std::false_type is_iterable_impl(...);
} // namespace detail

template <typename T>
using is_iterable = decltype(detail::is_iterable_impl<T>(0));

template <class T, size_t... Is>
constexpr auto doMakeItemZipProxy(const bool mayResize, const T &item,
                                  std::index_sequence<Is...>) noexcept {
  if constexpr ((std::is_const_v<
                     std::remove_reference_t<decltype(std::get<Is>(item))>> &&
                 ...)) {
    static_cast<void>(mayResize);
    return ConstItemZipProxy(std::get<Is>(item)...);

  } else {
    return ItemZipProxy(mayResize, std::get<Is>(item)...);
  }
}

template <class T, bool Resizable, class... Keys> struct ItemProxy {
  static constexpr auto get(const T &item) noexcept {
    if constexpr ((is_iterable<typename Keys::type>::value && ...))
      return doMakeItemZipProxy(Resizable, item,
                                std::make_index_sequence<sizeof...(Keys)>{});
    else
      return item;
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
      scipp::index count = 0;
      for (const auto &key2 : keyList) {
        if (name == std::get<std::string>(key2))
          ++count;
      }
      scipp::index requiredCount = 0;
      for (const auto & [ n, t, var ] : dataset) {
        if (t.isData() && name == n)
          ++requiredCount;
      }
      m_mayResizeItems &= (count == requiredCount);
    }
  }

  scipp::index size() const { return m_view.size(); }
  auto operator[](const scipp::index i) const {
    return m_mayResizeItems
               ? ItemProxy<item_type, true, Keys...>::get(m_view[i])
               : ItemProxy<item_type, false, Keys...>::get(m_view[i]);
  }

  // TODO WARNING: We are creating the zip view from temporary scipp::span
  // objects. ranges::view::zip_view takes ownership of this temporary objects.
  // The iterators then reference this object (if zip_view was based on directly
  // on std::vector, the iterator would reference the vector, so this problem
  // would disappear). Therefore, we need to make sure the zip_view stays alive
  // after iterators have been created. We are thus disabling the rvalue
  // overloads of begin() and end(). Furthermore, the Python export of __iter__
  // for this class requires the special pybind11 flag `py::keep_alive`.
  // Ultimately it would be good if we could avoid this complications and make
  // iterators valid on their own. However, we should also keep in mind that we
  // may want to support zipping slice views and may want to pass ownership of
  // the slice view to the zip view.
  auto begin() const && = delete;
  auto begin() const & {
    return m_mayResizeItems ? makeIt<true>(m_view.begin())
                            : makeIt<false>(m_view.begin());
  }
  auto end() const && = delete;
  auto end() const & {
    return m_mayResizeItems ? makeIt<true>(m_view.end())
                            : makeIt<false>(m_view.end());
  }

private:
  template <bool Resizable, class It> auto makeIt(It it) const {
    return boost::make_transform_iterator(
        it, ItemProxy<item_type, Resizable, Keys...>::get);
  }

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

} // namespace scipp::core

#endif // ZIP_VIEW_H
