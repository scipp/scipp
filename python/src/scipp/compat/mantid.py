# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock, Neil Vaytet

from .._scipp import core as sc
import numpy as np
from copy import deepcopy


def get_pos(pos):
    return [pos.X(), pos.Y(), pos.Z()]


def make_run(ws):
    return sc.Variable(value=deepcopy(ws.run()))


def make_sample(ws):
    return sc.Variable(value=deepcopy(ws.sample()))


def make_bin_masks(common_bins, dim, num_bins, num_spectra):
    if common_bins:
        return sc.Variable([dim], shape=(num_bins,),
                           dtype=sc.dtype.bool)
    else:
        return sc.Variable([sc.Dim.Spectrum, dim],
                           shape=(num_spectra, num_bins),
                           dtype=sc.dtype.bool)


def make_component_info(ws):
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


def make_detector_info(ws):
    det_info = ws.detectorInfo()
    # det -> spec mapping
    nDet = det_info.size()
    spectrum = sc.Variable([sc.Dim.Detector],
                           shape=(nDet, ),
                           dtype=sc.dtype.int32)
    has_spectrum = sc.Variable([sc.Dim.Detector],
                               values=np.full((nDet, ), False))
    spectrum_ = spectrum.values
    has_spectrum_ = has_spectrum.values
    spec_info = ws.spectrumInfo()
    for i, spec in enumerate(spec_info):
        spec_def = spec.spectrumDefinition
        # This assumes that each detector is part of exactly one spectrum
        for j in range(len(spec_def)):
            det, time = spec_def[j]
            if time != 0:
                raise RuntimeError(
                    "Conversion of Mantid Workspace with scanning instrument "
                    "not supported yet.")
            spectrum_[det] = i
            has_spectrum_[det] = True
    detector = sc.Variable([sc.Dim.Detector], values=det_info.detectorIDs())

    # Remove any information about detectors without data (a spectrum). This
    # mostly just gets in the way and including it in the default converter
    # is probably not required.
    spectrum = sc.filter(spectrum, has_spectrum)
    detector = sc.filter(detector, has_spectrum)

    # May want to include more information here, such as detector positions,
    # but for now this is not necessary.
    return sc.Variable(value=sc.Dataset(coords={sc.Dim.Detector: detector},
                                        labels={'spectrum': spectrum}))


def init_pos_spectrum_no(ws):
    nHist = ws.getNumberHistograms()
    pos = np.zeros([nHist, 3])
    num = np.zeros([nHist], dtype=np.int32)

    spec_info = ws.spectrumInfo()
    for i in range(nHist):
        if spec_info.hasDetectors(i):
            p = spec_info.position(i)
            pos[i, :] = [p.X(), p.Y(), p.Z()]
        else:
            pos[i, :] = [np.nan, np.nan, np.nan]
        num[i] = ws.getSpectrum(i).getSpectrumNo()
    pos = sc.Variable([sc.Dim.Spectrum], values=pos, unit=sc.units.m,
                      dtype=sc.dtype.vector_3_double)
    num = sc.Variable([sc.Dim.Spectrum], values=num)
    return pos, num


def set_common_bins_masks(bin_masks, dim, masked_bins):
    for masked_bin in masked_bins:
        bin_masks[dim, masked_bin].value = True


def set_bin_masks(bin_masks, dim, index, masked_bins):
    for masked_bin in masked_bins:
        bin_masks[sc.Dim.Spectrum, index][dim, masked_bin].value = True


