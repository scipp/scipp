import scipp as sc
import numpy as np


def get_pos(pos):
    return [pos.X(), pos.Y(), pos.Z()]


def convert_instrument(ws):
    compInfo = sc.Dataset(
        {'position': sc.Variable(dims=[sc.Dim.Row], shape=(2,),
                                 dtype=sc.dtype.vector_3_double,
                                 unit=sc.units.m)})
    # Current assumption: 0 is source, 1 is sample
    sourcePos = ws.componentInfo().sourcePosition()
    samplePos = ws.componentInfo().samplePosition()
    compInfo['position'].values[0] = get_pos(sourcePos)
    compInfo['position'].values[1] = get_pos(samplePos)
    return sc.Variable(value=compInfo)


def initPosSpectrumNo(nHist, ws):
    try:
        pos = sc.Variable(
            [sc.Dim.Position], values=[
                get_pos(ws.spectrumInfo().position(j)) for j in range(nHist)])
    except RuntimeError:
        pos = None
    num = sc.Variable(
        [sc.Dim.Position],
        values=[ws.getSpectrum(j).getSpectrumNo() for j in range(nHist)])
    return pos, num


def ConvertWorkspace2DToDataset(ws):
    cb = ws.isCommonBins()
    nHist = ws.getNumberHistograms()
    comp_info = convert_instrument(ws)
    pos, num = initPosSpectrumNo(nHist, ws)

    # TODO More cases?
    # tag = sc.Dim.EnergyTransfer if ws.getAxis(
    #     0).getUnit().unitID() == 'DeltaE' else sc.Dim.Tof
    dim = sc.Dim.EnergyTransfer if ws.getAxis(
        0).getUnit().unitID() == 'DeltaE' else sc.Dim.Tof
    if cb:
        coords = sc.Variable([dim], values=ws.readX(0))
    else:
        coords = sc.Variable([sc.Dim.Position, dim],
                             shape=(ws.getNumberHistograms(),
                                    len(ws.readX(0))))
        for i in range(ws.getNumberHistograms()):
            coords[sc.Dim.Position, i].values = ws.readX(i)

    # TODO Use unit information in workspace, if available.
    var = sc.Variable([sc.Dim.Position, dim],
                      shape=(ws.getNumberHistograms(), len(ws.readY(0))),
                      unit=sc.units.counts)

    for i in range(ws.getNumberHistograms()):
        var[sc.Dim.Position, i].values = ws.readY(i)
        var[sc.Dim.Position, i].variances = ws.readE(i) * ws.readE(i)

    return sc.DataArray(data=var, coords={dim: coords, sc.Dim.Position: pos},
                        labels={"spectrum_number": num,
                                "component_info": comp_info})


def ConvertEventWorkspaceToDataset(ws, drop_pulse_times):

    if ws.getAxis(0).getUnit().unitID() != "TOF":
        raise RuntimeError("Converting an EventWorkspace with non-Tof X-axis "
                           "to a Dataset is currently not supported.")

    nHist = ws.getNumberHistograms()
    comp_info = convert_instrument(ws)
    pos, num = initPosSpectrumNo(nHist, ws)

    coords = sc.Variable([sc.Dim.Position, sc.Dim.Tof],
                         shape=[nHist, sc.Dimensions.Sparse])
    if not drop_pulse_times:
        labs = sc.Variable([sc.Dim.Position, sc.Dim.Tof],
                           shape=[nHist, sc.Dimensions.Sparse])
    for i in range(nHist):
        coords[sc.Dim.Position, i].values = ws.getSpectrum(i).getTofs()
        if not drop_pulse_times:
            # Pulse times have a Mantid-specific format so the conversion is
            # very slow.
            # TODO: Find a more efficient way to do this.
            pt = ws.getSpectrum(i).getPulseTimes()
            labs[sc.Dim.Position, i].values = np.asarray([p.total_nanoseconds()
                                                          for p in pt])

    coords_and_labs = {"coords": {sc.Dim.Tof: coords, sc.Dim.Position: pos},
                       "labels": {"spectrum_number": num,
                                  "component_info": comp_info}}
    if not drop_pulse_times:
        coords_and_labs["labels"]["pulse_times"] = labs
    return sc.DataArray(**coords_and_labs)


def load(filename="", drop_pulse_times=False, **kwargs):
    """
    Wrapper function to provide a load method for a Nexus file, hiding mantid
    specific code from the scipp interface.
    """
    import mantid.simpleapi as mantid
    ws = mantid.LoadEventNexus(filename, **kwargs)
    if ws.id() == 'Workspace2D':
        return ConvertWorkspace2DToDataset(ws)
    if ws.id() == 'EventWorkspace':
        return ConvertEventWorkspaceToDataset(ws, drop_pulse_times)
    raise RuntimeError('Unsupported workspace type')
