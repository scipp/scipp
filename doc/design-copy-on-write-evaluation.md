## Copy-on-write

This extension to the `Dataset` design presents an evaluation of the copy-on-write mechanism (COW), which is part of the prototype.
See also the [section on copy-on-write](design.md#copy-on-write-mechanism) in the main design document.

## Current prototype implementation

The discussion in this document is for COW as compared to what is implemented in the prototype:
Every variable holds its multi-dimensional array in a single COW pointer (`cow_ptr`), i.e., either the whole array is copied, or none.
This is in contrast to the more fine-grained COW mechanism used in Mantid for histograms.
It therefore does not suffer from the overhead of potentially millions of COW pointers.

### 1 Memory overhead

A main justification for COW is reducing memory overhead.
For our application there are roughly 3 relevant cases.

#### 1.1 Scalar data (instrument and logs/metadata)

While both instrument and logs/metadata themselves contain non-scalar data, the current suggestion is to store them as a single dataset as part of a (typically) 0-dimensional variable owned by the parent dataset.

Both instrument and metadata can be relatively large.
For example, the instrument can consume several hundred of `Byte` per pixel, which translates to hundreds of `MByte` for large instruments, or even exceeding a `GByte`.

Since there typically is only a single (or a few) instrument or log it can easily be wrapped in a `cow_ptr` or a `std::shared_ptr` in case memory/copy overhead becomes too significant.
This does not require any design changes, we would simply change the item type of the respective variables from `Dataset` to `cow_ptr<Dataset>`.
The burden of handling COW thus falls to the user of the instrument/logs and is not imposed on the design as a whole.
For practical purposed access to the instrument/logs should then probably be handled via proxy objects (which is anyway the intended mode of operation for a series of other cases).

#### 1.2 Coordinates

Coordinates change relatively infrequently compared to data variables.
In case more than one dataset is present, sharing can thus save memory.

There are 2 relevant sub-cases.

##### 1.2.1 One-dimensional coordinates

One-dimensional coordinates are by far the most common.
Our largest dimension is typically `Dim::Position` (the spectrum), which can reach up to 10 million.
Coordinate entries are typically `4 Byte` or `8 Byte` in size, but could also be a position (`24 Byte`).
Nevertheless, if any two-dimensional data variable, or event-data is present, this overhead should be negligible, since, e.g., with only 100 bins the overhead is at the 1%-level.

##### 1.2.2 Multi-dimensional coordinates

After unit-conversion we typically end up with non-aligned bins.
The TOF-derived coordinate thus depends on the same dimensions as the data.
Typically we store data and uncertainties, the overhead from non sharing the coordinate is thus 50% (assuming there is a number datasets that could share the coordinate --- if there are just two the overhead is 20%, if sharing is not set up, e.g., if the datasets were created independently there is no overhead.

With the support of dataset for multiple data-variables in the same dataset the number of datasets in a given reduction workflow would typically also be lower than what we are used to from the number of workspaces in Mantid.



memory overhead
compute overhead -> coord comp
allocation overhead
library implementation effort
teaching effort
API simplicity
risk for bugs
multi-threading
