/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef EVENT_LIST_PROXY_H
#define EVENT_LIST_PROXY_H

#include "range/v3/view/zip.hpp"

#include "dataset.h"
#include "zip_view.h"

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
  EventListProxy(Fields &... fields)
      : ConstEventListProxy<Fields...>(fields...), m_fields(&fields...) {}

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
    doPushBack<Ts...>(values..., std::make_index_sequence<sizeof...(Ts)>{});
  }
  template <class... Ts>
  void push_back(const ranges::v3::common_pair<Ts &...> &values) const {
    static_assert(sizeof...(Fields) == sizeof...(Ts),
                  "Wrong number of fields in push_back.");
    doPushBack<Ts...>(values, std::make_index_sequence<sizeof...(Ts)>{});
  }
  template <class... Ts>
  void push_back(const ranges::v3::common_tuple<Ts &...> &values) const {
    static_assert(sizeof...(Fields) == sizeof...(Ts),
                  "Wrong number of fields in push_back.");
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
};

/// Note the plural in the name. This is a proxy for *all* event lists in a
/// dataset, i.e., this is a list of event lists. Each item returned by this
/// proxy if an EventListProxy, i.e., represents a single event list.
template <class... Keys>
class EventListsProxy {
public:
  EventListsProxy(Dataset &dataset, const Keys &... keys)
      : m_dataset(&dataset) {
    // All requested keys must be present in the dataset.
    if (!(dataset.contains(keys.tag, keys.name) && ...))
      throw std::runtime_error(
          "Dataset does not contain the requested event-data fields.");
  }

private:
  Dataset *m_dataset;
};

// TODO Rename to ConstEventListProxy and add EventListProxy, inheriting from
// ConstEventListProxy.
class EventListProxy2 {
public:
  // TODO Fix ZipView to work with const Dataset, or use something else here.
  EventListProxy2(Dataset &dataset) : m_dataset(&dataset) {}
  EventListProxy2(const typename Data::EventTofs_t::type &tofs,
                  const typename Data::EventPulseTimes_t::type &pulseTimes)
      : m_tofs(&tofs), m_pulseTimes(&pulseTimes) {
    if (tofs.size() != pulseTimes.size())
      throw std::runtime_error("Size mismatch for fields of event list.");
  }

  // TODO Either ZipView must be generalized, or we need to use a different view
  // type here, once we support another event storage format. Furthermore, we
  // want to support read-only access if only a subset of all fields is
  // requested, e.g., for reading only TOF, without need to know whether also
  // pulse-times or weights are present.
  template <class... Tags>
  ZipView<Tags...> getMutable(const Tags... tags) const {
    return ZipView<Tags...>(*m_dataset);
  }

  // TODO This should be template/overloaded for different field combination,
  // e.g., Tof-only, with weights, ...
  auto get() const {
    if (m_dataset) {
      const Dataset *d(m_dataset);
      return ranges::view::zip(d->get(Data::Tof), d->get(Data::PulseTime));
    }
    return ranges::view::zip(
        gsl::make_span(m_tofs->data(), m_tofs->data() + m_tofs->size()),
        gsl::make_span(m_pulseTimes->data(),
                       m_pulseTimes->data() + m_tofs->size()));
  }

private:
  Dataset *m_dataset{nullptr};
  const typename Data::EventTofs_t::type *m_tofs{nullptr};
  const typename Data::EventPulseTimes_t::type *m_pulseTimes{nullptr};
};

#endif // EVENT_LIST_PROXY_H
