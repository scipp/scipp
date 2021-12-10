from .. import array, Variable, DataArray, DimensionError, VariancesError

from typing import Callable


def validated_masks(da, dim):
    masks = {}
    for name, mask in da.masks.items():
        if dim in mask.dims:
            raise DimensionError(
                f"Cannot interpolate along '{dim}' since mask '{name}' depends"
                "on this dimension.")
        masks[name] = mask.copy()
    return masks


def interp1d(da: DataArray, *, dim, **kwargs) -> Callable:
    """
    interpolate

    :param rotvec: A variable with vector(s) giving rotation axis and angle. Unit must
                   be rad or deg.
    """
    import scipy.interpolate as inter
    if 'axis' in kwargs:
        raise ValueError("Use the 'dim' keyword argument instead of 'axis'.")
    if da.variances is not None:
        raise VariancesError("Cannot interpolate data with uncertainties. Try "
                             "'interp1d(sc.values(da), ...)' to ignore uncertainties.")
    kwargs['axis'] = da.dims.index(dim)

    coords = {k: v for k, v in da.coords.items() if dim not in v.dims}
    attrs = {k: v for k, v in da.attrs.items() if dim not in v.dims}

    def func(xnew: Variable, midpoints=False) -> DataArray:
        f = inter.interp1d(x=da.coords[dim].values, y=da.values, **kwargs)
        x_ = 0.5 * (xnew[dim, 1:] + xnew[dim, :-1]) if midpoints else xnew
        ynew = array(dims=xnew.dims, unit=da.unit, values=f(x_.values))
        masks = validated_masks(da, dim)  # NOT outside func, need one copy per call
        return DataArray(data=ynew,
                         coords={
                             **coords, dim: xnew
                         },
                         masks=masks,
                         attrs=attrs)

    return func
