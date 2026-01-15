# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew


from .._scipp import core as _cpp


def groupby(
    data: _cpp.DataArray | _cpp.Dataset,
    /,
    group: _cpp.Variable | str,
    *,
    bins: _cpp.Variable | None = None,
) -> _cpp.GroupByDataArray | _cpp.GroupByDataset:
    """Group dataset or data array based on values of specified labels.

    This implements the "split-apply-combine" approach for data reduction.
    The returned object can be used to apply reduction operations (mean, sum,
    etc.) to each group.

    Parameters
    ----------
    data:
        Input data to reduce.
    group:
        Name of labels to use for grouping or Variable to use for grouping
    bins:
        Optional bins for grouping label values.

    Returns
    -------
    :
        GroupBy helper object.

    See Also
    --------
    scipp.group:
        Group into bins based on unique values (creates binned data).
    scipp.bin:
        Create binned data by binning along one or more coordinates.

    Examples
    --------
    Group by a string label coordinate and compute the mean:

      >>> import scipp as sc
      >>> data = sc.DataArray(
      ...     sc.array(dims=['x'], values=[1.0, 2.0, 3.0, 4.0, 5.0, 6.0], unit='K'),
      ...     coords={
      ...         'x': sc.arange('x', 6),
      ...         'label': sc.array(dims=['x'], values=['a', 'b', 'a', 'b', 'a', 'b'])
      ...     }
      ... )
      >>> grouped = sc.groupby(data, 'label')
      >>> grouped.mean('x')
      <scipp.DataArray>
      Dimensions: Sizes[label:2, ]
      Coordinates:
      * label                      string        <no unit>  (label)  ["a", "b"]
      Data:
                                  float64              [K]  (label)  [3, 4]

      >>> grouped.sum('x')
      <scipp.DataArray>
      Dimensions: Sizes[label:2, ]
      Coordinates:
      * label                      string        <no unit>  (label)  ["a", "b"]
      Data:
                                  float64              [K]  (label)  [9, 12]

    Group continuous values into bins and compute the sum:

      >>> data = sc.DataArray(
      ...     sc.array(dims=['x'], values=[1.0, 2.0, 3.0, 4.0, 5.0], unit='K'),
      ...     coords={'x': sc.array(dims=['x'], values=[0.1, 0.5, 1.2, 1.8, 2.5], unit='m')}
      ... )
      >>> bins = sc.linspace('x', 0.0, 3.0, num=4, unit='m')
      >>> sc.groupby(data, 'x', bins=bins).sum('x')
      <scipp.DataArray>
      Dimensions: Sizes[x:3, ]
      Coordinates:
      * x                         float64              [m]  (x [bin-edge])  [0, 1, 2, 3]
      Data:
                                  float64              [K]  (x)  [3, 7, 5]

    Coordinates not used for grouping are dropped if they depend on the reduced
    dimension:

      >>> data = sc.DataArray(
      ...     sc.array(dims=['x'], values=[1.0, 2.0, 3.0, 4.0], unit='K'),
      ...     coords={
      ...         'x': sc.arange('x', 4),
      ...         'label': sc.array(dims=['x'], values=['a', 'b', 'a', 'b']),
      ...         'aux': sc.array(dims=['x'], values=[10, 20, 30, 40])
      ...     }
      ... )
      >>> result = sc.groupby(data, 'label').mean('x')
      >>> 'aux' in result.coords  # 'aux' depends on 'x' which is reduced
      False
      >>> 'label' in result.coords  # 'label' becomes a dimension coord
      True
    """  # noqa: E501
    if bins is None:
        return _cpp.groupby(data, group)
    else:
        return _cpp.groupby(data, group, bins)
