from ..._scipp import core as sc

import h5py
import numpy as np

# class NxsData:
#     """
#     Class to hold data in Nexus file
#     """

#     def __init__(self, fields=None):

#         for key in fields.keys():
#             setattr(self, key, fields[key]["data"])

#         return

from timeit import default_timer as timer


def load_nexus(filename, entry="/", verbose=False, convert_ids=False):
    """
    Load a hdf/nxs file and return required information.
    Note that the patterns are listed in order of preference,
    i.e. if more than one is present in the file, the data will be read
    from the first one found.
    """
    fields = {}

    # fields = {"x": {"pattern": ["/x_pixel_offset"],
    #                 "entry": None,
    #                 "data": None,
    #                 "dtype": np.float64},
    #           "y": {"pattern": ["/y_pixel_offset"],
    #                 "entry": None,
    #                 "data": None,
    #                 "dtype": np.float64},
    #           "title":{"pattern": ["/title"],
    #                    "entry": None,
    #                    "data": None,
    #                    "dtype": str}
    #           }

    fields["event_id"] = {
        "pattern": ["event_id"],
        "entry": None,
        "data": None,
        "dtype": np.int32
    }
    # if tofs:
    fields["event_time_offset"] = {
        "pattern": ["event_time_offset"],
        "entry": None,
        "data": None,
        "dtype": np.float64
    }

    fields["event_time_zero"] = {
        "pattern": ["event_time_zero"],
        "entry": None,
        "data": None,
        "dtype": np.float64
    }

    fields["event_index"] = {
        "pattern": ["event_index"],
        "entry": None,
        "data": None,
        "dtype": np.float64
    }

    entries = {
        "event_id": {},
        "event_time_offset": {},
        "event_time_zero": {},
        "event_index": {}
    }

    spec_min = np.Inf
    spec_max = 0

    spec_num = []

    start = timer()
    with h5py.File(filename, "r", libver='latest', swmr=True) as f:

        contents = []
        f[entry].visit(contents.append)

        # for item in contents:
        #     print(item)
        #     for key in fields.keys():
        #         if fields[key]["entry"] is None:
        #             for p in fields[key]["pattern"]:
        #                 if item.endswith(p):
        #                     fields[key]["entry"] = item

        # for key in fields.keys():
        #     fields[key]["data"] = np.array(f[fields[key]["entry"]][...],
        #                                    dtype=fields[key]["dtype"],
        #                                    copy=True)

        for item in contents:
            # print(item)
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

    data = {}
    for key, item in entries.items():
        data[key] = np.concatenate(list(item.values()))

    for key, item in data.items():
        print(key, item.shape)

    start = timer()
    spec_num = np.unique(np.concatenate(spec_num).ravel())
    spec_map = {val: key for (key, val) in enumerate(spec_num)}
    nspec = len(spec_num)
    data['spectrum'] = np.zeros_like(data['event_id'])
    for i in range(nspec):
        data['spectrum'][i] = spec_map[data['event_id'][i]]
    print(data)
    print("Spec map:", timer() - start)


    # ids = sc.Variable(['x'], values=data['event_id'])
    # temp = sc.Dataset()
    # temp['tof'] = sc.Variable(['x'], values=data['event_time_offset'])
    # temp['ids'] = ids


    start = timer()
    # sort = sc.sort(temp, ids)
    # print(temp)
    indices = np.argsort(data["event_id"])
    print("ArgSort:", timer() - start)
    start = timer()
    data['event_id'] = data['event_id'][indices]
    data['event_time_offset'] = data['event_time_offset'][indices]
    print("Sort indexing:", timer() - start)
    diff = np.ediff1d(data['event_id'])
    print(diff)
    locs = np.where(diff > 0)[0]
    print(locs)
    print(np.ediff1d(locs))
    n_events_per_id = np.concatenate([locs[0:1], np.ediff1d(locs)])
    locs = np.concatenate([[0], locs, [len(data['event_id'])]])



    var = sc.Variable(dims=['spectrum'],
                      shape=[nspec],
                      dtype=sc.dtype.event_list_float64)


    weights = sc.Variable(
        dims=['spectrum'],
        shape=[nspec],
        unit=sc.units.counts,
        dtype=sc.dtype.event_list_float64,
        variances=True)
    # for i in range(nspec):
    #     ones = np.ones(len(var['spectrum', i].values))
    #     weights['spectrum', i].values = ones
    #     weights['spectrum', i].variances = ones



    start = timer()
    # for key, item in entries["event_id"].items():
    #     print(key)
    print(len(locs))
    print(nspec)
    print(locs[-1])
    for n in range(nspec):
        # ispec = spec_map[it]
        # print(n)
        # print(locs[n])
        var['spectrum', n].values.extend(data["event_time_offset"][locs[n]:locs[n+1]])
        ones = np.ones_like(var['spectrum', n].values)
        weights['spectrum', n].values = ones
        weights['spectrum', n].variances = ones

        # break
    print("Filling events:", timer() - start)
    print(var)
    specs = sc.Variable(dims=['spectrum'], values=np.arange(nspec), dtype=sc.dtype.int32)
    ids = sc.Variable(dims=['spectrum'], values=spec_num, dtype=sc.dtype.int32)
    da = sc.DataArray(data=weights,
                      coords={
                          'spectrum': specs,
                          'ids': ids,
                          'tof': var
                      })
    da.coords['tof'].unit = sc.units.us




    return da







    specs = sc.Variable(dims=['spectrum'], values=np.arange(nspec), dtype=sc.dtype.int32)


    # var = sc.Variable(dims=['x'],
    #                   shape=[1],
    #                   dtype=sc.dtype.event_list_float64,
    #                   unit=sc.units.us)
    # var['x', 0].values.append(data['event_time_offset'])

    da = sc.DataArray(data=sc.Variable(dims=['x'], values=np.ones_like(data['event_time_offset'])),
          coords={'x': sc.Variable(dims=['x'], values=data['event_time_offset']),
          'spectrum': sc.Variable(dims=['x'], values=data['spectrum'])})

    grouped = sc.groupby(da, 'spectrum', bins=specs).flatten('x')
    print(grouped)
    return


    var = sc.Variable(dims=['spectrum'],
                      shape=[nspec],
                      dtype=sc.dtype.event_list_float64)

    start = timer()
    for key, item in entries["event_id"].items():
        print(key)
        for n, it in enumerate(item):
            ispec = spec_map[it]
            var['spectrum',
                ispec].values.append(entries["event_time_offset"][key][n])
        # break
    print("Filling events:", timer() - start)
    print(var)

    weights = sc.Variable(
        dims=['spectrum'],
        shape=[nspec],
        unit=sc.units.counts,
        dtype=sc.dtype.event_list_float64,
        variances=True)
    for i in range(nspec):
        ones = np.ones(len(var['spectrum', i].values))
        weights['spectrum', i].values = ones
        weights['spectrum', i].variances = ones

    da = sc.DataArray(data=weights,
                      coords={
                          'spectrum': specs,
                          'ids': ids,
                          'tof': var
                      })
    da.coords['tof'].unit = sc.units.us

    return da













    #         if fields[key]["entry"] is None:
    #             for p in fields[key]["pattern"]:
    #                 if item.endswith(p):
    #                     fields[key]["entry"] = item

    # for key in fields.keys():
    #     fields[key]["data"] = np.array(f[fields[key]["entry"]][...],
    #                                    dtype=fields[key]["dtype"],
    #                                    copy=True)

    # print(data)

    # # # Store the detector size in pixels
    # # nx, ny = np.shape(fields["x"]["data"])
    # # fields["nx"] = {"entry": None, "data": nx}
    # # fields["ny"] = {"entry": None, "data": ny}

    # # if ids and convert_ids and (np.amax(fields["ids"]["data"]) >= nx*ny):
    # #     print("Warning: maximum id exceeds total number of pixels. "
    # #           "Attempting to fix ids...")
    # #     fields["ids"]["data"] = __convert_id(fields["ids"]["data"], nx)

    # if verbose:
    #     for key in fields.keys():
    #         print("Loaded {} from: {}".format(key, fields[key]["entry"]))
    #         print("  - Data size: {} : Min={} , "
    #               "Max={}".format(np.shape(fields[key]["data"]),
    #                               np.amin(fields[key]["data"]),
    #                               np.amax(fields[key]["data"])))







    # print(spec_min, spec_max)

    # print(len(spec_num))
    # spec_num = np.unique(np.concatenate(spec_num).ravel())
    # print(spec_num.shape)

    # nspec = len(spec_num)

    # ids_bins = sc.Variable(['ids'], values=np.arange(spec_min, spec_max + 1))

    

    # spec_map = {val: key for (key, val) in enumerate(spec_num)}

    # ids = sc.Variable(dims=['spectrum'], values=spec_num, dtype=sc.dtype.int32)













