# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

from typing import Any

from .core import DataArray, Dataset, Variable


def serialize(obj: object) -> tuple[dict[str, Any], list[bytes]]:
    """Serialize Scipp object."""
    from io import BytesIO

    from .io.hdf5 import save_hdf5

    header: dict[str, Any] = {}
    buf = BytesIO()
    save_hdf5(obj, buf)
    frames = [buf.getvalue()]
    return header, frames


def deserialize(header: dict[str, Any], frames: list[bytes]) -> object:
    """Deserialize Scipp object."""
    from io import BytesIO

    from .io.hdf5 import load_hdf5

    return load_hdf5(BytesIO(frames[0]))


try:
    from distributed.protocol import register_serialization

    register_serialization(Variable, serialize, deserialize)
    register_serialization(DataArray, serialize, deserialize)
    register_serialization(Dataset, serialize, deserialize)
except ImportError:
    pass
__all__ = ['deserialize', 'serialize']
