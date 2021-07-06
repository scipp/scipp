# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import pytest
import scipp as sc
import numpy as np
from .nexus_helpers import (find_by_nx_class,
                            in_memory_nexus_file_with_event_data)


def test_dense_data_properties_are_none():
    var = sc.scalar(1)
    assert var.bins is None
    assert var.events is None


def test_bins_default_begin_end():
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    var = sc.bins(dim='x', data=data)
    assert var.dims == data.dims
    assert var.shape == data.shape
    for i in range(4):
        assert sc.identical(var['x', i].value, data['x', i:i + 1])


def test_bins_default_end():
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    begin = sc.Variable(dims=['y'], values=[1, 3], dtype=sc.dtype.int64)
    var = sc.bins(begin=begin, dim='x', data=data)
    assert var.dims == begin.dims
    assert var.shape == begin.shape
    assert sc.identical(var['y', 0].value, data['x', 1:3])
    assert sc.identical(var['y', 1].value, data['x', 3:4])


def test_bins_fail_only_end():
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    end = sc.Variable(dims=['y'], values=[1, 3], dtype=sc.dtype.int64)
    with pytest.raises(RuntimeError):
        sc.bins(end=end, dim='x', data=data)


def test_events_property():
    var = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    data = sc.DataArray(data=var,
                        coords={'coord': var},
                        masks={'mask': var},
                        attrs={'attr': var})
    begin = sc.Variable(dims=['y'], values=[0, 2], dtype=sc.dtype.int64)
    end = sc.Variable(dims=['y'], values=[2, 4], dtype=sc.dtype.int64)
    binned = sc.bins(begin=begin, end=end, dim='x', data=data)
    assert 'coord' in binned.events.coords
    assert 'mask' in binned.events.masks
    assert 'attr' in binned.events.attrs
    del binned.events.coords['coord']
    del binned.events.masks['mask']
    del binned.events.attrs['attr']
    # sc.bins makes a (shallow) copy of `data`
    assert 'coord' in data.coords
    assert 'mask' in data.masks
    assert 'attr' in data.attrs
    # ... but when buffer is accessed we can insert/delete meta data
    assert 'coord' not in binned.events.coords
    assert 'mask' not in binned.events.masks
    assert 'attr' not in binned.events.attrs
    binned.events.coords['coord'] = var
    binned.events.masks['mask'] = var
    binned.events.attrs['attr'] = var
    assert 'coord' in binned.events.coords
    assert 'mask' in binned.events.masks
    assert 'attr' in binned.events.attrs


def test_bins():
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    begin = sc.Variable(dims=['y'], values=[0, 2], dtype=sc.dtype.int64)
    end = sc.Variable(dims=['y'], values=[2, 4], dtype=sc.dtype.int64)
    var = sc.bins(begin=begin, end=end, dim='x', data=data)
    assert var.dims == begin.dims
    assert var.shape == begin.shape
    assert sc.identical(var['y', 0].value, data['x', 0:2])
    assert sc.identical(var['y', 1].value, data['x', 2:4])


def test_bins_view():
    col = sc.Variable(dims=['event'], values=[1, 2, 3, 4])
    table = sc.DataArray(data=col,
                         coords={'time': col * 2.2},
                         attrs={'attr': col * 3.3},
                         masks={'mask': col == col})
    begin = sc.Variable(dims=['y'], values=[0, 2], dtype=sc.dtype.int64)
    end = sc.Variable(dims=['y'], values=[2, 4], dtype=sc.dtype.int64)
    var = sc.bins(begin=begin, end=end, dim='event', data=table)
    assert 'time' in var.bins.coords
    assert 'time' in var.bins.meta
    assert 'attr' in var.bins.meta
    assert 'attr' in var.bins.attrs
    assert 'mask' in var.bins.masks
    with pytest.raises(sc.DTypeError):
        var.bins.coords['time2'] = col  # col is not binned

    def check(a, b):
        # sc.identical does not work for us directly since we have owning and
        # non-owning views which currently never compare equal.
        assert sc.identical(1 * a, 1 * b)

    var.bins.coords['time'] = var.bins.data
    assert sc.identical(var.bins.coords['time'], var.bins.data)
    var.bins.coords['time'] = var.bins.data * 2.0
    check(var.bins.coords['time'], var.bins.data * 2.0)
    var.bins.coords['time2'] = var.bins.data
    assert sc.identical(var.bins.coords['time2'], var.bins.data)
    var.bins.coords['time3'] = var.bins.data * 2.0
    check(var.bins.coords['time3'], var.bins.data * 2.0)
    var.bins.data = var.bins.coords['time']
    assert sc.identical(var.bins.data, var.bins.coords['time'])
    var.bins.data = var.bins.data * 2.0
    del var.bins.coords['time3']
    assert 'time3' not in var.bins.coords


