# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from . import Variable, DataArray
from .core import bins, dtype


def _encode_dict(meta):
    return {key: encode(var) for key, var in meta.items()}


def encode(obj):
    import msgpack_numpy as m
    if isinstance(obj, Variable):
        enc = {'__scipp.Variable__': True}
        if obj.bins is None:
            enc.update({
                'dims': obj.dims,
                'unit': str(obj.unit),
                'dtype': str(obj.dtype)
            })
            if obj.dtype == dtype.datetime64:
                enc['values'] = m.encode(obj.values.view('int64'))
            else:
                enc['values'] = m.encode(obj.values)
            if obj.variances is not None:
                enc['variances'] = m.encode(obj.variances)
        else:
            enc['bins'] = _encode_dict(obj.bins.constituents)
        return enc
    elif isinstance(obj, DataArray):
        return {
            '__scipp.DataArray__': True,
            'data': encode(obj.data),
            'coords': _encode_dict(obj.coords),
            'masks': _encode_dict(obj.masks),
            'attrs': _encode_dict(obj.attrs)
        }
    return obj


def decode(obj):
    import msgpack_numpy as m
    if '__scipp.Variable__' in obj:
        if 'bins' in obj:
            return bins(**obj['bins'])
        if obj['dtype'] == str(dtype.datetime64):
            values = m.decode(obj['values']).view(f"datetime64[{obj['unit']}]")
        else:
            values = m.decode(obj['values'])
        variances = m.decode(obj['variances']) if 'variances' in obj else None
        return Variable(dims=obj['dims'],
                        unit=obj['unit'],
                        values=values,
                        variances=variances)
    if '__scipp.DataArray__' in obj:
        return DataArray(data=obj['data'],
                         coords=obj['coords'],
                         masks=obj['masks'],
                         attrs=obj['attrs'])
    return obj
