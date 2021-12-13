# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

from .. import array, Variable, DataArray, DimensionError, UnitError
from ..compat.wrapping import wrap1d

from typing import Callable


@wrap1d(is_partial=True)
def interp1d(da: DataArray, dim: str, **kwargs) -> Callable:
    """Interpolate a 1-D function.

    A data array is used to approximate some function f: y = f(x), where y is given by
    the array values and x is is given by the coordinate for the given dimension. This
    class returns a function whose call method uses interpolation to find the value of
    new points.

    The function is a wrapper for scipy.interpolate.interp1d. The differences are:

    - Instead of x and y, a data array defining these is used as input.
    - Instead of an axis, a dimension label defines the interpolation dimension.
    - The returned function does not just return the values of f(x) but a new
      data array with values defined as f(x) and x as a coordinate for the
      interpolation dimension.
    - The returned function accepts an extra argument ``midpoints``. When setting
      ``midpoints=True`` the interpolation uses the midpoints of the new points
      instead of the points itself. The returned data array is then a histogram, i.e.,
      the new coordinate is a bin-edge coordinate.

    Parameters not described above are forwarded to scipy.interpolate.interp1d. The
    most relevant ones are (see :py:class:`scipy.interpolate.interp1d` for details):

    :param kind: Specifies the kind of interpolation as a string or as an integer
                 specifying the order of the spline interpolator to use. The string
                 has to be one of 'linear', 'nearest', 'nearest-up', 'zero', 'slinear',
                 'quadratic', 'cubic', 'previous', or 'next'. 'zero', 'slinear',
                 'quadratic' and 'cubic' refer to a spline interpolation of zeroth,
                 first, second or third order; 'previous' and 'next' simply return the
                 previous or next value of the point; 'nearest-up' and 'nearest' differ
                 when interpolating half-integers (e.g. 0.5, 1.5) in that 'nearest-up'
                 rounds up and 'nearest' rounds down. Default is 'linear'.
    :param fill_value: Set to 'extrapolate' to allow for extrapolation of points
                       outside the range.

    Examples:

      >>> x = sc.geomspace(dim='x', start=0.1, stop=0.4, num=4, unit='rad')
      >>> da = sc.DataArray(sc.sin(x), coords={'x': x})

      >>> from scipp.interpolate import interp1d
      >>> f = interp1d(da, 'x')

      >>> xnew = sc.linspace(dim='x', start=0.1, stop=0.4, num=5, unit='rad')
      >>> f(xnew)  # use interpolation function returned by `interp1d`
      <scipp.DataArray>
      Dimensions: Sizes[x:5, ]
      Coordinates:
        x                         float64            [rad]  (x)  [0.100000, 0.175000, 0.250000, 0.325000, 0.400000]
      Data:
                                  float64  [dimensionless]  (x)  [0.099833, 0.173987, 0.247384, 0.318433, 0.389418]

      >>> f(xnew, midpoints=True)
      <scipp.DataArray>
      Dimensions: Sizes[x:4, ]
      Coordinates:
        x                         float64            [rad]  (x [bin-edge])  [0.100000, 0.175000, 0.250000, 0.325000, 0.400000]
      Data:
                                  float64  [dimensionless]  (x)  [0.137015, 0.210685, 0.282941, 0.353926]
    """  # noqa #501
    import scipy.interpolate as inter
    kwargs['axis'] = da.dims.index(dim)

    def func(xnew: Variable, *, midpoints=False) -> DataArray:
        """Compute interpolation function defined by ``interp1d`` at interpolation points.

        :param xnew: Interpolation points.
        :param midpoints: Interpolate at midpoints of given points. The result will be
                          a histogram. Default is ``False``.
        :return: Interpolated data array with new coord given by interpolation points
                 and data given by interpolation function evaluated at the
                 interpolation points (or evaluated at the midpoints of the given
                 points).
        """
        if xnew.unit != da.coords[dim].unit:
            raise UnitError(
                f"Unit of interpolation points '{xnew.unit}' does not match unit "
                f"'{da.coords[dim].unit}' of points defining the interpolation "
                "function.")
        if xnew.dim != dim:
            raise DimensionError(
                f"Dimension of interpolation points '{xnew.dim}' does not match "
                f"interpolation dimension '{dim}'")
        f = inter.interp1d(x=da.coords[dim].values, y=da.values, **kwargs)
        x_ = 0.5 * (xnew[dim, 1:] + xnew[dim, :-1]) if midpoints else xnew
        ynew = array(dims=da.dims, unit=da.unit, values=f(x_.values))
        return DataArray(data=ynew, coords={dim: xnew})

    return func
