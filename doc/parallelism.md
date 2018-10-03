# Parallelism in Mantid

## Current implementation

### Overview

In the majority of algorithms a fork-join threading approach based on OpenMP is used.
There are a couple of exceptions:
A couple of algorithms use a thread pool to parallelize work.
For the case of `MatrixWorkspace` there is experimental MPI support for a fair number of algorithms.

### Shortcomings

- Very limited scaling with the number of threads, in particular also when compared to and MPI-based run on the same machine.
- Reading HDF5 files from multiple threads is severely hampered by locks in the HDF5 libraries.
- Supporting multiple threading models as well as MPI is a significant development overhead and a source of complexity.
- The fork-join approach makes bad use of CPU caches by repeatedly streaming through all data instead of applying a chain of operations to a small subset that fits into cache.
- Unexploited opportunities for parallelism in reduction workflows, which typically load multiple files and apply independent processing steps before combining data from files.
- MPI support is only available for non-interactive work, i.e., running a Python script.

## Scope

Any solution for parallelism should to support the following cases:

1. Interactive operation (modification and visualization) for `MatrixWorkspace`.
1. Interactive operation (modification and visualization) for `MDHistoWorkspace` and `MDEventWorkspace`.
1. Live reduction and visualization.
1. Script-based reduction.
1. Embarrassingly parallel batch reduction of many alike files.
1. Working with data exceeding the memory available locally or on a single machine.

## Potential solutions

### Dask

[Dask](https://dask.org/) looks like a very attractive solution, potentially providing a unified model of parallelism covering all our use cases.
Essentially it provides a mostly transparent mechanism to build a tree of computation tasks, which is subsequently executed by workers on multiple threads, on local processes, or on a compute cluster.

- Unified solution that could replace threading as well as MPI.
- Not limited in the same was as the fork-join threading approach since the task graph has independent branches for each chunk of data.
- Chunked operation on data used to process data that exceeds main memory, even if only a single machine is available.
- Can circumvent the scaling issues due to locks in the HDF5 libraries using the multi-process scheduler.
- Lazy computation or computation in the background can provide a snappy user experience.
- Parallel scheduling of the task graph will automatically exploit opportunities for parallelism in reduction workflows based on multiple inputs.
- Can provide an interactive way of working with data residing on multiple compute nodes on a cluster.

We have attempted to investigate the suitability of Dask for our purposed in a little more detail.

- A custom "Dask collection" for `Dataset` has been added.
  This is what is used to transparently build the task graph.
  With the most basic functionality this is quite simple with less then 150 lines of Python for the first prototype, in addition to serialization code in C++ which would have had to be added for other reasons anyway.
- Setup on a cluster *without root access* is extremely simple (based on, e.g., `miniconda`).
  [dask-jobqueue](https://github.com/dask/dask-jobqueue) eliminates the need to write Slurm submit scripts or similar.
- Throughout our simple tests we struggled a fair bit with sometimes cryptic exceptions and error messages, even when the computation is seemingly successful and done.
  We felt that the project was not 100% ready for production yet, which is maybe also indicated by the Dask version number, which has not reached 1.0 yet.
  That being said, once we figure out a proper configuration it would probably be usable, in particular also since the code size of the project is not massive, i.e., fixing our concrete problems could be a doable task.
- The community appears to be centered around only one core developer (employed by Anaconda), but proved to be very responsive after we reported a simple bug.
- The scheduler has an overhead of around `1 ms` per task.
  - If a data structure is made up of many chunks, there is one task per chunk per function call.
  - Dask can optimize the task graph to merge tasks, such a linear chains of function calls which often show up in our typical reduction scripts, thus reducing this overhead.
  - Considering Dask as a replacement for threading, this implies a significant limitation:
    To fully replace threading, we would probably needs at least tens of chunks per `Dataset`, ideally of the order of 100 chunks would be a good target if our goal is to improve upon our current threading model.
    100 chunks implies an overhead of `100 x 1 ms = 0.1 s` per task.
    For some applications this is perfectly fine, however:
    - In a relatively complex reduction workflow with 100 steps this would cause an overhead of `10 s`, unless many of the steps can be merged.
      With 1000 chunks this would be `100 s`.
    - For interactive visualization such as selecting slices in the `SliceViewer` this overhead alone pushes us over the [0.1 s response-time limit](https://www.nngroup.com/articles/response-times-3-important-limits/) for fluid user interaction.
      In a simple test including serialization and deserialization overhead we ended up with close to `0.5 s` (with room for optimization) overall response time.

    Neither of this rules out Dask as a solution.
    However, it seems to indicate that the overhead of using single-threaded tasks may be too large, i.e., our operations should also support threading internally using, e.g., OpenMP.
    Unfortunately this would imply that we lose some of the benefits listed above:
    - We no longer have a unified solution for parallelism.
    - We are back to using the fork-join threading approach, at least partially.
