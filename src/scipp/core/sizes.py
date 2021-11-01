def _make_sizes(obj):
    """
    Makes a dictionary of dimensions labels to dimension sizes
    """
    return dict(zip(obj.dims, obj.shape))
