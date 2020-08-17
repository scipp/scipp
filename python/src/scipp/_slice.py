import scipp as sc


def slice(object, coord_name, v_slice=slice(None, None, None)):
    '''
    Value based slicing using coordinate and start, end values along that axis.
    slice treats bins as closed on left and open on right: [left, right)

    Current implementation will only handle 1D coordinates which are monotonically increasing
    :param object: Dataset or DataArray to slice
    :param coord_name: Coordinate to use for slicing. Values taken against this coordinate.
    :param v_slice: Either a slice holding Variables for value(s) slice start, stop (step ignored), or single Variable representing a point value.
    :return: Slice view of object
    '''
    import builtins
    import numpy as np
    coord = object.coords[coord_name]
    if len(coord.shape) > 1:
        raise RuntimeError(
            "multi-dimensional coordinates not supported in slice")
    dim = coord.dims[0]
    ascending = sc.is_sorted(coord, dim, order='ascending')
    descending = sc.is_sorted(coord, dim, order='descending')
    if not (ascending or descending):
        raise RuntimeError(
            "Coordinate must be monotomically increasing or decreasing for value slicing"
        )
    bins = coord.shape[0]
    bin_edges = bins == object.shape[object.dims.index(dim)] + 1
    if isinstance(v_slice, builtins.slice):
        if v_slice.start is None:
            first = 0
        else:
            if bin_edges:
                # include lower bin edge boundary
                if ascending:
                    first = sc.sum(sc.less_equal(coord, v_slice.start),
                                   dim).value - 1
                else:
                    first = sc.sum(sc.greater_equal(coord, v_slice.start),
                                   dim).value - 1
            else:
                # First point >= value for non bin edges
                if ascending:
                    first = bins - sc.sum(
                        sc.greater_equal(coord, v_slice.start), dim).value
                else:
                    first = bins - sc.sum(sc.less_equal(coord, v_slice.start),
                                          dim).value
        if first < 0:
            first = 0

        # last index determination identical for bin-edges and non-bin edges
        if v_slice.stop is None:
            last = bins - 1
        else:
            if ascending:
                last = bins - sc.sum(sc.greater_equal(coord, v_slice.stop),
                                     dim).value
            else:
                last = bins - sc.sum(sc.less_equal(coord, v_slice.stop),
                                     dim).value
            if last > bins:
                last = bins - 1

        return object[dim, first:last]
    else:
        if bin_edges:
            if ascending:
                idx = sc.sum(sc.less_equal(coord, v_slice), dim).value - 1
            else:
                idx = sc.sum(sc.greater_equal(coord, v_slice), dim).value - 1
            if idx < 0:
                raise RuntimeError(
                    f"Point slice {v_slice.value} does not fall within any bin edges along {coord_name}"
                )
        else:
            res = np.where(sc.equal(coord, v_slice).values)
            if len(res[0]) < 1:
                raise RuntimeError(
                    f"Point slice {v_slice.value} does exactly match any point coordinate value along {coord_name}"
                )
            else:
                idx = res[0][0]  # Take first if there are many
        return object[dim, idx]
