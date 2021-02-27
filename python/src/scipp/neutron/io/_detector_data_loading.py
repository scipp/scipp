from dataclasses import dataclass, astuple
import h5py
from typing import Optional, List
import numpy as np
from ._loading_common import ensure_str, BadSource, ensure_not_unsigned
from ..._scipp import core as sc
from ..._bins import bin
from datetime import datetime
from warnings import warn
from itertools import groupby

_detector_dimension = "detector-id"
_event_dimension = "event"


def _all_equal(iterable):
    g = groupby(iterable)
    return next(g, True) and not next(g, False)


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


def _load_positions(detector_group: h5py.Group,
                    detector_ids_size: int) -> Optional[sc.Variable]:
    try:
        x_positions = detector_group["x_pixel_offset"][...].flatten()
        y_positions = detector_group["y_pixel_offset"][...].flatten()
    except KeyError:
        return None
    try:
        z_positions = detector_group["z_pixel_offset"][...].flatten()
    except KeyError:
        z_positions = np.zeros_like(x_positions)

    if not _all_equal((x_positions.size, y_positions.size, z_positions.size,
                       detector_ids_size)):
        warn(f"Skipped loading pixel positions as pixel offset and id "
             f"dataset sizes do not match in {detector_group.name}")
        return None

    if "depends_on" in detector_group:
        warn(f"Loaded pixel positions for "
             f"{detector_group.name.split('/')[-1]} are relative to the "
             f"detector, not sample position, as parsing transformations "
             f"is not yet implemented")

    array = np.array([x_positions, y_positions, z_positions]).T
    return sc.Variable([_detector_dimension],
                       values=array,
                       dtype=sc.dtype.vector_3_float64)


@dataclass
class DetectorData:
    events: sc.Variable
    detector_ids: np.ndarray
    pixel_positions: Optional[sc.Variable] = None


def _load_event_group(group: h5py.Group) -> DetectorData:
    error_msg = _check_for_missing_fields(group)
    if error_msg:
        raise BadSource(error_msg)

    # There is some variation in the last recorded event_index in files
    # from different institutions. We try to make sure here that it is what
    # would be the first index of the next pulse.
    # In other words, ensure that event_index includes the bin edge for
    # the last pulse.
    event_id_ds = group["event_id"]
    event_index = group["event_index"][...].astype(np.int64)
    if event_index[-1] < event_id_ds.len():
        event_index = np.append(
            event_index,
            np.array([event_id_ds.len() - 1]).astype(event_index.dtype),
        )
    else:
        event_index[-1] = event_id_ds.len()

    number_of_events = event_index[-1]
    event_time_offset_ds = group["event_time_offset"]
    event_time_offset = sc.Variable([_event_dimension],
                                    values=event_time_offset_ds[...],
                                    dtype=ensure_not_unsigned(
                                        event_time_offset_ds.dtype.type),
                                    unit=_get_units(event_time_offset_ds))
    event_id = sc.Variable(
        [_event_dimension], values=event_id_ds[...],
        dtype=np.int32)  # assume int32 is safe for detector ids

    # Weights are not stored in NeXus, so use 1s
    weights = sc.Variable([_event_dimension],
                          values=np.ones(event_id.shape),
                          dtype=np.float32)
    data = sc.DataArray(data=weights,
                        coords={
                            'tof': event_time_offset,
                            _detector_dimension: event_id
                        })

    detector_number_ds_name = "detector_number"
    if detector_number_ds_name in group.parent:
        # Hopefully the detector ids are recorded in the file
        detector_ids = group.parent[detector_number_ds_name][...].flatten()
    else:
        # Otherwise we'll just have to bin according to whatever
        # ids we have a events for (pixels with no recorded events
        # will not have a bin)
        detector_ids = np.unique(event_id.values)

    detector_group = group.parent
    pixel_positions = None
    if "x_pixel_offset" in detector_group:
        pixel_positions = _load_positions(detector_group, detector_ids.size)

    print(f"Loaded event data from {group.name} containing "
          f"{number_of_events} events")

    return DetectorData(data, detector_ids, pixel_positions)


def load_detector_data(
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

        def getDetectorId(events_and_max_det_id: DetectorData):
            # Assume different detector banks do not have
            # intersecting ranges of detector ids
            return events_and_max_det_id.detector_ids[0]

        event_data.sort(key=getDetectorId)

        detector_ids = sc.Variable(
            dims=[_detector_dimension],
            values=np.concatenate([data.detector_ids for data in event_data]),
            dtype=np.int32)

        pixel_positions_loaded = all(
            [data.pixel_positions is not None for data in event_data])
        events, _, pixel_positions = astuple(event_data.pop(0))
        while event_data:
            new_events, _, new_pixel_positions = astuple(event_data.pop(0))
            events = sc.concatenate(events, new_events, dim=_event_dimension)
            if pixel_positions_loaded:
                pixel_positions = sc.concatenate(pixel_positions,
                                                 new_pixel_positions,
                                                 dim=_detector_dimension)

        # Events in the NeXus file are effectively binned by pulse
        # (because they are recorded chronologically)
        # but for reduction it is more useful to bin by detector id
        events = bin(events, groups=[detector_ids])
        if pixel_positions_loaded:
            events.coords['position'] = pixel_positions

    return events
