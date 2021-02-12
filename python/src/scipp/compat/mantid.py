# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock, Neil Vaytet

import re
from copy import deepcopy
from contextlib import contextmanager
import uuid
import warnings

import numpy as np

from .. import detail
from .._utils import is_data_array
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
    # names in the global AnalysisDataService by using uuid.
    ws_name = f'scipp.run_mantid_alg.{uuid.uuid4()}'
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


def get_pos(pos):
    return [pos.X(), pos.Y(), pos.Z()]


def make_run(ws):
    return sc.Variable(value=deepcopy(ws.run()))


additional_unit_mapping = {
    " ": sc.units.one,
    "none": sc.units.one,
}


def make_variables_from_run_logs(ws):
    for property_name in ws.run().keys():
        units_string = ws.run()[property_name].units
        try:
            unit = additional_unit_mapping.get(units_string,
                                               sc.Unit(units_string))
        except RuntimeError:  # TODO catch UnitError once exposed from C++
            # Parsing unit string failed
            unit = None

        values = deepcopy(ws.run()[property_name].value)

        if units_string and unit is None:
            warnings.warn(f"Workspace run log '{property_name}' "
                          f"has unrecognised units: '{units_string}'")
        if unit is None:
            unit = sc.units.one

        try:
            times = deepcopy(ws.run()[property_name].times)
            is_time_series = True
            dimension_label = "time"
        except AttributeError:
            times = None
            is_time_series = False
            dimension_label = property_name

        if np.isscalar(values):
            property_data = sc.Variable(value=values, unit=unit)
        else:
            property_data = sc.Variable(values=values,
                                        unit=unit,
                                        dims=[dimension_label])

        if is_time_series:
            # If property has timestamps, create a DataArray
            data_array = sc.DataArray(data=property_data,
                                      coords={
                                          dimension_label:
                                          sc.Variable([dimension_label],
                                                      values=times,
                                                      dtype=sc.dtype.int64,
                                                      unit=sc.units.ns)
                                      })
            yield property_name, sc.Variable(value=data_array)
        elif not np.isscalar(values):
            # If property is multi-valued, create a wrapper single
            # value variable. This prevents interference with
            # global dimensions for for output Dataset.
            yield property_name, sc.Variable(value=property_data)
        else:
            yield property_name, property_data


def make_sample(ws):
    return sc.Variable(value=deepcopy(ws.sample()))


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
    spectrum = sc.Variable(['detector'], shape=(nDet, ), dtype=sc.dtype.int32)
    has_spectrum = sc.Variable(['detector'], values=np.full((nDet, ), False))
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
    detector = sc.Variable(['detector'], values=det_info.detectorIDs())

    # Remove any information about detectors without data (a spectrum). This
    # mostly just gets in the way and including it in the default converter
    # is probably not required.
    spectrum = sc.filter(spectrum, has_spectrum)
    detector = sc.filter(detector, has_spectrum)

    # May want to include more information here, such as detector positions,
    # but for now this is not necessary.

    return sc.Variable(value=sc.Dataset(coords={
        'detector': detector,
        'spectrum': spectrum
    }))


def md_dimension(mantid_dim, index):
    # Look for q dimensions
    patterns = ["^q.*{0}$".format(coord) for coord in ['x', 'y', 'z']]
    q_dims = ['Q_x', 'Q_y', 'Q_z']
    pattern_result = zip(patterns, q_dims)
    if mantid_dim.getMDFrame().isQ():
        for pattern, result in pattern_result:
            if re.search(pattern, mantid_dim.name, re.IGNORECASE):
                return result

    # Look for common/known mantid dimensions
    patterns = ["DeltaE", "T"]
    dims = ['energy-transfer', 'temperature']
    pattern_result = zip(patterns, dims)
    for pattern, result in pattern_result:
        if re.search(pattern, mantid_dim.name, re.IGNORECASE):
            return result

    # Look for common spacial dimensions
    patterns = ["^{0}$".format(coord) for coord in ['x', 'y', 'z']]
    dims = ['x', 'y', 'z']
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


