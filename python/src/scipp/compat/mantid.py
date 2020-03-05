# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock, Neil Vaytet

import re
from copy import deepcopy
from contextlib import contextmanager

import numpy as np

from .. import detail
from .._scipp import core as sc


@contextmanager
def run_mantid_alg(alg, *args, **kwargs):
    try:
        from mantid import simpleapi as mantid
        from mantid.api import AnalysisDataService
    except ImportError:
        raise ImportError(
            "Mantid Python API was not found, please install Mantid framework "
            "as detailed in the installation instructions (https://scipp."
            "github.io/getting-started/installation.html)")
    # Deal with multiple calls to this function, which may have conflicting
    # names in the global AnalysisDataService.
    run_mantid_alg.workspace_id += 1
    ws_name = f'scipp.run_mantid_alg.{run_mantid_alg.workspace_id}'
    # Deal with non-standard ways to define the prefix of output workspaces
    if alg == 'Fit':
        kwargs['Output'] = ws_name
    elif alg == 'LoadDiffCal':
        kwargs['WorkspaceName'] = ws_name
    else:
        kwargs['OutputWorkspace'] = ws_name
    ws = getattr(mantid, alg)(*args, **kwargs)
    try:
        yield ws
    finally:
        for name in AnalysisDataService.Instance().getObjectNames():
            if name.startswith(ws_name):
                mantid.DeleteWorkspace(name)


run_mantid_alg.workspace_id = 0


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
    component_info = ws.componentInfo()

    if component_info.hasSource():
        sourcePos = component_info.sourcePosition()
    else:
        sourcePos = None

    if component_info.hasSample():
        samplePos = component_info.samplePosition()
    else:
        samplePos = None

    def as_var(pos):
        if pos is None:
            return pos
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
        "Empty": [sc.Dim.Spectrum, sc.units.dimensionless]
    }

    if unit not in known_units.keys():
        raise RuntimeError("Axis unit not currently supported."
                           "Possible values are: {}, "
                           "got '{}'. ".format([k for k in known_units.keys()],
                                               unit))
    else:
        return known_units[unit]


def _to_array(positions, idx, pos, spec_idx):
    positions[idx, :] = np.array([spec_idx, pos.X(), pos.Y(), pos.Z()])


def _to_spherical(input):
    transformed = np.ones(shape=input.shape)
    transformed[:, 0] = input[:, 0]  # copy metadata
    transformed[:, 1] = np.sqrt(np.sum(input[:, 1:]**2, axis=1))  # r
    transformed[:, 2] = np.arccos(input[:, 3] /
                                  transformed[:, 1])  # arccos ( z / r)
    transformed[:, 3] = np.arctan2(input[:, 2], input[:, 1])  # arctan2(y, x)
    return transformed


def rotation_matrix_from_vectors(vec1, vec2):
    """ Find the rotation matrix that aligns vec1 to vec2
    :param vec1: A 3d "source" vector
    :param vec2: A 3d "destination" vector
    :return mat: A transform matrix (3x3) which when applied to vec1, aligns it with vec2.
    """
    a, b = (vec1 /
            np.linalg.norm(vec1)).reshape(3), (vec2 /
                                               np.linalg.norm(vec2)).reshape(3)
    v = np.cross(a, b)
    c = np.dot(a, b)
    s = np.linalg.norm(v)
    if s == 0:
        return np.eye(3)
    kmat = np.array([[0, -v[2], v[1]], [v[2], 0, -v[0]], [-v[1], v[0], 0]])
    rotation_matrix = np.eye(3) + kmat + kmat.dot(kmat) * ((1 - c) / (s**2))
    return rotation_matrix


