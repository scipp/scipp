ADR 0012: Rebin should apply irreducible masks
==============================================

- Status: accepted
- Deciders: Jan-Lukas, Neil, Simon
- Date: 2022-01-19

Context
-------

In its current implementation ``rebin`` preserves all masks, including masks that depend on the dimension being rebinned.
This is unlike other operations that remove a dimension or change the length of a dimension, such as ``sum``.
Historically the motivation for this was likely the behavior in Mantid, which also preserves such masks (called bin-masks in Mantid).
The difference is that Mantid turns such masks into fractional masks, whereas scipp kept masks as boolean.

The consequence of this behavior is that masks *grow* when ``rebin`` is called. If any bin adjacent to a masked bin has "wanted" nonzero values (and an output bin includes both the masked and the unmasked bin) the result is that the "wanted" values get masked.

The biggest problem with this is that it is hard to impossible for a user to tell when this is happening, and to what extent.
The user may experience that some of their data, e.g., a measured intensity, simply "disappears".
Rebinning multiple times makes more and more intensity disappear.

Note furthermore that this behavior of ``rebin`` is inconsistent with ``bin``, which already applies masks in its current implementation by ignoring all masked bins.

Decision
--------

Just like reduction operations such as ``sum``, ``rebin`` should apply irreducible masks instead of trying to preserve them.

Consequences
------------

Positive:
~~~~~~~~~

- No more mysteriously disappearing data.
- The effect of masking data is the same as zeroing data directly.
- Consistent with other operations.

Negative:
~~~~~~~~~

- Breaking change to current behavior that may affect some users.
- Masks have to be applied earlier than previously, even in cases where "rebinning" and growing the mask would have been acceptable.
