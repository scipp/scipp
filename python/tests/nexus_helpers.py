from dataclasses import dataclass
from typing import List, Union, Iterator
import h5py
import numpy as np
from contextlib import contextmanager

h5root = Union[h5py.File, h5py.Group]


def find_by_nx_class(nx_class_name: str, root: h5root) -> List[h5py.Group]:
    """
    Find groups in the HDF5 file or group which have the
    requested NX_class attribute
    Recursively searches all subgroups of the provided file or group

    :param nx_class_name: Name of the NX class, one of
      https://manual.nexusformat.org/classes/base_classes
    :param root: HDF5 file or group to search
    :return: List of groups matching requested NX_class
    """
    groups_with_requested_nx_class = []

    def _match_nx_class(name, object):
        if isinstance(object, h5py.Group):
            try:
                try:
                    nx_class = object.attrs["NX_class"].decode("utf8")
                except AttributeError:
                    nx_class = object.attrs["NX_class"]
                if nx_class == nx_class_name:
                    groups_with_requested_nx_class.append(object)
            except AttributeError:
                pass

    root.visititems(_match_nx_class)
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


@dataclass
class EventData:
    event_id: np.ndarray
    event_time_offset: np.ndarray
    event_time_zero: np.ndarray
    event_index: np.ndarray


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


class InMemoryNexusFileBuilder:
    """
    Allows building an in-memory NeXus file for use in tests
    """
    def __init__(self):
        self._event_data: List[EventData] = []

    def add_event_data(self, event_data: EventData):
        self._event_data.append(event_data)

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
            for event_data_index, event_data in enumerate(self._event_data):
                _add_event_data_group_to_file(event_data, entry_group,
                                              f"events_{event_data_index}")
            yield nexus_file
        finally:
            nexus_file.close()
