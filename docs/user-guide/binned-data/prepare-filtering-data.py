# This uses the VULCAN_221040 file, obtained from Jonathan Taylor (SNS). We got
# (oral) permission to use this file since it is from a course, i.e., not a user's data.
import scippneutron as scn

import scipp as sc

# Load with Mantid (slow)
# dg = scn.load_with_mantid('VULCAN_221040.nxs.h5')
# data = dg['data']
# strain = dg['loadframe.strain']
# proton_charge = dg['proton_charge']
# dg = sc.DataGroup({'data': data,
#                    'loadframe.strain': strain,
#                    'proton_charge': proton_charge})
# dg.save_hdf5('VULCAN_221040.h5')

# Load cached file with Scipp
dg = sc.io.load_hdf5('VULCAN_221040.h5')
data = dg['data']
strain = dg['loadframe.strain']
proton_charge = dg['proton_charge']

graph = {
    **scn.conversion.graph.beamline.beamline(scatter=True),
    **scn.conversion.graph.tof.elastic_dspacing('tof'),
}
dspacing = data.transform_coords(
    'dspacing', graph=graph, keep_inputs=False, keep_intermediate=False
)

start = 1.5 * sc.Unit('angstrom')
stop = 2.5 * sc.Unit('angstrom')
tmp = dspacing['spectrum', :10000].bins['dspacing', start:stop]
tmp = tmp.bins.concat('spectrum').bin(dspacing=100)
tmp.bins.coords['time'] = tmp.bins.coords.pop('pulse_time')

out = sc.DataGroup(
    {'data': tmp, 'loadframe.strain': strain, 'proton_charge': proton_charge}
)
out.save_hdf5('VULCAN_221040_processed.h5')