def init_pos(ws, source_pos, sample_pos):
    from scipp import Dim
    import numpy as np
    spec_info = ws.spectrumInfo()
    det_info = ws.detectorInfo()
    total_detectors = spec_info.detectorCount()
    non_detectors = sum(not spec.hasDetectors for spec in spec_info)
    det_pos = np.zeros([total_detectors, 3 + 1])
    no_detector_spectrum = np.full((non_detectors, 3 + 1), np.nan)

    if sample_pos and source_pos:
        act_beam = (sample_pos - source_pos).values
        rot = rotation_matrix_from_vectors(act_beam, [0, 0, 1])
        inv_rot = rot.transpose()
    else:
        rot = np.eye(3)
        inv_rot = rot

    idx, idx_orphaned = 0, 0
    for i, spec in enumerate(spec_info):
        if spec.hasDetectors:
            definition = spec_info.getSpectrumDefinition(i)
            n_dets = len(definition)
            for j in range(n_dets):
                det_idx = definition[j][0]
                p = det_info.position(det_idx)
                _to_array(det_pos, idx, p, i)
                idx += 1
        else:
            no_detector_spectrum[idx_orphaned, 0] = i
            idx_orphaned += 1
    det_pos[:, 1:] = det_pos[:, 1:].dot(rot)
    spherical_ = _to_spherical(det_pos)

    pos_d = sc.Dataset()
    pos_d.labels["spectrum_idx"] = sc.Variable([Dim.X],
                                               values=spherical_[:, 0])
    pos_d["spectrum_idx"] = sc.Variable([Dim.X], values=spherical_[:, 0])
    pos_d["r"] = sc.Variable([Dim.X], values=spherical_[:, 1], unit=sc.units.m)
    pos_d["t"] = sc.Variable([Dim.X],
                             values=spherical_[:, 2],
                             unit=sc.units.rad)
    pos_d["p"] = sc.Variable([Dim.X],
                             values=spherical_[:, 3],
                             unit=sc.units.rad)

    grouping = sc.groupby(pos_d, "spectrum_idx", Dim.Y)
    averaged = grouping.mean(Dim.X)
    spec_nos = grouping.min(Dim.X)  # Keep spectrum numbers
    averaged["spectrum_idx"] = spec_nos[
        "spectrum_idx"]  # All data averaged apart from spectrum numbers

    other = sc.Dataset()
    other.coords[Dim.Y] = sc.Variable([Dim.Y],
                                      values=np.arange(
                                          len(no_detector_spectrum)),
                                      dtype=np.float)
    other["spectrum_idx"] = sc.Variable([Dim.Y],
                                        values=no_detector_spectrum[:, 0])
    other["r"] = sc.Variable([Dim.Y],
                             values=np.array([np.nan] *
                                             len(no_detector_spectrum)),
                             unit=sc.units.m)
    other["t"] = sc.Variable([Dim.Y],
                             values=np.array([np.nan] *
                                             len(no_detector_spectrum)),
                             unit=sc.units.rad)
    other["p"] = sc.Variable([Dim.Y],
                             values=np.array([np.nan] *
                                             len(no_detector_spectrum)),
                             unit=sc.units.rad)

    averaged = sc.concatenate(averaged, other, Dim.Y)
    # TODO. We should sort by spectrum_idx here. but would need to be label/coord?

    averaged["x"] = averaged["r"].data * sc.sin(averaged["t"].data) * sc.cos(
        averaged["p"].data)
    averaged["y"] = averaged["r"].data * sc.sin(averaged["t"].data) * sc.sin(
        averaged["p"].data)
    averaged["z"] = averaged["r"].data * sc.cos(averaged["t"].data)

    pos = np.empty(shape=(averaged.shape[0], 3))
    pos[:, 0] = averaged["x"].values
    pos[:, 1] = averaged["y"].values
    pos[:, 2] = averaged["z"].values
    pos = pos.dot(inv_rot)
    return sc.Variable(['spectrum'],
                       values=pos,
                       unit=sc.units.m,
                       dtype=sc.dtype.vector_3_float64)


def _get_dtype_from_values(values):
    if hasattr(values, 'dtype'):
        dtype = values.dtype
    else:
        if len(values) > 0:
            dtype = type(values[0])
            if dtype is str:
                dtype = sc.dtype.string
            elif dtype is int:
                dtype = sc.dtype.int64
            elif dtype is float:
                dtype = sc.dtype.float64
            else:
                raise RuntimeError("Cannot handle the dtype that this "
                                   "workspace has on Axis 1.")
        else:
            raise RuntimeError("Axis 1 of this workspace has no values. "
                               "Cannot determine dtype.")
    return dtype


