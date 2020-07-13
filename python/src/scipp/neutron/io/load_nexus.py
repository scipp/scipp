from ..._scipp import core as sc

import h5py
import numpy as np
from os.path import join
from timeit import default_timer as timer


def load_nexus(filename, entry="/", verbose=False, convert_ids=False):
    """
    Load a hdf/nxs file and return required information.
    Note that the patterns are listed in order of preference,
    i.e. if more than one is present in the file, the data will be read
    from the first one found.
    """
    total_time = timer()

    fields = {}
    fields["event_id"] = {
        "pattern": ["event_id"],
        "dtype": np.int32
    }
    fields["event_time_offset"] = {
        "pattern": ["event_time_offset"],
        "dtype": np.float64
    }
    fields["event_time_zero"] = {
        "pattern": ["event_time_zero"],
        "dtype": np.float64
    }
    fields["event_index"] = {
        "pattern": ["event_index"],
        "dtype": np.float64
    }
    entries = { key: {} for key in fields}

    spec_min = np.Inf # Min spectrum number
    spec_max = 0 # Max spectrum number
    spec_num = [] # List of unique spectral numbers

    start = timer()
    with h5py.File(filename, "r", libver='latest', swmr=True) as f:

        contents = []
        f[entry].visit(contents.append)

        for item in contents:
            for key in fields.keys():
                for p in fields[key]["pattern"]:
                    if item.endswith(p):
                        root = item.replace(key, '')
                        entries[key][root] = np.array(
                            f[item][...],
                            dtype=fields[key]["dtype"],
                            copy=True)
                        if key == "event_id":
                            spec_min = min(spec_min, entries[key][root].min())
                            spec_max = max(spec_max, entries[key][root].max())
                            spec_num.append(np.unique(entries[key][root]))
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
    var = sc.Variable(dims=['spectrum'],
                      shape=[nspec],
                      dtype=sc.dtype.event_list_float64)
    # Weights are set to one by default
    weights = sc.Variable(
        dims=['spectrum'],
        shape=[nspec],
        unit=sc.units.counts,
        dtype=sc.dtype.event_list_float64,
        variances=True)

    # Populate event list chunk by chunk
    start = timer()
    for n in range(nspec):
        var['spectrum', n].values.extend(data["event_time_offset"][locs[n]:locs[n+1]])
        ones = np.ones_like(var['spectrum', n].values)
        weights['spectrum', n].values = ones
        weights['spectrum', n].variances = ones
    print("Filling events:", timer() - start)

    # arange variable for spectra indices
    specs = sc.Variable(dims=['spectrum'], values=np.arange(nspec), dtype=sc.dtype.int32)
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

    print("Total time:", timer() - total_time)
    return da


def load_positions(filename, entry='/', dim='position'):
    """
    Usage:
      d = sc.Dataset()
      d.coords['position'] = sc.neutron.load_positions('LOKI_Tube_Definition.hdf5')
    """

    # TODO: We need to think about how to link the correct position to the
    # correct pixel/spectrum, as currently the order is just the order in
    # which the banks are written to the file.

    pattern = 'transformations/location'

    xyz = "xyz"
    positions = {x: [] for x in xyz}

    with h5py.File(filename, "r", libver='latest', swmr=True) as f:

        contents = []
        f[entry].visit(contents.append)

        for item in contents:

            if item.endswith(pattern) and (item.count('detector') > 0):
                root = item.replace(pattern, '')
                pos = f[item][()] * np.array(f[item].attrs['vector'])
                offsets = {x: None for x in xyz}
                size = None
                for i, x in enumerate(xyz):
                    entry = join(root, '{}_pixel_offset'.format(x))
                    if entry in f:
                        offsets[x] = f[entry][()].astype(np.float64) + pos[i]
                        size = len(offsets[x])
                for i, x in enumerate(xyz):
                    if offsets[x] is not None:
                        positions[x].append(offsets[x])
                    else:
                        positions[x].append(np.zeros(size))

    array = np.array([np.concatenate(positions['x']),
                      np.concatenate(positions['y']),
                      np.concatenate(positions['z'])]).T
    return sc.Variable([dim], values=array, dtype=sc.dtype.vector_3_float64)
