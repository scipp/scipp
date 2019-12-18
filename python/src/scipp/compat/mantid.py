# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock, Neil Vaytet

from .._scipp import core as sc
import numpy as np
from copy import deepcopy
import re


def get_pos(pos):
    return [pos.X(), pos.Y(), pos.Z()]


def make_run(ws):
    return sc.Variable(value=deepcopy(ws.run()))


def make_sample(ws):
    return sc.Variable(value=deepcopy(ws.sample()))


def make_bin_masks(common_bins, spec_dim, dim, num_bins, num_spectra):
    if common_bins:
        return sc.Variable([dim], shape=(num_bins, ), dtype=sc.dtype.bool)
    else:
        return sc.Variable([spec_dim, dim],
                           shape=(num_spectra, num_bins),
                           dtype=sc.dtype.bool)


def make_component_info(ws):
    sourcePos = ws.componentInfo().sourcePosition()
    samplePos = ws.componentInfo().samplePosition()

    def as_var(pos):
        return sc.Variable(value=np.array(get_pos(pos)),
                           dtype=sc.dtype.vector_3_float64,
                           unit=sc.units.m)

    return as_var(sourcePos), as_var(samplePos)


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


def md_dimension(mantid_dim, index):
    # Look for q dimensions
    patterns = ["^q.*{0}$".format(coord) for coord in ['x', 'y', 'z']]
    q_dims = [sc.Dim.Qx, sc.Dim.Qy, sc.Dim.Qz]
    pattern_result = zip(patterns, q_dims)
    if mantid_dim.getMDFrame().isQ():
        for pattern, result in pattern_result:
            if re.search(pattern, mantid_dim.name, re.IGNORECASE):
                return result

    # Look for common/known mantid dimensions
    patterns = ["DeltaE", "T"]
    dims = [sc.Dim.EnergyTransfer, sc.Dim.Temperature]
    pattern_result = zip(patterns, dims)
    for pattern, result in pattern_result:
        if re.search(pattern, mantid_dim.name, re.IGNORECASE):
            return result

    # Look for common spacial dimensions
    patterns = ["^{0}$".format(coord) for coord in ['x', 'y', 'z']]
    dims = [sc.Dim.X, sc.Dim.Y, sc.Dim.Z]
    pattern_result = zip(patterns, dims)
    for pattern, result in pattern_result:
        if re.search(pattern, mantid_dim.name, re.IGNORECASE):
            return result

    raise ValueError(
        "Cannot infer scipp dimension from input mantid dimension {}".format(
            mantid_dim.name()))


def md_unit(frame):
    known_md_units = {
        "Angstrom^-1": sc.units.dimensionless / sc.units.angstrom,
        "r.l.u": sc.units.dimensionless,
        "T": sc.units.K,
        "DeltaE": sc.units.meV
    }
    if frame.getUnitLabel().ascii() in known_md_units:
        return known_md_units[frame.getUnitLabel().ascii()]
    else:
        return sc.units.dimensionless


def validate_and_get_unit(unit):
    known_units = {
        "DeltaE": [sc.Dim.EnergyTransfer, sc.units.meV],
        "TOF": [sc.Dim.Tof, sc.units.us],
        "Wavelength": [sc.Dim.Wavelength, sc.units.angstrom],
        "Energy": [sc.Dim.Energy, sc.units.meV],
        "dSpacing": [sc.Dim.DSpacing, sc.units.angstrom],
        "MomentumTransfer":
        [sc.Dim.Q, sc.units.dimensionless / sc.units.angstrom],
        "QSquared": [
            sc.Dim.QSquared,
            sc.units.dimensionless / (sc.units.angstrom * sc.units.angstrom)
        ],
        "Label": [sc.Dim.Spectrum, sc.units.dimensionless],
    }

    if unit not in known_units.keys():
        raise RuntimeError("Axis unit not currently supported."
                           "Possible values are: {}, "
                           "got '{}'. ".format([k for k in known_units.keys()],
                                               unit))
    else:
        return known_units[unit]


def init_pos(ws):
    nHist = ws.getNumberHistograms()
    pos = np.zeros([nHist, 3])

    spec_info = ws.spectrumInfo()
    for i in range(nHist):
        if spec_info.hasDetectors(i):
            p = spec_info.position(i)
            pos[i, :] = [p.X(), p.Y(), p.Z()]
        else:
            pos[i, :] = [np.nan, np.nan, np.nan]
    return sc.Variable([sc.Dim.Spectrum],
                       values=pos,
                       unit=sc.units.m,
                       dtype=sc.dtype.vector_3_float64)


