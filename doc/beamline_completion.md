
# Beamline in Dataset

The purpose of this document is ultimately to determine how we will properly support Instrument 2.0 (a.k.a Beamine) in Dataset and what the cost will be. **The best way to understand what will be needed for Dataset is first to map out how the remaining rollout would be done outside of dataset, against existing Workspace**. Since Dataset can and will support all Geometry, we can use these estimates to inform the rollout strategy and effort for Geometry in Dataset.

## Phases without Dataset

What is presented here are phases that would need to be completed against Mantid (v.3.14) to get to a point that Beamline types (`DetectorInfo`, `ComponentInfo`, `SpectrumInfo`) could be used without the need whatsoever for `Instrument 1.0`

### Phase 1 - Push Instrument 1.0 to IO aspects only

1. Build new `BeamlineParameterMap` working off component indexes. This would not need to wrap existing `ParameterMap` to support `makeLegacyParameterMap`, but the point of this is to get away from `ComponentIDs`
1. Replace the `ParameterMap` calls with the above, use `ComponentInfo::indexOfAny` to replace the common `ICompAssembly::getComponentByName()` -> `IComponent::getComponentID()` -> `ComponentInfo::indexOf()` call pattern
1. Using above, strip aspects all aspects like getComponentID() out of interfaces. In fact, ComponentID would not be allowed to feature as part of function signatures. Involves changing `IComponent` so probably quite involved.
1. For InstrumentActor, this would be the prime place to use the BeamlineParameterMap (https://github.com/mantidproject/mantid/blob/master/qt/widgets/instrumentview/src/InstrumentActor.cpp#L1108)
1. We do not need to consider major IO changes as part of this phase. Even makeLegacyParameterMap remains roughly the same
1. Additional time would be needed to remove other non-ComponentID, Instrument 1.0, types from interfaces (`Detector*`) for example

### Phase 2 - Saving

This is the primary reason the base instrument (instrument 1.0) is still required.

**Options**

a. Break with old file format. 
  - This would be the simplest from a coding perspective, 
  - Offers best performance benefits as no special Instrument::makeLegacyParameterMap() required
  - Only `NexusGeometry` would be suitable for this, this cannot be done with IDFs as Instruments are non-serializeable to IDFs. Probably the limiting factor with this approach.
  - Could break a large number of system test even if above point not considered. 
  
b. Keep a “base” `ComponentInfo/DetectorInfo`. `ComponentInfo/DetectorInfo` would have optional links to a base `ComponentInfo`. 
  - Avoid changes to file format, **but a lot messier with the need for null checking.** 
  - Memory overhead greater, though only base positions and rotations need to be stored
  - Would likely be faster than current Instrument::makeLegacyParameterMap()
  
c. Store the delta at the point of writing in `ComponentInfo/DetectorInfo` in a `std::unordered_map<size_t, V3D>` and `std::unordered_map<size_t, Quat>`
  - Smaller memory overhead than above.
  - Performance probably the same as the current Instrument::makeLegacyParameterMap(), not advantage 
  - Write performance would suffer for all geometry operations
  - As above bakes "work around" for legacy file problem into data type. However much easier to remove and separate than above solution

### Phase 3: Solve Physical Instrument Issue

Parameter map is shared with Neutronic One. Base Instrument is shared. Currently we handle this via a lazy visit in the `InstrumentView`, that means keeping things as Instrument 1.0. The physical instrument is immutable from the IDF as returned as const ref `Instrument::getPhysicalInstrument()`. In any event, this is ONLY used for instrument view visualisation. 

**Options**
a. Changing the current setup to be a eager conversion for the cases where a physical instrument existed. 
  - There are relatively few cases where separate geometries are really needed. Does not affect all instruments.
  - Allows complete elimination of the physicalInstrument. No need for `ExperimentInfo::getPhysicalInstrument`
  - The cost in time and memory overhead would occur as part of all data reduction for the affected instruments. Whether the instrument was being visualised or not.
  
b. Lazy Physical Instrument Loading.
  - Essentially pass the specification for getting the instrument from disk, but don't do that at all until required in the `InstrumentView`.
  
c. Optional `physical_position`, `physical_rotation` in ComponentInfo, DetectorInfo
  - Bakes workaround to creating legacy files formats into the data structures, so don't like this a lot

### Phase 4: Remove Instrument 1.0 (almost) entirely

1. Remove Instrument Visitor compatibility mode, 
  - Do away with builders or constructional objects entirely (new ones may be required), but to stop using `Instrument/IComponent` etc as an intermediary
    - Early rollout of this to `NexusGeometryParser` would be a good proof of concept since Instrument 1.0 has no place their and is simply being used as a builder
  - We need a converter Instrument 2.0 -> Instrument 1.0 for purposes of maintaining our python exports till such a time they can be eliminated (see below).
- remove from `ExperimentInfo` `setInstrument/getInstrument`. These would be replaced with something like `std::pair<uptr<const DetectorInfo>, uptr<const ComponentInfo>>` Note that `ExperimentInfo::getInstrument` is exposed to python and would have to remain intact.

### Phase 5: Remove backwards compatibility. i.e. No Instrument Exports

This is a breaking change so would have to happen at some controlled point.
1. Remove API
1. Update internal scripts and algorithms
1. Amensty for scripts to update
1. Document and publicise to community removal

### Notes

- ComponentIDs are not written to or read disk, and cannot be since they are simply the address of the in-memory component
- Important to separate the need for storage of arbitrary parameters against a component and the copy hack employed by Instrument 1.0 to store a modified geometry via parameters of a ParameterMap. The former is still very important.

### Workspace 1.0 Geometry Estimates

Effort estimtates based on above considerations

| Phase  | Lower Estimate (Weeks) | Upper Estimate (Weeks) | Required for Dataset approach y/n | Notes |
| ------------- | ------------- | ------------- | ------------- | ------------- |
| 1  | 4  | 12 | n | Changing signatures will be the largest effort and unknown |
| 2(b)  | 1 | 3 | y | Relatively well understood. Does not require removing base instrument |
| 3(c)  | 1 | 3 | y | Solution similar to the above, but fields are optional |
| 4 |  12 | 32 | n | Almost total removal as set out in above plan |
| 5 | 6 | 14 | n |Complete removal and clean-up |
| **Total Weeks** | 24 | 64 | | | 

## Dataset Objectives

The `Dataset` objectives are slightly different from everything outlined above. Firstly `Dataset` provides a more "green field" approach rather than the "brown field" that has to be considered when supporting existing workspaces. For example `Dataset` rollout will involve a new stream of non-compatible (with existing base `API::Algorithm`) `Algorithms` and that gives significant flexibility when breaking with old behaviour. 

Here are some key differences as relating to the Geometry:

* Fundamental non-compatibility between `Workspace` and `Dataset`. `Dataset` has more information than possible in `Workspace`. All interop via conversion, API compatibility not maintained.
  - Existing saving would have to "slice" `Dataset`, so would be required to write new saving functionality and format for `Dataset`
  - Because of saving requirement, no requirement to save geometry deltas. Geometry can be written to be directly correspond to the mutated Beamline geometry - not origninal (IDF) + delta
  - Loading functionality would follow from the above
* No ExperimentInfo
  - Geometry, alongside run information is part of `Dataset`
* No ParameterMap
  - Exact solution(s) not yet apparent, but there are several options. Given that a number of parameters are linked to the instrument and not individual components, this might affect how things are done
* No `ComponentID` by design. Infact, there may be no requirement for unique identification of `Components` at all.

Another key difference with the phases for non-dataset approach is that the approach concludes with the complete removal of `Instrument 1.0` but not `Workspace 1.0`. The current preferred rollout for `Dataset` is to develop alongside the existing codebase, and not to consider removal of legacy data structures i.e. Workspace until `Dataset` usage has acted as a full replacement. We would simultaneously remove `Workspace 1.0`, `Algorithms 1.0`, `Instrument 1.0`, `ExperimentInfo`. 

The net result of these differences is that the activities and timescales differ from those presented in the above section. We refer to these as activities rather than phases, since the order of the activities is not known and possibly not interdependent.

### Activity 1 - Functional wrappers to Dataset for Geometry

Need to provide classes of the form `SpectrumInfo`, `ComponentInfo` and `DetectorInfo` that wrap `Dataset`. These classes should point to a `Dataset` containing all required information. Note that `ComponentInfo` and `DetectorInfo` are inter-dependent, a higher-level grouping may be required. `SpectrumInfo` uses `DetectorInfo`. We would need to consider time spent to reproduce functional and performance testing.

### Activity 2 - Saving Loading

As described above, no need to preserve compatibility with existing intermediate formats as far as the total `Dataset` is concerned. Time would need to be invested to trial and optimise these. Requirements for end-users should also be considered, for example if `Dataset` libraries are used outside of Mantid. Note that `Saving` and `Loading` and format specifics must not be mixed into the `Dataset` library. 

### Activity 3 - Serialization/De-serialization NexusGeometry

I have split this from activity above for estimation purposes.

We will also need to serialize/de-serialize between in-memory geometry and target formats. For example serializing `ComponentInfo` to `NexusGeometryFormat` would likely be required for some kind of `SaveBeamline` (a rare example of something useful between Mantid as of v3.14 and with dataset).

### Activity 4 - Physical Instrument

This is a relatively minor consideration. Favoured approaches are to either lazy load the physical instrument as part of the instrument visualisation or to attach it as some extra variables. Infact, only `positions` and `rotations` should be allowed to differ between neutronic and physical beamlines.

### Activity 5 - Advise on Parameter Map

Probably should research and provide best practice for storing ParameterMap equivlent information

### Notes

This is slightly beyond the remit of the discussion here, but one concern with the `Dataset` rollout plan is the surge in technical debt. Conceptional understanding for a time will be problematic as there will be two very different ways of supporting data reduction. The size of the codebase will also increase significantly for a time. It may be many years before the codebase can be collapsed. 

### Dataset Geometry Estimates

Effort based on above consideration. **Note when comparing with Workspace 1.0 estimates that this does not include effort to take away existing approaches**

| Activity  | Lower Estimate (Weeks) | Upper Estimate (Weeks) |  Notes |
| ------------- | ------------- | ------------- |------------- |
| 1  | 2  | 8 | Mantid already provides a good template for this. Work may be rapid |
| 2  | 4 | 12 | Looking at xarray implementation may help |
| 3  | 2 | 12 |  |
| 4  | 1 | 2 |  |
| 5  | 1 | 2 |  |
| **Total Weeks** | 10 | 36 | | 





