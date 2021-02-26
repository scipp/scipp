import h5py
from typing import Optional, List, Tuple
import numpy as np
from ._loading_common import ensure_str, BadSource, ensure_no_unsigned_type
from ..._scipp import core as sc
from ..._bins import bin
from datetime import datetime
from warnings import warn


def _get_units(dataset: h5py.Dataset) -> Optional[str]:
    try:
        units = dataset.attrs["units"]
    except AttributeError:
        return None
    return ensure_str(units)


def _check_for_missing_fields(group: h5py.Group) -> str:
    error_message = ""
    required_fields = (
        "event_time_zero",
        "event_index",
        "event_id",
        "event_time_offset",
    )
    for field in required_fields:
        if field not in group:
            error_message += f"Unable to load data from NXevent_data " \
                             f"at '{group.name}' due to missing '{field}'" \
                             f" field\n"
    return error_message


def _iso8601_to_datetime(iso8601: str) -> Optional[datetime]:
    try:
        return datetime.strptime(
            iso8601.translate(str.maketrans('', '', ':-Z')),
            "%Y%m%dT%H%M%S.%f")
    except ValueError:
        # Did not understand the format of the input string
        return None


def _load_event_group(group: h5py.Group) -> Tuple[sc.Variable, np.ndarray]:
    error_msg = _check_for_missing_fields(group)
    if error_msg:
        raise BadSource(error_msg)

    # There is some variation in the last recorded event_index in files
    # from different institutions. We try to make sure here that it is what
    # would be the first index of the next pulse.
    # In other words, ensure that event_index includes the bin edge for
    # the last pulse.
    event_index = group["event_index"][...].astype(np.int64)
    if event_index[-1] < group["event_id"].len():
        event_index = np.append(
            event_index,
            np.array([group["event_id"].len() - 1]).astype(event_index.dtype),
        )
    else:
        event_index[-1] = group["event_id"].len()

    number_of_events = event_index[-1]
    event_time_offset = sc.Variable(
        ['event'],
        values=group["event_time_offset"][...],
        dtype=ensure_no_unsigned_type(group["event_time_offset"].dtype.type),
        unit=_get_units(group["event_time_offset"]))
    event_id = sc.Variable(
        ['event'], values=group["event_id"][...],
        dtype=np.int32)  # assume int32 is safe for detector ids

    # Weights are not stored in NeXus, so use 1s
    weights = sc.Variable(['event'],
                          values=np.ones(event_id.shape),
                          dtype=np.float32)
    data = sc.DataArray(data=weights,
                        coords={
                            'tof': event_time_offset,
                            'detector-id': event_id
                        })

    detector_number = "detector_number"
    if detector_number in group.parent:
        # Hopefully the detector ids are recorded in the file
        detector_ids = group.parent[detector_number][...]
    else:
        # Otherwise we'll just have to bin according to whatever
        # ids we have a events for (pixels with no recorded events
        # will not have a bin)
        detector_ids = np.unique(event_id.values)

    print(f"Loaded event data from {group.name} containing "
          f"{number_of_events} events")

    return data, detector_ids


def load_event_data(
        event_data_groups: List[h5py.Group]) -> Optional[sc.DataArray]:
    event_data = []
    for group in event_data_groups:
        try:
            new_event_data = _load_event_group(group)
            event_data.append(new_event_data)
        except BadSource as e:
            warn(f"Skipped loading {group.name} due to:\n{e}")

    if not event_data:
        return
    else:

        def getDetectorId(events_and_max_det_id):
            return events_and_max_det_id[1][0]

        event_data.sort(key=getDetectorId)

        detector_ids = sc.Variable(dims=['detector-id'],
                                   values=np.concatenate(
                                       [data[1] for data in event_data]),
                                   dtype=np.int32)

        events, _ = event_data.pop(0)
        while event_data:
            new_events, _ = event_data.pop(0)
            events = sc.concatenate(events, new_events, dim="event")

        # Events in the NeXus file are effectively binned by pulse
        # (because they are recorded chronologically)
        # but for reduction it is more useful to bin by detector id
        events = bin(events, groups=[detector_ids])

    return events
