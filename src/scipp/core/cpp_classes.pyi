from io import BytesIO, StringIO
from os import PathLike
from typing import (
    Any,
    Callable,
    Iterable,
    Iterator,
    Literal,
    Mapping,
    Optional,
    Sequence,
    Union,
    overload,
)

from .._binding import _NoDefault
from ..coords.graph import GraphDict
from ..typing import Dims, VariableLike, VariableLikeType
from ..units import default_unit
from .bins import Bins

try:
    import h5py as h5
except ModuleNotFoundError:
    h5 = Any

__all__ = [
    "BinEdgeError",
    "BinnedDataError",
    "CoordError",
    "Coords",
    "Coords_items_view",
    "Coords_keys_view",
    "Coords_values_view",
    "DType",
    "DTypeError",
    "DataArray",
    "DataArrayError",
    "Dataset",
    "DatasetError",
    "Dataset_items_view",
    "Dataset_keys_view",
    "Dataset_values_view",
    "DefaultUnit",
    "DimensionError",
    "GroupByDataArray",
    "GroupByDataset",
    "Masks",
    "Masks_items_view",
    "Masks_keys_view",
    "Masks_values_view",
    "Slice",
    "Unit",
    "UnitError",
    "Variable",
    "VariableError",
    "VariancesError",
]

class BinEdgeError(RuntimeError): ...
class BinnedDataError(RuntimeError): ...
class CoordError(RuntimeError): ...

class Coords(Mapping[str, Variable]):
    def __contains__(self, arg0: str) -> bool: ...
    def __copy__(self) -> Coords: ...
    def __deepcopy__(self, arg0: dict) -> Coords: ...
    def __delitem__(self, arg0: str) -> None: ...
    def __eq__(self, arg0: object) -> bool:  # type: ignore[override, unused-ignore]
        ...
    def __getitem__(self, arg0: str) -> Variable: ...
    def __iter__(self) -> Iterator[str]: ...
    def __len__(self) -> int: ...
    def __ne__(self, arg0: object) -> bool:  # type: ignore[override, unused-ignore]
        ...
    def __repr__(self) -> str: ...
    def __setitem__(self, arg0: str, arg1: Variable) -> None: ...
    def __str__(self) -> str: ...
    def _ipython_key_completions_(self) -> list: ...
    def _pop(self, k: str) -> Variable: ...
    def clear(self) -> None: ...
    def copy(self, deep: bool = True) -> Coords: ...
    def get(self, key: str, default: Variable | None = None) -> Variable | None: ...
    def is_edges(self, key: str, dim: Optional[str] = None) -> bool: ...
    def items(self) -> Coords_items_view: ...
    def keys(self) -> Coords_keys_view: ...
    def pop(self, key: str, default: Any = _NoDefault) -> Variable | None: ...
    def popitem(self) -> tuple[str, Variable]: ...
    def set_aligned(self, key: str, aligned: bool) -> None: ...
    def update(self, other: Any = None, /, **kwargs) -> None: ...
    def values(self) -> Coords_values_view: ...

class Coords_items_view:
    def __eq__(self, arg0: object) -> bool:  # type: ignore[override, unused-ignore]
        ...
    def __iter__(self) -> Iterator[tuple[str, Variable]]: ...
    def __len__(self) -> int: ...
    def __repr__(self) -> str: ...
    def __str__(self) -> str: ...

class Coords_keys_view:
    def __eq__(self, arg0: object) -> bool:  # type: ignore[override, unused-ignore]
        ...
    def __iter__(self) -> Iterator[str]: ...
    def __len__(self) -> int: ...
    def __repr__(self) -> str: ...
    def __str__(self) -> str: ...

class Coords_values_view:
    def __iter__(self) -> Iterator[Variable]: ...
    def __len__(self) -> int: ...
    def __repr__(self) -> str: ...
    def __str__(self) -> str: ...

class DType:
    DataArray: DType = ...
    DataArrayView: DType = ...
    Dataset: DType = ...
    DatasetView: DType = ...
    PyObject: DType = ...
    Variable: DType = ...
    VariableView: DType = ...

    def __eq__(self, arg0: object) -> bool:  # type: ignore[override, unused-ignore]
        ...
    def __init__(self, arg0: Any) -> None: ...
    def __repr__(self) -> str: ...
    def __str__(self) -> str: ...

    affine_transform3: DType = ...
    bool: DType = ...
    datetime64: DType = ...
    float32: DType = ...
    float64: DType = ...
    int32: DType = ...
    int64: DType = ...
    linear_transform3: DType = ...
    rotation3: DType = ...
    string: DType = ...
    translation3: DType = ...
    vector3: DType = ...

class DTypeError(TypeError): ...

