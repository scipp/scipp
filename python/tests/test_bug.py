import numpy as np
import pytest
import scipp as sc


def test_slice_dataset_with_data_only():
    d = sc.Dataset()
    d['data'] = sc.Variable(['y'], values=np.arange(10))
    sliced = d['y', :]
    assert d == sliced


def test_slice_dataset_with_coords_only():
    d = sc.Dataset()
    d.coords['y-coord'] = sc.Variable(['y'], values=np.arange(10))
    sliced = d['y', :]
    assert d == sliced


def test_slice_dataset_with_attrs_only():
    d = sc.Dataset()
    d.attrs['y-attr'] = sc.Variable(['y'], values=np.arange(10))
    sliced = d['y', :]
    assert d == sliced


def test_slice_dataset_with_masks_only():
    d = sc.Dataset()
    d.masks['y-mask'] = sc.Variable(['y'], values=np.arange(10))
    sliced = d['y', :]
    assert d == sliced
