# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock, Neil Vaytet


import scipp as sc
import numpy as np


def get_pos(pos):
    return [pos.X(), pos.Y(), pos.Z()]


def convert_instrument(ws):
    compInfo = sc.Dataset({
        'position':
        sc.Variable(dims=[sc.Dim.Row],
                    shape=(2, ),
                    dtype=sc.dtype.vector_3_double,
                    unit=sc.units.m)
    })
    # Current assumption: 0 is source, 1 is sample
    sourcePos = ws.componentInfo().sourcePosition()
    samplePos = ws.componentInfo().samplePosition()
    compInfo['position'].values[0] = get_pos(sourcePos)
    compInfo['position'].values[1] = get_pos(samplePos)
    return sc.Variable(value=compInfo)


def init_pos_spectrum_no(nHist, ws):

    pos = np.zeros([nHist, 3])
    num = np.zeros([nHist], dtype=np.int32)

    spec_info = ws.spectrumInfo()
    for i in range(nHist):
        p = spec_info.position(i)
        pos[i, :] = [p.X(), p.Y(), p.Z()]
        num[i] = ws.getSpectrum(i).getSpectrumNo()
    pos = sc.Variable([sc.Dim.Position], values=pos, unit=sc.units.m)
    num = sc.Variable([sc.Dim.Position], values=num)
    return pos, num


def convert_Workspace2D_to_dataset(ws):
    cb = ws.isCommonBins()
    nHist = ws.getNumberHistograms()
    comp_info = convert_instrument(ws)
    pos, num = init_pos_spectrum_no(nHist, ws)

    # TODO More cases?
    allowed_units = {
        "DeltaE": [sc.Dim.EnergyTransfer, sc.units.meV],
        "TOF": [sc.Dim.Tof, sc.units.us]
    }
    xunit = ws.getAxis(0).getUnit().unitID()
    if xunit not in allowed_units.keys():
        raise RuntimeError("X-axis unit not currently supported for "
                           "Workspace2D. Possible values are: {}, "
                           "got '{}'. ".format(
                               [k for k in allowed_units.keys()], xunit))
    else:
        [dim, unit] = allowed_units[xunit]

    if cb:
        coords = sc.Variable([dim], values=ws.readX(0), unit=unit)
    else:
        coords = sc.Variable([sc.Dim.Position, dim],
                             shape=(ws.getNumberHistograms(),
                                    len(ws.readX(0))),
                             unit=unit)
        for i in range(ws.getNumberHistograms()):
            coords[sc.Dim.Position, i].values = ws.readX(i)

    # TODO Use unit information in workspace, if available.
    array = sc.DataArray(data=sc.Variable([sc.Dim.Position, dim],
                                          shape=(ws.getNumberHistograms(),
                                                 len(ws.readY(0))),
                                          unit=sc.units.counts,
                                          variances=True),
                         coords={
                             dim: coords,
                             sc.Dim.Position: pos
                         },
                         labels={
                             "spectrum_number": num,
                             "component_info": comp_info
                         })

    data = array.data
    for i in range(ws.getNumberHistograms()):
        data[sc.Dim.Position, i].values = ws.readY(i)
        data[sc.Dim.Position, i].variances = np.power(ws.readE(i), 2)

    return array


def convert_EventWorkspace_to_dataset(ws, load_pulse_times, EventType):

    allowed_units = {"TOF": [sc.Dim.Tof, sc.units.us]}
    xunit = ws.getAxis(0).getUnit().unitID()
    if xunit not in allowed_units.keys():
        raise RuntimeError("X-axis unit not currently supported for "
                           "EventWorkspace. Possible values are: {}, "
                           "got '{}'. ".format(
                               [k for k in allowed_units.keys()], xunit))
    else:
        [dim, unit] = allowed_units[xunit]

    nHist = ws.getNumberHistograms()
    comp_info = convert_instrument(ws)
    pos, num = init_pos_spectrum_no(nHist, ws)

    # TODO Use unit information in workspace, if available.
    coords = sc.Variable([sc.Dim.Position, dim],
                         shape=[nHist, sc.Dimensions.Sparse],
                         unit=unit)
    if load_pulse_times:
        labs = sc.Variable([sc.Dim.Position, dim],
                           shape=[nHist, sc.Dimensions.Sparse])

    # Check for weighted events
    evtp = ws.getSpectrum(0).getEventType()
    contains_weighted_events = ((evtp == EventType.WEIGHTED)
                                or (evtp == EventType.WEIGHTED_NOTIME))
    if contains_weighted_events:
        weights = sc.Variable([sc.Dim.Position, dim],
                              shape=[nHist, sc.Dimensions.Sparse])

    for i in range(nHist):
        sp = ws.getSpectrum(i)
        coords[sc.Dim.Position, i].values = sp.getTofs()
        if load_pulse_times:
            # Pulse times have a Mantid-specific format so the conversion is
            # very slow.
            # TODO: Find a more efficient way to do this.
            pt = sp.getPulseTimes()
            labs[sc.Dim.Position,
                 i].values = np.asarray([p.total_nanoseconds() for p in pt])
        if contains_weighted_events:
            weights[sc.Dim.Position, i].values = sp.getWeights()
            weights[sc.Dim.Position, i].variances = sp.getWeightErrors()

    coords_labs_data = {
        "coords": {
            dim: coords,
            sc.Dim.Position: pos
        },
        "labels": {
            "spectrum_number": num,
            "component_info": comp_info
        }
    }
    if load_pulse_times:
        coords_labs_data["labels"]["pulse_times"] = labs
    if contains_weighted_events:
        coords_labs_data["data"] = weights
    return sc.DataArray(**coords_labs_data)


def load(filename="",
         load_pulse_times=True,
         instrument_filename=None,
         **kwargs):
    """
    Wrapper function to provide a load method for a Nexus file, hiding mantid
    specific code from the scipp interface. All other keyword arguments not
    specified in the parameters below are passed on to the mantid.Load
    function.

    Example of use:

      from scipp.neutron import load
      d = sc.Dataset()
      d["sample"] = load(filename='PG3_4844_event.nxs', \
                         BankName='bank184', load_pulse_times=True)

    See also the neutron-data tutorial.

    :param str filename: The name of the Nexus/HDF file to be loaded.
    :param bool load_pulse_times: Read the pulse times if True.
    :param str instrument_filename: If specified, over-write the instrument
                                    definition in the final Dataset with the
                                    geometry contained in the file.
    :raises: If the Mantid workspace type returned by the Mantid loader is not
             either EventWorkspace or Workspace2D.
    :return: A Dataset containing the neutron event/histogram data and the
             instrument geometry.
    :rtype: Dataset
    """

    import mantid.simpleapi as mantid
    from mantid.api import EventType

    ws = mantid.Load(filename, **kwargs)
    if instrument_filename is not None:
        mantid.LoadInstrument(ws,
                              FileName=instrument_filename,
                              RewriteSpectraMap=True)
    if ws.id() == 'Workspace2D':
        return convert_Workspace2D_to_dataset(ws)
    if ws.id() == 'EventWorkspace':
        return convert_EventWorkspace_to_dataset(ws, load_pulse_times,
                                                 EventType)
    raise RuntimeError('Unsupported workspace type')
