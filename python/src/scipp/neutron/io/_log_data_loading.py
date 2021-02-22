import numpy as np
from typing import Tuple
from ..._scipp import core as sc
import h5py
from ._loading_common import ensure_str, BadSource


def load_log_data_from_group(group: h5py.Group) -> Tuple[str, sc.Variable]:
    property_name = group.name.split("/")[-1]
    value_dataset_name = "value"
    time_dataset_name = "time"

    try:
        values = group[value_dataset_name][...]
    except KeyError:
        raise BadSource(f"NXlog at {group.name} has no value dataset, "
                        f"skipping loading")

    try:
        unit = ensure_str(group[value_dataset_name].attrs["units"])
    except KeyError:
        raise BadSource(f"value dataset of NXlog at {group.name} has no "
                        f"unit attrs, skipping loading")

    try:
        times = group[time_dataset_name][...]
        dimension_label = "time"
        is_time_series = True
        time_unit = ensure_str(group[time_dataset_name].attrs["units"])
        # TODO convert them to datetime?
        #  should have a units attribute to check,
        #  they are float32 in seconds relative to start attribute
        #  in SNS and ISIS files
        #  uint64 in nanoseconds relative to unix epoch in ESS files
    except KeyError:
        dimension_label = property_name
        is_time_series = False

    if np.isscalar(values):
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
        data_array = sc.DataArray(data=property_data,
                                  coords={
                                      dimension_label:
                                      sc.Variable([dimension_label],
                                                  values=times,
                                                  dtype=sc.dtype.int64,
                                                  unit=time_unit)
                                  })
        return property_name, sc.Variable(value=data_array)
    elif not np.isscalar(values):
        # If property is multi-valued, create a wrapper single
        # value variable. This prevents interference with
        # global dimensions for for output Dataset.
        return property_name, sc.Variable(value=property_data)
    return property_name, property_data