class DataArray:
    def __abs__(self) -> DataArray: ...
    @overload
    def __add__(self, arg0: Dataset) -> Dataset: ...
    @overload
    def __add__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __add__(self, arg0: Variable) -> DataArray: ...
    @overload
    def __add__(self, arg0: float) -> DataArray: ...
    @overload
    def __and__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __and__(self, arg0: Variable) -> DataArray: ...
    def __bool__(self) -> None: ...
    def __copy__(self) -> DataArray: ...
    def __deepcopy__(self, arg0: dict) -> DataArray: ...
    def __eq__(self, arg0: object) -> DataArray:  # type: ignore[override, unused-ignore]
        ...
    @overload
    def __floordiv__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __floordiv__(self, arg0: Variable) -> DataArray: ...
    @overload
    def __floordiv__(self, arg0: float) -> DataArray: ...
    @overload
    def __ge__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __ge__(self, arg0: Variable) -> DataArray: ...
    @overload
    def __getitem__(self, arg0: int) -> DataArray: ...
    @overload
    def __getitem__(self, arg0: slice) -> DataArray: ...
    @overload
    def __getitem__(self, arg0: Variable) -> DataArray: ...
    @overload
    def __getitem__(self, arg0: tuple[str, Variable]) -> DataArray: ...
    @overload
    def __getitem__(self, arg0: tuple[str, int]) -> DataArray: ...
    @overload
    def __getitem__(self, arg0: tuple[str, slice]) -> DataArray: ...
    @overload
    def __getitem__(self, arg0: ellipsis) -> DataArray: ...
    @overload
    def __getitem__(self, arg0: list[int]) -> DataArray: ...
    @overload
    def __getitem__(self, arg0: tuple[str, list[int]]) -> DataArray: ...
    @overload
    def __gt__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __gt__(self, arg0: Variable) -> DataArray: ...
    @overload
    def __iadd__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __iadd__(self, arg0: Variable) -> DataArray: ...
    @overload
    def __iadd__(self, arg0: float) -> DataArray: ...
    @overload
    def __iand__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __iand__(self, arg0: Variable) -> DataArray: ...
    @overload
    def __ifloordiv__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __ifloordiv__(self, arg0: Variable) -> DataArray: ...
    @overload
    def __ifloordiv__(self, arg0: float) -> DataArray: ...
    @overload
    def __imod__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __imod__(self, arg0: Variable) -> DataArray: ...
    @overload
    def __imod__(self, arg0: float) -> DataArray: ...
    @overload
    def __imul__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __imul__(self, arg0: Variable) -> DataArray: ...
    @overload
    def __imul__(self, arg0: float) -> DataArray: ...
    def __init__(
        self,
        data: Variable,
        coords: Union[Mapping[str, Variable], Iterable[tuple[str, Variable]]] = {},
        masks: Union[Mapping[str, Variable], Iterable[tuple[str, Variable]]] = {},
        attrs: Union[Mapping[str, Variable], Iterable[tuple[str, Variable]]] = {},
        name: str = '',
    ) -> None: ...
    def __invert__(self) -> DataArray: ...
    @overload
    def __ior__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __ior__(self, arg0: Variable) -> DataArray: ...
    @overload
    def __isub__(self, arg0: DataArray) -> Any: ...
    @overload
    def __isub__(self, arg0: Variable) -> Any: ...
    @overload
    def __isub__(self, arg0: float) -> Any: ...
    @overload
    def __itruediv__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __itruediv__(self, arg0: Variable) -> DataArray: ...
    @overload
    def __itruediv__(self, arg0: float) -> DataArray: ...
    @overload
    def __ixor__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __ixor__(self, arg0: Variable) -> DataArray: ...
    @overload
    def __le__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __le__(self, arg0: Variable) -> DataArray: ...
    def __len__(self) -> int: ...
    @overload
    def __lt__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __lt__(self, arg0: Variable) -> DataArray: ...
    @overload
    def __mod__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __mod__(self, arg0: Variable) -> DataArray: ...
    @overload
    def __mod__(self, arg0: float) -> DataArray: ...
    @overload
    def __mul__(self, arg0: Dataset) -> Dataset: ...
    @overload
    def __mul__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __mul__(self, arg0: Variable) -> DataArray: ...
    @overload
    def __mul__(self, arg0: float) -> DataArray: ...
    def __ne__(self, arg0: object) -> DataArray:  # type: ignore[override, unused-ignore]
        ...
    def __neg__(self) -> DataArray: ...
    @overload
    def __or__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __or__(self, arg0: Variable) -> DataArray: ...
    @overload
    def __pow__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __pow__(self, arg0: Variable) -> DataArray: ...
    @overload
    def __pow__(self, arg0: float) -> DataArray: ...
    def __radd__(self, arg0: float) -> DataArray: ...
    def __repr__(self) -> str: ...
    def __rfloordiv__(self, arg0: float) -> DataArray: ...
    def __rmod__(self, arg0: float) -> DataArray: ...
    def __rmul__(self, arg0: float) -> DataArray: ...
    def __rpow__(self, arg0: float) -> DataArray: ...
    def __rsub__(self, arg0: float) -> DataArray: ...
    def __rtruediv__(self, arg0: float) -> DataArray: ...
    @overload
    def __setitem__(self, arg0: tuple[str, Variable], arg1: Variable) -> None: ...
    @overload
    def __setitem__(self, arg0: tuple[str, Variable], arg1: DataArray) -> None: ...
    @overload
    def __setitem__(self, arg0: int, arg1: Any) -> None: ...
    @overload
    def __setitem__(self, arg0: slice, arg1: Any) -> None: ...
    @overload
    def __setitem__(self, arg0: tuple[str, int], arg1: Any) -> None: ...
    @overload
    def __setitem__(self, arg0: tuple[str, slice], arg1: Any) -> None: ...
    @overload
    def __setitem__(self, arg0: ellipsis, arg1: Any) -> None: ...
    def __sizeof__(self) -> int: ...
    @overload
    def __sub__(self, arg0: Dataset) -> Dataset: ...
    @overload
    def __sub__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __sub__(self, arg0: Variable) -> DataArray: ...
    @overload
    def __sub__(self, arg0: float) -> DataArray: ...
    @overload
    def __truediv__(self, arg0: Dataset) -> Dataset: ...
    @overload
    def __truediv__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __truediv__(self, arg0: Variable) -> DataArray: ...
    @overload
    def __truediv__(self, arg0: float) -> DataArray: ...
    @overload
    def __xor__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __xor__(self, arg0: Variable) -> DataArray: ...
    def _ipython_key_completions_(self) -> list: ...
    def _rename_dims(self, arg0: dict[str, str]) -> DataArray: ...
    def _repr_html_(self) -> str: ...
    def all(self, dim: str | None = None) -> DataArray: ...
    def any(self, dim: str | None = None) -> DataArray: ...
    def assign_attrs(
        self, attrs: dict[str, Variable] | None = None, /, **attrs_kwargs: Variable
    ) -> DataArray: ...
    def assign_coords(
        self, coords: dict[str, Variable] | None = None, /, **coords_kwargs: Variable
    ) -> _T: ...
    def assign_masks(
        self, masks: dict[str, Variable] | None = None, /, **masks_kwargs: Variable
    ) -> DataArray: ...
    def astype(self, type: Any, *, copy: bool = True) -> DataArray: ...
    @property
    def attrs(self): ...
    def bin(self, arg_dict=None, /, **kwargs): ...
    @property
    def bins(self) -> Bins | None: ...
    @bins.setter
    def bins(self, bins: Bins) -> None: ...
    def broadcast(
        self,
        dims: list[str] | tuple[str, ...] | None = None,
        shape: Sequence[int] | None = None,
        sizes: dict[str, int] | None = None,
    ) -> DataArray: ...
    def ceil(self, *, out: VariableLike | None = None) -> VariableLike: ...
    @property
    def coords(self) -> Coords: ...
    def copy(self, deep: bool = True) -> DataArray: ...
    @property
    def data(self) -> Variable: ...
    @data.setter
    def data(self, arg1: Variable) -> None: ...
    @property
    def deprecated_attrs(self) -> Coords: ...
    @overload
    def deprecated_drop_attrs(self, arg0: str) -> DataArray: ...
    @overload
    def deprecated_drop_attrs(self, arg0: list[str]) -> DataArray: ...
    @property
    def deprecated_meta(self) -> Coords: ...
    @property
    def dim(self) -> str: ...
    @property
    def dims(self) -> tuple[str, ...]: ...
    def drop_attrs(self, *args, **kwargs): ...
    @overload
    def drop_coords(self, arg0: str) -> DataArray: ...
    @overload
    def drop_coords(self, arg0: list[str]) -> DataArray: ...
    @overload
    def drop_masks(self, arg0: str) -> DataArray: ...
    @overload
    def drop_masks(self, arg0: list[str]) -> DataArray: ...
    @property
    def dtype(self) -> DType: ...
    def flatten(
        self, dims: list[str] | tuple[str, ...] | None = None, to: str | None = None
    ) -> DataArray: ...
    def floor(self, *, out: VariableLike | None = None) -> VariableLike: ...
    def fold(
        self,
        dim: str,
        sizes: dict[str, int] | None = None,
        dims: list[str] | tuple[str, ...] | None = None,
        shape: Sequence[int] | None = None,
    ) -> DataArray: ...
    def group(self, /, *args: str | Variable): ...
    def groupby(
        self, /, group: Variable | str, *, bins: Variable | None = None
    ) -> GroupByDataArray | GroupByDataset: ...
    def hist(self, arg_dict=None, /, **kwargs): ...
    @property
    def masks(self) -> Masks: ...
    def max(self, dim: str | None = None) -> DataArray: ...
    def mean(self, dim: str | None = None) -> DataArray: ...
    def median(self, dim: Dims = None) -> DataArray: ...
    @property
    def meta(self): ...
    def min(self, dim: str | None = None) -> DataArray: ...
    @property
    def name(self) -> str: ...
    @name.setter
    def name(self, arg1: str) -> None: ...
    def nanhist(self, arg_dict=None, /, **kwargs): ...
    def nanmax(self, dim: str | None = None) -> DataArray: ...
    def nanmean(self, dim: str | None = None) -> DataArray: ...
    def nanmedian(self, dim: Dims = None) -> DataArray: ...
    def nanmin(self, dim: str | None = None) -> DataArray: ...
    def nanstd(self, dim: Dims = None, *, ddof: int) -> DataArray: ...
    def nansum(self, dim: str | None = None) -> DataArray: ...
    def nanvar(self, dim: Dims = None, *, ddof: int) -> DataArray: ...
    @property
    def ndim(self) -> int: ...
    def plot(*args: Any, **kwargs: Any) -> None: ...
    def rebin(self, arg_dict=None, /, **kwargs): ...
    def rename(
        self, dims_dict: dict[str, str] | None = None, /, **names: str
    ) -> DataArray: ...
    def rename_dims(
        self, dims_dict: dict[str, str] | None = None, /, **names: str
    ) -> DataArray: ...
    def round(self, *, out: VariableLike | None = None) -> VariableLike: ...
    def save_hdf5(
        self, filename: str | PathLike[str] | StringIO | BytesIO | h5.Group
    ) -> None: ...
    @property
    def shape(self) -> tuple[int, ...]: ...
    @property
    def size(self) -> int: ...
    @property
    def sizes(self) -> dict[str, int]: ...
    def squeeze(
        self, dim: str | list[str] | tuple[str, ...] | None = None
    ) -> DataArray: ...
    def std(self, dim: Dims = None, *, ddof: int) -> DataArray: ...
    def sum(self, dim: Dims = None) -> DataArray: ...
    def to(
        self,
        *,
        unit: Unit | str | None = None,
        dtype: Any | None = None,
        copy: bool = True,
    ) -> DataArray: ...
    def transform_coords(
        self,
        targets: str | Iterable[str] | None = None,
        /,
        graph: GraphDict | None = None,
        *,
        rename_dims: bool = True,
        keep_aliases: bool = True,
        keep_intermediate: bool = True,
        keep_inputs: bool = True,
        quiet: bool = False,
        **kwargs: Callable,
    ) -> DataArray | Dataset: ...
    def transpose(
        self, dims: list[str] | tuple[str, ...] | None = None
    ) -> DataArray: ...
    def underlying_size(self) -> int: ...
    @property
    def unit(self) -> Optional[Unit]: ...
    @unit.setter
    def unit(self, arg1: Union[str, Unit, None, DefaultUnit]) -> None: ...
    @property
    def value(self) -> Any: ...
    @value.setter
    def value(self, arg1: Any) -> None: ...
    @property
    def values(self) -> Any: ...
    @values.setter
    def values(self, arg1: Any) -> None: ...
    def var(self, dim: Dims = None, *, ddof: int) -> DataArray: ...
    @property
    def variance(self) -> Any: ...
    @variance.setter
    def variance(self, arg1: Any) -> None: ...
    @property
    def variances(self) -> Any: ...
    @variances.setter
    def variances(self, arg1: Any) -> None: ...

