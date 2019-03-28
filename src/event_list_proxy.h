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

// TODO This proxy provides a unified view for different underlying data
// formats. It is currently only used by zipMD and at this point it is unclear
// whether we want to and can provide such a unified access. `zip` is the "new"
// (but different) way to support event-list-style access.
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
        gsl::span(m_tofs->data(), m_tofs->data() + m_tofs->size()),
        gsl::span(m_pulseTimes->data(), m_pulseTimes->data() + m_tofs->size()));
  }

private:
  Dataset *m_dataset{nullptr};
  const typename Data::EventTofs_t::type *m_tofs{nullptr};
  const typename Data::EventPulseTimes_t::type *m_pulseTimes{nullptr};
};

#endif // EVENT_LIST_PROXY_H
