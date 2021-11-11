# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from .core import Variable, DataArray, Dataset
from typing import List, Dict, Tuple, Union


def serialize(var: Union[Variable, DataArray, Dataset]) -> Tuple[Dict, List[bytes]]:
    """Serialize scipp object."""
    from io import BytesIO
    from .io.hdf5 import HDF5IO
    import h5py
    header = {}
    buf = BytesIO()
    with h5py.File(buf, "w") as f:
        HDF5IO.write(f, var)
    frames = [buf.getvalue()]
    return header, frames


def deserialize(header: Dict,
                frames: List[bytes]) -> Union[Variable, DataArray, Dataset]:
    """Deserialize scipp object."""
    from io import BytesIO
    from .io.hdf5 import HDF5IO
    import h5py
    return HDF5IO.read(h5py.File(BytesIO(frames[0]), "r"))


try:
    from distributed.protocol import register_serialization
    register_serialization(Variable, serialize, deserialize)
    register_serialization(DataArray, serialize, deserialize)
    register_serialization(Dataset, serialize, deserialize)
except ImportError:
    pass
__all__ = ['serialize', 'deserialize']
