import scipp as sc


def slice(array, coord_name, start=None, end=None):
    coord = array.coords[coord_name]
    if len(coord.shape) > 1:
        raise RuntimeError(
            "multi-dimensional coordinates not supported in slice")
    dim = coord.dims[0]
    bins = coord.shape[0]
    # scipp treats bins as closed on left and open on right: [left, right)
    first = sc.sum(sc.less_equal(coord, start), dim).value - 1
    last = bins - sc.sum(sc.greater_equal(coord, end), dim).value
    if first < 0:
        first = 0
    if last > bins:
        last = bins - 1
    return array[dim, first:last]
