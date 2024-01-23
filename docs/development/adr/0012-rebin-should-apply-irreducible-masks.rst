ADR 0012: Rebin should apply irreducible masks
==============================================

- Status: accepted
- Deciders: Jan-Lukas, Neil, Simon
- Date: 2022-01-19

Context
-------

In its current implementation ``rebin`` preserves all masks, including masks that depend on the dimension being rebinned.
This is unlike other operations that remove a dimension or change the length of a dimension, such as ``sum``:
Those operations apply "irreducible" masks, i.e., masks that depend on the operation dimension.

Historically the motivation for this was likely the behavior in Mantid, which also preserves such masks (called bin-masks in Mantid).
The main argument for this approach is that for normalization purposes it is often crucial to be able to distinguish true (measured) zero values from zeros that result from applying a mask.
The difference is that Mantid turns such masks into fractional masks, whereas Scipp kept masks as boolean.

The consequence of this behavior is that masks *grow* when ``rebin`` is called. If any bin adjacent to a masked bin has "wanted" nonzero values (and an output bin includes both the masked and the unmasked bin) the result is that the "wanted" values get masked.

The biggest problem with this is that it is hard to impossible for a user to tell when this is happening, and to what extent.
The user may experience that some of their data, e.g., a measured intensity, simply "disappears".
Rebinning multiple times makes more and more intensity disappear.

Note furthermore that this behavior of ``rebin`` is inconsistent with ``bin``, which already applies masks in its current implementation by ignoring all masked bins.

A crucial shortcoming of applying the mask is that for normalization purposes it is important whether a zero is true (measured) or an artifact (such as a masked dead detector).
If the mask is applied we cannot tell the difference any more.
A partial solution could be to use NaN for fully or partially masked bins.
But the boundary problem persists:
If we convert only fully masked bins to NaN the mask (or rather then invalid data area previously marked by the mask, now marked by NaN values) "shrinks", whereas if we do so also for partially masked bins the mask "grows", as in the current implementation.

Overall there does not appear to be a solution to this.
Attempts to solve the "masking and normalization" problem are likely complicated, hard to document, and error prone.
Furthermore, any "solution" appears to leave substantial holes, i.e., the problem does not disappear.
We therefore believe that this problem cannot be addressed in the basic data structures and operations.
Instead, workflows that rely on normalization involving masked data must handle this adequately on a higher level.

Decision
--------

- Just like reduction operations such as ``sum``, ``rebin`` should apply irreducible masks instead of trying to preserve them.
- Consider whether we can promote or provide options that can delay or avoid rebinning.
  For example, one could rebin the scaling factor instead of the masked data.

Consequences
------------

Positive:
~~~~~~~~~

- No more mysteriously disappearing data.
- Masking removes only data values that are explicitly masked, rather than removing also *additional* data values.
- Consistent with ``bin``.
- Consistent with reduction operations.

Negative:
~~~~~~~~~

- Breaking change to current behavior that may affect some users.
- Masks have to be applied earlier than previously, even in cases where "rebinning" and growing the mask would have been acceptable.
- After rebinning it is no longer possible to visually inspect masks in a plot.
- Workflows that rely on normalization involving masked data must handle this adequately on a higher level.
  This has been partially the case anyway, so this is not actually a new problem.
