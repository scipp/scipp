# This uses the VULCAN_221040 file, obtained from Jonathan Taylor (SNS). We got
# (oral) permission to use this file since it is from a course, i.e., not a user's data.
import scipp as sc
import scippneutron as scn

da = sc.io.open_hdf5('/home/simon/mantid/instruments/VULCAN/VULCAN_221040.h5')
dspacing = scn.convert(da, 'tof', 'dspacing', scatter=True).squeeze()
dspacing

start = 1.5 * sc.Unit('angstrom')
stop = 2.5 * sc.Unit('angstrom')
tmp = dspacing['spectrum', :10000].bins['dspacing', start:stop]
tmp = tmp.bins.concat('spectrum').bin(dspacing=100)
tmp.hist(dspacing=200).plot()
del tmp.bins.attrs['tof']
for name in list(tmp.attrs):
    if name not in ['proton_charge', 'loadframe.strain']:
        del tmp.attrs[name]
tmp.bins.coords['time'] = tmp.bins.coords.pop('pulse_time')
tmp.attrs['proton_charge'].value = tmp.attrs['proton_charge'].value.rename(
    pulse_time='time')
tmp.to_hdf5('scipp-filtering-docs-data.h5')
