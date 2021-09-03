# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

# flake8: noqa
from .._scipp import _debug_
if _debug_:
    import warnings

    def custom_formatwarning(msg, *args, **kwargs):
        return str(msg) + '\n'

    warnings.formatwarning = custom_formatwarning
    warnings.warn(
        'You are running a "Debug" build of scipp. For optimal performance use a "Release" build.'
    )

from .._scipp import __version__
from .._scipp.core import Variable, DataArray, Dataset, GroupByDataArray, \
                         GroupByDataset, Unit
# Import errors
from .._scipp.core import BinEdgeError, BinnedDataError, CoordError, \
                         DataArrayError, DatasetError, DimensionError, \
                         DTypeError, NotFoundError, SizeError, SliceError, \
                         UnitError, VariableError, VariancesError
# Import submodules
from .._scipp.core import units, dtype

from ..html import make_html

setattr(Variable, '_repr_html_', make_html)
setattr(DataArray, '_repr_html_', make_html)
setattr(Dataset, '_repr_html_', make_html)

from ..io.hdf5 import to_hdf5 as _to_hdf5

setattr(Variable, 'to_hdf5', _to_hdf5)
setattr(DataArray, 'to_hdf5', _to_hdf5)
setattr(Dataset, 'to_hdf5', _to_hdf5)

from ..sizes import _make_sizes

setattr(Variable, 'sizes', property(_make_sizes))
setattr(DataArray, 'sizes', property(_make_sizes))
setattr(Dataset, 'sizes', property(_make_sizes))

from .._bins import _bins, _set_bins, _events

setattr(Variable, 'bins', property(_bins, _set_bins))
setattr(DataArray, 'bins', property(_bins, _set_bins))
setattr(Dataset, 'bins', property(_bins, _set_bins))
setattr(Variable, 'events', property(_events))
setattr(DataArray, 'events', property(_events))

from .._structured import _fields

setattr(
    Variable, 'fields',
    property(
        _fields,
        doc=
        """Provides access to fields of structured types such as vectors or matrices."""
    ))

from .._bins import _groupby_bins

setattr(GroupByDataArray, 'bins', property(_groupby_bins))
setattr(GroupByDataset, 'bins', property(_groupby_bins))

from ..plotting import plot

setattr(Variable, 'plot', plot)
setattr(DataArray, 'plot', plot)
setattr(Dataset, 'plot', plot)

# Prevent unwanted conversion to numpy arrays by operations. Properly defining
# __array_ufunc__ should be possible by converting non-scipp arguments to
# variables. The most difficult part is probably mapping the ufunc to scipp
# functions.
for _cls in (Variable, DataArray, Dataset):
    setattr(_cls, '__array_ufunc__', None)
del _cls