def validate_and_get_unit(unit, allow_empty=False):
    if hasattr(unit, 'unitID'):
        if unit.unitID() == 'Label':
            unit = unit.name()
        else:
            unit = unit.unitID()
    known_units = {
        "DeltaE": ['energy-transfer', sc.units.meV],
        "TOF": ['tof', sc.units.us],
        "Wavelength": ['wavelength', sc.units.angstrom],
        "Energy": ['energy', sc.units.meV],
        "dSpacing": ['d-spacing', sc.units.angstrom],
        "MomentumTransfer": ['Q', sc.units.dimensionless / sc.units.angstrom],
        "QSquared": [
            'Q^2',
            sc.units.dimensionless / (sc.units.angstrom * sc.units.angstrom)
        ],
        "Spectrum": ['spectrum', sc.units.dimensionless],
        "Empty": ['empty', sc.units.dimensionless],
        "Counts": ['counts', sc.units.counts]
    }

    if unit not in known_units.keys():
        return [str(unit), sc.units.dimensionless]
    else:
        return known_units[unit]


def _to_spherical(pos, output):
    output["r"] = sc.sqrt(sc.dot(pos, pos))
    output["t"] = sc.acos(sc.geometry.z(pos) / output["r"].data)
    output["p-sign"] = sc.atan2(sc.geometry.y(pos), sc.geometry.x(pos))
    output["p-delta"] = sc.Variable(value=np.pi, unit=sc.units.rad) - sc.abs(
        output["p-sign"].data)


def _rot_from_vectors(vec1, vec2):
    a = sc.Variable(value=vec1 / np.linalg.norm(vec1),
                    dtype=sc.dtype.vector_3_float64)
    b = sc.Variable(value=vec2 / np.linalg.norm(vec2),
                    dtype=sc.dtype.vector_3_float64)
    c = sc.Variable(value=np.cross(a.value, b.value),
                    dtype=sc.dtype.vector_3_float64)
    angle = sc.acos(sc.dot(a, b)).value
    q = sc.rotation_matrix_from_quaternion_coeffs(
        list(c.value * np.sin(angle / 2)) + [np.cos(angle / 2)])
    return sc.Variable(value=q)


def get_detector_pos(ws):
    nHist = ws.getNumberHistograms()
    pos = np.zeros([nHist, 3])

    spec_info = ws.spectrumInfo()
    for i in range(nHist):
        if spec_info.hasDetectors(i):
            p = spec_info.position(i)
            pos[i, 0] = p.X()
            pos[i, 1] = p.Y()
            pos[i, 2] = p.Z()
        else:
            pos[i, :] = [np.nan, np.nan, np.nan]
    return sc.Variable(['spectrum'],
                       values=pos,
                       unit=sc.units.m,
                       dtype=sc.dtype.vector_3_float64)


