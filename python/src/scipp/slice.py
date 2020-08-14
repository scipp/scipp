import scipp as sc


def slice(object, coord_name, start=None, end=None):
    '''
    Value based slicing using coordinate and start, end values along that axis.
    slice treats bins as closed on left and open on right: [left, right)
    :param object: Dataset or DataArray to slice
    :param coord_name: Coordinate to use for slicing. Values taken against this coordinate.
    :param start: start value. If None (default) starts at left edge of coordinate
    :param end: end value. If None (default) ends at right edge of coordinate
    :return: Slice view of object
    '''
    coord = object.coords[coord_name]
    if len(coord.shape) > 1:
        raise RuntimeError(
            "multi-dimensional coordinates not supported in slice")
    dim = coord.dims[0]
    bins = coord.shape[0]
    bin_edges = bins == object.shape[object.dims.index(dim)] + 1
    if bin_edges:
        if start is None:
            first = 0
        else:
            first = sc.sum(sc.less_equal(coord, start), dim).value - 1
            if first < 0:
                first = 0
        if end is None:
            last = bins - 1
        else:
            last = bins - sc.sum(sc.greater_equal(coord, end), dim).value
            if last > bins:
                last = bins - 1
    else:
        if start is None:
            first = 0
        else:
            first = bins - sc.sum(sc.greater_equal(coord, start), dim).value
            if first < 0:
                first = 0
        if end is None:
            last = bins - 1
        else:
            last = bins - sc.sum(sc.greater_equal(coord, end), dim).value
            print(end, last)
            if last > bins:
                last = bins - 1

    return object[dim, first:last]
