# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from .core import Variable, DataArray, Dataset

try:
    from typing import List, Dict, Tuple, Union
    from distributed.protocol import dask_serialize, dask_deserialize

    @dask_serialize.register(Variable)
    @dask_serialize.register(DataArray)
    @dask_serialize.register(Dataset)
    def serialize(var: Union[Variable, DataArray, Dataset]) -> Tuple[Dict, List[bytes]]:
        from io import BytesIO
        from .io.hdf5 import HDF5IO
        import h5py
        header = {}
        buf = BytesIO()
        with h5py.File(buf, "w") as f:
            HDF5IO.write(f, var)
        frames = [buf.getvalue()]
        return header, frames

    @dask_deserialize.register(Variable)
    @dask_deserialize.register(DataArray)
    @dask_deserialize.register(Dataset)
    def deserialize(header: Dict,
                    frames: List[bytes]) -> Union[Variable, DataArray, Dataset]:
        from io import BytesIO
        from .io.hdf5 import HDF5IO
        import h5py
        return HDF5IO.read(h5py.File(BytesIO(frames[0]), "r"))

    __all__ = ['serialize', 'deserialize']
except ImportError:
    __all__ = []
    pass
