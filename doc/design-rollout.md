# Dataset design --- Implementation

This is the continuation of the [Dataset design document](design.md), describing how it might be implemented.


## Contents

- [Implementation](#implementation)
  - [Implementation path](#implementation-path)
    - [Phase 1](#implementation-phase-1)
    - [Phase 2](#implementation-phase-2)
    - [Phase 2+WB](#implementation-phase-2-wb)
    - [Phase 2+X](#implementation-phase-2-x)
  - [Notes on reuse](#implementation-notes-on-reuse)
  - [Goals and non-goals](#implementation-goals-and-non-goals)
  - [Milestones](#implementation-milestones)
  - [Effort](#implementation-effort)
    - [Phase 1](#implementation-effort-phase-1)
    - [Phase 2](#implementation-effort-phase-2)
    - [Phase 2+WB](#implementation-effort-phase-2-wb)
    - [Phase 2+X](#implementation-effort-phase-2-x)
- [Discussion](#discussion)
  - [Impact](#discussion-impact)
  - [New challenges](#discussion-new-challenges)
  - [Summary](#discussion-summary)


## <a name="implementation"></a>Implementation


### <a name="implementation-path"></a>Implementation path

A high level overview of an implementation path is given in the figure below.
- Time flows roughly from top to bottom.
- The size of blocks is often *not* an indication of the required effort.
  See the section on [Effort](#implementation-effort) for the required resources.
- Outcomes are shown encircled.
- Red lines indicate completion major outcomes/milestones grouped into phases.
  - `Phase 2+WB` (`Workbench` integration) is flexible and relatively disconnected.
    It may be done sooner or later in the second phase, or even after.
- In `Phase 2+X` the precise plan is deliberately unspecified, i.e., we list various options.
  We firmly believe that it is impossible to make a qualified decision on this part before we have at least completed `Phase 1` and parts of `Phase 2`.
  At this point it is entirely unclear how and how quickly `Dataset` would be adopted, so the decision is postponed.
  Note that this is possible because everything in `Phase 1`, `Phase 2`, and even `Phase 2+WB` is independent of the choice for completing (or not completing) the rollout.

A more detailed discussion of the individual tasks and components is given after the diagram.

![figure: implementation flow chart](implementation-flow.png)


#### <a name="implementation-phase-1"></a>Phase 1

- Detailed API design.
- Core implementation.
  - `Dataset`, `Variable`, assortment of views.
  - Units.
  - Various helpers.
  - Basic operations.
- Python exports.
  This is probably relatively simple since almost everything is just a 1:1 export of a corresponding C++ function or method.
  - `numpy` interoperability is almost out of the box.
  - `xarray` interoperability would likely provided via a zero-copy wrapper, with certain exceptions (event data, bin edges converted to bin centers).
    This will also immediately give access to 1D and 2D plotting via `matplotlib`, see [plotting in xarray](http://xarray.pydata.org/en/stable/plotting.html).
    Higher-dimensional visualization in shape of, e.g., a simplified slice viewer is also available via other libraries compatible with `xarray`, e.g., [HoloViews](http://holoviews.org/) or [hvplot](https://hvplot.pyviz.org/user_guide/Gridded_Data.html).
- Converters from `Workspace2D` and `EventWorkspace` will support testing with real data and early adoption.


#### <a name="implementation-phase-2"></a>Phase 2

- More-custom visualization:
  - Support `Dataset` natively in `InstrumentView`.
  - Support in `SliceViewer`.
  - Provide a table viewer.
  - Support 1D and 2D plotting a `Dataset` natively in the `matplotlib` wrappers and widgets that are part of the new `Workbench`.
- Implement realistic workflows.
  This servers several purposes:
  - Drive the implementation of supporting libraries on top of the generic core functionality.
  - Weed out issues with the API, bugs in the implementation, and performance issues.
  - Provide real-life examples that demonstrate the power of `Dataset` in actual workflows.
- Port/adapt the Mantid fitting module (`CuverFitting`).
  - Fitting is an essential part of Mantid and must be supported rather sooner than later.
    The module is currently coupled to the current workspace APIs and needs to be refactored to make it usable without making use of converters.
- Native `Dataset` I/O.
  - Initially this would mainly be for saving processed data (equivalent to `SaveNexusProcessed`).
  - Medium-term we will want to provide native support for directly loading event-NeXus files, instead of using `LoadEventNexus` with a subsequent conversion step.
  - If we support saving `Dataset`, we must at this point consider how to handle recording of history.
    Due to a more Python-based workflow with potentially a lot of use of `numpy` we cannot realistically hope that tracking algorithm execution in the current way is giving sufficient coverage of the actual history of data.
    - Can we somehow save the full Python script?
    - If we run in the GUI, can we just copy the executed Python code into a generic tracking-helper algorithm and add that to the history?
      Maybe this is possible even without the GUI if we somehow hook into the interpreter?


#### <a name="implementation-phase-2-wb"></a>Phase 2+WB

- Implement a thin wrapper of `Dataset`, inheriting `API::Workspace`.
  - Named `DataObjects::DatasetWorkspace`?
  - Might not even forward methods, just provide access to held dataset.
  - The base class provides `AnalysisDataService` (ADS) support, i.e., `DatasetWorkspace` can be shown in the workspace list widget.
  - The base class Provides processing history.
  - Using `DatasetWorkspace` in Python will be as broken as for all other workspaces.
- Wrap functions for working with `Dataset` as new algorithms.
  - Inherit `API::Algorithm` as usual.
  - Will automatically show up in history of `DatasetWorkspace`.
  - To avoid confusion and name clashes all algorithms for `DatasetWorkspace` are prefixed with `dataset.`, i.e., the newly added algorithms are part of a new namespace.
    - Examples: `dataset.Plus`, `dataset.Rebin`.
    - If there are concerns about confusion we can provide the option to hide these algorithms from the algorithm list.
    - This will be no more confusing than the current distinction between "normal" algorithms and "MD" algorithms.
- Provide algorithms for inserting a dataset into and extracting a dataset from the ADS.
  - Can leverage the power of Python working directly with `Dataset`.
  - Insert into the ADS wrapped into `DatasetWorkspace`, passing ownership to the ADS if GUI interaction is required.
  - Extract from ADS, passing ownership back to Python to return to pythonic operation.

It should be emphasised that we do *not* intend to use `DatasetWorkspace` for everything.
This is only meant for GUI integration in the way we are used to it.
In many cases it will be advantageous to use `Dataset` directly due to the higher flexibility.


#### <a name="implementation-phase-2-x"></a>Phase 2+X

After completion of `Phase 2` and `Phase 2+WB` there will be a series of things that is required to increase the adoption of `Dataset`.
An incomplete list of potential requirements is:

- Improved performance to motivate transition.
- Ported or newly implemented technique-specific or instrument-specific algorithms and workflow algorithms.
- Support in more interfaces for visualization.
- Support in more technique specific interfaces.

As discussed earlier this will be done in an agile manner since there are no benefits to defining a precise plan or strategy now.

Note that this list does deliberately not include refactoring all existing workspace types or all existing algorithms.
It is certainly possible that refactoring is what we will want to do, but the connected effort would be massive and the benefits may be limited (on top of the downside of introducing many bugs during refactoring).
That is, we would need very good reasons to do so.
A better option may be to simple let `Dataset` and workspaces coexist with their respective algorithms.
*Note:
It has been criticised that this would mean we have two types of workspaces and algorithms that are not compatible.
However, this is not a valid argument since we already have this situation right now, e.g., with `MDWorkspace` and `MatrixWorkspace` and the respective algorithms such as `PlusMD` and `Plus`.*

##### Random notes on various rollout options

1. In the long run it would be nice if we could use `Dataset` in the GUI without a wrapping `DatasetWorkspace` as decribed in `Phase 2+WB`.
   However, `Dataset` must remain a valid Python object, i.e., the issues with the Python interface for current workspaces must not be reintroduced for `Dataset` at this point.
   The solution for providing workspace-list integration is not clear, but there are probably a couple of options, e.g., based on introspection.
   For example, [Spyder](https://www.spyder-ide.org/) provides a similar variable explorer.

2. It has been suggested to refactor `MatrixWorkspace` to use `Dataset` internally.
   However, we believe that the involved effort and risk of bugs would be large, with no real benefit:
   If we a bound be the `MatrixWorkspace` API there is no advantage to using `Dataset` internally.
   On the contrary:
   `Dataset` works in a different way so we will put even more restrictions on the use of `MatrixWorkspace`, e.g., when implementing multi-threading in algorithms.

3. Another option is to keep `MatrixWorkspace` and its child classes untouched, replacing less-used workspaces by `Dataset`.
   - However, even the "less" used workspaces like `TableWorkspace` show up a lot, that is the connected effort would also be major:
     `TableWorkspace` is used in 128 C++ algorithms, the related `PeaksWorkspace` in 72 C++ algorithms, `MDHistoWorkspace` in 57 C++ algorithms.
   - Unclear benefit since the ubiquitous use of `MatrixWorkspace` (`Workspace2D` and `EventWorkspace`) may turn out very limiting.
   - Is a workspace-by-workspace rollout useful, unless we intend to ultimately keep all those algorithms?
     - `PlusMD` will be gone (or merged into a common `Plus`), why spend time on refactoring it to use a new data container?
       Should the refactoring to use the new workspace type include refactoring related algorithms completely, i.e., they may be renamed, dropped, or have major interface changes?
   - Instead of replacing existing workspaces, what about adding new but required ones, i.e., for imaging, constant-wavelength, etc., which cannot be supported at all or only based on workarounds right now?

4. Yet another option would be to do an "atomic" rollout.
   Instead of attempting to change a whole workspace type at once to `Dataset`, we could replace individual methods, step by step.
   For example, we could start by putting the spectrum numbers into a dataset with `Coord::SpectrumNumber`.
   As a next step, we can move other coordinates, or units.

Overall, it seems we have roughly 4 options for the long-term strategy regarding workspaces and algorithms:

1. Rollout workspace-by-workspace (replace existing workspace type by `Dataset`).
2. Rollout algorithm-by-algorithm (add equivalent algorithm for `Dataset`, deprecate and remove old algorithms once not needed anymore).
3. Rollout method-by-method (replace individual methods in all workspace types by access to equivalent data in a dataset stored as member in workspaces).
4. Coexistence (implement algorithms for `Dataset` as needed, make no attempt to replace existing workspace types or algorithms). 


### <a name="implementation-notes-on-reuse"></a>Notes on reuse

At this point we would like to clarify which existing components and could and should be reused with `Dataset`.
Otherwise the above discussion may give the impression that `Dataset` would amount to a rewrite of Mantid, which it certainly is not.
The following key parts of Mantid will be reusable when working with `Dataset`:

1. Metadata and handling of metadata.
   `API::Run` would simply be stored as a variable in `Dataset`.
   Consequently existing code to handle metadata such as for event filtering would be reused.
   *Note: The structure of `Dataset` is actually very similar to that of `API::Run` and `Kernel::TimeSeriesProperty` so there is the option to eventually replace those two classes to obtain a more uniform interface.
   However, it is *not* suggested to do this initially.*

2. The instrument and related code in `Geometry`.
   Components that have been refactored as part of the Instrument-2.0 effort (`Beamline::DetectorInfo` and `Beamline::ComponentInfo`) are already in the same data layout that is required for `Dataset`.
   The corresponding data will simply be moved over, additional functionality would be part of helper classes, without changes.
   The legacy instrument (including the parameter map, etc.) could simply be stored as a variable in `Dataset`, without change.

3. Most `Load` and `Save` algorithms would be leveraged via converters (the `DataHandling` module amounts to about `146 kLOC` of algorithm code).
   Apart from key algorithms like `LoadEventNexus` there is no intention to reimplement any, unless there is an actual requirement to do so, e.g., for performance reasons.

4. Fitting code (the `CurveFitting` module amounts to about `105 kLOC` of (algorithm) code).
   Some adaption/refactoring will be required to make use of this since it is currently coupled to `MatrixWorkspace` and `TableWorkspace`.

5. The `Workbench`.

6. 1D and 2D plotting functionality.
   Minor adaption and refactoring will be required.

7. The `InstrumentView`.
   Some adaption and refactoring will be required.

8. The `SliceViewer`.
   Some adaption and refactoring will be required.

9. `MDEventWorkspace` and corresponding algorithms.
   The recursively refined data structure is not a good fit for `Dataset` so it will be kept as is.
   Some adaption/refactoring for algorithms transition from `Dataset` to `MDEventWorkspace` would be required.

10. Existing algorithms would not be removed and are still accessible via converters, albeit not recommended for standard operation.

There may be many more smaller components that would be reusable without major change, such as technique-specific or instrument-specific interfaces or workflow scripts.


### <a name="implementation-goals-and-non-goals"></a>Goals and non-goals

##### Goals in `Phase 1` and `Phase 2`

1. Provide a simple, lightweight, functional and well-tested library that can replace and enhance most of the workspaces types existing in Mantid.
   *Note: "Replace" is not meant to imply that the old workspaces would disappear, simply that a workflow could be implemented using `Dataset` in place of those workspace types.*
1. Reach a point where this library (in combination with some supporting libraries) is useful in practice.
   We define useful as:
   - Complete low-level functionality in C++ and Python.
   - `numpy` interoperability.
   - Visualization using a slice viewer.
   - Visualization using a multi-dimensional table viewer.
   - Instrument visualization using the Mantid instrument viewer.
   - Can leverage existing Mantid code such as `DataHandling::LoadEventNexus` by a subsequent conversion into a `Dataset`.
1. Demonstrate usefulness on at least two real examples, ideally bringing them to a production ready stage.
   Candidates are:
   - Histogram-based and event-based workflow, e.g., for `WISH` at ISIS and for `POWGEN` at SNS, demonstrating a performance gain if applicable.
   - Workflow with parameter scan or polarization analysis to demonstrate new capabilities of extra dimensions and/or multiple data variables.
   - Imaging workflows, which would benefit from presenting data as a stack of images rather than a list of histograms, i.e., the dimension order would be the opposite of what we are used to in `API::MatrixWorkspace`.
   - Constant-wavelength (reactor) workflows, which currently use `API::MatrixWorkspace` with length-1 histograms as a workaround.

##### Non-goals in `Phase 1` and `Phase 2`

We have the following non-goals, and this may continue even beyond those implementation phases:

1. It is a non-goal to provide a drop-in replacement for all existing workspaces, i.e., there will by no type compatibility.
1. It is a non-goal to convert all 970 algorithms to use `Dataset` instead of `API::Workspace`.
1. It is a non-goal to provide support for all visualization widgets.

This may seem like a harsh and drastic step, and it would be, if we were to consider only the next couple of years of Mantid's life.
It is justified as follows:
- We focus on ensuring long term maintainability and functionality, as well as other aspects detailed in the [core-team proposal](https://github.com/mantidproject/documents/blob/master/Project-Management/PMB/Mantid%20core%20team%20proposal.docx).
  That is, the focus is on how Mantid would and should look like in 5-10 years from now.
- For a more concrete reasons, we consider the two big refactoring projects the team has undertaken in the past couple of years:
  - The `Histogram` type as part of the `HistogramData` library is a relatively small addition.
    It mostly kept API compatibilty (for `API::MatrixWorkspace`) but nevertheless a relatively time-consuming rollout was necessary.
    In retrorespect the choice to keep the API compatible feels like it led to too many compromises and missed opportunities for solving some underlying problems.
    In particular, with a couple of exceptions, it did not lead to the cleanup in algorithm implementations we had hoped for.
  - The **Instrument-2.0** effort (with the underlying goal to improve performance and make the design more flexible)  has taken several years of effort so far and is not complete.
    In this case the API was not kept compatible and we have not encountered any serious drawbacks so far.
    The main issue here is the big discrepancy in effort between the implementation and rollout, with the latter taking the big majority, as well as the incomplete rollout state.
    Additionally, since the rollout is not complete and the old data structures are still in place we cannot fully reap the benefits of the more flexible design.

  Compared these two cases, a new "workspace" implementation and rollout to all existing algorithms is vastly more effort, probably by an order of magnitude.
  In light of the experience with `Histogram`, where the need to do a rollout severely limited the design as well as the improvements in client code, we also run the risk of severely diminishing our opportunities for actual improvement.
  That is, if were to attempt a full rollout with a mostly compatible API, we could potentially be faced with several decades worth of effort with little tangible benefit.
  This is clearly not an option.


### <a name="implementation-milestones"></a>Milestones

The milestones for implementation are given by the diagram in section [Implementation path](#implementation-path).
We note that user/developer testing when reaching milestone is used to ensure that the project is on track and stays in line with what is actually required.
Driving implementation of supporting libraries by means of implementing actual workflows is another tool to ensure that we end up with a product that matches the actual needs.


### <a name="implementation-effort"></a>Effort

The following estimates for the required effort are as usual not very precise.
Most of it is based on gut feeling from spending approximately 100 working days on the design and prototype.
The prototyped scope is roughly equivalent to what is contained in `Phase 1`.
If we arbitrarily assume that the prototype amounts to 10% of the actual required effort we obtain 1000 days for phase 1, which is in the same ballpark as the sum of the listed subtasks.


#### <a name="implementation-effort-phase-1"></a>Phase 1


| Task | Weeks min | Weeks max | Comment |
| --- | --- | --- | --- |
| detailed design | 4 | 8 | interface details, investigate open design questions, evaluate candidates, motivate choice to get TSC approval |
| `Dataset` | 1 | 2 |
| `Variable` | 1 | 2 |
| views for `Dataset` and `Variable` | 8 | 16 | variety of views, crucial for a lot of basic usage |
| `Dimensions` | 2 | 3 |
| units | 2 | 8 | scope/requirements unclear thus extra effort |
| helpers and other classes | 4 | 8 |
| exception system | 0.5 | 1 |
| operations: essentials | 4 | 8 | arithmetics and shape operations |
| performance optimization | 1 | 2 |
| Python exports | 4 | 8 | includes `numpy` interoperability |
| doc: internal | 1 | 2 |
| doc: C++ API | 0.5 | 1 |
| doc: Python API | 0.5 | 1 |
| doc: Usage examples | 2 | 3 |
| build system and packaging | 2 | 4 |
| ramp-up time for every additional developer | 2 | 3 |
| converters: `API::MatrixWorkspace` and `DataObjects::MDHistoWorkspace` to `Dataset` | 2 | 4 |
| code reviews by multiple parties | 4 | 8 |


#### <a name="implementation-effort-phase-2"></a>Phase 2

| Task | Weeks min | Weeks max | Comment |
| --- | --- | --- | --- |
| core consolidation | 4 | 8 |
| build system and packaging | 2 | 4 | need to link/bundle with Mantid somehow |
| adapt `CurveFitting` module | 4 | 12 |
| operations: other | 4 | 8 | beyond basic arithmetics and shape operations |
| converters: instrument | 2 | 4 |
| helpers for instrument | 1 | 2 | e.g., functionality that is now in `Beamline::ComponentInfo` |
| implement real workflow 1 | 8 | 12 |
| implement real workflow 2 | 6 | 10 |
| implement real workflow 3 | 4 | 8 |
| implement real workflow 4 | 4 | 8 |
| 1D and 2D plotting support | 1 | 2 | same as in Mantid-4.0 |
| slice viewer support | 3 | 6 | should be same as in Mantid-4.0, only effort for making it compatible is listed |
| table viewer support | 1 | 2 | should be same as in Mantid-4.0, only effort for making it compatible is listed |
| `InstrumentView` support | 3 | 6 |
| performance optimization | 4 | 8 |
| documentation | 4 | 6 |
| operations: I/O | 2 | 6 | only basic types, no NeXus |


#### <a name="implementation-effort-phase-2-wb"></a>Phase 2+WB

| Task | Weeks min | Weeks max | Comment |
| --- | --- | --- | --- |
| wrap `Dataset` in `API::Workspace` | 0.5 | 1 |
| wrap `Dataset` functions in `API::Algorithm` | 1 | 3 | may be very simple, can we have a Python meta class doing the work? |
| documentation | 1 | 2 |
| build system and packaging | 1 | 3 | need to link/bundle with Mantid somehow |
| `Workbench` adaptions | 2 | 8 | smooth GUI integration, e.g., right-click menus in workspace list |


#### <a name="implementation-effort-phase-2-x"></a>Phase 2+X

Undefined scope and flexible plan.
Effort estimates cannot be given.


## <a name="discussion"></a>Discussion


### <a name="discussion-impact"></a>Impact

##### General improvements

- Simple and flexible, and thus ready for the future both in terms of maintainability and new requirements.
- Guarantees the same API across all "workspace types".
- Full Python exports for all "workspace types".
- Supports a pythonic way of working.
- Improved `numpy` interoperability.
- Provides us with a chance to cull our algorithms to a well defined core plus well structured technique/facility specific extensions.
- Key features such as multidimensional variables and multiple data variables support complex workflows such as sample environment parameter scans.
- New features can be used to enable improved performance, depending on the application.
- Improved confidence in correctness due to well-defined low-level functionality and more widespread unit handling.

##### Enabling "new" use-cases

- Supporting constant-wavelength (reactor) "workspaces" without workarounds.
- Supporting workspaces where time-of-flight is not the inner dimension, e.g., imaging.
- Multiple-dimension support for all variables and "workspace types" and multiple data variables.
  Provides natural support for:
  - Sample environment parameter scans.
  - Multi-dimensional slicing.
  - Polarization analysis.


### <a name="discussion-new-challenges"></a>New challenges

We believe that this simple design will solve many of the problems we face currently when using the existing workspace types.
Obviously there will be new problems, not all of which can be foreseen.
Based on the limited experience when working with the prototype, there are a couple of new challenges we *do* foresee:
1. Avoiding an "old Mantid" vs. "new Mantid" split.
   This design is not a rewrite and real datasets will in fact reuse several of the components in existing workspaces.
   However, given that we do not intend to port all algorithms there is a real risk of getting stuck with two separate sub-projects for a very long time.
1. `Dataset` can contain anything.
   - We may need to develop strategies or standards that avoid an unmaintainable zoo of datasets.
     One example might be standards for naming variables.
   - Implementing algorithms for completely generic datasets may be more challenging, e.g., if multiple data variables are present.
1. Type-erasure makes some code slightly more cumbersome and verbose.



# <a name="further-reading"></a>Further reading

- [`xtensor`](https://github.com/QuantStack/xtensor) and [`xframe`](https://github.com/QuantStack/xframe) may be of interest for implementation details.
- For an internal serialization format [`TileDB`](https://tiledb.io/) could a solution.
- [NEP 22 -- Duck typing for NumPy arrays](https://www.numpy.org/neps/nep-0022-ndarray-duck-typing-overview.html) discusses a potential future change to NumPy which would make implementation of features we need for `Dataset` easier in, e.g., `xarray`.
- [NEP 20 -- Expansion of Generalized Universal Function Signatures](https://www.numpy.org/neps/nep-0020-gufunc-signature-enhancement.html) also discusses future NumPy features that we should keep in mind and potentially make use of or support in a similar manner.
- [Iris](https://scitools.org.uk/iris/docs/latest/) is another project for multi-dimensional data, building on NumPy and Dask like `xarray`.
