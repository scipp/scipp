import dataset as ds

def ConvertWorkspase2DToDataset(ws):
    d = ds.Dataset()
    cb = ws.isCommonBins()
    nHist = ws.getNumberHistograms()
    d[ds.Coord.Position] = ([ds.Dim.Position], (nHist,))
    d[ds.Coord.SpectrumNumber] = ([ds.Dim.Position], (nHist,))
    
    for j, pos in enumerate(d[ds.Coord.Position].data):
        pos[0] = ws.spectrumInfo().position(j).X()
        pos[1] = ws.spectrumInfo().position(j).Y()
        pos[2] = ws.spectrumInfo().position(j).Z()
    
    for j, pos in enumerate(d[ds.Coord.SpectrumNumber].data):
        pos = ws.getSpectrum(j).getSpectrumNo()
        
    if cb:
        d[ds.Coord.Tof] = ([ds.Dim.Tof], ws.readX(0))        
    else:
        d[ds.Coord.Tof] = ([ds.Dim.Position, ds.Dim.Tof], (ws.getNumberHistograms(), len(ws.readX(0))))
        for i in range(ws.getNumberHistograms()):
            d[ds.Coord.Tof][ds.Dim.Position, i] = ws.readX(i);
        
    
    d[ds.Data.Value] = ([ds.Dim.Position, ds.Dim.Tof], (ws.getNumberHistograms(), len(ws.readY(0))))     
    d[ds.Data.Variance] = ([ds.Dim.Position, ds.Dim.Tof], (ws.getNumberHistograms(), len(ws.readE(0))))
    for i in range(ws.getNumberHistograms()):
        d[ds.Data.Value][ds.Dim.Position, i] = ws.readY(i)
        d[ds.Data.Variance][ds.Dim.Position, i] = ws.readE(i)*ws.readE(i)
        
    return d

def ConvertEventWorkspaseToDataset(ws):
    d = ds.Dataset()
    nHist = ws.getNumberHistograms()
    d[ds.Coord.Position] = ([ds.Dim.Position], (nHist,))        
    d[ds.Data.Events] = ([ds.Dim.Position], (nHist,))    
   
    for j, pos in enumerate(d[ds.Coord.Position].data):
        pos[0] = ews.spectrumInfo().position(j).X()
        pos[1] = ews.spectrumInfo().position(j).Y()
        pos[2] = ews.spectrumInfo().position(j).Z()

    for i, e in enumerate(d[ds.Data.Events].data):
        e[ds.Data.Tof] = ([ds.Dim.Event], ws.getSpectrum(i).getTofs())
        pt = ws.getSpectrum(i).getPulseTimes()
        e[ds.Data.PulseTime] = ([ds.Dim.Event], [p.total_nanoseconds() for p in pt])        
    return d