def get_detector_properties(ws,
                            source_pos,
                            sample_pos,
                            advanced_geometry=False):
    if not advanced_geometry:
        return (get_detector_pos(ws), None, None)
    spec_info = ws.spectrumInfo()
    det_info = ws.detectorInfo()
    comp_info = ws.componentInfo()
    nspec = len(spec_info)
    det_rot = np.zeros([nspec, 3, 3])
    det_bbox = np.zeros([nspec, 3])

    if sample_pos is not None and source_pos is not None:
        total_detectors = spec_info.detectorCount()
        act_beam = (sample_pos - source_pos).values
        rot = _rot_from_vectors(act_beam, [0, 0, 1])
        inv_rot = _rot_from_vectors([0, 0, 1], act_beam)

        pos_d = sc.Dataset()
        # Create empty to hold position info for all spectra detectors
        pos_d["x"] = sc.Variable(["detector"],
                                 shape=[total_detectors],
                                 unit=sc.units.m)
        pos_d["y"] = pos_d["x"]
        pos_d["z"] = pos_d["x"]
        pos_d.coords["spectrum"] = sc.Variable(
            ["detector"], values=np.empty(total_detectors))

        spectrum_values = pos_d.coords["spectrum"].values

        x_values = pos_d["x"].values
        y_values = pos_d["y"].values
        z_values = pos_d["z"].values

        idx = 0
        for i, spec in enumerate(spec_info):
            if spec.hasDetectors:
                definition = spec_info.getSpectrumDefinition(i)
                n_dets = len(definition)
                quats = []
                bboxes = []
                for j in range(n_dets):
                    det_idx = definition[j][0]
                    p = det_info.position(det_idx)
                    r = det_info.rotation(det_idx)
                    s = comp_info.shape(det_idx)
                    spectrum_values[idx] = i
                    x_values[idx] = p.X()
                    y_values[idx] = p.Y()
                    z_values[idx] = p.Z()
                    idx += 1
                    quats.append(
                        np.array([r.imagI(),
                                  r.imagJ(),
                                  r.imagK(),
                                  r.real()]))
                    bboxes.append(s.getBoundingBox().width())
                det_rot[i, :] = sc.rotation_matrix_from_quaternion_coeffs(
                    np.mean(quats, axis=0))
                det_bbox[i, :] = np.sum(bboxes, axis=0)

        rot_pos = rot * sc.geometry.position(pos_d["x"].data, pos_d["y"].data,
                                             pos_d["z"].data)

        _to_spherical(rot_pos, pos_d)

        averaged = sc.groupby(pos_d,
                              "spectrum",
                              bins=sc.Variable(["spectrum"],
                                               values=np.arange(
                                                   -0.5,
                                                   len(spec_info) + 0.5,
                                                   1.0))).mean("detector")

        averaged["p-delta"].values *= np.sign(averaged["p-sign"].values)
        averaged["p"] = sc.Variable(
            value=np.pi, unit=sc.units.rad) + averaged["p-delta"].data
        averaged["x"] = averaged["r"].data * sc.sin(
            averaged["t"].data) * sc.cos(averaged["p"].data)
        averaged["y"] = averaged["r"].data * sc.sin(
            averaged["t"].data) * sc.sin(averaged["p"].data)
        averaged["z"] = averaged["r"].data * sc.cos(averaged["t"].data)

        pos = sc.geometry.position(averaged["x"].data, averaged["y"].data,
                                   averaged["z"].data)

        return (inv_rot * pos,
                sc.Variable(['spectrum'],
                            values=det_rot,
                            dtype=sc.dtype.matrix_3_float64),
                sc.Variable(['spectrum'],
                            values=det_bbox,
                            unit=sc.units.m,
                            dtype=sc.dtype.vector_3_float64))
    else:
        pos = np.zeros([nspec, 3])

        for i, spec in enumerate(spec_info):
            if spec.hasDetectors:
                definition = spec_info.getSpectrumDefinition(i)
                n_dets = len(definition)
                vec3s = []
                quats = []
                bboxes = []
                for j in range(n_dets):
                    det_idx = definition[j][0]
                    p = det_info.position(det_idx)
                    r = det_info.rotation(det_idx)
                    s = comp_info.shape(det_idx)
                    vec3s.append([p.X(), p.Y(), p.Z()])
                    quats.append(
                        np.array([r.imagI(),
                                  r.imagJ(),
                                  r.imagK(),
                                  r.real()]))
                    bboxes.append(s.getBoundingBox().width())
                pos[i, :] = np.mean(vec3s, axis=0)
                det_rot[i, :] = sc.rotation_matrix_from_quaterion_cooffs(
                    np.mean(quats, axis=0))
                det_bbox[i, :] = np.sum(bboxes, axis=0)
            else:
                pos[i, :] = [np.nan, np.nan, np.nan]
                det_rot[i, :] = [np.nan, np.nan, np.nan, np.nan]
                det_bbox[i, :] = [np.nan, np.nan, np.nan]
        return (sc.Variable(['spectrum'],
                            values=pos,
                            unit=sc.units.m,
                            dtype=sc.dtype.vector_3_float64),
                sc.Variable(['spectrum'],
                            values=det_rot,
                            dtype=sc.dtype.matrix_3_float64),
                sc.Variable(['spectrum'],
                            values=det_bbox,
                            unit=sc.units.m,
                            dtype=sc.dtype.vector_3_float64))