class DataArrayError(RuntimeError): ...

class Dataset(Mapping[str, DataArray]):
    def __abs__(self) -> Dataset: ...
    @overload
    def __add__(self, arg0: Dataset) -> Dataset: ...
    @overload
    def __add__(self, arg0: DataArray) -> Dataset: ...
    @overload
    def __add__(self, arg0: Variable) -> Dataset: ...
    def __bool__(self) -> None: ...
    def __contains__(self, arg0: str) -> bool: ...
    def __copy__(self) -> Dataset: ...
    def __deepcopy__(self, arg0: dict) -> Dataset: ...
    def __delitem__(self, arg0: str) -> None: ...
    @overload
    def __getitem__(self, arg0: str) -> DataArray: ...
    @overload
    def __getitem__(self, arg0: int) -> Dataset: ...
    @overload
    def __getitem__(self, arg0: slice) -> Dataset: ...
    @overload
    def __getitem__(self, arg0: Variable) -> Dataset: ...
    @overload
    def __getitem__(self, arg0: tuple[str, Variable]) -> Dataset: ...
    @overload
    def __getitem__(self, arg0: tuple[str, int]) -> Dataset: ...
    @overload
    def __getitem__(self, arg0: tuple[str, slice]) -> Dataset: ...
    @overload
    def __getitem__(self, arg0: ellipsis) -> Dataset: ...
    @overload
    def __getitem__(self, arg0: list[int]) -> Dataset: ...
    @overload
    def __getitem__(self, arg0: tuple[str, list[int]]) -> Dataset: ...
    @overload
    def __iadd__(self, arg0: Dataset) -> Dataset: ...
    @overload
    def __iadd__(self, arg0: DataArray) -> Dataset: ...
    @overload
    def __iadd__(self, arg0: Variable) -> Dataset: ...
    @overload
    def __iadd__(self, arg0: float) -> Dataset: ...
    @overload
    def __imul__(self, arg0: Dataset) -> Dataset: ...
    @overload
    def __imul__(self, arg0: DataArray) -> Dataset: ...
    @overload
    def __imul__(self, arg0: Variable) -> Dataset: ...
    @overload
    def __imul__(self, arg0: float) -> Dataset: ...
    def __init__(
        self,
        data: Union[
            Mapping[str, Union[Variable, DataArray]],
            Iterable[tuple[str, Union[Variable, DataArray]]],
        ] = {},
        coords: Union[Mapping[str, Variable], Iterable[tuple[str, Variable]]] = {},
    ) -> None: ...
    @overload
    def __isub__(self, arg0: Dataset) -> Any: ...
    @overload
    def __isub__(self, arg0: DataArray) -> Any: ...
    @overload
    def __isub__(self, arg0: Variable) -> Any: ...
    @overload
    def __isub__(self, arg0: float) -> Any: ...
    def __iter__(self) -> Iterator[str]: ...
    @overload
    def __itruediv__(self, arg0: Dataset) -> Dataset: ...
    @overload
    def __itruediv__(self, arg0: DataArray) -> Dataset: ...
    @overload
    def __itruediv__(self, arg0: Variable) -> Dataset: ...
    @overload
    def __itruediv__(self, arg0: float) -> Dataset: ...
    def __len__(self) -> int: ...
    @overload
    def __mul__(self, arg0: Dataset) -> Dataset: ...
    @overload
    def __mul__(self, arg0: DataArray) -> Dataset: ...
    @overload
    def __mul__(self, arg0: Variable) -> Dataset: ...
    def __repr__(self) -> str: ...
    @overload
    def __setitem__(self, arg0: str, arg1: Variable) -> None: ...
    @overload
    def __setitem__(self, arg0: str, arg1: DataArray) -> None: ...
    @overload
    def __setitem__(self, arg0: tuple[str, Variable], arg1: Dataset) -> None: ...
    @overload
    def __setitem__(self, arg0: int, arg1: Any) -> None: ...
    @overload
    def __setitem__(self, arg0: slice, arg1: Any) -> None: ...
    @overload
    def __setitem__(self, arg0: tuple[str, int], arg1: Any) -> None: ...
    @overload
    def __setitem__(self, arg0: tuple[str, slice], arg1: Any) -> None: ...
    @overload
    def __setitem__(self, arg0: ellipsis, arg1: Any) -> None: ...
    def __sizeof__(self) -> int: ...
    @overload
    def __sub__(self, arg0: Dataset) -> Dataset: ...
    @overload
    def __sub__(self, arg0: DataArray) -> Dataset: ...
    @overload
    def __sub__(self, arg0: Variable) -> Dataset: ...
    @overload
    def __truediv__(self, arg0: Dataset) -> Dataset: ...
    @overload
    def __truediv__(self, arg0: DataArray) -> Dataset: ...
    @overload
    def __truediv__(self, arg0: Variable) -> Dataset: ...
    def _ipython_key_completions_(self) -> list: ...
    def _pop(self, k: str) -> DataArray: ...
    def _rename_dims(self, arg0: dict[str, str]) -> Dataset: ...
    def _repr_html_(self) -> str: ...
    def all(self, dim: str | None = None) -> Dataset: ...
    def any(self, dim: str | None = None) -> Dataset: ...
    def assign_coords(
        self, coords: dict[str, Variable] | None = None, /, **coords_kwargs: Variable
    ) -> _T: ...
    @property
    def bins(self) -> Bins | None: ...
    @bins.setter
    def bins(self, bins: Bins) -> None: ...
    def clear(self) -> None: ...
    @property
    def coords(self) -> Coords: ...
    def copy(self, deep: bool = True) -> Dataset: ...
    @property
    def dim(self) -> str: ...
    @property
    def dims(self) -> tuple[str, ...]: ...
    @overload
    def drop_coords(self, arg0: str) -> Dataset: ...
    @overload
    def drop_coords(self, arg0: list[str]) -> Dataset: ...
    def get(self, key: str, default: DataArray | None = None) -> DataArray | None: ...
    def groupby(
        self, /, group: Variable | str, *, bins: Variable | None = None
    ) -> GroupByDataArray | GroupByDataset: ...
    def hist(self, arg_dict=None, /, **kwargs): ...
    def items(self) -> Dataset_items_view: ...
    def keys(self) -> Dataset_keys_view: ...
    def max(self, dim: str | None = None) -> Dataset: ...
    def mean(self, dim: str | None = None) -> Dataset: ...
    def median(self, dim: Dims = None) -> Dataset: ...
    @property
    def meta(self) -> Coords: ...
    def min(self, dim: str | None = None) -> Dataset: ...
    def nanmax(self, dim: str | None = None) -> Dataset: ...
    def nanmean(self, dim: str | None = None) -> Dataset: ...
    def nanmedian(self, dim: Dims = None) -> Dataset: ...
    def nanmin(self, dim: str | None = None) -> Dataset: ...
    def nanstd(self, dim: Dims = None, *, ddof: int) -> Dataset: ...
    def nansum(self, dim: str | None = None) -> Dataset: ...
    def nanvar(self, dim: Dims = None, *, ddof: int) -> Dataset: ...
    @property
    def ndim(self) -> int: ...
    def plot(*args: Any, **kwargs: Any) -> None: ...
    def pop(self, key: str, default: Any = _NoDefault) -> DataArray | None: ...
    def rebin(self, arg_dict=None, /, **kwargs): ...
    def rename(
        self, dims_dict: dict[str, str] | None = None, /, **names: str
    ) -> Dataset: ...
    def rename_dims(
        self, dims_dict: dict[str, str] | None = None, /, **names: str
    ) -> Dataset: ...
    def save_hdf5(
        self, filename: str | PathLike[str] | StringIO | BytesIO | h5.Group
    ) -> None: ...
    @property
    def shape(self) -> tuple[int, ...]: ...
    @property
    def sizes(self) -> dict[str, int]: ...
    def squeeze(
        self, dim: str | list[str] | tuple[str, ...] | None = None
    ) -> Dataset: ...
    def std(self, dim: Dims = None, *, ddof: int) -> Dataset: ...
    def sum(self, dim: Dims = None) -> Dataset: ...
    def transform_coords(
        self,
        targets: str | Iterable[str] | None = None,
        /,
        graph: GraphDict | None = None,
        *,
        rename_dims: bool = True,
        keep_aliases: bool = True,
        keep_intermediate: bool = True,
        keep_inputs: bool = True,
        quiet: bool = False,
        **kwargs: Callable,
    ) -> DataArray | Dataset: ...
    def underlying_size(self) -> int: ...
    def update(self, other: Any = None, /, **kwargs) -> None: ...
    def values(self) -> Dataset_values_view: ...
    def var(self, dim: Dims = None, *, ddof: int) -> Dataset: ...

