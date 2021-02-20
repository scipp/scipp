import numpy as np
from typing import Tuple
from ..._scipp import core as sc
import h5py
from ._loading_common import ensure_str, BadSource


def load_log_data_from_group(group: h5py.Group) -> Tuple[str, sc.Variable]:
    property_name = group.name.split("/")[-1]
    if property_name == "value_log":
        property_name = group.parent.name.split("/")[-1]

    try:
        values = group["value"][...]
    except KeyError:
        raise BadSource(f"NXlog at {group.name} has no value dataset, "
                        f"skipping loading")

    try:
        unit = ensure_str(group["values"].attrs["units"])
    except KeyError:
        raise BadSource(f"value dataset of NXlog at {group.name} has no "
                        f"unit attrs, skipping loading")

    try:
        times = group["time"][...]
        dimension_label = "time"
        is_time_series = True
        # TODO convert them to datetime?
        #  should have a units attribute to check,
        #  they are float32 in seconds relative to start attribute
        #  in SNS and ISIS files
        #  uint64 in nanoseconds relative to unix epoch in ESS files
    except KeyError:
        dimension_label = property_name
        is_time_series = False

    if np.isscalar(values):
        property_data = sc.Variable(value=values, unit=unit)
    else:
        property_data = sc.Variable(values=values,
                                    unit=unit,
                                    dims=[dimension_label])

    if is_time_series:
        # If property has timestamps, create a DataArray
        data_array = sc.DataArray(data=property_data,
                                  coords={
                                      dimension_label:
                                      sc.Variable([dimension_label],
                                                  values=times,
                                                  dtype=sc.dtype.int64,
                                                  unit=sc.units.ns)
                                  })
        yield property_name, sc.Variable(value=data_array)
    elif not np.isscalar(values):
        # If property is multi-valued, create a wrapper single
        # value variable. This prevents interference with
        # global dimensions for for output Dataset.
        yield property_name, sc.Variable(value=property_data)
    else:
        yield property_name, property_data