def _get_dtype_from_values(values, coerce_floats_to_ints):
    if coerce_floats_to_ints and np.all(np.mod(values, 1.0) == 0.0):
        dtype = sc.dtype.int32
    elif hasattr(values, 'dtype'):
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
    dim, unit = validate_and_get_unit(axis.getUnit())
    values = axis.extractValues()
    dtype = _get_dtype_from_values(values, dim == 'spectrum')
    return dim, sc.Variable([dim], values=values, unit=unit, dtype=dtype)


def set_bin_masks(bin_masks, dim, index, masked_bins):
    for masked_bin in masked_bins:
        bin_masks['spectrum', index][dim, masked_bin].value = True


def _convert_MatrixWorkspace_info(ws,
                                  advanced_geometry=False,
                                  load_run_logs=True):
    from mantid.kernel import DeltaEModeType
    common_bins = ws.isCommonBins()
    dim, unit = validate_and_get_unit(ws.getAxis(0).getUnit())
    source_pos, sample_pos = make_component_info(ws)
    pos, rot, shp = get_detector_properties(
        ws, source_pos, sample_pos, advanced_geometry=advanced_geometry)
    spec_dim, spec_coord = init_spec_axis(ws)

    if common_bins:
        coord = sc.Variable([dim], values=ws.readX(0), unit=unit)
    else:
        coord = sc.Variable([spec_dim, dim], values=ws.extractX(), unit=unit)

    info = {
        "coords": {
            dim: coord,
            spec_dim: spec_coord
        },
        "masks": {},
        "attrs": {
            "sample":
            make_sample(ws),
            "instrument-name":
            sc.Variable(
                value=ws.componentInfo().name(ws.componentInfo().root()))
        },
    }

    if load_run_logs:
        for run_log_name, run_log_variable in make_variables_from_run_logs(ws):
            info["attrs"][run_log_name] = run_log_variable

    if advanced_geometry:
        info["coords"]["detector-info"] = make_detector_info(ws)

    if not np.all(np.isnan(pos.values)):
        info["coords"].update({"position": pos})

    if rot is not None and shp is not None and not np.all(np.isnan(
            pos.values)):
        info["attrs"].update({"rotation": rot, "shape": shp})

    if source_pos is not None:
        info["coords"]["source-position"] = source_pos

    if sample_pos is not None:
        info["coords"]["sample-position"] = sample_pos

    if ws.detectorInfo().hasMaskedDetectors():
        spectrum_info = ws.spectrumInfo()
        mask = np.array([
            spectrum_info.isMasked(i) for i in range(ws.getNumberHistograms())
        ])
        info["masks"]["spectrum"] = sc.Variable([spec_dim], values=mask)

    if ws.getEMode() == DeltaEModeType.Direct:
        info["coords"]["incident-energy"] = _extract_einitial(ws)
    elif ws.getEMode() == DeltaEModeType.Indirect:
        info["coords"]["final-energy"] = _extract_efinal(ws)
    return info


def convert_monitors_ws(ws, converter, **ignored):
    dim, unit = validate_and_get_unit(ws.getAxis(0).getUnit())
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
        # We only ExtractSpectra for compatibility with
        # existing convert_Workspace2D_to_dataarray. This could instead be
        # refactored if found to be slow
        with run_mantid_alg('ExtractSpectra',
                            InputWorkspace=ws,
                            WorkspaceIndexList=[index]) as monitor_ws:
            # Run logs are already loaded in the data workspace
            single_monitor = converter(monitor_ws, load_run_logs=False)
        # Remove redundant information that is duplicated from workspace
        # We get this extra information from the generic converter reuse
        del single_monitor.coords['sample-position']
        if 'detector-info' in single_monitor.coords:
            del single_monitor.coords['detector-info']
        del single_monitor.attrs['sample']
        monitors.append((comp_info.name(det_index), single_monitor))
    return monitors


