
# Beamline in Dataset

The purpose of this document is ultimately to determine how we will properly support Instrument 2.0 (a.k.a Beamine) in Dataset and what the cost will be. **The best way to understand what will be needed for Dataset is first to map out how the remaining rollout would be done ouside of dataset**. Since Dataset can and will support all Geometry, we can use these estimates to inform the rollout strategy and effort for Geometry in Dataset.

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
  - Memory overhead for geometry up to 2x greater
  - Would likely be faster than current Instrument::makeLegacyParameterMap()
c. Store the delta at the point of writing in `ComponentInfo/DetectorInfo` in a `std::unordered_map<size_t, V3D>` and `std::unordered_map<size_t, Quat>`
  - Smaller memory overhead than above.
  - Performance probably the same as the current Instrument::makeLegacyParameterMap(), not advantage 
  - Write performance would suffer for all geometry operations

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
  - Bakes workaround to creating legacy files formats into the data structures, so don't like this much


  



2. Remove Instrument 1.0 entirely
- Remove Instrument Visitor compatibility mode, 
	- Do away with builders or constructional objects entirely (new ones may be required), but to stop using Instrument/IComponent etc as an intermediary
		- Early rollout of this to NexusGeometryParser would be a good proof of concept since Instrument 1.0 has no place their and is simply being used as a builder
	- We need a converter Instrument 2.0 -> Instrument 1.0 for purposes of maintaining our python exports till such a time they can be eliminated (see below).
- remove from ExperimentInfo setInstrument/getInstrument. These would be replaced with something like `std::pair<uptr<const DetectorInfo>, uptr<const ComponentInfo>>` Note that getInstrument is exposed to python and would have to remain intact.


Excluded/Orphaned:

Saving/Loading format changes.
    - No legacy parameter map (diffs for positions/rotations) in new style, write straight as NexusGeometry much like `ExportGeometry`
    - Still need to load old-style files. 
	- format may be different ParameterMap::saveNexus - Doesn't have to change but DS group are advising on a preferred way of storing information. i.e not in NXNote [here](https://github.com/mantidproject/mantid/issues/23804)

Notes:


- ComponentIDs are not written to or read disk, and cannot be since they are simply the address of the in-memory component
- Important to separate the need for storage of arbitrary parameters against a component and the copy hack employed by Instrument 1.0 to store a modified geometry via parameters of a ParameterMap. The former is still very important.


Questions:

- How to handle the Physical Instrument. Parameter map is shared with Neutronic One. Base Instrument is shared. 
	- Currently we handle this via a lazy visit in the InstrumentView, that means keeping things as Instrument 1.0.
	- Suggested approach 1. In line with the policy not using Instrument 1.0 outside loading and removing Instrument 1.0 from all signatures, we changing the current setup to be a eager conversion for the cases where a physical instrument existed. 
		- There are relatively few cases where this is really happening
		- Allows complete elimination of the physicalInstrument
		- The cost in time and memory overhead would occur as part of all data reduction, whether the instrument was being visualised or not.
	- Suggested approach 2. Lazy Physical Instrument Loading.
		- The physical instrument is immutable from the IDF as returned as const ref Instrument::getPhysicalInstrument(). In any event, this is ONLY used for instrument view visualisation. 
        - Suggested approach 3. Optional physical_position, physical_rotation in ComponentInfo, DetectorInfo
	- Suggested approach 4. Use new-style ParameterMap



