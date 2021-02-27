import numpy as np
from typing import Tuple
from ..._scipp import core as sc
import h5py
from ._loading_common import ensure_str, BadSource, ensure_not_unsigned
from warnings import warn
from ... import detail


def load_logs(loaded_data, log_groups):
    for group in log_groups:
        try:
            log_data_name, log_data = _load_log_data_from_group(group)
            _add_log_to_data(log_data_name, log_data, group.name, loaded_data)
        except BadSource as e:
            warn(f"Skipped loading {group.name} due to:\n{e}")


def _add_log_to_data(log_data_name: str, log_data: sc.Variable,
                     group_path: str, data: sc.Variable):
    try:
        data = data.attrs
    except AttributeError:
        pass

    group_path = group_path.split('/')
    path_position = -2
    name_changed = False
    unique_name_found = False
    while not unique_name_found:
        if log_data_name not in data.keys():
            data[log_data_name] = detail.move(log_data)
            unique_name_found = True
        else:
            name_changed = True
            log_data_name = f"{group_path[path_position]}-{log_data_name}"
            path_position -= 1
    if name_changed:
        warn(f"Name of log group at {group_path} is not unique: "
             f"{log_data_name} used as attribute name.")


def _load_log_data_from_group(group: h5py.Group) -> Tuple[str, sc.Variable]:
    property_name = group.name.split("/")[-1]
    value_dataset_name = "value"
    time_dataset_name = "time"

    try:
        values = group[value_dataset_name][...]
    except KeyError:
        raise BadSource(f"NXlog at {group.name} has no value dataset")

    try:
        unit = ensure_str(group[value_dataset_name].attrs["units"])
    except KeyError:
        unit = ""

    try:
        times = group[time_dataset_name][...]
        dimension_label = "time"
        is_time_series = True
        try:
            time_unit = ensure_str(group[time_dataset_name].attrs["units"])
        except KeyError:
            time_unit = ""
    except KeyError:
        dimension_label = property_name
        is_time_series = False

    values = np.squeeze(values)
    if np.ndim(values) > 1:
        raise BadSource(f"NXlog at {group.name} has {value_dataset_name} "
                        f"dataset with more than 1 dimension, handling "
                        f"this is not yet implemented")

    if np.ndim(values) == 0:
        property_data = sc.Variable(value=values,
                                    unit=unit,
                                    dtype=group[value_dataset_name].dtype.type)
    else:
        property_data = sc.Variable(values=values,
                                    unit=unit,
                                    dims=[dimension_label],
                                    dtype=group[value_dataset_name].dtype.type)

    if is_time_series:
        # If property has timestamps, create a DataArray
        data_array = sc.DataArray(
            data=property_data,
            coords={
                dimension_label:
                sc.Variable([dimension_label],
                            values=times,
                            dtype=ensure_not_unsigned(
                                group[time_dataset_name].dtype.type),
                            unit=time_unit)
            })
        return property_name, sc.Variable(value=data_array)
    elif not np.isscalar(values):
        # If property is multi-valued, create a wrapper single
        # value variable. This prevents interference with
        # global dimensions for for output Dataset.
        return property_name, sc.Variable(value=property_data)
    return property_name, property_data