def convert_Workspace2D_to_data_array(ws,
                                      load_run_logs=True,
                                      advanced_geometry=False,
                                      **ignored):

    dim, unit = validate_and_get_unit(ws.getAxis(0).getUnit())
    spec_dim, spec_coord = init_spec_axis(ws)

    coords_labs_data = _convert_MatrixWorkspace_info(
        ws, advanced_geometry=advanced_geometry, load_run_logs=load_run_logs)
    _, data_unit = validate_and_get_unit(ws.YUnit(), allow_empty=True)
    if ws.id() == 'MaskWorkspace':
        coords_labs_data["data"] = sc.Variable([spec_dim],
                                               unit=data_unit,
                                               values=ws.extractY().flatten(),
                                               dtype=sc.dtype.bool)
    else:
        stddev2 = ws.extractE()
        np.multiply(stddev2, stddev2, out=stddev2)  # much faster than np.power
        coords_labs_data["data"] = sc.Variable([spec_dim, dim],
                                               unit=data_unit,
                                               values=ws.extractY(),
                                               variances=stddev2)
    array = detail.move_to_data_array(**coords_labs_data)

    if ws.hasAnyMaskedBins():
        bin_mask = sc.Variable(dims=array.dims,
                               shape=array.shape,
                               dtype=sc.dtype.bool)
        for i in range(ws.getNumberHistograms()):
            # maskedBinsIndices throws instead of returning empty list
            if ws.hasMaskedBins(i):
                set_bin_masks(bin_mask, dim, i, ws.maskedBinsIndices(i))
        common_mask = sc.all(bin_mask, 'spectrum')
        if sc.is_equal(common_mask, sc.any(bin_mask, 'spectrum')):
            array.masks["bin"] = detail.move(common_mask)
        else:
            array.masks["bin"] = detail.move(bin_mask)

    # Avoid creating dimensions that are not required since this mostly an
    # artifact of inflexible data structures and gets in the way when working
    # with scipp.
    if len(spec_coord.values) == 1:
        if 'position' in array.coords:
            array.coords['position'] = array.coords['position'][spec_dim, 0]
        array = array[spec_dim, 0].copy()
    return array


def convert_EventWorkspace_to_data_array(ws,
                                         load_pulse_times=True,
                                         advanced_geometry=False,
                                         load_run_logs=True,
                                         **ignored):
    from mantid.api import EventType

    dim, unit = validate_and_get_unit(ws.getAxis(0).getUnit())
    spec_dim, spec_coord = init_spec_axis(ws)
    nHist = ws.getNumberHistograms()
    _, data_unit = validate_and_get_unit(ws.YUnit(), allow_empty=True)

    n_event = ws.getNumberEvents()
    coord = sc.Variable(dims=['event'],
                        shape=[n_event],
                        unit=unit,
                        dtype=sc.dtype.float64)
    weights = sc.broadcast(sc.Variable(value=1.0,
                                       variance=1.0,
                                       unit=data_unit,
                                       dtype=sc.dtype.float32),
                           dims=['event'],
                           shape=[n_event])
    pulse_times = sc.Variable(
        dims=['event'],
        shape=[n_event],
        unit=sc.units.ns,
        dtype=sc.dtype.int64) if load_pulse_times else None

    evtp = ws.getSpectrum(0).getEventType()
    contains_weighted_events = ((evtp == EventType.WEIGHTED)
                                or (evtp == EventType.WEIGHTED_NOTIME))

    begins = sc.Variable([spec_dim, dim],
                         shape=[nHist, 1],
                         dtype=sc.dtype.int64)
    ends = begins.copy()
    current = 0
    for i in range(nHist):
        sp = ws.getSpectrum(i)
        size = sp.getNumberEvents()
        coord['event', current:current + size].values = sp.getTofs()
        if load_pulse_times:
            pulse_times['event', current:current +
                        size].values = sp.getPulseTimesAsNumpy()
        if contains_weighted_events:
            weights['event', current:current + size].values = sp.getWeights()
            weights['event',
                    current:current + size].variances = sp.getWeightErrors()
        begins.values[i] = current
        ends.values[i] = current + size
        current += size

    proto_events = {'data': weights, 'coords': {dim: coord}}
    if load_pulse_times:
        proto_events["coords"]["pulse-time"] = pulse_times
    events = detail.move_to_data_array(**proto_events)

    coords_labs_data = _convert_MatrixWorkspace_info(
        ws, advanced_geometry=advanced_geometry, load_run_logs=load_run_logs)
    # For now we ignore potential finer bin edges to avoid creating too many
    # bins. Use just a single bin along dim and use extents given by workspace
    # edges.
    # TODO If there are events outside edges this might create bins with
    # events that are not within bin bounds. Consider using `bin` instead
    # of `bins`?
    edges = coords_labs_data['coords'][dim]
    # Using range slice of thickness 1 to avoid transposing 2-D coords
    coords_labs_data['coords'][dim] = sc.concatenate(edges[dim, :1],
                                                     edges[dim, -1:], dim)

    coords_labs_data["data"] = sc.bins(begin=begins,
                                       end=ends,
                                       dim='event',
                                       data=detail.move(events))
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
                sc.Variable(['row'], values=ws.column(i)))
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
                sc.Variable(['row'],
                            values=np.array(ws.column(i)),
                            variances=variance))
        elif data_name not in error_connection.values():
            # This data is not an error for another dataset, and has no error
            dataset[data_name] = detail.move(
                sc.Variable(['row'], values=ws.column(i)))

    return dataset


