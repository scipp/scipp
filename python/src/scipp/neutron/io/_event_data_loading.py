import h5py
from typing import Optional, List, Tuple, Any
import numpy as np
from ._loading_common import ensure_str, BadSource
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


def _get_pulse_time_offset(pulse_time_dataset: h5py.Dataset) -> Optional[str]:
    try:
        pulse_offset_iso8601 = pulse_time_dataset.attrs["offset"]
    except KeyError:
        return None
    return ensure_str(pulse_offset_iso8601)


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


unsigned_to_signed = {
    np.uint32: np.int32,
    np.uint64: np.int64,
}


def _ensure_no_unsigned_type(dataset_type: Any):
    try:
        return unsigned_to_signed[dataset_type]
    except KeyError:
        return dataset_type


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
        dtype=_ensure_no_unsigned_type(group["event_time_offset"].dtype.type),
        unit=_get_units(group["event_time_offset"]))
    event_id = sc.Variable(
        ['event'], values=group["event_id"][...],
        dtype=np.int32)  # assume int32 is safe for detector ids
    # event_time_zero = sc.Variable(['pulse'],
    #                               values=group["event_time_zero"][...],
    #                               dtype=group["event_time_zero"].dtype.type)
    pulse_time_offset = _get_pulse_time_offset(group["event_time_zero"])

    unix_epoch = datetime(1970, 1, 1)
    if pulse_time_offset is not None and pulse_time_offset != unix_epoch:
        # TODO correct for time offset:
        #  make times relative to run start or unix epoch?
        NotImplementedError(
            "Found offset for pulse times but dealing with this "
            "is not implemented yet")

    # The end index for a pulse is the start index of the next pulse
    # begin_indices = sc.Variable(['pulse'], values=event_index[:-1])
    # end_indices = sc.Variable(['pulse'], values=event_index[1:])

    # Weights are not stored in NeXus, so use 1s
    weights = sc.Variable(['event'],
                          values=np.ones(event_id.shape),
                          dtype=np.float32)
    data = sc.DataArray(data=weights,
                        coords={
                            'tof': event_time_offset,
                            'detector-id': event_id
                        })
    # try:
    #     events = sc.DataArray(data=sc.bins(begin=begin_indices,
    #                                        end=end_indices,
    #                                        dim='event',
    #                                        data=data),
    #                           coords={'pulse-time': event_time_zero})
    # except IndexError:
    #     # For example found max uint64 at end of some event_index
    #     # datasets in SNS files
    #     raise BadSource("Unexpected values for event indices in "
    #                     "event_index dataset")

    # event_index may be large, so we try the maximum detector
    # id from a detector_numbers dataset file
    if "detector_numbers" in group:
        detector_ids = group['detector_numbers'][...]
    else:
        detector_ids = np.arange(sc.min(event_id).value,
                                 stop=sc.max(event_id).value + 1)

    print(f"Loaded event data from {group.name} containing "
          f"{number_of_events} events")

    return data, detector_ids


def load_event_data(event_data_groups: List[h5py.Group]) -> sc.DataArray:
    event_data = []
    for group in event_data_groups:
        try:
            new_event_data = _load_event_group(group)
            event_data.append(new_event_data)
        except BadSource as e:
            warn(f"Skipped loading {group.name} due to:\n{e}")

    if not event_data:
        raise RuntimeError("No valid event data found in file")
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
