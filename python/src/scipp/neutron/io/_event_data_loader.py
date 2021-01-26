import h5py
from typing import Optional, Union, Any, List
import numpy as np


class BadSource(Exception):
    pass


def _ensure_str(str_or_bytes: Union[str, bytes]) -> str:
    try:
        str_or_bytes = str(str_or_bytes, encoding="utf8")  # type: ignore
    except TypeError:
        pass
    return str_or_bytes


class EventDataLoader:
    def __init__(self, group: h5py.Group):
        """
        Load data, one pulse at a time from NXevent_data in NeXus file
        :raises BadSource if there is a critical problem with the data source
        """
        self._group = group
        self._warnings: List[str] = []

        if self._has_missing_fields():
            raise BadSource("\n".join(self._warnings))

        self._event_time_zero = self._group["event_time_zero"][...]
        self._event_index = self._group["event_index"][...]

        # There is some variation in the last recorded event_index in files
        # from different institutions. We try to make sure here that it is what
        # would be the first index of the next pulse.
        # In other words, ensure that event_index includes the bin edge for
        # the last pulse.
        if self._event_index[-1] < self._group["event_id"].len():
            self._event_index = np.append(
                self._event_index,
                np.array([self._group["event_id"].len() - 1
                          ]).astype(self._event_index.dtype),
            )
        else:
            self._event_index[-1] = self._group["event_id"].len()
        self.number_of_events = self._event_index[-1]

    @property
    def tof(self) -> np.ndarray:
        return self._group["event_time_offset"][...]

    @property
    def ids(self) -> np.ndarray:
        return self._group["event_id"][...]

    @property
    def tof_units(self) -> Optional[str]:
        try:
            units = self._group["event_time_offset"].attrs["units"]
        except AttributeError:
            return None
        return _ensure_str(units)

    @property
    def pulse_time_units(self) -> Optional[str]:
        try:
            units = self._group["event_time_zero"].attrs["units"]
        except AttributeError:
            return None
        return _ensure_str(units)

    @property
    def tof_dtype(self) -> Optional[Any]:
        try:
            dtype = self._group["event_time_offset"][0].dtype
        except AttributeError:
            return None
        return dtype

    @property
    def pulse_time_dtype(self) -> Optional[Any]:
        try:
            dtype = self._group["event_time_zero"][0].dtype
        except AttributeError:
            return None
        return dtype

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
                self._warnings.append(
                    f"Unable to load data from NXevent_data at"
                    f"'{self._group.name}' due to missing '{field}' field")
                missing_field = True
        return missing_field