def convert_WorkspaceGroup_to_dataarray_dict(group_workspace, **kwargs):
    workspace_dict = {}
    for i in range(group_workspace.getNumberOfEntries()):
        workspace = group_workspace.getItem(i)
        workspace_name = workspace.name().replace(f'{group_workspace.name()}',
                                                  '').strip('_')
        workspace_dict[workspace_name] = from_mantid(workspace, **kwargs)

    return workspace_dict


def from_mantid(workspace, **kwargs):
    """Convert Mantid workspace to a scipp data array or dataset.
    :param workspace: Mantid workspace to convert.
    """
    scipp_obj = None  # This is either a Dataset or DataArray
    monitor_ws = None
    workspaces_to_delete = []
    w_id = workspace.id()
    if (w_id == 'Workspace2D' or w_id == 'RebinnedOutput'
            or w_id == 'MaskWorkspace'):
        n_monitor = 0
        spec_info = workspace.spectrumInfo()
        for i in range(len(spec_info)):
            if spec_info.hasDetectors(i) and spec_info.isMonitor(i):
                n_monitor += 1
        # If there are *only* monitors we do not move them to an attribute
        if n_monitor > 0 and n_monitor < len(spec_info):
            import mantid.simpleapi as mantid
            workspace, monitor_ws = mantid.ExtractMonitors(workspace)
            workspaces_to_delete.append(workspace)
            workspaces_to_delete.append(monitor_ws)
        scipp_obj = convert_Workspace2D_to_data_array(workspace, **kwargs)
    elif w_id == 'EventWorkspace':
        scipp_obj = convert_EventWorkspace_to_data_array(workspace, **kwargs)
    elif w_id == 'TableWorkspace':
        scipp_obj = convert_TableWorkspace_to_dataset(workspace, **kwargs)
    elif w_id == 'MDHistoWorkspace':
        scipp_obj = convert_MDHistoWorkspace_to_data_array(workspace, **kwargs)
    elif w_id == 'WorkspaceGroup':
        scipp_obj = convert_WorkspaceGroup_to_dataarray_dict(
            workspace, **kwargs)

    if scipp_obj is None:
        raise RuntimeError('Unsupported workspace type {}'.format(w_id))

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
        if monitor_ws.id() == 'MaskWorkspace' or monitor_ws.id(
        ) == 'Workspace2D':
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
         mantid_alg='Load',
         mantid_args=None,
         advanced_geometry=False):
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
    :param str mantid_alg: Mantid algorithm to use for loading. Default is
                           `Load`.
    :param dict mantid_args: Dict of keyword arguments to forward to Mantid.
    :param bool advanced_geometry: If True, load the full detector geometry
                                   including shapes and rotations. The
                                   positions of grouped detectors are
                                   spherically averaged. If False,
                                   load only the detector position, and return
                                   the cartesian average of the grouped
                                   detector positions.
    :raises: If the Mantid workspace type returned by the Mantid loader is not
             either EventWorkspace or Workspace2D.
    :return: A Dataset containing the neutron event/histogram data and the
             instrument geometry.
    :rtype: Dataset
    """

    if mantid_args is None:
        mantid_args = {}

    with run_mantid_alg(mantid_alg, filename, **mantid_args) as loaded:
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
                           error_connection=error_connection,
                           advanced_geometry=advanced_geometry)


def load_component_info(ds, file, advanced_geometry=False):
    """
    Adds the component info coord into the dataset. The following are added:

    - source-position
    - sample-position
    - detector positions
    - detector rotations
    - detector shapes

    :param ds: Dataset on which the component info will be added as coords.
    :param file: File from which the IDF will be loaded.
                 This can be anything that mantid.Load can load.
    :param bool advanced_geometry: If True, load the full detector geometry
                                   including shapes and rotations. The
                                   positions of grouped detectors are
                                   spherically averaged. If False,
                                   load only the detector position, and return
                                   the cartesian average of the grouped
                                   detector positions.
    """
    with run_mantid_alg('Load', file) as ws:
        source_pos, sample_pos = make_component_info(ws)

        ds.coords["source-position"] = source_pos
        ds.coords["sample-position"] = sample_pos
        pos, rot, shp = get_detector_properties(
            ws, source_pos, sample_pos, advanced_geometry=advanced_geometry)
        ds.coords["position"] = pos
        if rot is not None:
            ds.attrs["rotation"] = rot
        if shp is not None:
            ds.attrs["shape"] = shp


def validate_dim_and_get_mantid_string(unit_dim):
    known_units = {
        'energy-transfer': "DeltaE",
        'tof': "TOF",
        'wavelength': "Wavelength",
        'energy': "Energy",
        'd-spacing': "dSpacing",
        'Q': "MomentumTransfer",
        'Q^2': "QSquared",
    }

    user_k = str(unit_dim).casefold()
    known_units = {k.casefold(): v for k, v in known_units.items()}

    if user_k not in known_units:
        raise RuntimeError("Axis unit not currently supported."
                           "Possible values are: {}, "
                           "got '{}'. ".format([k for k in known_units.keys()],
                                               unit_dim))
    else:
        return known_units[user_k]


def to_mantid(data, dim, instrument_file=None):
    """
    Convert data to a Mantid workspace.

    The Mantid layout expect the spectra to be the Outer-most dimension,
    i.e. y.shape[0]. If that is not the case you might have to transpose
    your data to fit that, otherwise it will not be aligned correctly in the
    Mantid workspace.

    :param data: Data to be converted.
    :param dim: Coord to use for Mantid's first axis (X).
    :param instrument_file: Instrument file that will be
                            loaded into the workspace
    :returns: Workspace containing converted data. The concrete workspace type
              may differ depending on the content of `data`.
    """
    if not is_data_array(data):
        raise RuntimeError(
            "Currently only data arrays can be converted to a Mantid workspace"
        )
    try:
        import mantid.simpleapi as mantid
    except ImportError:
        raise ImportError(
            "Mantid Python API was not found, please install Mantid framework "
            "as detailed in the installation instructions (https://scipp."
            "github.io/getting-started/installation.html)")
    x = data.coords[dim].values
    y = data.values
    e = data.variances

    assert (len(y.shape) == 2 or len(y.shape) == 1), \
        "Currently can only handle 2D data."

    e = np.sqrt(e) if e is not None else np.sqrt(y)

    # Convert a single array (e.g. single spectra) into 2d format
    if len(y.shape) == 1:
        y = np.array([y])

    if len(e.shape) == 1:
        e = np.array([e])

    unitX = validate_dim_and_get_mantid_string(dim)

    nspec = y.shape[0]
    if len(x.shape) == 1:
        # SCIPP is using a  1:n spectra coord mapping, Mantid needs
        # a 1:1 mapping so expand this out
        x = np.broadcast_to(x, shape=(nspec, len(x)))

    nbins = x.shape[1]
    nitems = y.shape[1]

    ws = mantid.WorkspaceFactory.create("Workspace2D",
                                        NVectors=nspec,
                                        XLength=nbins,
                                        YLength=nitems)
    if data.unit != sc.units.counts:
        ws.setDistribution(True)

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


def _table_to_data_array(table, key, value, stddev):
    stddevs = table[stddev].values
    dim = 'parameter'
    coord = table[key].data.copy()
    coord.rename_dims({'row': dim})
    return sc.DataArray(data=sc.Variable(dims=[dim],
                                         values=table[value].values,
                                         variances=stddevs * stddevs),
                        coords={dim: coord})


def _fit_workspace(ws, mantid_args):
    """
    Performs a fit on the workspace.

    :param ws: The workspace on which the fit will be performed
    :returns: Dataset containing all of Fit's outputs
    """
    with run_mantid_alg('Fit',
                        InputWorkspace=ws,
                        **mantid_args,
                        CreateOutput=True) as fit:
        # This is assuming that all parameters are dimensionless. If this is
        # not the case we should use a dataset with a scalar variable per
        # parameter instead. Or better, a dict of scalar variables?
        parameters = convert_TableWorkspace_to_dataset(fit.OutputParameters)
        parameters = _table_to_data_array(parameters,
                                          key='Name',
                                          value='Value',
                                          stddev='Error')
        out = convert_Workspace2D_to_data_array(fit.OutputWorkspace)
        data = sc.Dataset()
        data['data'] = out['empty', 0]
        data['calculated'] = out['empty', 1]
        data['diff'] = out['empty', 2]
        parameters.coords['status'] = sc.Variable(fit.OutputStatus)
        parameters.coords['chi^2/d.o.f.'] = sc.Variable(fit.OutputChi2overDoF)
        parameters.coords['function'] = sc.Variable(str(fit.Function))
        parameters.coords['cost-function'] = sc.Variable(fit.CostFunction)
        return parameters, data


def fit(data, mantid_args):
    if len(data.dims) != 1 or 'WorkspaceIndex' in mantid_args:
        raise RuntimeError(
            "Only 1D fitting is supported. Use scipp slicing and do not"
            "provide a WorkspaceIndex.")
    dim = data.dims[0]
    ws = to_mantid(data, dim)
    mantid_args['workspace_index'] = 0
    return _fit_workspace(ws, mantid_args)


def _try_except(op, possible_except, failure, **kwargs):
    try:
        return op(**kwargs)
    except possible_except:
        return failure


def _get_instrument_efixed(workspace):
    inst = workspace.getInstrument()
    if inst.hasParameter('Efixed'):
        return inst.getNumberParameter('EFixed')[0]

    if inst.hasParameter('analyser'):
        analyser_name = inst.getStringParameter('analyser')[0]
        analyser_comp = inst.getComponentByName(analyser_name)

        if analyser_comp is not None and analyser_comp.hasParameter('Efixed'):
            return analyser_comp.getNumberParameter('EFixed')[0]

    return None


def _extract_einitial(ws):
    ei = None
    if ws.run().hasProperty("Ei"):
        ei = ws.run().getProperty("Ei").value
    elif ws.run().hasProperty('EnergyRequest'):
        ei = ws.run().getProperty('EnergyRequest').value[-1]
    return sc.Variable(value=ei, unit=sc.Unit("meV"))


def _extract_efinal(ws):
    detInfo = ws.detectorInfo()
    specInfo = ws.spectrumInfo()
    ef = np.empty(shape=(specInfo.size(), ), dtype=float)
    ef[:] = np.nan
    analyser_ef = _get_instrument_efixed(workspace=ws)
    ids = detInfo.detectorIDs()
    for spec_index in range(len(specInfo)):
        detector_ef = None
        if specInfo.hasDetectors(spec_index):
            # Just like mantid, we only take the first entry of the group.
            det_index = specInfo.getSpectrumDefinition(spec_index)[0][0]
            detector_ef = _try_except(op=ws.getEFixed,
                                      possible_except=RuntimeError,
                                      failure=None,
                                      detId=int(ids[det_index]))
        detector_ef = detector_ef if detector_ef is not None else analyser_ef
        if not detector_ef:
            continue  # Cannot assign an Ef. May or may not be an error
            # - i.e. a diffraction detector, monitor etc.
        ef[spec_index] = detector_ef

    return sc.Variable(dims=['spectrum'], values=ef, unit=sc.Unit("meV"))