class DatasetError(RuntimeError): ...

class Dataset_items_view:
    def __eq__(self, arg0: object) -> bool:  # type: ignore[override, unused-ignore]
        ...
    def __iter__(self) -> Iterator[tuple[str, DataArray]]: ...
    def __len__(self) -> int: ...
    def __repr__(self) -> str: ...
    def __str__(self) -> str: ...

class Dataset_keys_view:
    def __eq__(self, arg0: object) -> bool:  # type: ignore[override, unused-ignore]
        ...
    def __iter__(self) -> Iterator[str]: ...
    def __len__(self) -> int: ...
    def __repr__(self) -> str: ...
    def __str__(self) -> str: ...

class Dataset_values_view:
    def __iter__(self) -> Iterator[DataArray]: ...
    def __len__(self) -> int: ...
    def __repr__(self) -> str: ...
    def __str__(self) -> str: ...

class DefaultUnit:
    def __repr__(self) -> str: ...

class DimensionError(RuntimeError): ...

class GroupByDataArray:
    def all(self, dim: str) -> DataArray: ...
    def any(self, dim: str) -> DataArray: ...
    def concat(self, dim: str) -> DataArray: ...
    def max(self, dim: str) -> DataArray: ...
    def mean(self, dim: str) -> DataArray: ...
    def min(self, dim: str) -> DataArray: ...
    def nanmax(self, dim: str) -> DataArray: ...
    def nanmin(self, dim: str) -> DataArray: ...
    def nansum(self, dim: str) -> DataArray: ...
    def sum(self, dim: str) -> DataArray: ...