def init_spec_axis(ws):
    axis = ws.getAxis(1)
    dim, unit = validate_and_get_unit(axis.getUnit().unitID())
    values = axis.extractValues()
    dtype = _get_dtype_from_values(values)
    return dim, sc.Variable([dim], values=values, unit=unit, dtype=dtype)


def set_common_bins_masks(bin_masks, dim, masked_bins):
    for masked_bin in masked_bins:
        bin_masks[dim, masked_bin].value = True


def set_bin_masks(bin_masks, dim, index, masked_bins):
    for masked_bin in masked_bins:
        bin_masks[sc.Dim.Spectrum, index][dim, masked_bin].value = True


def _convert_MatrixWorkspace_info(ws):
    source_pos, sample_pos = make_component_info(ws)
    det_info = make_detector_info(ws)
    pos = init_pos(ws, source_pos, sample_pos)
    spec_dim, spec_coord = init_spec_axis(ws)

    info = {
        "coords": {
            spec_dim: spec_coord
        },
        "labels": {
            "position": pos,
            "detector_info": det_info
        },
        "masks": {},
        "attrs": {
            "run": make_run(ws),
            "sample": make_sample(ws)
        },
    }
    if source_pos is not None:
        info["labels"]["source_position"] = source_pos

    if sample_pos is not None:
        info["labels"]["sample_position"] = sample_pos

    if ws.detectorInfo().hasMaskedDetectors():
        spectrum_info = ws.spectrumInfo()
        mask = np.array([
            spectrum_info.isMasked(i) for i in range(ws.getNumberHistograms())
        ])
        info["masks"]["spectrum"] = sc.Variable([spec_dim], values=mask)
    return info


def convert_monitors_ws(ws, converter, **ignored):
    dim, unit = validate_and_get_unit(ws.getAxis(0).getUnit().unitID())
    spec_dim, spec_coord = init_spec_axis(ws)
    spec_info = spec_info = ws.spectrumInfo()
    comp_info = ws.componentInfo()
    monitors = []
    indexes = (ws.getIndexFromSpectrumNumber(int(i))
               for i in spec_coord.values)
    for index in indexes:
        definition = spec_info.getSpectrumDefinition(index)
        if not definition.size() == 1:
            raise RuntimeError("Cannot deal with grouped monitor detectors")
        det_index = definition[0][0]  # Ignore time index
        # We only ExtractSpectra for compability with
        # exising convert_Workspace2D_to_dataarray. This could instead be
        # refactored if found to be slow
        with run_mantid_alg('ExtractSpectra',
                            InputWorkspace=ws,
                            WorkspaceIndexList=[index]) as monitor_ws:
            single_monitor = converter(monitor_ws)
        # Remove redundant information that is duplicated from workspace
        # We get this extra information from the generic converter reuse
        del single_monitor.labels['sample_position']
        del single_monitor.labels['detector_info']
        del single_monitor.attrs['run']
        del single_monitor.attrs['sample']
        monitors.append((comp_info.name(det_index), single_monitor))
    return monitors


def convert_Workspace2D_to_data_array(ws, **ignored):
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
    array = detail.move_to_data_array(**coords_labs_data)

    if ws.hasAnyMaskedBins():
        array.masks["bin"] = detail.move(
            make_bin_masks(common_bins, spec_dim, dim, ws.blocksize(),
                           ws.getNumberHistograms()))
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


def convert_EventWorkspace_to_data_array(ws, load_pulse_times=True, **ignored):
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
            labs[spec_dim, i].values = sp.getPulseTimesAsNumpy()
        if contains_weighted_events:
            weights[spec_dim, i].values = sp.getWeights()
            weights[spec_dim, i].variances = sp.getWeightErrors()

    coords_labs_data = _convert_MatrixWorkspace_info(ws)
    coords_labs_data["coords"][dim] = coord

    if load_pulse_times:
        coords_labs_data["labels"]["pulse_times"] = labs
    if contains_weighted_events:
        coords_labs_data["data"] = weights
    return detail.move_to_data_array(**coords_labs_data)


def convert_MDHistoWorkspace_to_data_array(md_histo, **ignored):
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
    return detail.move_to_data_array(coords=coords,
                                     data=data,
                                     attrs={'nevents': nevents})