def convert_Workspace2D_to_dataset(ws):
    common_bins = ws.isCommonBins()
    comp_info = make_component_info(ws)
    det_info = make_detector_info(ws)
    pos, num = init_pos_spectrum_no(ws)

    # TODO More cases?
    allowed_units = {
        "DeltaE": [sc.Dim.EnergyTransfer, sc.units.meV],
        "TOF": [sc.Dim.Tof, sc.units.us],
        "Wavelength": [sc.Dim.Wavelength, sc.units.angstrom]
    }
    xunit = ws.getAxis(0).getUnit().unitID()
    if xunit not in allowed_units.keys():
        raise RuntimeError("X-axis unit not currently supported for "
                           "Workspace2D. Possible values are: {}, "
                           "got '{}'. ".format(
                               [k for k in allowed_units.keys()], xunit))
    else:
        [dim, unit] = allowed_units[xunit]

    if common_bins:
        coord = sc.Variable([dim], values=ws.readX(0), unit=unit)
    else:
        coord = sc.Variable([sc.Dim.Spectrum, dim],
                            shape=(ws.getNumberHistograms(), len(ws.readX(0))),
                            unit=unit)
        for i in range(ws.getNumberHistograms()):
            coord[sc.Dim.Spectrum, i].values = ws.readX(i)

    # TODO Use unit information in workspace, if available.
    array = sc.DataArray(data=sc.Variable([sc.Dim.Spectrum, dim],
                                          shape=(ws.getNumberHistograms(),
                                                 len(ws.readY(0))),
                                          unit=sc.units.counts,
                                          variances=True),
                         coords={dim: coord,
                                 sc.Dim.Spectrum: num
                                 },
                         labels={"position": pos,
                                 "component_info": comp_info,
                                 "detector_info": det_info
                                 },
                         attrs={"run": make_run(ws),
                                "sample": make_sample(ws)
                                }
                         )

    spectrum_masks = None
    if ws.detectorInfo().hasMaskedDetectors():
        array.masks["spectrum"] = sc.Variable(
            [sc.Dim.Spectrum],
            shape=(ws.getNumberHistograms(),),
            dtype=sc.dtype.bool)
        spectrum_masks = array.masks["spectrum"]

    if ws.hasAnyMaskedBins():
        array.masks["bin"] = make_bin_masks(common_bins, dim,
                                            ws.blocksize(),
                                            ws.getNumberHistograms())
        bin_masks = array.masks["bin"]
        # set all the bin masks now - they're all the same
        if common_bins:
            set_common_bins_masks(bin_masks, dim, ws.maskedBinsIndices(0))

    data = array.data
    spectrum_info = ws.spectrumInfo()
    for i in range(ws.getNumberHistograms()):
        data[sc.Dim.Spectrum, i].values = ws.readY(i)
        data[sc.Dim.Spectrum, i].variances = np.power(ws.readE(i), 2)

        if spectrum_masks:
            spectrum_masks[sc.Dim.Spectrum,
                           i].value = spectrum_info.isMasked(i)

        if not common_bins and ws.hasMaskedBins(i):
            set_bin_masks(bin_masks, dim, i, ws.maskedBinsIndices(i))

    return array


def convert_EventWorkspace_to_dataset(ws, load_pulse_times):
    from mantid.api import EventType
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
    comp_info = make_component_info(ws)
    det_info = make_detector_info(ws)
    pos, num = init_pos_spectrum_no(ws)

    # TODO Use unit information in workspace, if available.
    coord = sc.Variable([sc.Dim.Spectrum, dim],
                        shape=[nHist, sc.Dimensions.Sparse],
                        unit=unit)
    if load_pulse_times:
        labs = sc.Variable([sc.Dim.Spectrum, dim],
                           shape=[nHist, sc.Dimensions.Sparse])

    # Check for weighted events
    evtp = ws.getSpectrum(0).getEventType()
    contains_weighted_events = ((evtp == EventType.WEIGHTED)
                                or (evtp == EventType.WEIGHTED_NOTIME))
    if contains_weighted_events:
        weights = sc.Variable([sc.Dim.Spectrum, dim],
                              shape=[nHist, sc.Dimensions.Sparse])

    for i in range(nHist):
        sp = ws.getSpectrum(i)
        coord[sc.Dim.Spectrum, i].values = sp.getTofs()
        if load_pulse_times:
            # Pulse times have a Mantid-specific format so the conversion is
            # very slow.
            # TODO: Find a more efficient way to do this.
            pt = sp.getPulseTimes()
            labs[sc.Dim.Spectrum, i].values = np.asarray(
                [p.totalNanoseconds() for p in pt])
        if contains_weighted_events:
            weights[sc.Dim.Spectrum, i].values = sp.getWeights()
            weights[sc.Dim.Spectrum, i].variances = sp.getWeightErrors()

    coords_labs_data = {
        "coords": {
            dim: coord,
            sc.Dim.Spectrum: num
        },
        "labels": {
            "position": pos,
            "component_info": comp_info,
            "detector_info": det_info
        },
        "attrs": {
            "run": make_run(ws),
            "sample": make_sample(ws)
        }
    }
    if load_pulse_times:
        coords_labs_data["labels"]["pulse_times"] = labs
    if contains_weighted_events:
        coords_labs_data["data"] = weights
    return sc.DataArray(**coords_labs_data)


