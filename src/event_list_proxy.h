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

template <class... Fields>
class ConstEventListProxy {
public:
  ConstEventListProxy(const Fields &... fields) : m_fields(&fields...) {}

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

private:
  template <class... Ts, size_t... Is>
  void doPushBack(const Ts &... values, std::index_sequence<Is...>) const {
    (std::get<Is>(m_fields)->push_back(values), ...);
  }

  std::tuple<Fields *...> m_fields;
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