def convert_TableWorkspace_to_dataset(ws, error_connection=None, **ignored):
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

    dataset = sc.Dataset()
    for i in range(n_columns):
        if columnTypes[i] in blacklist_types:
            continue  # skips loading data of this type

        data_name = columnNames[i]
        if error_connection is None:
            dataset[data_name] = detail.move(
                sc.Variable([sc.Dim.Row], values=ws.column(i)))
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
            dataset[data_name] = detail.move(
                sc.Variable([sc.Dim.Row],
                            values=np.array(ws.column(i)),
                            variances=variance))
        elif data_name not in error_connection.values():
            # This data is not an error for another dataset, and has no error
            dataset[data_name] = detail.move(
                sc.Variable([sc.Dim.Row], values=ws.column(i)))

    return dataset


def from_mantid(workspace, **kwargs):
    """Convert Mantid workspace to a scipp data array or dataset
    :param workspace: Mantid workspace to convert.
    """
    scipp_obj = None  # This is either a Dataset or DataArray
    monitor_ws = None
    workspaces_to_delete = []
    if workspace.id() == 'Workspace2D' or workspace.id() == 'RebinnedOutput':
        has_monitors = False
        for spec in workspace.spectrumInfo():
            has_monitors |= spec.isMonitor
            if has_monitors:
                break
        if has_monitors:
            import mantid.simpleapi as mantid
            workspace, monitor_ws = mantid.ExtractMonitors(workspace)
            workspaces_to_delete.append(workspace)
            workspaces_to_delete.append(monitor_ws)
        scipp_obj = convert_Workspace2D_to_data_array(workspace, **kwargs)
    elif workspace.id() == 'EventWorkspace':
        scipp_obj = convert_EventWorkspace_to_data_array(workspace, **kwargs)
    elif workspace.id() == 'TableWorkspace':
        scipp_obj = convert_TableWorkspace_to_dataset(workspace, **kwargs)
    elif workspace.id() == 'MDHistoWorkspace':
        scipp_obj = convert_MDHistoWorkspace_to_data_array(workspace, **kwargs)

    if scipp_obj is None:
        raise RuntimeError('Unsupported workspace type {}'.format(
            workspace.id()))

    # TODO Is there ever a case where a Workspace2D has a separate monitor
    # workspace? This is not handled by ExtractMonitors above, I think.
    if monitor_ws is None:
        if hasattr(workspace, 'getMonitorWorkspace'):
            try:
                monitor_ws = workspace.getMonitorWorkspace()
            except RuntimeError:
                # Have to try/fail here. No inspect method on Mantid for this.
                pass

    if monitor_ws is not None:
        if monitor_ws.id() == 'Workspace2D':
            converter = convert_Workspace2D_to_data_array
        elif monitor_ws.id() == 'EventWorkspace':
            converter = convert_EventWorkspace_to_data_array

        monitors = convert_monitors_ws(monitor_ws, converter, **kwargs)
        for name, monitor in monitors:
            scipp_obj.attrs[name] = detail.move(sc.Variable(value=monitor))
    for ws in workspaces_to_delete:
        mantid.DeleteWorkspace(ws)

    return scipp_obj


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

    if mantid_args is None:
        mantid_args = {}

    with run_mantid_alg('Load', filename, **mantid_args) as loaded:
        # Determine what Load has provided us
        from mantid.api import Workspace
        if isinstance(loaded, Workspace):
            # A single workspace
            data_ws = loaded
        else:
            # Seperate data and monitor workspaces
            data_ws = loaded.OutputWorkspace

        if instrument_filename is not None:
            import mantid.simpleapi as mantid
            mantid.LoadInstrument(data_ws,
                                  FileName=instrument_filename,
                                  RewriteSpectraMap=True)

        return from_mantid(data_ws,
                           load_pulse_times=load_pulse_times,
                           error_connection=error_connection)


def load_component_info(ds, file):
    """
    Adds the component info labels into the dataset. The following are added:

    - source_position
    - sample_position
    - position

    :param ds: Dataset on which the component info will be added as labels.
    :param file: File from which the IDF will be loaded.
                 This can be anything that mantid.Load can load.
    """
    with run_mantid_alg('Load', file) as ws:
        source_pos, sample_pos = make_component_info(ws)

        ds.labels["source_position"] = source_pos
        ds.labels["sample_position"] = sample_pos
        ds.labels["position"] = init_pos(ws, source_pos, sample_pos)


