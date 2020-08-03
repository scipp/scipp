import scipp as sc


def slice(array, dim, start, end):
    coord = array.coords[dim]
    bins = coord.shape[0]  # TODO check dimensionality
    # scipp treats bins as closed on left and open on right: [left, right)
    first = sc.sum(sc.less_equal(coord, start), dim).value - 1
    last = bins - sc.sum(sc.greater_equal(coord, end), dim).value
    assert first >= 0
    assert last < bins
    if first == last:
        return array[dim, first]
    else:
        return array[dim, first:last]
