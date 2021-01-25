import h5py
from typing import Tuple, Optional, Generator, Union
import numpy as np
import warnings


class BadSource(Exception):
    pass


def _ensure_str(str_or_bytes: Union[str, bytes]) -> str:
    try:
        str_or_bytes = str(str_or_bytes, encoding="utf8")  # type: ignore
    except TypeError:
        pass
    return str_or_bytes


class _ChunkedDataLoader:
    """
    Loads one dataset chunk at a time and avoids repeated
    loading of the same chunk
    Very important for performance, particularly if data is compressed
    """
    def __init__(self, dataset: h5py.Dataset):
        self._dataset = dataset
        self._chunk_iterator = self._dataset.iter_chunks()
        next_slice = next(self._chunk_iterator)
        self._current_chunk = self._dataset[next_slice]
        self._start_index: int = 0

    def get_data_for_pulse(self, pulse_start_event: int,
                           pulse_end_event: int) -> np.ndarray:
        start_index = int(pulse_start_event - self._start_index)
        end_index = int(pulse_end_event - self._start_index)

        data_for_pulse = np.array([], dtype=self._current_chunk.dtype)
        while True:
            # If all the data we need is in the current, cached chunk,
            # then just append and return it
            if end_index < self._current_chunk.size:
                return np.append(data_for_pulse,
                                 self._current_chunk[start_index:end_index])
            # else...
            # we need all the data in the current chunk...
            data_for_pulse = np.append(data_for_pulse,
                                       self._current_chunk[start_index:])
            # and at least some from the next chunk,
            # so load the next chunk and continue
            end_index -= self._current_chunk.size
            start_index = 0
            self._start_index += self._current_chunk.size
            try:
                next_slice = next(self._chunk_iterator)
            except StopIteration:
                return data_for_pulse
            self._current_chunk = self._dataset[next_slice]


class _ContiguousDataLoader:
    """
    Loads the whole dataset from file in constructor
    Not ideal for memory use, hopefully large datasets are
    instead chunked
    """
    def __init__(self, dataset: h5py.Dataset):
        self._dataset = dataset[...]

    def get_data_for_pulse(self, pulse_start_event: int,
                           pulse_end_event: int) -> np.ndarray:
        return self._dataset[pulse_start_event:pulse_end_event]


_DataLoader = Union[_ChunkedDataLoader, _ContiguousDataLoader]


class EventDataLoader:
    def __init__(self, group: h5py.Group):
        """
        Load data, one pulse at a time from NXevent_data in NeXus file
        :raises BadSource if there is a critical problem with the data source
        """
        self._group = group
        self._tof_loader: _DataLoader
        self._id_loader: _DataLoader

        if self._has_missing_fields():
            raise BadSource()

        self._event_time_zero = self._group["event_time_zero"][...]
        self._event_index = self._group["event_index"][...]

        # There is some variation in the last recorded event_index in files
        # from different institutions for example ISIS files often have what
        # would be the first index of the next pulse at the end.
        # This logic hopefully covers most cases
        if self._event_index[-1] < self._group["event_id"].len():
            self._event_index = np.append(
                self._event_index,
                np.array([self._group["event_id"].len() - 1
                          ]).astype(self._event_index.dtype),
            )
        else:
            self._event_index[-1] = self._group["event_id"].len()
        self.number_of_events = self._event_index[-1]

        try:
            self._group["event_time_offset"].iter_chunks()
            self._tof_loader = _ChunkedDataLoader(
                self._group["event_time_offset"])
        except TypeError:
            self._tof_loader = _ContiguousDataLoader(
                self._group["event_time_offset"])

        try:
            self._group["event_id"].iter_chunks()
            self._id_loader = _ChunkedDataLoader(self._group["event_id"])
        except TypeError:
            self._id_loader = _ContiguousDataLoader(self._group["event_id"])

    def get_data(
        self,
    ) -> Generator[Tuple[Optional[np.ndarray], Optional[np.ndarray], int],
                   None, None]:
        """
        Returns None instead of a data when there is no more data
        """
        # -1 as last event index is the end index of the last pulse,
        # not start of a new pulse
        for pulse_number in range(self._group["event_index"].len() - 1):
            pulse_time = self._event_time_zero[pulse_number]
            start_event = self._event_index[pulse_number]
            end_event = self._event_index[pulse_number + 1]
            yield self._tof_loader.get_data_for_pulse(
                start_event, end_event), self._id_loader.get_data_for_pulse(
                    start_event, end_event), pulse_time
        yield None, None, 0

    def get_tof_units(self) -> Optional[str]:
        try:
            units = self._group["event_time_offset"].attrs["units"]
        except AttributeError:
            return None
        return _ensure_str(units)

    def get_pulse_time_units(self) -> Optional[str]:
        try:
            units = self._group["event_time_zero"].attrs["units"]
        except AttributeError:
            return None
        return _ensure_str(units)

    def get_pulse_time_offset(self) -> Optional[str]:
        try:
            pulse_offset_iso8601 = self._group["event_time_zero"].attrs[
                "offset"]
        except KeyError:
            return None
        return _ensure_str(pulse_offset_iso8601)

    def _has_missing_fields(self) -> bool:
        missing_field = False
        required_fields = (
            "event_time_zero",
            "event_index",
            "event_id",
            "event_time_offset",
        )
        for field in required_fields:
            if field not in self._group:
                warnings.warn(
                    f"Unable to load data from NXevent_data at"
                    f"{self._group.name} due to missing {field} field")
                missing_field = True
        return missing_field
