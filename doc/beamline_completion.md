
# Beamline in Dataset

The purpose of this document is ultimately to determine how we will properly support Instrument 2.0 (a.k.a Beamine) in Dataset and what the cost will be. **The best way to understand what will be needed for Dataset is first to map out how the remaining rollout would be done outside of dataset, against existing Workspace**. Since Dataset can and will support all Geometry, we can use these estimates to inform the rollout strategy and effort for Geometry in Dataset.

## Phases

What is presented here are phases that would need to be completed against Mantid (v.3.14) to get to a point that the Dataset like Beamline types (`DetectorInfo`, `ComponentInfo`, `SpectrumInfo`) could be used without the need whatsoever for `Instrument 1.0`

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

## Notes

- ComponentIDs are not written to or read disk, and cannot be since they are simply the address of the in-memory component
- Important to separate the need for storage of arbitrary parameters against a component and the copy hack employed by Instrument 1.0 to store a modified geometry via parameters of a ParameterMap. The former is still very important.

## Effort

Effort estimtates based on above considerations

| Phase  | Lower Estimate (Weeks) | Upper Estimate (Weeks) | Notes |
| ------------- | ------------- | ------------- | ------------- |
| 1  | 4  | 12 | Changing signatures will be the largest effort and unknown |
| 2(b)  | 1 | 3 | Relatively well understood. Does not require removing base instrument |
| 3(c)  | 1 | 3 | Solution similar to the above, but fields are optional |
| 4 |  12 | 32 | Almost total removal as set out in above plan |
| 5 | 6 | 14 | Complete removal and clean-up |
| **Total Weeks** | 24 | 64 | | 

**Mapping to Dataset, it is suggested that the above form only the base part of the estimate** The above solutions do not factor in any change or new file format. It may reasonably take 6+ months to design and, implement and optimise a file format and reading/writing functionality.