class GroupByDataset:
    def all(self, dim: str) -> Dataset: ...
    def any(self, dim: str) -> Dataset: ...
    def concat(self, dim: str) -> Dataset: ...
    def max(self, dim: str) -> Dataset: ...
    def mean(self, dim: str) -> Dataset: ...
    def min(self, dim: str) -> Dataset: ...
    def nanmax(self, dim: str) -> Dataset: ...
    def nanmin(self, dim: str) -> Dataset: ...
    def nansum(self, dim: str) -> Dataset: ...
    def sum(self, dim: str) -> Dataset: ...

class Masks(Mapping[str, Variable]):
    def __contains__(self, arg0: str) -> bool: ...
    def __copy__(self) -> Masks: ...
    def __deepcopy__(self, arg0: dict) -> Masks: ...
    def __delitem__(self, arg0: str) -> None: ...
    def __eq__(self, arg0: object) -> bool:  # type: ignore[override, unused-ignore]
        ...
    def __getitem__(self, arg0: str) -> Variable: ...
    def __iter__(self) -> Iterator[str]: ...
    def __len__(self) -> int: ...
    def __ne__(self, arg0: object) -> bool:  # type: ignore[override, unused-ignore]
        ...
    def __repr__(self) -> str: ...
    def __setitem__(self, arg0: str, arg1: Variable) -> None: ...
    def __str__(self) -> str: ...
    def _ipython_key_completions_(self) -> list: ...
    def _pop(self, k: str) -> Variable: ...
    def clear(self) -> None: ...
    def copy(self, deep: bool = True) -> Masks: ...
    def get(self, key: str, default: Variable | None = None) -> Variable | None: ...
    def is_edges(self, key: str, dim: Optional[str] = None) -> bool: ...
    def items(self) -> Masks_items_view: ...
    def keys(self) -> Masks_keys_view: ...
    def pop(self, key: str, default: Any = _NoDefault) -> Variable | None: ...
    def popitem(self) -> tuple[str, Variable]: ...
    def update(self, other: Any = None, /, **kwargs) -> None: ...
    def values(self) -> Masks_values_view: ...

