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
    print(spec_min, spec_max)

    print(len(spec_num))
    spec_num = np.unique(np.concatenate(spec_num).ravel())
    print(spec_num.shape)

    nspec = len(spec_num)

    spec_map = {val: key for (key, val) in enumerate(spec_num)}

    ids = sc.Variable(dims=['spectrum'], values=spec_num, dtype=sc.dtype.int32)

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

    da = sc.DataArray(data=sc.Variable(['spectrum'], values=np.ones(nspec)),
                      coords={
                          'spectrum': ids,
                          'tof': var
                      })
    da.coords['tof'].unit = sc.units.us

    return da


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
