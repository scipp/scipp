# API Documentation Examples Checklist

**Issue**: [#3811](https://github.com/scipp/scipp/issues/3811) - Add more examples in docs
**Goal**: Add examples to API documentation following NumPy's style - showing how functions should be called and what inputs/outputs look like.

**Status**: Last updated 2026-01-14

---

## Overview

This checklist tracks the work to add docstring examples throughout the Scipp Python API. The API is organized into:
- **Core data structures**: Variable, DataArray, Dataset, DataGroup
- **Functional API**: ~245 free functions across multiple modules
- **Auxiliary classes**: Bins, Coords, Masks, Lookup, GroupBy
- **Submodules**: spatial, io, compat, scipy wrappers, etc.

**Current coverage**: Significant progress made - see details below.

---

## Phase 1: Core Data Structures (Foundational)

These are the primary APIs that every user interacts with. C++ classes with examples added to constructors via C++ bindings.

### Variable Class (~15 examples needed)

**Location**: C++ bindings in `lib/python/variable_init.cpp`

#### Constructor
- [x] Basic constructor with examples pointing to `sc.array()` and `sc.scalar()`

#### Properties (High Priority) ✅ COMPLETE
- [x] `.dims` - Dimension labels of the variable
- [x] `.shape` - Shape tuple
- [x] `.sizes` - Dict of dimension:size mappings
- [x] `.ndim` - Number of dimensions
- [x] `.dtype` - Data type
- [x] `.unit` - Physical unit
- [x] `.values` - Access underlying NumPy array
- [x] `.variances` - Access variance array (uncertainty propagation)
- [x] `.value` / `.variance` - Single value/variance for 0-D variables
- [x] `.size` - Number of elements

#### Basic Operations (High Priority)
- [ ] Indexing with integers: `var[0]`, `var[-1]`
- [ ] Indexing with dimension labels: `var['x', 0]`, `var['x', 0:5]`
- [ ] Slicing: `var['x', 0:10:2]`
- [ ] `.copy()` - Shallow vs deep copy
- [ ] `.to()` - Unit/dtype conversion
- [ ] `.astype()` - Type conversion

#### Other Important Methods
- [ ] `.rename()` / `.rename_dims()` - Rename dimensions
- [ ] `.transpose()` - Permute dimensions
- [ ] `.squeeze()` - Remove length-1 dimensions

### DataArray Class (~15 examples needed)

**Location**: C++ bindings in `lib/python/dataset.cpp`

#### Construction (Critical)
- [x] Basic constructor: `DataArray(data=..., coords={...})`
- [ ] From Variable with coordinates
- [ ] Adding metadata after creation

#### Properties (High Priority)
- [ ] `.data` - Access/modify data variable
- [ ] `.coords` - Access coordinate dict
- [ ] `.masks` - Access mask dict
- [ ] `.name` - Data array name

#### Coordinate and Mask Operations (High Priority)
- [ ] `.assign_coords()` - Add/update coordinates
- [ ] `.assign_masks()` - Add/update masks
- [ ] `.drop_coords()` - Remove coordinates
- [ ] `.drop_masks()` - Remove masks
- [ ] Working with bin-edge coordinates

#### Indexing and Slicing (Critical)
- [ ] Label-based indexing: `da['x', sc.scalar(5.0, unit='m')]`
- [ ] Integer indexing: `da['x', 0]`
- [ ] Slicing with labels: `da['x', 0.0:10.0]`
- [ ] Boolean masking
- [ ] Ellipsis indexing

#### Other Important Methods
- [ ] `.groupby()` - Split-apply-combine operations
- [ ] `.bin()` - Create binned data
- [ ] `.hist()` - Histogram data
- [ ] `.rebin()` - Rebin histogrammed data

### Dataset Class (~10 examples needed)

**Location**: C++ bindings in `lib/python/dataset.cpp`

#### Construction
- [x] Basic constructor: `Dataset(data={...}, coords={...})`
- [ ] Building incrementally with dict operations

#### Dict-like Interface (High Priority)
- [ ] Accessing items: `ds['item_name']`
- [ ] Setting items: `ds['new_item'] = data_array`
- [ ] `.keys()`, `.values()`, `.items()` iteration
- [ ] `.get()` with default values
- [ ] `.pop()` - Remove and return item

#### Operations
- [ ] Shared coordinates across data items
- [ ] Operations that broadcast across items
- [ ] Indexing/slicing datasets

### Coords & Masks Classes (~5 examples needed)

**Location**: `src/scipp/core/cpp_classes.pyi` (C++ classes)

- [ ] Accessing coords: `da.coords['x']`
- [ ] Setting coords: `da.coords['x'] = ...`
- [ ] Checking for coordinates: `'x' in da.coords`
- [ ] `.is_edges()` - Check if coord is bin edges
- [ ] `.set_aligned()` - Set coordinate alignment flag
- [ ] Similar operations for masks

---

## Phase 2: Binned Data & Essential Workflows

Critical for event data analysis (neutron scattering, particle physics, etc.)

### Bins Class (~20 examples needed)

**Location**: `src/scipp/core/bins.py`

#### Accessing Bins (Critical)
- [ ] Accessing `.bins` property
- [ ] Checking if binned: `.is_binned`
- [ ] Basic binned data concept example

#### Bins Properties (High Priority) ✅ COMPLETE
- [x] `.bins.data` - Access/modify event data
- [x] `.bins.coords` - Access event coordinates
- [x] `.bins.masks` - Access event masks
- [x] `.bins.constituents` - Get underlying structure
- [x] `.bins.unit`, `.bins.dtype` - Metadata access

#### Bins Reduction Operations (High Priority) - Mostly Complete
- [ ] `.bins.sum()` - Sum events in each bin (has docstring, needs example)
- [ ] `.bins.mean()` - Average events in each bin (has docstring, needs example)
- [x] `.bins.nansum()` / `.bins.nanmean()` - Ignoring NaN
- [x] `.bins.max()` / `.bins.min()` - Extrema in bins
- [x] `.bins.nanmax()` / `.bins.nanmin()` - NaN-ignoring extrema
- [x] `.bins.size()` - Count events per bin
- [x] `.bins.all()` / `.bins.any()` - Logical operations

#### Bins Metadata Operations ✅ COMPLETE
- [x] `.bins.assign()` - Assign data to bin contents
- [x] `.bins.assign_coords()` - Add coordinates to events
- [x] `.bins.drop_coords()` - Remove event coordinates
- [x] `.bins.assign_masks()` / `.bins.drop_masks()` - Event masks

#### Advanced Bins Operations
- [ ] `.bins.concat()` - Concatenate bins along dimension
- [ ] `.bins.concatenate()` - Element-wise bin concatenation
- [ ] Slicing bins by event coordinates: `da.bins['time', start:stop]`
- [ ] Scaling events with lookup tables: `da.bins * lookup_table`

### Binning Functions (~5 examples needed)

**Location**: Various, some already have examples

- [ ] `sc.bin()` - Create binned data from coordinates (has 4 examples - check coverage)
- [ ] `sc.bins()` - Create binned data from indices (has docstring - add examples)
- [ ] `sc.bins_like()` - Create bins with fill values (has docstring - add examples)
- [ ] `sc.hist()` - Histogram data (has examples - verify completeness)
- [ ] `sc.group()` - Group by integer labels

### Lookup Class (~3 examples needed)

**Location**: `src/scipp/core/bins.py` - Has 1 example

- [x] Basic `sc.lookup()` usage (has 1 example)
- [ ] Using lookup with bins: `da.bins * lookup_table`
- [ ] Lookup with mode='previous' vs mode='nearest'
- [ ] Handling out-of-range values with fill_value

### GroupBy Operations (~5 examples needed)

**Location**: C++ classes, no examples

- [ ] Basic `.groupby()` usage
- [ ] GroupBy with bins parameter
- [ ] Reduction operations on grouped data: `.mean()`, `.sum()`
- [ ] `.concat()` to undo grouping
- [ ] Working with multi-dimensional groups

---

## Phase 3: Free Functions - Core Operations

### Comparison Operations ✅ COMPLETE

**Location**: `src/scipp/core/comparison.py`

- [x] `sc.less()` - Element-wise less than
- [x] `sc.greater()` - Element-wise greater than
- [x] `sc.less_equal()` / `sc.greater_equal()`
- [x] `sc.equal()` / `sc.not_equal()`
- [x] `sc.isclose()` - Floating-point comparison
- [x] `sc.allclose()` - Array-wise close comparison
- [x] `sc.identical()` - Exact equality including metadata

### Logical Operations ✅ COMPLETE

**Location**: `src/scipp/core/logical.py`

- [x] `sc.logical_and()` - Element-wise AND
- [x] `sc.logical_or()` - Element-wise OR
- [x] `sc.logical_not()` - Element-wise NOT
- [x] `sc.logical_xor()` - Element-wise XOR

### Mathematical Functions (Mostly Complete)

**Location**: `src/scipp/core/math.py`

#### Basic Math (High Priority) ✅
- [x] `sc.abs()` - Absolute value
- [x] `sc.sqrt()` - Square root
- [x] `sc.pow()` - Power function
- [x] `sc.exp()` / `sc.log()` / `sc.log10()`
- [x] `sc.reciprocal()` - 1/x

#### Trigonometric Functions ✅
- [x] `sc.sin()` / `sc.cos()` / `sc.tan()` (in trigonometry.py)
- [ ] `sc.asin()` / `sc.acos()` / `sc.atan()` / `sc.atan2()`
- [x] Unit requirements demonstrated (rad/deg)

#### Hyperbolic Functions
- [ ] `sc.sinh()` / `sc.cosh()` / `sc.tanh()`
- [ ] `sc.asinh()` / `sc.acosh()` / `sc.atanh()`

#### Vector Operations ✅
- [x] `sc.norm()` - Vector magnitude
- [x] `sc.dot()` - Dot product
- [x] `sc.cross()` - Cross product

#### Special Functions ✅
- [x] `sc.erf()` / `sc.erfc()` - Error functions
- [x] `sc.midpoints()` - Bin edge to center conversion
- [x] `sc.nan_to_num()` - Replace NaN/inf values

### Arithmetic Operations (~3 examples needed)

**Location**: `src/scipp/core/arithmetic.py` - Has **3 examples**, verify coverage

- [x] Basic arithmetic examples exist
- [ ] Verify unit arithmetic examples
- [ ] Verify variance propagation examples

### Reduction Operations (~6 examples needed)

**Location**: `src/scipp/core/reduction.py` - Has **6 examples**

- [x] Basic reductions covered
- [ ] Verify multi-dimensional reduction examples
- [ ] Verify ddof parameter examples for std/var
- [ ] Check median examples

### Unary Operations ✅ COMPLETE

**Location**: `src/scipp/core/unary.py`

- [x] `sc.isnan()` - Check for NaN
- [x] `sc.isinf()` / `sc.isposinf()` / `sc.isneginf()`
- [x] `sc.isfinite()` - Check for finite values
- [x] `sc.to_unit()` - Unit conversion

### Shape Operations (Partially Complete)

**Location**: `src/scipp/core/shape.py`

- [x] `sc.broadcast()` - Broadcast to target shape
- [x] `sc.concat()` - Concatenate along dimension
- [x] `sc.fold()` - Reshape by folding dimension
- [x] `sc.flatten()` - Reshape by flattening dimensions
- [ ] `sc.squeeze()` - Remove size-1 dimensions
- [x] `sc.transpose()` - Permute dimensions

### Operations Module ✅ COMPLETE

**Location**: `src/scipp/operations.py`

- [x] `sc.where()` - Conditional selection
- [x] `sc.merge()` - Merge data arrays
- [x] `sc.sort()` - Sort by coordinate
- [x] `sc.values()` / `sc.variances()` / `sc.stddevs()` - Extract arrays
- [x] `sc.islinspace()` - Check for linear spacing
- [x] `sc.issorted()` / `sc.allsorted()` - Check sorting
- [x] `sc.as_const()` - Mark as read-only
- [x] `sc.to()` - Conversion function
- [x] `sc.reduce()` - General reduction

### Variable Construction (~15 examples needed)

**Location**: `src/scipp/core/variable.py` - Has **16 examples**, verify coverage

Most constructors already documented, but verify:
- [x] `sc.scalar()` - Has examples
- [x] `sc.array()` - Has examples
- [x] `sc.zeros()` / `sc.ones()` / `sc.empty()` - Have examples
- [x] `sc.linspace()` / `sc.arange()` / `sc.geomspace()` / `sc.logspace()` - Have examples
- [ ] Verify dimension labels in all examples
- [ ] Verify unit parameter usage
- [ ] Verify dtype parameter usage
- [ ] `sc.vector()` / `sc.vectors()` - 3D vector construction
- [ ] `sc.datetime()` / `sc.datetimes()` / `sc.epoch()` - Datetime handling

---

## Phase 4: Specialized Features & Submodules

### Spatial Module ✅ COMPLETE

**Location**: `src/scipp/spatial/__init__.py`

All functions now have examples:

#### Translation ✅
- [x] `sc.spatial.translation()` - Single translation
- [x] `sc.spatial.translations()` - Array of translations

#### Rotation ✅
- [x] `sc.spatial.rotation()` - Single rotation from quaternion
- [x] `sc.spatial.rotations()` - Array of rotations
- [x] `sc.spatial.rotations_from_rotvecs()` - From rotation vectors
- [x] `sc.spatial.rotation_as_rotvec()` - Convert to rotation vector

#### Scaling ✅
- [x] `sc.spatial.scaling_from_vector()` - Single scaling
- [x] `sc.spatial.scalings_from_vectors()` - Array of scalings

#### Linear Transforms ✅
- [x] `sc.spatial.linear_transform()` - Single 3x3 matrix
- [x] `sc.spatial.linear_transforms()` - Array of matrices

#### Affine Transforms ✅
- [x] `sc.spatial.affine_transform()` - Single affine (3x4) transform
- [x] `sc.spatial.affine_transforms()` - Array of affine transforms

#### Operations ✅
- [x] `sc.spatial.inv()` - Invert transformations

### I/O Operations (~5 examples needed)

**Location**: `src/scipp/io/` - Minimal examples

#### HDF5
- [ ] `sc.io.load_hdf5()` - Load from HDF5
- [ ] `sc.io.save_hdf5()` - Save to HDF5
- [ ] Round-trip example with complex data

#### CSV
- [ ] `sc.io.load_csv()` - Load from CSV (has 1 example - verify)
- [ ] Handling headers and column types

### Compat Module ✅ COMPLETE

**Location**: `src/scipp/compat/`

#### Pandas ✅
- [x] `sc.compat.from_pandas()` - Convert from pandas DataFrame

#### Xarray ✅
- [x] `sc.compat.from_xarray()` - Convert from xarray
- [x] `sc.compat.to_xarray()` - Convert to xarray

### Coordinate Transformation (~3 examples needed)

**Location**: `src/scipp/coords/transform_coords.py` - Has **1 example**

- [ ] Basic `sc.transform_coords()` usage (verify existing example)
- [ ] Complex transformation graphs
- [ ] Using with spatial transformations

### Units Module (~3 examples needed)

**Location**: `src/scipp/units/` - Has **1 example** for UnitAliases

- [x] `sc.units.UnitAliases` context manager (has example)
- [ ] Using predefined units: `sc.units.m`, `sc.units.s`, etc.
- [ ] Unit arithmetic examples

### SciPy Wrappers

**Location**: `src/scipp/scipy/` - Most have 1-2 examples each

#### Review existing examples:
- [x] `integrate` module - Has 2 examples (trapezoid, simpson)
- [x] `signal` module - Has 2 examples
- [x] `interpolate` module - Has 1 example
- [x] `ndimage` module - Has 2 examples
- [x] `optimize` module - Has 1 example

These likely have sufficient coverage but verify quality and completeness.

---

## Guidelines for Writing Examples

Based on the NumPy documentation style:

1. **Show actual usage**: Include imports, construction, and the function call
2. **Show output**: Display the result using repr (not just `print`)
3. **Keep it simple**: Focus on the most common use case first
4. **Add complexity gradually**: Start simple, then show variations
5. **Show dimension labels**: Always use labeled dimensions, not just integer indices
6. **Show units**: Include unit specifications where relevant
7. **Explain the "why"**: Brief comment on when/why to use this function
8. **Show common patterns**: Demonstrate typical workflows

### Example Template:

```python
"""
Short description of what the function does.

Parameters
----------
param1 : type
    Description
...

Returns
-------
: type
    Description

Examples
--------
Basic usage:

  >>> import scipp as sc
  >>> x = sc.array(dims=['x'], values=[1, 2, 3], unit='m')
  >>> result = sc.some_function(x)
  >>> result
  <scipp.Variable> (x: 3)    float64              [m]  [2, 4, 6]

With optional parameters:

  >>> result = sc.some_function(x, param='value')
  >>> result
  <scipp.Variable> (x: 3)    float64              [m]  [1, 4, 9]

See Also
--------
related_function: Related functionality
"""
```

---

## Progress Tracking

**Last Updated**: 2026-01-14

### Summary Statistics
- **Phase 1 (Core Structures)**: 14 / ~45 examples complete (constructors + all properties done)
- **Phase 2 (Binned Data)**: 18 / ~33 examples complete (properties + metadata ops done)
- **Phase 3 (Free Functions)**: ~55 / ~85 examples complete
- **Phase 4 (Specialized)**: ~20 / ~32 examples complete

**Total Progress**: ~107 / ~195 examples complete (~55%)

### Recent Changes (2026-01-14)
- Extended C++ `Docstring` class with `.examples()` method
- Added constructor examples for Variable, DataArray, Dataset
- **Added examples to all C++ properties**: dims, shape, sizes, ndim, dtype, unit, values, variances, value, variance, size
- Completed: comparison.py, logical.py, unary.py, operations.py
- Completed: spatial module (all 12 functions)
- Completed: compat module (pandas, xarray)
- Added many examples to math.py, trigonometry.py, shape.py, bins.py
- **Completed Bins properties**: data, masks, constituents, unit, dtype
- **Completed Bins metadata ops**: assign, assign_coords, drop_coords, assign_masks, drop_masks
- **Added Bins reduction examples**: nansum, nanmean, max, min, nanmax, nanmin, all, any

### Remaining Work
- Phase 1: Class methods (indexing, slicing, copy, to, astype, rename, transpose, squeeze)
- Phase 2: bins.sum/mean examples, GroupBy operations, advanced bins operations
- Phase 3: Hyperbolic functions, inverse trig, some shape operations
- Phase 4: I/O module, coordinate transformations, units module

---

## Notes

- C++ classes (Variable, DataArray, Dataset) now have constructor examples via C++ bindings in `lib/python/`
- Extended the `Docstring` helper class with `.examples()` method for adding examples to generated docstrings
- **C++ property examples added via raw string literals in `bind_data_access.h`** - same pattern can be used for methods
- The scipy wrapper examples are generally good but brief - consider expanding them with more context about when to use each function
