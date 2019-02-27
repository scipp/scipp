import dataset as ds
from mantid.simpleapi import *

def ConvertWorkspase2DToDataset(ws):
    d = ds.Dataset()
    if ws.getNumberHistograms() == 0:
        return d
    if ws.isCommonBins():
        d[ds.Coord.Tof] = ([ds.Dim.Tof], ws.readX(0))        
    else:
        d[ds.Coord.Tof] = ([ds.Dim.Spectrum, ds.Dim.Tof], (ws.getNumberHistograms(), len(ws.readX(0))))
        for i in range(ws.getNumberHistograms()):
            d[ds.Coord.Tof][ds.Dim.Spectrum, i] = ws.readX(i);
        
    
    d[ds.Data.Value] = ([ds.Dim.Spectrum, ds.Dim.Tof], (ws.getNumberHistograms(), len(ws.readY(0))))     
    d[ds.Data.Variance] = ([ds.Dim.Spectrum, ds.Dim.Tof], (ws.getNumberHistograms(), len(ws.readE(0))))
    for i in range(ws.getNumberHistograms()):
        d[ds.Data.Value][ds.Dim.Spectrum, i] = ws.readY(i)
        d[ds.Data.Variance][ds.Dim.Spectrum, i] = ws.readE()
    return d

def ConvertEventWorkspaseToDataset(ws):
    d = ds.Dataset()
    nHist = ws.getNumberHistograms()
    d[ds.Coord.Position] = ([ds.Dim.Position], (nHist,))        
    d[ds.Data.Events] = ([ds.Dim.Position], (nHist,))    
   
    for i in range(nHist):
        e = ds.Dataset()
        e[ds.Data.Tof] = ([ds.Dim.Event], ws.getSpectrum(i).getTofs())
        pt = ews.getSpectrum(i).getPulseTimes()
        e[ds.Data.PulseTime] = ([ds.Dim.Event], [pt[j].total_nanoseconds() for j in range(len(pt))])        
    return d
