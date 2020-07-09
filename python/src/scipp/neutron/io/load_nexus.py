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


def load_nexus(filename, entry="/",
         verbose=False, convert_ids=False):
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

    fields["event_id"] = {"pattern": ["event_id"],
                       "entry": None,
                       "data": None,
                       "dtype": np.int32}
    # if tofs:
    fields["event_time_offset"] = {"pattern": ["event_time_offset"],
                      "entry": None,
                      "data": None,
                      "dtype": np.float64}

    fields["event_time_zero"] = {"pattern": ["event_time_zero"],
                            "entry": None,
                            "data": None,
                            "dtype": np.float64}

    fields["event_index"] = {"pattern": ["event_index"],
                            "entry": None,
                            "data": None,
                            "dtype": np.float64}

    data = {}

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
                        data[item] = np.array(f[item][...],
                                           dtype=fields[key]["dtype"],
                                           copy=True)


        #         if fields[key]["entry"] is None:
        #             for p in fields[key]["pattern"]:
        #                 if item.endswith(p):
        #                     fields[key]["entry"] = item

        # for key in fields.keys():
        #     fields[key]["data"] = np.array(f[fields[key]["entry"]][...],
        #                                    dtype=fields[key]["dtype"],
        #                                    copy=True)

    print(data)


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

    return data


# def __convert_id(ids, nx, id_offset=0):
#     """
#     Attempt to convert the event ids if their range exceeds the maximum index
#     allowed by the pixel geometry.
#     """
#     x = np.bitwise_and(ids, 0xFFFF)
#     y = np.right_shift(ids, 16)
#     return id_offset + x + (nx * y)
