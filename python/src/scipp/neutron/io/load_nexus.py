from ..._scipp import core as sc
from ._event_data_loading import load_event_data
from ._log_data_loading import load_log_data_from_group
from ... import detail

import h5py
import numpy as np
from os.path import join
from timeit import default_timer as timer
from typing import Union, Tuple, Dict, Optional, List
from contextlib import contextmanager


def _get_attr_as_str(h5_object, attribute_name: str):
    try:
        return h5_object.attrs[attribute_name].decode("utf8")
    except AttributeError:
        return h5_object.attrs[attribute_name]


def _find_by_nx_class(
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
                if _get_attr_as_str(h5_object, "NX_class") in nx_class_names:
                    groups_with_requested_nx_class[_get_attr_as_str(
                        h5_object, "NX_class")].append(h5_object)
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
    Load a NeXus file and return required information.

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


def _load_nexus(data_file: Union[str, h5py.File],
                root: str = "/",
                instrument_file: Union[str, h5py.File, None] = None):
    """
    Allows h5py.File objects to be passed in place of
    file path strings in tests
    """
    print("Load NeXus")
    total_time = timer()

    with _open_if_path(data_file) as nexus_file:
        nx_event_data = "NXevent_data"
        nx_log = "NXlog"
        groups = _find_by_nx_class((nx_event_data, nx_log), nexus_file[root])

        loaded_data = load_event_data(groups[nx_event_data])

        for group in groups[nx_log]:
            log_data_name, log_data = load_log_data_from_group(group)
            loaded_data.attrs[log_data_name] = detail.move(log_data)

        # TODO instrument name, sample info...

    # Load positions?
    # start = timer()
    # if instrument_file is None:
    #     instrument_file = data_file
    # da.coords['position'] = load_positions(instrument_file, dim='spectrum')
    # print("Loading positions:", timer() - start)

    print("Total time:", timer() - total_time)
    return loaded_data


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
