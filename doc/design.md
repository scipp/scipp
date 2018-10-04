# Dataset design

Author: Simon Heybrock
Prototype reviewers: Owen Arnold, Martyn Gigg

## Overview

## Context

![test](workspace-hierarchy.png)

https://github.com/mantidproject/documents/blob/master/Project-Management/CoreTeam/workspace-notes.md

What has been done so far:
Gathered requirements.
Several concepts prototyped, one in great detailed, reviewed.

## Goals and non-goals

## Milestones

 - user stories?

- alternative/abandoned approaches

## Impact

## Discussion





## High level overview

- numpy read/write wrappers, as old readX/dataX, but now multi-dimensional!

## Details

## Naming

### Components of the library are part of namespace `dataset`

##### Rationale

Mantid currently uses upper-case namespace names.
However, this can lead to artificial and unintuitive namespace or class names.
Instead, namespaces like `dataset` should be lower-case, as opposed to types which are upper-case.
We then have, e.g., `dataset::Dataset`.
This also matches the Python convention where module names are typically lower-case, thus reducing the differences between the C++ and Python APIs.

##

### Uncertainties are stored as variances, not standard deviations

##### Rationale

* [R: test](#Rr-scoped)

### <a name="Rr-scoped"></a>R.5: 



- efforts estimates (10x prototyping time and detailed list)
  - documentation (developer and user)
  - testing
- rollout
  - Python ADS issues
- do not give estimates for things that we do not know about, e.g., time for implementing higher level algorithms --- we do not know if this is what people will want to do, how it works, ...
- guarantees about not breaking inputs on failed operations (exception safety) --- explicitly specify in each case! clear if broken

- list of basic operations: unary -, binary, transpose, slice, dice, concatenate, extract, insert, erase, convertDimension, integrate, rebin, sort, boolen, project?, statistical operations (min, max, mean, std dev.)?, filter
- many parts of Instrument-2.0 can be mapped nearly 1:1
- scanning
- within Mantid repo or separate?
- units
