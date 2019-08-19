import scipp as sc


def get_pos(pos):
    return [pos.X(), pos.Y(), pos.Z()]


def convert_instrument(dataset, ws):
    compInfo = sc.Dataset(
        {'position': sc.Variable(dims=[sc.Dim.Row], shape=(2,),
                                 dtype=sc.dtype.vector_3_double,
                                 unit=sc.units.m)})
    # Current assumption: 0 is source, 1 is sample
    sourcePos = ws.componentInfo().sourcePosition()
    samplePos = ws.componentInfo().samplePosition()
    compInfo['position'].values[0] = get_pos(sourcePos)
    compInfo['position'].values[1] = get_pos(samplePos)
    dataset.labels['component_info'] = sc.Variable(dtype=sc.dtype.Dataset)
    dataset.labels['component_info'].value = compInfo


def initPosSpectrumNo(d, nHist, ws):
    try:
        d.coords[sc.Dim.Position] = sc.Variable(
            [sc.Dim.Position], values=[
                get_pos(ws.spectrumInfo().position(j)) for j in range(nHist)])
    except RuntimeError:
        pass
    d.labels["SpectrumNumber"] = sc.Variable(
        [sc.Dim.Position],
        values=[ws.getSpectrum(j).getSpectrumNo() for j in range(nHist)])


def ConvertWorkspace2DToDataset(ws, name):
    d = sc.Dataset()
    cb = ws.isCommonBins()
    nHist = ws.getNumberHistograms()
    convert_instrument(d, ws)
    initPosSpectrumNo(d, nHist, ws)

    # TODO More cases?
    # tag = sc.Dim.EnergyTransfer if ws.getAxis(
    #     0).getUnit().unitID() == 'DeltaE' else sc.Dim.Tof
    dim = sc.Dim.EnergyTransfer if ws.getAxis(
        0).getUnit().unitID() == 'DeltaE' else sc.Dim.Tof
    if cb:
        d.coords[dim] = sc.Variable([dim], values=ws.readX(0))
    else:
        d.coords[dim] = sc.Variable([sc.Dim.Position, dim],
                                    shape=(ws.getNumberHistograms(),
                                           len(ws.readX(0))))
        for i in range(ws.getNumberHistograms()):
            d.coords[dim][sc.Dim.Position, i].values = ws.readX(i)

    # TODO Use unit information in workspace, if available.
    d[name] = sc.Variable([sc.Dim.Position, dim],
                          shape=(ws.getNumberHistograms(), len(ws.readY(0))),
                          unit=sc.units.counts)

    for i in range(ws.getNumberHistograms()):
        d[name][sc.Dim.Position, i].values = ws.readY(i)
        d[name][sc.Dim.Position, i].variances = ws.readE(i) * ws.readE(i)

    return d


def ConvertEventWorkspaceToDataset(ws, name, drop_pulse_times):
    d = sc.Dataset()
    nHist = ws.getNumberHistograms()
    convert_instrument(d, ws)
    initPosSpectrumNo(d, nHist, ws)

    coords = sc.Variable([sc.Dim.Position, sc.Dim.Tof],
                         values=[nHist, sc.Dimensions.Sparse])
    if not drop_pulse_times:
        labs = sc.Variable([sc.Dim.Position, sc.Dim.Tof],
                           values=[nHist, sc.Dimensions.Sparse])
    for i in range(nHist):
        coords[sc.Dim.Position, i].values = ws.getSpectrum(i).getTofs()
        if not drop_pulse_times:
            pt = ws.getSpectrum(i).getPulseTimes()
            labs[sc.Dim.Position, i].values = np.asarray([p.total_nanoseconds() for p in pt])

    coords_and_labs = {"coords": {sc.Dim.Tof: coords}}
    if not drop_pulse_times:
        coords_and_labs["labels"] = {"pulse_times": labs}
    d[name] = sc.DataArray(**coords_and_labs)
    return d

# Pulse times have a Mantid-specific format so the conversion is very slow.
# Dataset is flexible and can work without pulse times, so we can drop them.


def to_dataset(ws, name='', drop_pulse_times=False):
    if ws.id() == 'Workspace2D':
        return ConvertWorkspace2DToDataset(ws, name)
    if ws.id() == 'EventWorkspace':
        return ConvertEventWorkspaceToDataset(ws, name, drop_pulse_times)
    raise 'Unsupported workspace type'
