#| # Dataset in a Nutshell - Part 3: Neutron Data
#|
#| This is the continuation of [Dataset in a Nutshell - Part 2](demo-part2.ipynb).

import numpy as np

import dataset as ds
from dataset import Dim, Coord, Data, Attr

#| For convenience and testing, very basic converters from Mantid's `EventWorkspace` and `Workspace2D` to `Dataset` are available:

import mantid.simpleapi as mantid
import dataset.compat.mantid as mantidcompat
filename = 'PG3_4844_event.nxs'
filename_vanadium = 'PG3_4866_event.nxs'

sampleWS = mantid.LoadEventNexus(filename)
vanadiumWS = mantid.LoadEventNexus(filename_vanadium)

#-------------------------------

d = mantidcompat.to_dataset(sampleWS, name='sample', drop_pulse_times=False)
d.merge(mantidcompat.to_dataset(vanadiumWS, name='vanadium', drop_pulse_times=True))

#| Some of the content of the workspaces is not easily representable as a plain multi-dimensional array.
#| A number of variables in the dataset obtained from the workspace thus have item type `Dataset`:

d

#| `Coord.ComponentInfo` is similar to the `ComponentInfo` in Mantid.
#| It contains information about the components in the beamline.
#| For the time being, it only contains the positions for source and sample:

d[Coord.ComponentInfo].scalar

#| *Bonus note 1:
#| For the most part, the structure of `ComponentInfo` (and `DetectorInfo`) in Mantid is easily represented by a `Dataset`, i.e., very little change is required.
#| For example, scanning is simply handled by an extra dimension of, e.g., the position and rotation variables.
#| By using `Dataset` to handle this, we can use exactly the same tools and do not need to implement or learn a new API.*

#| `Data.Events` is the equivalent to a vector of `EventList`s in Mantid.
#| Here, we chose to represent an event list as a `Dataset`:

d[Data.Events, 'sample'].data[10000]

#| We could in principle also choose other, more efficient, event storage formats.
#| At this point, using a dataset as an event list is convenient because it lests us reuse the same functionality:

events = d[Data.Events, 'sample'].data[10000]
d[Data.Events, 'sample'].data[10000] = ds.sort(events, Data.Tof)[Dim.Event, 100:-100]
ds.table(d[Data.Events, 'sample'].data[10000])

#| ### Exercise 1
#| How many events are there in total? TODO This is boring/useless, find a better task!

count = 0
for event_list in d[Data.Events, 'sample'].data:
    count += event_list.dimensions[Dim.Event]
print(count)

#| We histogram the event data:

ds.events.sort_by_tof(d)
coord = ds.Variable(Coord.Tof, [Dim.Tof], np.arange(1000.0, 20000.0, 50.0))
binned = ds.histogram(d, coord)
d.merge(binned)
d

#-------------------------------

ds.plot(d.subset[Data.Value, 'sample'][Dim.Position, 5000:6000], axes=[Coord.SpectrumNumber, Coord.Tof])

#-------------------------------

del(d[Data.Events, 'sample'])
del(d[Data.Events, 'vanadium'])

#| Monitors are not handled by the Mantid converter yet, but we can add some fake ones to demonstrate the versatility  of `Dataset`:

# Histogram-mode beam monitor
beam = ds.Dataset()
beam[Coord.Tof] = ([Dim.Tof], np.arange(1001.0))
beam[Data.Value] = ([Dim.Tof], np.random.rand(1000))
beam[Data.Variance] = beam[Data.Value]
beam[Data.Value].unit = ds.units.counts
beam[Data.Variance].unit = ds.units.counts * ds.units.counts

# Event-mode transmission monitor
transmission = ds.Dataset()
transmission[Data.Tof] = ([Dim.Event], 20000.0 * np.random.rand(123456))

# Beam profile monitor
profile = ds.Dataset()
profile[Coord.X] = ([Dim.X], np.arange(-0.1, 0.11, 0.01))
profile[Coord.Y] = ([Dim.Y], np.arange(-0.1, 0.11, 0.01))
profile[Data.Value] = ([Dim.Y, Dim.X], np.random.rand(20, 20))
for i in 1,2,3,4:
    profile[Dim.X, i:-i][Dim.Y, i:-i] += 1.0
profile[Data.Value].unit = ds.units.counts

ds.plot(profile)

#-------------------------------

d[Coord.Monitor, 'transmission'] = ([], transmission)
d[Coord.Monitor, 'beam'] = ([], beam)
d[Coord.Monitor, 'profile'] = ([], profile)
d

#| ### Exercise 1
#| Normalize the sample data to the "beam" monitor.
#|
#| ### Solution 1
#| The binning of the monitor does not match that of the data, so we need to rebin it before the division:

d.subset['sample'] /= ds.rebin(d[Coord.Monitor, 'beam'].scalar, d[Coord.Tof])

#-------------------------------

# TODO only scale sample, so vanadium does not get extra dim?
d = ds.concatenate(d, d * 0.8, Dim.Temperature)
d = ds.concatenate(d, d * 0.64, Dim.Temperature)
d[Coord.Temperature] = ([Dim.Temperature], [273.0, 180.0, 100.0, 4.3])
d

#-------------------------------

d.subset[Data.Value, 'sample'][Dim.Position, 10000:11000]

#-------------------------------

ds.plot(d.subset[Data.Value, 'sample'][Dim.Position, 10000:11000], axes=[Coord.SpectrumNumber, Coord.Temperature, Coord.Tof])

#-------------------------------

# TODO For the demo at the dev. meeting we will have Dim.DSpacing here, so summing below makes sense
d = ds.convert(d, Dim.Tof, Dim.Energy)

#-------------------------------

d

#-------------------------------

#ds.plot(d.subset[Data.Value, 'sample'][Dim.Position, 5000:8000], axes=[Coord.SpectrumNumber, Coord.Energy], logcb=False)

#-------------------------------

normalized = d.subset['sample'] / d.subset['vanadium']
summed = ds.sum(normalized, Dim.Position)

#-------------------------------

summed

#| ### Exercise 1
#| Instead of summing over all spectra, sum only over parts?

