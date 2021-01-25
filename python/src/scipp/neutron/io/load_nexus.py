from ..._scipp import core as sc
from ._event_data_loader import EventDataLoader, BadSource
from ...compat._unit_map import lookup_units

import h5py
import numpy as np
from os.path import join
from timeit import default_timer as timer
from typing import Union, List, Dict, Optional, Any
from contextlib import contextmanager
from dataclasses import dataclass


def _find_by_nx_class(
        nx_class_names: List[str],
        root: Union[h5py.File, h5py.Group]) -> Dict[str, h5py.Group]:
    groups_with_requested_nx_class = {
        class_name: []
        for class_name in nx_class_names
    }

    def _match_nx_class(_, h5_object):
        if isinstance(h5_object, h5py.Group):
            try:
                if h5_object.attrs["NX_class"].decode(
                        "utf8") in nx_class_names:
                    groups_with_requested_nx_class[h5_object.attrs[
                        "NX_class"].decode("utf8")].append(h5_object)
            except AttributeError:
                pass

    root.visititems(_match_nx_class)
    return groups_with_requested_nx_class


@contextmanager
def _open_if_path(file_in: Union[str, h5py.File]):
    """
    Open if file path is provided,
    otherwise yield the existing h5py.File object
    """
    if isinstance(file_in, str):
        with h5py.File(file_in, "r", libver='latest', swmr=True) as nexus_file:
            yield nexus_file
    else:
        yield file_in


def load_nexus(data_file: str,
               root: str = "/",
               instrument_file: Optional[str] = None):
    """
    Load a hdf/nxs file and return required information.
    Note that the patterns are listed in order of preference,
    i.e. if more than one is present in the file, the data will be read
    from the first one found.

    :param data_file: path of NeXus file containing data to load
    :param root: path of group in file, only load data from the subtree of
      this group
    :param instrument_file: path of separate NeXus file containing
      detector positions, load_nexus will look in the data file for
      this information if instrument_file is not provided

    Usage example:
      data = sc.neutron.load_nexus('PG3_4844_event.nxs')
    """
    return _load_nexus(data_file, root, instrument_file)


@dataclass
class _Field:
    name: str
    dtype: Any  # numpy.typing only available on Python 3.8


def _all_equal(iterator):
    iterator = iter(iterator)
    try:
        first = next(iterator)
    except StopIteration:
        return True
    return all(first == rest for rest in iterator)


def _check_all_event_groups_use_same_units():
    pass


