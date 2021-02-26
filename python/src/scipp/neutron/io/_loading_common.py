from typing import Union, Any

import numpy as np


def ensure_str(str_or_bytes: Union[str, bytes]) -> str:
    try:
        str_or_bytes = str(str_or_bytes, encoding="utf8")  # type: ignore
    except TypeError:
        pass
    return str_or_bytes


class BadSource(Exception):
    pass


unsigned_to_signed = {
    np.uint32: np.int32,
    np.uint64: np.int64,
}


def ensure_not_unsigned(dataset_type: Any):
    try:
        return unsigned_to_signed[dataset_type]
    except KeyError:
        return dataset_type
