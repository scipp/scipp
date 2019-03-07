import dataset as ds

def get_pos(pos):
    return [pos.X(), pos.Y(), pos.Z()]

def convert_instrument(dataset, ws):
    compInfo = ds.Dataset()
    compInfo[ds.Coord.Position] = ([ds.Dim.Component], (2,))
    # Current assumption: 0 is source, 1 is sample
    samplePos = ws.componentInfo().samplePosition()
    sourcePos = ws.componentInfo().sourcePosition()
    compInfo[ds.Coord.Position].data[0] = get_pos(samplePos)
    compInfo[ds.Coord.Position].data[1] = get_pos(sourcePos)
    dataset[ds.Coord.ComponentInfo] = ([], compInfo)

def initPosSpectrumNo(d, nHist, ws):
    d[ds.Coord.Position] = ([ds.Dim.Position], [ get_pos(ws.spectrumInfo().position(j)) for j in range(nHist) ])
    d[ds.Coord.SpectrumNumber] = ([ds.Dim.Position], [ ws.getSpectrum(j).getSpectrumNo() for j in range(nHist) ])

def ConvertWorkspace2DToDataset(ws):
    d = ds.Dataset()
    cb = ws.isCommonBins()
    nHist = ws.getNumberHistograms()
    convert_instrument(d, ws)
    initPosSpectrumNo(d, nHist, ws)

    # TODO More cases?
    tag = ds.Coord.DeltaE if ws.getAxis(0).getUnit().unitID() == 'DeltaE' else ds.Coord.Tof
    dim = ds.Dim.DeltaE if ws.getAxis(0).getUnit().unitID() == 'DeltaE' else ds.Dim.Tof
    if cb:
        d[tag] = ([dim], ws.readX(0))
    else:
        d[tag] = ([ds.Dim.Position, dim], (ws.getNumberHistograms(), len(ws.readX(0))))
        for i in range(ws.getNumberHistograms()):
            d[tag][ds.Dim.Position, i] = ws.readX(i);


    d[ds.Data.Value] = ([ds.Dim.Position, dim], (ws.getNumberHistograms(), len(ws.readY(0))))
    d[ds.Data.Variance] = ([ds.Dim.Position, dim], (ws.getNumberHistograms(), len(ws.readE(0))))

    # TODO Use unit information in workspace, if available.
    d[ds.Data.Value].unit = ds.units.counts
    d[ds.Data.Variance].unit = ds.units.counts * ds.units.counts

    for i in range(ws.getNumberHistograms()):
        d[ds.Data.Value][ds.Dim.Position, i] = ws.readY(i)
        d[ds.Data.Variance][ds.Dim.Position, i] = ws.readE(i)*ws.readE(i)

    return d

def ConvertEventWorkspaceToDataset(ws, drop_pulse_times):
    d = ds.Dataset()
    nHist = ws.getNumberHistograms()
    convert_instrument(d, ws)
    initPosSpectrumNo(d, nHist, ws)

    d[ds.Data.Events] = ([ds.Dim.Position], (nHist,))
    for i, e in enumerate(d[ds.Data.Events].data):
        e[ds.Data.Tof] = ([ds.Dim.Event], ws.getSpectrum(i).getTofs())
        if not drop_pulse_times:
            pt = ws.getSpectrum(i).getPulseTimes()
            e[ds.Data.PulseTime] = ([ds.Dim.Event], [p.total_nanoseconds() for p in pt])
    return d

# Pulse times have a Mantid-specific format so the conversion is very slow.
# Dataset is flexible and can work without pulse times, so we can drop them.
def to_dataset(ws, drop_pulse_times=False):
    if ws.id() == 'Workspace2D':
        return ConvertWorkspace2DToDataset(ws)
    if ws.id() == 'EventWorkspace':
        return ConvertEventWorkspaceToDataset(ws, drop_pulse_times)
    raise 'Unsupported workspace type'
