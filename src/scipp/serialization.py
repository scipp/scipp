# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from . import Variable, DataArray
from .core import bins, dtype, array, vectors


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
            elif obj.dtype == dtype.PyObject:
                enc['values'] = None
            elif obj.dtype == dtype.DataArray:
                if obj.ndim == 0:
                    enc['values'] = encode(obj.value)
                else:
                    enc['values'] = list[encode(da for da in obj.values)]
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
        args = {'dims': obj['dims'], 'unit': obj['unit']}
        if obj['dtype'] == str(dtype.datetime64):
            args['values'] = m.decode(obj['values']).view(f"datetime64[{obj['unit']}]")
        elif obj['dtype'] == str(dtype.DataArray):
            args['values'] = obj['values']
        elif obj['dtype'] == str(dtype.string):
            # TODO Does msgpack have problems with arrays of strings?
            args['values'] = obj['values']
        else:
            args['values'] = m.decode(obj['values'])
        if 'variances' in obj:
            args['variances'] = m.decode(obj['variances'])
        if obj['dtype'] == str(dtype.vector_3_float64):
            return vectors(**args)
        else:
            return array(**args)
    if '__scipp.DataArray__' in obj:
        return DataArray(data=obj['data'],
                         coords=obj['coords'],
                         masks=obj['masks'],
                         attrs=obj['attrs'])
    return obj
