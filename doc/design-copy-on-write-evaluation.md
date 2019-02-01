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

Since there typically is only a single (or a few) instrument and log, they can easily be wrapped in a `cow_ptr` or a `std::shared_ptr` in case memory/copy overhead becomes too significant.
This does not require any design changes, we would simply change the item type of the respective variables from `Dataset` to `cow_ptr<Dataset>`.
The burden of handling COW thus falls to the user of the instrument/logs and is not imposed on the design as a whole.
For practical purposed access to the instrument/logs should then probably be handled via proxy objects, which is anyway the intended mode of operation for a series of other cases.

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
Typically we store at least data and uncertainties, the "worst-case" overhead from non sharing the coordinate is thus:

- 50% assuming there is a large number of datasets that could share the coordinate.
- 20% if there are just two sharing datasets.
- If sharing is not set up, e.g., if the datasets were created independently there is no overhead.

With the support of dataset for multiple data-variables in the same dataset the number of datasets in a given reduction workflow would typically also be lower than what we are used to from the number of workspaces in Mantid.

#### 1.3 Data variables

Data variables change more frequently than coordinates.
However, in particular with datasets containing multiple variables it is likely that not all of them will change.
Data variables typically take up more memory than coordinates, since they are almost always multi-dimensional or hold event-data.

This case is hardest to evaluate since the exact amount of sharing depends a lot on how a workflow is written.
It is easy to imagine a workflow written in a way that becomes really inefficient without COW.
However, it is certainly possible to write workflows without making too many copies, operating mostly in-place.

- The prime example for this is maybe Mantid's `EventWorkspace`.
  In contrast to histogrammed data, `EventWorkspace` does not use COW.
  Nevertheless many efficient workflows have been written in Mantid, utilizing in-place modifications wherever possible.

The take-away here is that we have to focus on in-place operation when performance is critical.


### 2 Compute overhead

#### 2.1 Coordinate comparison in operations

Operations between datasets compare coordinate-variables to ensure that they match.
With COW, the comparison is essentially for free, provided that sharing is set up.
If there is no sharing, the cost is the same without COW.

If there is sharing, the worst case is again a multi-dimensional coordinate.
Consider an in-place binary operation such as `+=` for data with uncertainty.
This is bound by memory bandwidth so the cost is:

- With COW (and sharing): 2+2 reads + 2 writes.
- Without COW: 3+3 reads + 2 writes.

Assuming that reads have the same cost as writes, the worst-case overhead is thus 33%.

#### 2.2 Copy overheads

Operations that copy datasets or create new ones must also copy the coordinates.
Assuming we can share only coordinates but not data, the worst case is again a multi-dimensional coordinate.
Consider `+`, i.e., a non-in-place binary operation.
It is not obvious what the cost of memory allocation and initialization is compared to the actual copy operation.

- If we assume the cost is dominated by the allocation/initialization we obtain an overhead of 50%.
  In practice there should also be contributions from the actual copy operation and the addition.
- There are opportunities for optimization, such as object pools which can nearly eliminate the allocation/initialization cost (assuming the pool is not at capacity), and streaming-stores which eliminate the read of the output buffer.
  The overhead would thus only be a single write (for the coordinate) --- in addition to the reads for coordinate comparison.
  We obtain for a (hypothetical) optimized implementation:

  - With COW (and sharing): 2+2 reads + 2 writes.
  - Without COW: 3+3 reads + 3 writes.

  The overhead is thus 50% (*including* the overhead from coordinate comparison) for this worst-case.

As for the memory overhead, the recommendation to use in-place modification holds also here and would completely eliminate the overhead.


### 3 Complexity and other aspects

In addition to memory and compute overheads there is a series of other aspects to consider.
The brevity of the listed items is not an indicator of their importance, in many cases they may weigh *more* than what was described in the previous sections.

1. Without COW the implementation of the `Dataset` library is significantly simplified.

1. Without COW the design of the public-facing API is much simpler.
   COW would lock us into something that exposes a fair bit more of the underlying implementation than we would like.

1. Without COW user (developer) training is simpler, reducing the need for documentation and the teaching effort.

1. Implementation of multi-threaded code using `Dataset` has a fair number of pitfalls if COW is present.
   Without COW it becomes pretty much straightforward.

1. The "worst" that can happen in a design without COW is some waste of memory.
   With COW, developers have to be aware of invalidation of iterators and invalidation of views such as slice-views.
   The risk of subtle bugs is thus high.

1. With COW, invalidation of iterators and views and the ensuing bugs will lead to instabilities.
   In most cases an invalid iterator will temporarily still point to valid memory, but this may change randomly in multi-threaded environments and can lead to hard-to-reproduce crashes.

1. To actually get any benefit out of COW, developers need to be aware of the concept and take care not to break sharing.


### 4 Conclusion

Considered in isolation, the worst-case overheads for memory and computation are not insignificant.
In a realistic workflow the relative importance of these overheads is likely to reduce drastically.

- Not all operations/data in a workflow falls into the worst case.
- In practice sharing may happen less often than we would like to.

We can thus conjecture that the overall effect might be less than 10%, or approach zero in many cases.

In comparison to the various aspects listed in Sec. 3) we may thus argue that COW is not beneficial or even counter-productive (note that we still have the freedom to use COW for scalar data, without having it as part of the library design).
Freeing up developer time (by not looking for subtle bugs, or having to think hard about COW) is likely to have a larger impact, e.g., by having time to optimize other aspects, or to work on science-related code.