def convert_TableWorkspace_to_dataset(ws, error_connection=None):
    """
    Converts from a Mantid TableWorkspace to a scipp dataset. It is possible
    to assign a column as the error for another column, in which case a
    the data from the two columns will be represented by a single scipp
    variable with variance. This is done using the error_connection Keyword
    argument. The error is transformed to variance in this converter.

    Parameters
    ----------
        ws : Mantid TableWorkspace
            Mantid TableWorkspace to be converted into scipp dataset

    Keyword arguments
    -----------------
        error_connection : Dict
            Dict with data column names as keys to names of their error column
    """

    # Extract information from workspace
    n_columns = ws.columnCount()
    columnNames = ws.getColumnNames()  # list of names matching each column
    columnTypes = ws.columnTypes()  # list of types matching each column

    # Types available in TableWorkspace that can not be loaded into scipp
    blacklist_types = []
    # Types for which the transformation from error to variance will fail
    blacklist_variance_types = ["str"]

    variables = {}
    for i in range(n_columns):
        if columnTypes[i] in blacklist_types:
            continue  # skips loading data of this type

        data_name = columnNames[i]
        if error_connection is None:
            variables[data_name] = sc.Variable([sc.Dim.Row],
                                               values=ws.column(i))
        elif data_name in error_connection:
            # This data has error availble
            error_name = error_connection[data_name]
            error_index = columnNames.index(error_name)

            if columnTypes[error_index] in blacklist_variance_types:
                # Raise error to avoid numpy square error for strings
                raise RuntimeError("Variance can not have type string. \n" +
                                   "Data:     " + str(data_name) + "\n" +
                                   "Variance: " + str(error_name) + "\n")

            variance = np.array(ws.column(error_name))**2
            variables[data_name] = sc.Variable([sc.Dim.Row],
                                               values=np.array(ws.column(i)),
                                               variances=variance)
        elif data_name not in error_connection.values():
            # This data is not an error for another dataset, and has no error
            variables[data_name] = sc.Variable([sc.Dim.Row],
                                               values=ws.column(i))

    return sc.Dataset(variables)  # Return scipp dataset with the variables


def load(filename="",
         load_pulse_times=True,
         instrument_filename=None,
         error_connection=None,
         mantid_args=None):
    """
    Wrapper function to provide a load method for a Nexus file, hiding mantid
    specific code from the scipp interface. All other keyword arguments not
    specified in the parameters below are passed on to the mantid.Load
    function.

    Example of use:

      from scipp.neutron import load
      d = sc.Dataset()
      d["sample"] = load(filename='PG3_4844_event.nxs', \
                         load_pulse_times=True, \
                         mantid_args={'BankName': 'bank184'})

    See also the neutron-data tutorial.

    Note that this function requires mantid to be installed and available in
    the same Python environment as scipp.

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

    try:
        import mantid.simpleapi as mantid
        from mantid.api import Workspace
    except ImportError:
        raise ImportError(
            "Mantid Python API was not found, please install Mantid framework "
            "as detailed in the installation instructions (https://scipp."
            "readthedocs.io/en/latest/getting-started/installation.html)")

    if mantid_args is None:
        mantid_args = {}

    loaded = mantid.Load(filename, StoreInADS=False, **mantid_args)

    # Determine what Load has provided us
    if isinstance(loaded, Workspace):
        # A single workspace
        data_ws = loaded
        monitor_ws = None
    else:
        # Seperate data and monitor workspaces
        data_ws = loaded.OutputWorkspace
        monitor_ws = loaded.MonitorWorkspace

    if instrument_filename is not None:
        mantid.LoadInstrument(data_ws,
                              FileName=instrument_filename,
                              RewriteSpectraMap=True)

    dataset = None
    if data_ws.id() == 'Workspace2D':
        has_monitors = False
        for spec in data_ws.spectrumInfo():
            has_monitors |= spec.isMonitor
            if has_monitors:
                break
        if has_monitors:
            data_ws, monitor_ws = mantid.ExtractMonitors(data_ws,
                                                         StoreInADS=False)
        dataset = convert_Workspace2D_to_dataset(data_ws)
    elif data_ws.id() == 'EventWorkspace':
        dataset = convert_EventWorkspace_to_dataset(data_ws, load_pulse_times)
    elif data_ws.id() == 'TableWorkspace':
        dataset = convert_TableWorkspace_to_dataset(data_ws, error_connection)

    if dataset is None:
        raise RuntimeError('Unsupported workspace type')
    elif monitor_ws is not None:
        if monitor_ws.id() == 'Workspace2D':
            dataset.attrs["monitors"] = sc.Variable(
                value=convert_Workspace2D_to_dataset(monitor_ws))
        elif monitor_ws.id() == 'EventWorkspace':
            dataset.attrs["monitors"] = sc.Variable(
                value=convert_EventWorkspace_to_dataset(monitor_ws,
                                                        load_pulse_times))

    return dataset