class Masks_items_view:
    def __eq__(self, arg0: object) -> bool:  # type: ignore[override, unused-ignore]
        ...
    def __iter__(self) -> Iterator[tuple[str, Variable]]: ...
    def __len__(self) -> int: ...
    def __repr__(self) -> str: ...
    def __str__(self) -> str: ...

class Masks_keys_view:
    def __eq__(self, arg0: object) -> bool:  # type: ignore[override, unused-ignore]
        ...
    def __iter__(self) -> Iterator[str]: ...
    def __len__(self) -> int: ...
    def __repr__(self) -> str: ...
    def __str__(self) -> str: ...

class Masks_values_view:
    def __iter__(self) -> Iterator[Variable]: ...
    def __len__(self) -> int: ...
    def __repr__(self) -> str: ...
    def __str__(self) -> str: ...

class Slice: ...

class Unit:
    def __abs__(self) -> Unit: ...
    def __add__(self, arg0: Unit) -> Unit: ...
    def __eq__(self, arg0: object) -> bool:  # type: ignore[override, unused-ignore]
        ...
    def __hash__(self) -> int: ...
    def __init__(self, arg0: str) -> None: ...
    def __mul__(self, arg0: Unit) -> Unit: ...
    def __ne__(self, arg0: object) -> bool:  # type: ignore[override, unused-ignore]
        ...
    def __pow__(self, arg0: int) -> Unit: ...
    def __repr__(self) -> str: ...
    def __rmul__(self, value): ...
    def __rtruediv__(self, value): ...
    def __str__(self) -> str: ...
    def __sub__(self, arg0: Unit) -> Unit: ...
    def __truediv__(self, arg0: Unit) -> Unit: ...
    def _repr_html_(self) -> str: ...
    def _repr_pretty_(self, arg0: Any, arg1: bool) -> None: ...
    def from_dict(self) -> Unit: ...
    @property
    def name(self) -> str: ...
    def to_dict(self) -> dict: ...