def validate_dim_and_get_mantid_string(unit_dim):
    known_units = {
        sc.Dim.EnergyTransfer: "DeltaE",
        sc.Dim.Tof: "TOF",
        sc.Dim.Wavelength: "Wavelength",
        sc.Dim.Energy: "Energy",
        sc.Dim.DSpacing: "dSpacing",
        sc.Dim.Q: "MomentumTransfer",
        sc.Dim.QSquared: "QSquared",
    }

    if unit_dim not in known_units.keys():
        raise RuntimeError("Axis unit not currently supported."
                           "Possible values are: {}, "
                           "got '{}'. ".format([k for k in known_units.keys()],
                                               unit_dim))
    else:
        return known_units[unit_dim]


def to_workspace_2d(x, y, e, coord_dim, instrument_file=None):
    """
    Use the values provided to create a Mantid workspace.

    The Mantid layout expect the spectra to be the Outer-most dimension,
    i.e. y.shape[0]. If that is not the case you might have to transpose
    your data to fit that, otherwise it will not be aligned correctly in the
    Mantid workspace.

    :param x: Data to be used as X for the Mantid workspace.
    :param y: Data to be used as Y for the Mantid workspace.
    :param e: Data to be used as error for the Mantid workspace.
              If `None` the np.sqrt of y will be used.
    :param coord_dim: Dim of the coordinate, to be set as the equivalent
                      UnitX on the Mantid workspace.
    :param instrument_file: Instrument file that will be
                            loaded into the workspace
    :returns: Workspace2D containing the data for X, Y and E
    """
    try:
        import mantid.simpleapi as mantid
    except ImportError:
        raise ImportError(
            "Mantid Python API was not found, please install Mantid framework "
            "as detailed in the installation instructions (https://scipp."
            "github.io/getting-started/installation.html)")

    assert len(y.shape) == 2, "Currently can only handle 2D data."

    e = e if e is not None else np.sqrt(y)

    unitX = validate_dim_and_get_mantid_string(coord_dim)

    nspec = y.shape[0]
    nbins = x.shape[1]
    nitems = y.shape[1]

    ws = mantid.WorkspaceFactory.create("Workspace2D",
                                        NVectors=nspec,
                                        XLength=nbins,
                                        YLength=nitems)

    for i in range(nspec):
        ws.setX(i, x[i])
        ws.setY(i, y[i])
        ws.setE(i, e[i])

    # Set X-Axis unit
    ws.getAxis(0).setUnit(unitX)

    if instrument_file is not None:
        mantid.LoadInstrument(ws,
                              FileName=instrument_file,
                              RewriteSpectraMap=True)

    return ws


def fit(ws, function, workspace_index, start_x, end_x):
    """
    Performs a fit on the workspace.

    :param ws: The workspace on which the fit will be performed
    :param function: The function used for the fit. This is anything
                     that mantid.Fit's Function parameter can handle.
    :param workspace_index: Workspace index which will be fitted.
    :param start_x: Start X for the fit
    :param end_x: End X for the fit
    :returns: Dataset containing all of Fit's outputs
    """
    with run_mantid_alg('Fit',
                        Function=function,
                        InputWorkspace=ws,
                        WorkspaceIndex=workspace_index,
                        StartX=start_x,
                        EndX=end_x,
                        CreateOutput=True) as fit:
        ds = sc.Dataset(data={
            'workspace':
            sc.Variable(convert_Workspace2D_to_data_array(
                fit.OutputWorkspace)),
            'parameters':
            sc.Variable(convert_TableWorkspace_to_dataset(
                fit.OutputParameters)),
            'normalised_covariance_matrix':
            sc.Variable(
                convert_TableWorkspace_to_dataset(
                    fit.OutputNormalisedCovarianceMatrix)),
        },
                        attrs={
                            'status': sc.Variable(fit.OutputStatus),
                            'chi2_over_DoF':
                            sc.Variable(fit.OutputChi2overDoF),
                            'function': sc.Variable(str(fit.Function)),
                            'cost_function': sc.Variable(fit.CostFunction)
                        })

        return ds
