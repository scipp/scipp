from ..._scipp import core as sc
from ._detector_data_loading import load_detector_data
from ._log_data_loading import load_logs

import h5py
from timeit import default_timer as timer
from typing import Union, Tuple, Dict, List
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
                nx_class = _get_attr_as_str(h5_object, "NX_class")
                if nx_class in nx_class_names:
                    groups_with_requested_nx_class[nx_class].append(h5_object)
            except KeyError:
                pass

    root.visititems(_match_nx_class)
    # We should also check if root itself is an NX_class
    _match_nx_class(None, root)
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


def _add_instrument_name(instrument_group: h5py.Group, data: sc.Variable):
    try:
        data = data.attrs
    except AttributeError:
        pass

    try:
        data["instrument-name"] = sc.Variable(
            value=instrument_group["name"].asstr()[...].item())
    except KeyError:
        # No instrument name found to add
        pass


def load_nexus(data_file: Union[str, h5py.File], root: str = "/"):
    """
    Load a NeXus file and return required information.

    :param data_file: path of NeXus file containing data to load
    :param root: path of group in file, only load data from the subtree of
      this group

    Usage example:
      data = sc.neutron.load_nexus('PG3_4844_event.nxs')
    """
    total_time = timer()

    with _open_if_path(data_file) as nexus_file:
        nx_event_data = "NXevent_data"
        nx_log = "NXlog"
        nx_entry = "NXentry"
        nx_instrument = "NXinstrument"
        groups = _find_by_nx_class(
            (nx_event_data, nx_log, nx_entry, nx_instrument), nexus_file[root])

        if len(groups[nx_entry]) > 1:
            # We can't sensibly load from multiple NXentry, for example each
            # could could contain a description of the same detector bank
            # and lead to problems with clashing detector ids etc
            raise RuntimeError(
                "More than one NXentry group in file, use 'root' argument "
                "to specify which to load data from, for example"
                f"{__name__}('my_file.nxs', '/entry_2')")

        loaded_data = load_detector_data(groups[nx_event_data])
        if loaded_data is None:
            no_event_data = True
            loaded_data = sc.Dataset({})
        else:
            no_event_data = False

        load_logs(loaded_data, groups[nx_log])

        if groups[nx_instrument]:
            _add_instrument_name(groups[nx_instrument][0], loaded_data)

    # Return None if we have an empty dataset at this point
    if no_event_data and not loaded_data.keys():
        loaded_data = None

    print("Total time:", timer() - total_time)
    return loaded_data
