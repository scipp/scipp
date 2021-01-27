# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import pytest
import scipp as sc
import numpy as np
from .nexus_helpers import (find_by_nx_class,
                            in_memory_nexus_file_with_event_data)


def test_bins_default_begin_end():
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    var = sc.bins(dim='x', data=data)
    assert var.dims == data.dims
    assert var.shape == data.shape
    for i in range(4):
        assert sc.is_equal(var['x', i].value, data['x', i:i + 1])


def test_bins_default_end():
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    begin = sc.Variable(dims=['y'], values=[1, 3], dtype=sc.dtype.int64)
    var = sc.bins(begin=begin, dim='x', data=data)
    assert var.dims == begin.dims
    assert var.shape == begin.shape
    assert sc.is_equal(var['y', 0].value, data['x', 1:3])
    assert sc.is_equal(var['y', 1].value, data['x', 3:4])


def test_bins_fail_only_end():
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    end = sc.Variable(dims=['y'], values=[1, 3], dtype=sc.dtype.int64)
    with pytest.raises(RuntimeError):
        sc.bins(end=end, dim='x', data=data)


def test_bins():
    data = sc.Variable(dims=['x'], values=[1, 2, 3, 4])
    begin = sc.Variable(dims=['y'], values=[0, 2], dtype=sc.dtype.int64)
    end = sc.Variable(dims=['y'], values=[2, 4], dtype=sc.dtype.int64)
    var = sc.bins(begin=begin, end=end, dim='x', data=data)
    assert var.dims == begin.dims
    assert var.shape == begin.shape
    assert sc.is_equal(var['y', 0].value, data['x', 0:2])
    assert sc.is_equal(var['y', 1].value, data['x', 2:4])


def test_bins_arithmetic():
    var = sc.Variable(dims=['event'], values=[1.0, 2.0, 3.0, 4.0])
    table = sc.DataArray(var, {'x': var})
    binned = sc.bin(table, [sc.Variable(dims=['x'], values=[1.0, 5.0])])
    hist = sc.DataArray(
        data=sc.Variable(dims=['x'], values=[1.0, 2.0]),
        coords={'x': sc.Variable(dims=['x'], values=[1.0, 3.0, 5.0])})
    binned.bins *= sc.lookup(func=hist, dim='x')
    assert sc.is_equal(
        binned.bins.data.data,
        sc.Variable(dims=['event'], values=[1.0, 2.0, 6.0, 8.0]))


def test_load_events_bins():
    with in_memory_nexus_file_with_event_data() as nexus_file:
        event_data_groups = find_by_nx_class("NXevent_data", nexus_file)

        # Load only the first event data group we found
        event_group = event_data_groups[0]
        event_index_np = event_group["event_index"][...]
        event_time_offset = sc.Variable(
            ['event'], values=event_group["event_time_offset"][...])
        event_id = sc.Variable(['event'], values=event_group["event_id"][...])
        event_index = sc.Variable(['pulse'],
                                  values=event_index_np,
                                  dtype=np.int64)
        event_time_zero = sc.Variable(
            ['pulse'], values=event_group["event_time_zero"][...])

    # Calculate the end index for each pulse
    # The end index for a pulse is the start index of the next pulse
    end_indices = event_index_np.astype(np.int64)
    end_indices = np.roll(end_indices, -1)
    end_indices[-1] = event_id.shape[0]
    end_indices = sc.Variable(['pulse'], values=end_indices, dtype=np.int64)

    # Weights are not stored in NeXus, so use 1s
    weights = sc.Variable(['event'],
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
    assert sc.is_equal(events['pulse', 0].value.coords['detector-id'],
                       data.coords['detector-id']['event', 0:3])
