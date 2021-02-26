from dataclasses import dataclass
from typing import List, Union, Iterator, Optional, Tuple, Dict
import h5py
import numpy as np
from contextlib import contextmanager

h5root = Union[h5py.File, h5py.Group]


def _get_attr_as_str(h5_object, attribute_name: str):
    try:
        return h5_object.attrs[attribute_name].decode("utf8")
    except AttributeError:
        return h5_object.attrs[attribute_name]


def find_by_nx_class(
        nx_class_names: Tuple[str, ...],
        root: Union[h5py.File, h5py.Group]) -> Dict[str, List[h5py.Group]]:
    """
    Finds groups with requested NX_class in the subtree of root

    Returns a dictionary with NX_class name as the key and list of matching
    groups as the value
    """
    groups_with_requested_nx_class: Dict[str, List[h5py.Group]] = {
        class_name: []
        for class_name in nx_class_names
    }

    def _match_nx_class(_, h5_object):
        if isinstance(h5_object, h5py.Group):
            try:
                nx_class = _get_attr_as_str(h5_object, "NX_class")
                if nx_class in nx_class_names:
                    groups_with_requested_nx_class[nx_class].append(h5_object)
            except KeyError:
                pass

    root.visititems(_match_nx_class)
    # We should also check if root itself is an NX_class
    _match_nx_class(None, root)
    return groups_with_requested_nx_class


def _create_nx_class(group_name: str, nx_class_name: str,
                     parent: h5root) -> h5py.Group:
    nx_class = parent.create_group(group_name)
    nx_class.attrs["NX_class"] = nx_class_name
    return nx_class


@contextmanager
def in_memory_nexus_file_with_event_data() -> Iterator[h5py.File]:
    """
    Creates an in-memory NeXus file with an NXentry containing
    an NXevent_data group for use in tests
    """
    # "core" driver means file is "in-memory" not on disk.
    # backing_store=False prevents file being written to
    # disk on flush() or close().
    nexus_file = h5py.File('in_memory_events.nxs',
                           mode='w',
                           driver="core",
                           backing_store=False)
    try:
        entry_group = _create_nx_class("entry", "NXentry", nexus_file)
        # Add 5 events from 4 pulses
        event_data = EventData(
            np.array([1, 2, 3, 1, 3]), np.array([456, 743, 347, 345, 632]),
            np.array([
                1600766730000000000, 1600766731000000000, 1600766732000000000,
                1600766733000000000
            ]), np.array([0, 3, 3, 5]))
        _add_event_data_group_to_file(event_data, entry_group, "events")
        yield nexus_file
    finally:
        nexus_file.close()


@contextmanager
def in_memory_hdf5_file_with_two_nxentry() -> Iterator[h5py.File]:
    nexus_file = h5py.File('in_memory_events.nxs',
                           mode='w',
                           driver="core",
                           backing_store=False)
    try:
        _create_nx_class("entry_1", "NXentry", nexus_file)
        _create_nx_class("entry_2", "NXentry", nexus_file)
        yield nexus_file
    finally:
        nexus_file.close()


@dataclass
class EventData:
    event_id: np.ndarray
    event_time_offset: np.ndarray
    event_time_zero: np.ndarray
    event_index: np.ndarray


@dataclass
class Log:
    name: str
    value: np.ndarray
    time: Optional[np.ndarray] = None
    value_units: Optional[str] = None
    time_units: Optional[str] = None


@dataclass
class Detector:
    detector_numbers: np.ndarray
    event_data: Optional[EventData] = None
    log: Optional[Log] = None
    x_offsets: Optional[np.ndarray] = None
    y_offsets: Optional[np.ndarray] = None
    z_offsets: Optional[np.ndarray] = None


def _add_event_data_group_to_file(data: EventData, parent_group: h5py.Group,
                                  group_name: str):
    event_group = _create_nx_class(group_name, "NXevent_data", parent_group)
    event_group.create_dataset("event_id", data=data.event_id)
    event_time_offset_ds = event_group.create_dataset(
        "event_time_offset", data=data.event_time_offset)
    event_time_offset_ds.attrs["units"] = "ns"
    event_time_zero_ds = event_group.create_dataset("event_time_zero",
                                                    data=data.event_time_zero)
    event_time_zero_ds.attrs["units"] = "ns"
    event_group.create_dataset("event_index", data=data.event_index)


