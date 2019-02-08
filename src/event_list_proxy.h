/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef EVENT_LIST_PROXY_H
#define EVENT_LIST_PROXY_H

#include "dataset.h"
#include "zip_view.h"

// Need also ConstEventListProxy?
class EventListProxy {
public:
  EventListProxy(Dataset &dataset) : m_dataset(&dataset) {}

  // TODO Either ZipView must be generalized, or we need to use a different view
  // type here, once we support another event storage format. Furthermore, we
  // want to support read-only access if only a subset of all fields is
  // requested, e.g., for reading only TOF, without need to know whether also
  // pulse-times or weights are present.
  template <class... Tags> ZipView<Tags...> get(const Tags... tags) const {
    return ZipView<Tags...>(*m_dataset);
  }

private:
  Dataset *m_dataset;
};

#endif // EVENT_LIST_PROXY_H