def test_bins_arithmetic():
    var = sc.Variable(dims=['event'], values=[1.0, 2.0, 3.0, 4.0])
    table = sc.DataArray(var, coords={'x': var})
    binned = sc.bin(table, [sc.Variable(dims=['x'], values=[1.0, 5.0])])
    hist = sc.DataArray(
        data=sc.Variable(dims=['x'], values=[1.0, 2.0]),
        coords={'x': sc.Variable(dims=['x'], values=[1.0, 3.0, 5.0])})
    binned.bins *= sc.lookup(func=hist, dim='x')
    assert sc.identical(
        binned.events.data,
        sc.Variable(dims=['event'], values=[1.0, 2.0, 6.0, 8.0]))


def test_load_events_bins():
    with in_memory_nexus_file_with_event_data() as nexus_file:
        event_data_groups = find_by_nx_class("NXevent_data", nexus_file)

        # Load only the first event data group we found
        event_group = event_data_groups[0]
        event_index_np = event_group["event_index"][...]
        event_time_offset = sc.Variable(
            dims=['event'], values=event_group["event_time_offset"][...])
        event_id = sc.Variable(dims=['event'],
                               values=event_group["event_id"][...])
        event_index = sc.Variable(dims=['pulse'],
                                  values=event_index_np,
                                  dtype=np.int64)
        event_time_zero = sc.Variable(
            dims=['pulse'], values=event_group["event_time_zero"][...])

    # Calculate the end index for each pulse
    # The end index for a pulse is the start index of the next pulse
    end_indices = event_index_np.astype(np.int64)
    end_indices = np.roll(end_indices, -1)
    end_indices[-1] = event_id.shape[0]
    end_indices = sc.Variable(dims=['pulse'],
                              values=end_indices,
                              dtype=np.int64)

    # Weights are not stored in NeXus, so use 1s
    weights = sc.Variable(dims=['event'],
                          values=np.ones(event_id.shape),
                          dtype=np.float32)
    data = sc.DataArray(data=weights,
                        coords={
                            'tof': event_time_offset,
                            'detector-id': event_id
                        })
    events = sc.DataArray(data=sc.bins(begin=event_index,
                                       end=end_indices,
                                       dim='event',
                                       data=data),
                          coords={'pulse-time': event_time_zero})

    assert events.dims == event_index.dims
    assert events.shape == event_index.shape
    assert sc.identical(events['pulse', 0].value.coords['detector-id'],
                        data.coords['detector-id']['event', 0:3])


def test_bins_sum_with_masked_buffer():
    N = 5
    values = np.ones(N)
    data = sc.DataArray(
        data=sc.Variable(dims=['position'],
                         unit=sc.units.counts,
                         values=values,
                         variances=values),
        coords={
            'position':
            sc.Variable(dims=['position'],
                        values=['site-{}'.format(i) for i in range(N)]),
            'x':
            sc.Variable(dims=['position'], unit=sc.units.m, values=[0.2] * 5),
            'y':
            sc.Variable(dims=['position'], unit=sc.units.m, values=[0.2] * 5)
        },
        masks={
            'test-mask':
            sc.Variable(dims=['position'],
                        unit=sc.units.one,
                        values=[True, False, True, False, True])
        })
    xbins = sc.Variable(dims=['x'], unit=sc.units.m, values=[0.1, 0.5, 0.9])
    binned = sc.bin(data, [xbins])
    assert binned.bins.sum().values[0] == 2