# def __convert_id(ids, nx, id_offset=0):
#     """
#     Attempt to convert the event ids if their range exceeds the maximum index
#     allowed by the pixel geometry.
#     """
#     x = np.bitwise_and(ids, 0xFFFF)
#     y = np.right_shift(ids, 16)
#     return id_offset + x + (nx * y)


def load_positions(filename, entry='/'):

    instrument = {}
    pattern = 'transformations/location'

    lx = []
    ly = []
    lz = []

    # nmax = 8
    # i = 0

    with h5py.File(filename, "r", libver='latest', swmr=True) as f:

        contents = []
        f[entry].visit(contents.append)

        # for item in contents:
        #     print(item)
        #     for key in fields.keys():
        #         if fields[key]["entry"] is None:
        #             for p in fields[key]["pattern"]:
        #                 if item.endswith(p):
        #                     fields[key]["entry"] = item

        # for key in fields.keys():
        #     fields[key]["data"] = np.array(f[fields[key]["entry"]][...],
        #                                    dtype=fields[key]["dtype"],
        #                                    copy=True)

        for item in contents:

            print(item)
            if item.endswith(pattern) and (item.count('detector') > 0):
                # i += 1
                root = item.replace(pattern, '')
                pos = f[item][()] * np.array(f[item].attrs['vector'])
                
                xoffset = f[root + 'x_pixel_offset'][()].astype(np.float64) + pos[0]
                yoffset = f[root + 'y_pixel_offset'][()].astype(np.float64) + pos[1]
                zoffset = f[root + 'z_pixel_offset'][()].astype(np.float64) + pos[2]

                # positions = np.array([xoffset, yoffset, zoffset]).T
                # print(positions)
                # break
                # return xoffset, yoffset, zoffset
                lx.append(xoffset)
                ly.append(yoffset)
                lz.append(zoffset)

                # if i > nmax:
                #     return np.concatenate(lx), np.concatenate(ly), np.concatenate(lz)

            # for key, value in f[item].attrs.items():
            #     print(key, value)

            # # print(item)
            # for key in fields.keys():
            #     for p in fields[key]["pattern"]:
            #         if item.endswith(p):
            #             root = item.replace(key, '')
            #             entries[key][root] = np.array(f[item][...],
            #                                dtype=fields[key]["dtype"],
            #                                copy=True)
            #             if key == "event_id":
            #                 spec_min = min(spec_min, entries[key][root].min())
            #                 spec_max = max(spec_max, entries[key][root].max())
            #                 spec_num.append(np.unique(entries[key][root]))
    return np.array([np.concatenate(lx), np.concatenate(ly), np.concatenate(lz)]).T
