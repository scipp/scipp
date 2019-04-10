import scippy as sp


def get_pos(pos):
    return [pos.X(), pos.Y(), pos.Z()]


def convert_instrument(dataset, ws):
    compInfo = sp.Dataset()
    compInfo[sp.Coord.Position] = ([sp.Dim.Component], (2,))
    # Current assumption: 0 is source, 1 is sample
    sourcePos = ws.componentInfo().sourcePosition()
    samplePos = ws.componentInfo().samplePosition()
    compInfo[sp.Coord.Position].data[0] = get_pos(sourcePos)
    compInfo[sp.Coord.Position].data[1] = get_pos(samplePos)
    dataset[sp.Coord.ComponentInfo] = ([], compInfo)


def initPosSpectrumNo(d, nHist, ws):
    try:
        d[sp.Coord.Position] = ([sp.Dim.Position], [get_pos(
            ws.spectrumInfo().position(j)) for j in range(nHist)])
    except RuntimeError:
        pass
    d[sp.Coord.SpectrumNumber] = (
        [sp.Dim.Position],
        [ws.getSpectrum(j).getSpectrumNo() for j in range(nHist)])


def ConvertWorkspace2DToDataset(ws, name):
    d = sp.Dataset()
    cb = ws.isCommonBins()
    nHist = ws.getNumberHistograms()
    convert_instrument(d, ws)
    initPosSpectrumNo(d, nHist, ws)

    # TODO More cases?
    tag = sp.Coord.DeltaE if ws.getAxis(
        0).getUnit().unitID() == 'DeltaE' else sp.Coord.Tof
    dim = sp.Dim.DeltaE if ws.getAxis(
        0).getUnit().unitID() == 'DeltaE' else sp.Dim.Tof
    if cb:
        d[tag] = ([dim], ws.readX(0))
    else:
        d[tag] = ([sp.Dim.Position, dim],
                  (ws.getNumberHistograms(), len(ws.readX(0))))
        for i in range(ws.getNumberHistograms()):
            d[tag][sp.Dim.Position, i] = ws.readX(i)

    d[sp.Data.Value, name] = ([sp.Dim.Position, dim],
                              (ws.getNumberHistograms(), len(ws.readY(0))))
    d[sp.Data.Variance, name] = (
        [sp.Dim.Position, dim], (ws.getNumberHistograms(), len(ws.readE(0))))

    # TODO Use unit information in workspace, if available.
    d[sp.Data.Value, name].unit = sp.units.counts
    d[sp.Data.Variance, name].unit = sp.units.counts * sp.units.counts

    for i in range(ws.getNumberHistograms()):
        d[sp.Data.Value, name][sp.Dim.Position, i] = ws.readY(i)
        d[sp.Data.Variance, name][sp.Dim.Position,
                                  i] = ws.readE(i) * ws.readE(i)

    return d


def ConvertEventWorkspaceToDataset(ws, name, drop_pulse_times):
    d = sp.Dataset()
    nHist = ws.getNumberHistograms()
    convert_instrument(d, ws)
    initPosSpectrumNo(d, nHist, ws)

    d[sp.Data.Events, name] = ([sp.Dim.Position], (nHist,))
    for i, e in enumerate(d[sp.Data.Events, name].data):
        e[sp.Data.Tof] = ([sp.Dim.Event], ws.getSpectrum(i).getTofs())
        if not drop_pulse_times:
            pt = ws.getSpectrum(i).getPulseTimes()
            e[sp.Data.PulseTime] = ([sp.Dim.Event],
                                    [p.total_nanoseconds() for p in pt])
    return d

# Pulse times have a Mantid-specific format so the conversion is very slow.
# Dataset is flexible and can work without pulse times, so we can drop them.


def to_dataset(ws, name='', drop_pulse_times=False):
    if ws.id() == 'Workspace2D':
        return ConvertWorkspace2DToDataset(ws, name)
    if ws.id() == 'EventWorkspace':
        return ConvertEventWorkspaceToDataset(ws, name, drop_pulse_times)
    raise 'Unsupported workspace type'
