# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from typing import Dict, List, Tuple, Union

from .core import DataArray, Dataset, Variable


def serialize(var: Union[Variable, DataArray, Dataset]) -> Tuple[Dict, List[bytes]]:
    """Serialize Scipp object."""
    from io import BytesIO

    import h5py

    from .io.hdf5 import HDF5IO
    header = {}
    buf = BytesIO()
    with h5py.File(buf, "w") as f:
        HDF5IO.write(f, var)
    frames = [buf.getvalue()]
    return header, frames


def deserialize(header: Dict,
                frames: List[bytes]) -> Union[Variable, DataArray, Dataset]:
    """Deserialize Scipp object."""
    from io import BytesIO

    import h5py

    from .io.hdf5 import HDF5IO
    return HDF5IO.read(h5py.File(BytesIO(frames[0]), "r"))


try:
    from distributed.protocol import register_serialization
    register_serialization(Variable, serialize, deserialize)
    register_serialization(DataArray, serialize, deserialize)
    register_serialization(Dataset, serialize, deserialize)
except ImportError:
    pass
__all__ = ['serialize', 'deserialize']