def _add_detector_group_to_file(detector: Detector, parent_group: h5py.Group,
                                group_name: str) -> h5py.Group:
    detector_group = _create_nx_class(group_name, "NXdetector", parent_group)
    detector_group.create_dataset("detector_number",
                                  data=detector.detector_numbers)
    if detector.x_offsets is not None:
        detector_group.create_dataset("x_pixel_offset",
                                      data=detector.x_offsets)
    if detector.y_offsets is not None:
        detector_group.create_dataset("y_pixel_offset",
                                      data=detector.y_offsets)
    if detector.z_offsets is not None:
        detector_group.create_dataset("z_pixel_offset",
                                      data=detector.z_offsets)
    return detector_group


def _add_log_group_to_file(log, parent_group):
    log_group = _create_nx_class(log.name, "NXlog", parent_group)
    value_ds = log_group.create_dataset("value", data=log.value)
    if log.value_units is not None:
        value_ds.attrs.create("units", data=log.value_units)
    if log.time is not None:
        time_ds = log_group.create_dataset("time", data=log.time)
        if log.time_units is not None:
            time_ds.attrs.create("units", data=log.time_units)


class InMemoryNexusFileBuilder:
    """
    Allows building an in-memory NeXus file for use in tests
    """
    def __init__(self):
        self._event_data: List[EventData] = []
        self._detectors: List[Detector] = []
        self._logs: List[Log] = []
        self._instrument_name = None
        self._sample_position = None

    def add_detector(self, detector: Detector):
        self._detectors.append(detector)

    def add_event_data(self, event_data: EventData):
        self._event_data.append(event_data)

    def add_log(self, log: Log):
        self._logs.append(log)

    def add_instrument(self, name: str):
        self._instrument_name = name

    def add_sample(self, position: Optional[np.ndarray] = None):
        self._sample_position = position

    @contextmanager
    def file(self) -> Iterator[h5py.File]:
        # "core" driver means file is "in-memory" not on disk.
        # backing_store=False prevents file being written to
        # disk on flush() or close().
        nexus_file = h5py.File('in_memory_events.nxs',
                               mode='w',
                               driver="core",
                               backing_store=False)
        try:
            entry_group = _create_nx_class("entry", "NXentry", nexus_file)
            self._write_event_data(entry_group)
            self._write_logs(entry_group)
            self._write_sample(entry_group)
            if self._instrument_name is None:
                parent_group = entry_group
            else:
                parent_group = self._write_instrument(entry_group)
            self._write_detectors(parent_group)
            yield nexus_file
        finally:
            nexus_file.close()

    def _write_sample(self, parent_group: h5py.Group):
        _create_nx_class("sample", "NXsample", parent_group)

    def _write_instrument(self, parent_group: h5py.Group) -> h5py.Group:
        instrument_group = _create_nx_class("instrument", "NXinstrument",
                                            parent_group)
        instrument_group.create_dataset("name", data=self._instrument_name)
        return instrument_group

    def _write_detectors(self, parent_group: h5py.Group):
        for detector_index, detector in enumerate(self._detectors):
            detector_group = _add_detector_group_to_file(
                detector, parent_group, f"detector_{detector_index}")
            if detector.event_data is not None:
                _add_event_data_group_to_file(detector.event_data,
                                              detector_group, "events")
            if detector.log is not None:
                _add_log_group_to_file(detector.log, detector_group)

    def _write_event_data(self, parent_group: h5py.Group):
        for event_data_index, event_data in enumerate(self._event_data):
            _add_event_data_group_to_file(event_data, parent_group,
                                          f"events_{event_data_index}")

    def _write_logs(self, parent_group: h5py.Group):
        for log in self._logs:
            _add_log_group_to_file(log, parent_group)