def init_spec_axis(ws):
    axis = ws.getAxis(1)
    dim, unit = validate_and_get_unit(axis.getUnit().unitID())
    dtype = sc.dtype.int32 if dim == sc.Dim.Spectrum else None
    return dim, sc.Variable([dim],
                            values=axis.extractValues(),
                            unit=unit,
                            dtype=dtype)


def set_common_bins_masks(bin_masks, dim, masked_bins):
    for masked_bin in masked_bins:
        bin_masks[dim, masked_bin].value = True


def set_bin_masks(bin_masks, dim, index, masked_bins):
    for masked_bin in masked_bins:
        bin_masks[sc.Dim.Spectrum, index][dim, masked_bin].value = True


def _convert_MatrixWorkspace_info(ws):
    source_pos, sample_pos = make_component_info(ws)
    det_info = make_detector_info(ws)
    pos = init_pos(ws)
    spec_dim, spec_coord = init_spec_axis(ws)

    info = {
        "coords": {
            spec_dim: spec_coord
        },
        "labels": {
            "position": pos,
            "source_position": source_pos,
            "sample_position": sample_pos,
            "detector_info": det_info
        },
        "masks": {},
        "attrs": {
            "run": make_run(ws),
            "sample": make_sample(ws)
        },
    }

    if ws.detectorInfo().hasMaskedDetectors():
        spectrum_info = ws.spectrumInfo()
        mask = np.array([
            spectrum_info.isMasked(i) for i in range(ws.getNumberHistograms())
        ])
        info["masks"]["spectrum"] = sc.Variable([spec_dim], values=mask)
    return info


def convert_Workspace2D_to_dataarray(ws):
    common_bins = ws.isCommonBins()
    dim, unit = validate_and_get_unit(ws.getAxis(0).getUnit().unitID())
    spec_dim, spec_coord = init_spec_axis(ws)

    if common_bins:
        coord = sc.Variable([dim], values=ws.readX(0), unit=unit)
    else:
        coord = sc.Variable([spec_dim, dim],
                            shape=(ws.getNumberHistograms(), len(ws.readX(0))),
                            unit=unit)
        for i in range(ws.getNumberHistograms()):
            coord[spec_dim, i].values = ws.readX(i)

    coords_labs_data = _convert_MatrixWorkspace_info(ws)
    coords_labs_data["coords"][dim] = coord
    coords_labs_data["data"] = sc.Variable([spec_dim, dim],
                                           shape=(ws.getNumberHistograms(),
                                                  len(ws.readY(0))),
                                           unit=sc.units.counts,
                                           variances=True)
    array = sc.DataArray(**coords_labs_data)

    if ws.hasAnyMaskedBins():
        array.masks["bin"] = make_bin_masks(common_bins, spec_dim, dim,
                                            ws.blocksize(),
                                            ws.getNumberHistograms())
        bin_masks = array.masks["bin"]
        # set all the bin masks now - they're all the same
        if common_bins:
            set_common_bins_masks(bin_masks, dim, ws.maskedBinsIndices(0))

    data = array.data
    for i in range(ws.getNumberHistograms()):
        data[spec_dim, i].values = ws.readY(i)
        data[spec_dim, i].variances = np.power(ws.readE(i), 2)

        if not common_bins and ws.hasMaskedBins(i):
            set_bin_masks(bin_masks, dim, i, ws.maskedBinsIndices(i))

    # Avoid creating dimensions that are not required since this mostly an
    # artifact of inflexible data structures and gets in the way when working
    # with scipp.
    if len(spec_coord.values) == 1:
        array.labels['position'] = array.labels['position'][spec_dim, 0]
        array = array[spec_dim, 0].copy()
    return array