def _load_nexus(data_file: Union[str, h5py.File],
                root: str = "/",
                instrument_file: Union[str, h5py.File, None] = None):
    """
    Allows h5py.File objects to be passed in place of
    file path strings in tests
    """
    total_time = timer()

    with _open_if_path(data_file) as nexus_file:
        nx_event_data = "NXevent_data"
        groups = _find_by_nx_class([nx_event_data], nexus_file)

        event_sources = []
        for group in groups[nx_event_data]:
            try:
                event_sources.append(EventDataLoader(group))
            except BadSource:
                # Reason for error is printed as warning
                pass

        # TODO complain if there are multiple event data groups and
        #  they use different units
        #   complain if we don't recognise the units
        #   complain if units don't make sense for the data they correspond to?
        _check_all_event_groups_use_same_units

        # TODO convert unit string to scipp unit
        lookup_units

        # TODO preallocate events Variable
        #   look at streaming notebook for what it should look like
        # total_number_of_events = sum(
        #     [loader.number_of_events for loader in event_sources])

    ########################################################################
    fields = [
        _Field("event_id", np.int64),
        _Field("event_time_offset", np.float64),
        _Field("event_time_zero", np.float64),
        _Field("event_index", np.float64),
    ]

    entries = {field.name: {} for field in fields}

    spec_min = np.Inf  # Min spectrum number
    spec_max = 0  # Max spectrum number
    spec_num = []  # List of unique spectral numbers

    start = timer()
    with _open_if_path(data_file) as f:
        contents = []
        f[root].visit(contents.append)

        for item in contents:
            for field in fields:
                if item.endswith(field.name):
                    root = item.replace(field.name, '')
                    entries[field.name][root] = np.array(f[item][...],
                                                         dtype=field.dtype,
                                                         copy=True)
                    if field.name == "event_id":
                        spec_min = min(spec_min,
                                       entries[field.name][root].min())
                        spec_max = max(spec_max,
                                       entries[field.name][root].max())
                        spec_num.append(np.unique(entries[field.name][root]))
    print("Reading file:", timer() - start)

    # Combine all banks into single arrays into a new dict
    data = {}
    for key, item in entries.items():
        data[key] = np.concatenate(list(item.values()))

    # Determine number of spectra and create map from spectrum number to
    # detector id
    start = timer()
    spec_num = np.unique(np.concatenate(spec_num).ravel())
    spec_map = {val: key for (key, val) in enumerate(spec_num)}
    nspec = len(spec_num)
    data['spectrum'] = np.zeros_like(data['event_id'])
    for i in range(nspec):
        data['spectrum'][i] = spec_map[data['event_id'][i]]
    print("Building spectrum number map:", timer() - start)

    # Sort the event ids so we can fill in event lists in chunks instead of one
    # event at a time.
    # TODO: sort using scipp.sort()
    start = timer()
    indices = np.argsort(data["event_id"])
    print("ArgSort:", timer() - start)
    start = timer()
    data['event_id'] = data['event_id'][indices]
    data['event_time_offset'] = data['event_time_offset'][indices]
    print("Sort indexing:", timer() - start)

    # Now find the locations where the event ids change/jump. This will tell
    # us the number of events that are in each pixel.
    # TODO: Using scipp Variables and indexing to get diff1d:
    # var[dim, 1:] - var[dim, :-1]
    diff = np.ediff1d(data['event_id'])
    locs = np.where(diff > 0)[0]
    # Need to add leading zero and the total size of the array at the end
    locs = np.concatenate([[0], locs, [len(data['event_id'])]])

    # Create event list
    var = sc.Variable(dims=['spectrum'], shape=[nspec], dtype=sc.dtype.float64)
    # Weights are set to one by default
    weights = sc.Variable(dims=['spectrum'],
                          shape=[nspec],
                          unit=sc.units.counts,
                          dtype=sc.dtype.float64,
                          variances=True)

    # Populate event list chunk by chunk
    start = timer()
    for n in range(nspec):
        var['spectrum',
            n].values.extend(data["event_time_offset"][locs[n]:locs[n + 1]])
        ones = np.ones_like(var['spectrum', n].values)
        weights['spectrum', n].values = ones
        weights['spectrum', n].variances = ones
    print("Filling events:", timer() - start)

    # arange variable for spectra indices
    specs = sc.Variable(dims=['spectrum'],
                        values=np.arange(nspec),
                        dtype=sc.dtype.int32)
    # Also store the indices in the output? Are they every needed?
    ids = sc.Variable(dims=['spectrum'], values=spec_num, dtype=sc.dtype.int32)
    # Put everything together in a DataArray
    da = sc.DataArray(data=weights,
                      coords={
                          'spectrum': specs,
                          'ids': ids,
                          'tof': var
                      })
    da.coords['tof'].unit = sc.units.us

    # Load positions?
    start = timer()
    if instrument_file is None:
        instrument_file = data_file
    da.coords['position'] = load_positions(instrument_file, dim='spectrum')
    print("Loading positions:", timer() - start)

    print("Total time:", timer() - total_time)
    return da


def load_positions(instrument_file: str, entry='/', dim='position'):
    """
    Usage:
      d = sc.Dataset()
      d.coords['position'] = sc.neutron.load_positions('LOKI_Definition.hdf5')
    """
    return _load_positions(instrument_file, entry, dim)


def _load_positions(instrument_file: Union[str, h5py.File],
                    entry='/',
                    dim='position'):
    """
    Allows h5py.File objects to be passed in place of
    file path string in tests
    """

    # TODO: We need to think about how to link the correct position to the
    # correct pixel/spectrum, as currently the order is just the order in
    # which the banks are written to the file.

    pattern = 'transformations/location'

    xyz = "xyz"
    positions = {x: [] for x in xyz}

    with _open_if_path(instrument_file) as f:
        contents = []
        f[entry].visit(contents.append)

        for item in contents:

            if item.endswith(pattern) and (item.count('detector') > 0):
                print(item)
                root = item.replace(pattern, '')
                pos = f[item][()] * np.array(f[item].attrs['vector'])
                offsets = {x: None for x in xyz}
                size = None
                for i, x in enumerate(xyz):
                    entry = join(root, '{}_pixel_offset'.format(x))
                    if entry in f:
                        offsets[x] = f[entry][()].astype(
                            np.float64).ravel() + pos[i]
                        size = len(offsets[x])
                for i, x in enumerate(xyz):
                    if offsets[x] is not None:
                        positions[x].append(offsets[x])
                    else:
                        positions[x].append(np.zeros(size))

    array = np.array([
        np.concatenate(positions['x']),
        np.concatenate(positions['y']),
        np.concatenate(positions['z'])
    ]).T
    return sc.Variable([dim], values=array, dtype=sc.dtype.vector_3_float64)
