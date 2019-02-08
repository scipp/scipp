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

// TODO Rename to ConstEventListProxy and add EventListProxy, inheriting from
// ConstEventListProxy.
class EventListProxy {
public:
  // TODO Fix ZipView to work with const Dataset, or use something else here.
  EventListProxy(Dataset &dataset) : m_dataset(&dataset) {}
  EventListProxy(const typename Data::EventTofs_t::type &tofs,
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
  template <class... Tags> ZipView<Tags...> get(const Tags... tags) const {
    return ZipView<Tags...>(*m_dataset);
  }

  auto get2() const {
    if (m_dataset)
      throw std::runtime_error(
          "TODO implement get for Data::Events storage format");
    return ranges::view::zip(*m_tofs, *m_pulseTimes);
  }

private:
  Dataset *m_dataset{nullptr};
  const typename Data::EventTofs_t::type *m_tofs{nullptr};
  const typename Data::EventPulseTimes_t::type *m_pulseTimes{nullptr};
};

#endif // EVENT_LIST_PROXY_H