def convertEventWorkspace_to_dataarray(ws, load_pulse_times):
    from mantid.api import EventType

    dim, unit = validate_and_get_unit(ws.getAxis(0).getUnit().unitID())
    spec_dim, spec_coord = init_spec_axis(ws)
    nHist = ws.getNumberHistograms()

    coord = sc.Variable([spec_dim, dim],
                        shape=[nHist, sc.Dimensions.Sparse],
                        unit=unit)
    if load_pulse_times:
        labs = sc.Variable([spec_dim, dim],
                           shape=[nHist, sc.Dimensions.Sparse],
                           dtype=sc.dtype.int64)

    # Check for weighted events
    evtp = ws.getSpectrum(0).getEventType()
    contains_weighted_events = ((evtp == EventType.WEIGHTED)
                                or (evtp == EventType.WEIGHTED_NOTIME))
    if contains_weighted_events:
        weights = sc.Variable([spec_dim, dim],
                              shape=[nHist, sc.Dimensions.Sparse])

    for i in range(nHist):
        sp = ws.getSpectrum(i)
        coord[spec_dim, i].values = sp.getTofs()
        if load_pulse_times:
            labs[spec_dim, i].values = \
                sp.getPulseTimesAsNumpy().astype(np.int64)
        if contains_weighted_events:
            weights[spec_dim, i].values = sp.getWeights()
            weights[spec_dim, i].variances = sp.getWeightErrors()

    coords_labs_data = _convert_MatrixWorkspace_info(ws)
    coords_labs_data["coords"][dim] = coord

    if load_pulse_times:
        coords_labs_data["labels"]["pulse_times"] = labs
    if contains_weighted_events:
        coords_labs_data["data"] = weights
    return sc.DataArray(**coords_labs_data)


def convertMDHistoWorkspace_to_dataset(md_histo):
    ndims = md_histo.getNumDims()
    coords = dict()
    dims_used = []
    for i in range(ndims):
        dim = md_histo.getDimension(i)
        frame = dim.getMDFrame()
        sc_dim = md_dimension(dim, i)
        coords[sc_dim] = sc.Variable(dims=[sc_dim],
                                     values=np.linspace(
                                         dim.getMinimum(), dim.getMaximum(),
                                         dim.getNBins()),
                                     unit=md_unit(frame))
        dims_used.append(sc_dim)
    data = sc.Variable(dims=dims_used,
                       values=md_histo.getSignalArray(),
                       variances=md_histo.getErrorSquaredArray(),
                       unit=sc.units.counts)
    nevents = sc.Variable(dims=dims_used, values=md_histo.getNumEventsArray())
    return sc.DataArray(coords=coords, data=data, attrs={'nevents': nevents})


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

    .. highlight:: python
    .. code-block:: python

        from scipp.neutron import load
        d = sc.Dataset()
        d["sample"] = load(filename='PG3_4844_event.nxs',
                           load_pulse_times=False,
                           mantid_args={'BankName': 'bank184',
                                        'LoadMonitors': True})

    See also the neutron-data tutorial.

    Note that this function requires mantid to be installed and available in
    the same Python environment as scipp.

    :param str filename: The name of the Nexus/HDF file to be loaded.
    :param bool load_pulse_times: Read the pulse times if True.
    :param str instrument_filename: If specified, over-write the instrument
                                    definition in the final Dataset with the
                                    geometry contained in the file.
    :param dict mantid_args: Dict of keyword arguments to forward to Mantid.
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
    if data_ws.id() == 'Workspace2D' or data_ws.id() == 'RebinnedOutput':
        has_monitors = False
        for spec in data_ws.spectrumInfo():
            has_monitors |= spec.isMonitor
            if has_monitors:
                break
        if has_monitors:
            data_ws, monitor_ws = mantid.ExtractMonitors(data_ws,
                                                         StoreInADS=False)
        dataset = convert_Workspace2D_to_dataarray(data_ws)
    elif data_ws.id() == 'EventWorkspace':
        dataset = convertEventWorkspace_to_dataarray(data_ws, load_pulse_times)
    elif data_ws.id() == 'TableWorkspace':
        dataset = convert_TableWorkspace_to_dataset(data_ws, error_connection)
    elif data_ws.id() == 'MDHistoWorkspace':
        dataset = convertMDHistoWorkspace_to_dataset(data_ws)

    if dataset is None:
        raise RuntimeError('Unsupported workspace type {}'.format(
            data_ws.id()))
    if monitor_ws is not None:
        if monitor_ws.id() == 'Workspace2D':
            dataset.attrs["monitors"] = sc.Variable(
                value=convert_Workspace2D_to_dataarray(monitor_ws))
        elif monitor_ws.id() == 'EventWorkspace':
            dataset.attrs["monitors"] = sc.Variable(
                value=convertEventWorkspace_to_dataarray(
                    monitor_ws, load_pulse_times))
        # Remove some redundant information that is duplicated from data_ws
        mon = dataset.attrs["monitors"].value
        del mon.labels['sample_position']
        del mon.labels['detector_info']
        del mon.attrs['run']
        del mon.attrs['sample']

    return dataset