class UnitError(RuntimeError): ...

class Variable:
    def __abs__(self) -> Variable: ...
    @overload
    def __add__(self, arg0: Variable) -> Variable: ...
    @overload
    def __add__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __add__(self, arg0: float) -> Variable: ...
    def __and__(self, arg0: Variable) -> Variable: ...
    def __bool__(self) -> bool: ...
    def __copy__(self) -> Variable: ...
    def __deepcopy__(self, arg0: dict) -> Variable: ...
    def __eq__(self, arg0: object) -> Variable:  # type: ignore[override, unused-ignore]
        ...
    def __float__(self) -> float: ...
    @overload
    def __floordiv__(self, arg0: Variable) -> Variable: ...
    @overload
    def __floordiv__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __floordiv__(self, arg0: float) -> Variable: ...
    def __format__(self, format_spec: str) -> str: ...
    @overload
    def __ge__(self, arg0: Variable) -> Variable: ...
    @overload
    def __ge__(self, arg0: float) -> Variable: ...
    @overload
    def __getitem__(self, arg0: int) -> Variable: ...
    @overload
    def __getitem__(self, arg0: slice) -> Variable: ...
    @overload
    def __getitem__(self, arg0: Variable) -> Variable: ...
    @overload
    def __getitem__(self, arg0: tuple[str, int]) -> Variable: ...
    @overload
    def __getitem__(self, arg0: tuple[str, slice]) -> Variable: ...
    @overload
    def __getitem__(self, arg0: ellipsis) -> Variable: ...
    @overload
    def __getitem__(self, arg0: list[int]) -> Variable: ...
    @overload
    def __getitem__(self, arg0: tuple[str, list[int]]) -> Variable: ...
    @overload
    def __gt__(self, arg0: Variable) -> Variable: ...
    @overload
    def __gt__(self, arg0: float) -> Variable: ...
    @overload
    def __iadd__(self, arg0: Variable) -> Variable: ...
    @overload
    def __iadd__(self, arg0: float) -> Variable: ...
    def __iand__(self, arg0: Variable) -> Variable: ...
    @overload
    def __ifloordiv__(self, arg0: Variable) -> Variable: ...
    @overload
    def __ifloordiv__(self, arg0: float) -> Variable: ...
    @overload
    def __imod__(self, arg0: Variable) -> Variable: ...
    @overload
    def __imod__(self, arg0: float) -> Variable: ...
    @overload
    def __imul__(self, arg0: Variable) -> Variable: ...
    @overload
    def __imul__(self, arg0: float) -> Variable: ...
    def __init__(
        self,
        *,
        dims: Any,
        values: Any = None,
        variances: Any = None,
        unit: Union[str, Unit, None, DefaultUnit] = default_unit,
        dtype: Any = None,
        aligned: bool = True,
    ) -> None: ...
    def __int__(self) -> int: ...
    def __invert__(self) -> Variable: ...
    def __ior__(self, arg0: Variable) -> Variable: ...
    @overload
    def __ipow__(self, arg0: Variable) -> Variable: ...
    @overload
    def __ipow__(self, arg0: float) -> Variable: ...
    @overload
    def __isub__(self, arg0: Variable) -> Any: ...
    @overload
    def __isub__(self, arg0: float) -> Any: ...
    @overload
    def __itruediv__(self, arg0: Variable) -> Variable: ...
    @overload
    def __itruediv__(self, arg0: float) -> Variable: ...
    def __ixor__(self, arg0: Variable) -> Variable: ...
    @overload
    def __le__(self, arg0: Variable) -> Variable: ...
    @overload
    def __le__(self, arg0: float) -> Variable: ...
    def __len__(self) -> int: ...
    @overload
    def __lt__(self, arg0: Variable) -> Variable: ...
    @overload
    def __lt__(self, arg0: float) -> Variable: ...
    @overload
    def __mod__(self, arg0: Variable) -> Variable: ...
    @overload
    def __mod__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __mod__(self, arg0: float) -> Variable: ...
    @overload
    def __mul__(self, arg0: Variable) -> Variable: ...
    @overload
    def __mul__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __mul__(self, arg0: float) -> Variable: ...
    def __ne__(self, arg0: object) -> Variable:  # type: ignore[override, unused-ignore]
        ...
    def __neg__(self) -> Variable: ...
    def __or__(self, arg0: Variable) -> Variable: ...
    @overload
    def __pow__(self, arg0: Variable) -> Variable: ...
    @overload
    def __pow__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __pow__(self, arg0: float) -> Variable: ...
    def __radd__(self, arg0: float) -> Variable: ...
    def __repr__(self) -> str: ...
    def __rfloordiv__(self, arg0: float) -> Variable: ...
    def __rmod__(self, arg0: float) -> Variable: ...
    def __rmul__(self, arg0: float) -> Variable: ...
    def __rpow__(self, arg0: float) -> Variable: ...
    def __rsub__(self, arg0: float) -> Variable: ...
    def __rtruediv__(self, arg0: float) -> Variable: ...
    @overload
    def __setitem__(self, arg0: int, arg1: Any) -> None: ...
    @overload
    def __setitem__(self, arg0: slice, arg1: Any) -> None: ...
    @overload
    def __setitem__(self, arg0: tuple[str, int], arg1: Any) -> None: ...
    @overload
    def __setitem__(self, arg0: tuple[str, slice], arg1: Any) -> None: ...
    @overload
    def __setitem__(self, arg0: ellipsis, arg1: Any) -> None: ...
    def __sizeof__(self) -> int: ...
    @overload
    def __sub__(self, arg0: Variable) -> Variable: ...
    @overload
    def __sub__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __sub__(self, arg0: float) -> Variable: ...
    @overload
    def __truediv__(self, arg0: Variable) -> Variable: ...
    @overload
    def __truediv__(self, arg0: DataArray) -> DataArray: ...
    @overload
    def __truediv__(self, arg0: float) -> Variable: ...
    def __xor__(self, arg0: Variable) -> Variable: ...
    def _ipython_key_completions_(self) -> list: ...
    def _rename_dims(self, arg0: dict[str, str]) -> Variable: ...
    def _repr_html_(self) -> str: ...
    @property
    def aligned(self) -> bool: ...
    def all(self, dim: str | None = None) -> Variable: ...
    def any(self, dim: str | None = None) -> Variable: ...
    def astype(self, type: Any, *, copy: bool = True) -> Variable: ...
    def bin(self, arg_dict=None, /, **kwargs): ...
    @property
    def bins(self) -> Bins | None: ...
    @bins.setter
    def bins(self, bins: Bins) -> None: ...
    def broadcast(
        self,
        dims: list[str] | tuple[str, ...] | None = None,
        shape: Sequence[int] | None = None,
        sizes: dict[str, int] | None = None,
    ) -> Variable: ...
    def ceil(self, *, out: VariableLike | None = None) -> VariableLike: ...
    def copy(self, deep: bool = True) -> Variable: ...
    def cumsum(
        self,
        dim: str | None = None,
        mode: Literal['exclusive', 'inclusive'] = 'inclusive',
    ) -> Variable: ...
    @property
    def dim(self) -> str: ...
    @property
    def dims(self) -> tuple[str, ...]: ...
    @property
    def dtype(self) -> DType: ...
    @property
    def fields(self) -> Any: ...
    def flatten(
        self, dims: list[str] | tuple[str, ...] | None = None, to: str | None = None
    ) -> Variable: ...
    def floor(self, *, out: VariableLike | None = None) -> VariableLike: ...
    def fold(
        self,
        dim: str,
        sizes: dict[str, int] | None = None,
        dims: list[str] | tuple[str, ...] | None = None,
        shape: Sequence[int] | None = None,
    ) -> Variable: ...
    def hist(self, arg_dict=None, /, **kwargs): ...
    def max(self, dim: str | None = None) -> Variable: ...
    def mean(self, dim: str | None = None) -> Variable: ...
    def median(self, dim: Dims = None) -> Variable: ...
    def min(self, dim: str | None = None) -> Variable: ...
    def nanhist(self, arg_dict=None, /, **kwargs): ...
    def nanmax(self, dim: str | None = None) -> Variable: ...
    def nanmean(self, dim: str | None = None) -> Variable: ...
    def nanmedian(self, dim: Dims = None) -> Variable: ...
    def nanmin(self, dim: str | None = None) -> Variable: ...
    def nanstd(self, dim: Dims = None, *, ddof: int) -> Variable: ...
    def nansum(self, dim: str | None = None) -> Variable: ...
    def nanvar(self, dim: Dims = None, *, ddof: int) -> Variable: ...
    @property
    def ndim(self) -> int: ...
    def plot(*args: Any, **kwargs: Any) -> None: ...
    def rename(
        self, dims_dict: dict[str, str] | None = None, /, **names: str
    ) -> Variable: ...
    def rename_dims(
        self, dims_dict: dict[str, str] | None = None, /, **names: str
    ) -> Variable: ...
    def round(self, *, out: VariableLike | None = None) -> VariableLike: ...
    def save_hdf5(
        self, filename: str | PathLike[str] | StringIO | BytesIO | h5.Group
    ) -> None: ...
    @property
    def shape(self) -> tuple[int, ...]: ...
    @property
    def size(self) -> int: ...
    @property
    def sizes(self) -> dict[str, int]: ...
    def squeeze(
        self, dim: str | list[str] | tuple[str, ...] | None = None
    ) -> Variable: ...
    def std(self, dim: Dims = None, *, ddof: int) -> Variable: ...
    def sum(self, dim: Dims = None) -> Variable: ...
    def to(
        self,
        *,
        unit: Unit | str | None = None,
        dtype: Any | None = None,
        copy: bool = True,
    ) -> Variable: ...
    def transpose(
        self, dims: list[str] | tuple[str, ...] | None = None
    ) -> Variable: ...
    def underlying_size(self) -> int: ...
    @property
    def unit(self) -> Optional[Unit]: ...
    @unit.setter
    def unit(self, arg1: Union[str, Unit, None, DefaultUnit]) -> None: ...
    @property
    def value(self) -> Any: ...
    @value.setter
    def value(self, arg1: Any) -> None: ...
    @property
    def values(self) -> Any: ...
    @values.setter
    def values(self, arg1: Any) -> None: ...
    def var(self, dim: Dims = None, *, ddof: int) -> Variable: ...
    @property
    def variance(self) -> Any: ...
    @variance.setter
    def variance(self, arg1: Any) -> None: ...
    @property
    def variances(self) -> Any: ...
    @variances.setter
    def variances(self, arg1: Any) -> None: ...

class VariableError(RuntimeError): ...
class VariancesError(RuntimeError): ...
