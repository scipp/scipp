# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock, Neil Vaytet


import scipp as sc
import numpy as np


def get_pos(pos):
    return [pos.X(), pos.Y(), pos.Z()]


def convert_instrument(ws):
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


def init_pos_spectrum_no(nHist, ws):

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
    pos = sc.Variable([sc.Dim.Position], values=pos, unit=sc.units.m)
    num = sc.Variable([sc.Dim.Position], values=num)
    return pos, num


def convert_Workspace2D_to_dataset(ws):
    cb = ws.isCommonBins()
    nHist = ws.getNumberHistograms()
    comp_info = convert_instrument(ws)
    pos, num = init_pos_spectrum_no(nHist, ws)

    # TODO More cases?
    allowed_units = {
        "DeltaE": [sc.Dim.EnergyTransfer, sc.units.meV],
        "TOF": [sc.Dim.Tof, sc.units.us]
    }
    xunit = ws.getAxis(0).getUnit().unitID()
    if xunit not in allowed_units.keys():
        raise RuntimeError("X-axis unit not currently supported for "
                           "Workspace2D. Possible values are: {}, "
                           "got '{}'. ".format(
                               [k for k in allowed_units.keys()], xunit))
    else:
        [dim, unit] = allowed_units[xunit]

    if cb:
        coords = sc.Variable([dim], values=ws.readX(0), unit=unit)
    else:
        coords = sc.Variable([sc.Dim.Position, dim],
                             shape=(ws.getNumberHistograms(),
                                    len(ws.readX(0))),
                             unit=unit)
        for i in range(ws.getNumberHistograms()):
            coords[sc.Dim.Position, i].values = ws.readX(i)

    # TODO Use unit information in workspace, if available.
    array = sc.DataArray(data=sc.Variable([sc.Dim.Position, dim],
                                          shape=(ws.getNumberHistograms(),
                                                 len(ws.readY(0))),
                                          unit=sc.units.counts,
                                          variances=True),
                         coords={
                             dim: coords,
                             sc.Dim.Position: pos
                         },
                         labels={
                             "spectrum_number": num,
                             "component_info": comp_info
                         })

    data = array.data
    for i in range(ws.getNumberHistograms()):
        data[sc.Dim.Position, i].values = ws.readY(i)
        data[sc.Dim.Position, i].variances = np.power(ws.readE(i), 2)

    return array


def convert_EventWorkspace_to_dataset(ws, load_pulse_times, EventType):

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
    comp_info = convert_instrument(ws)
    pos, num = init_pos_spectrum_no(nHist, ws)

    # TODO Use unit information in workspace, if available.
    coords = sc.Variable([sc.Dim.Position, dim],
                         shape=[nHist, sc.Dimensions.Sparse],
                         unit=unit)
    if load_pulse_times:
        labs = sc.Variable([sc.Dim.Position, dim],
                           shape=[nHist, sc.Dimensions.Sparse])

    # Check for weighted events
    evtp = ws.getSpectrum(0).getEventType()
    contains_weighted_events = ((evtp == EventType.WEIGHTED)
                                or (evtp == EventType.WEIGHTED_NOTIME))
    if contains_weighted_events:
        weights = sc.Variable([sc.Dim.Position, dim],
                              shape=[nHist, sc.Dimensions.Sparse])

    for i in range(nHist):
        sp = ws.getSpectrum(i)
        coords[sc.Dim.Position, i].values = sp.getTofs()
        if load_pulse_times:
            # Pulse times have a Mantid-specific format so the conversion is
            # very slow.
            # TODO: Find a more efficient way to do this.
            pt = sp.getPulseTimes()
            labs[sc.Dim.Position,
                 i].values = np.asarray([p.total_nanoseconds() for p in pt])
        if contains_weighted_events:
            weights[sc.Dim.Position, i].values = sp.getWeights()
            weights[sc.Dim.Position, i].variances = sp.getWeightErrors()

    coords_labs_data = {
        "coords": {
            dim: coords,
            sc.Dim.Position: pos
        },
        "labels": {
            "spectrum_number": num,
            "component_info": comp_info
        }
    }
    if load_pulse_times:
        coords_labs_data["labels"]["pulse_times"] = labs
    if contains_weighted_events:
        coords_labs_data["data"] = weights
    return sc.DataArray(**coords_labs_data)

def convert_TableWorkspace_to_dataset(ws, error_connection=None):
    """
    Converts from a Mantid TableWorkspace to a scipp dataset. A TableWorkspace
    have optional declaration of connection between data and error using the
    plottype which can be none, X, Y, Z, Xerr, Yerr. This function collects
    the data and its error into a single scipp variable if there is no ambiguity
    in what column contains the error for what data column. This means a maximum
    of one Xerr and one Yerr column. For more advanced TableWorkspaces, the user
    can specify the connection between data and error manually using the
    error_connections keyword argument, which overwrites automatic assignment.

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
    columnNames = ws.getColumnNames() # list of names matching each column
    columnTypes = ws.columnTypes() # list of types matching each column

    if error_connection is None:
        # Generate error connection
        error_connection = {}
        # Dictionary with data column keys for names of their error columns
        # Uses the plottype that is built into TableWorkspace
        plottype_list = []
        for i in range(n_columns):
            plottype_list.append(ws.getPlotType(i))

        # Relevant plottype indices
        X_index = 1
        Xerr_index = 4
        Y_index = 2
        Yerr_index = 5

        if plottype_list.count(Xerr_index) > 1: # More than one Xerr
            raise RuntimeError(
                "Multiple Xerr columns in TableWorkspace, error_connection "
                "need to be specified manually for the scipp "
                "convert_TableWorkspace_to_dataset function.")
        elif plottype_list.count(Xerr_index) is 1:
            error_index = plottype_list.index(Xerr_index)
            errorName = columNames[error_index]
            if plottype_list.count(X_index) is 1: # Unique X for Xerr
                data_index = plottype_list.index(X_index)
                data_name = columnNames[data_index]
                error_connection[data_name] = errorName
            else:
                raise RuntimeError(
                    "No unique X data to be matched with Xerr column, "
                    "error_connection need to be specified manually for the "
                    "scipp convert_TableWorkspace_to_dataset function.")

        if plottype_list.count(Yerr_index) > 1: # More than one Yerr
            raise RuntimeError(
                "Multiple Yerr columns in TableWorkspace, error_connection "
                "need to be specified manually for the scipp "
                "convert_TableWorkspace_to_dataset function.")
        elif plottype_list.count(Yerr_index) is 1:
            error_index = plottype_list.index(Yerr_index)
            errorName = columNames[error_index]
            if plottype_list.count(Y_index) is 1: # Unique Y for Yerr
                data_index = plottype_list.index(Y_index)
                data_name = columnNames[data_index]
                error_connection[data_name] = errorName
            else:
                raise RuntimeError(
                    "No unique Y data to be matched with Yerr column, "
                    "error_connection need to be specified manually for the "
                    "scipp convert_TableWorkspace_to_dataset function.")

    # Types available in TableWorkspace that can not be loaded into scipp
    blacklist_types = []
    # Types which will fail if they have defined variance
    blacklist_variance_types = ["str"]

    variables = {}
    for i in range(n_columns):
        if columnTypes[i] in blacklist_types:
            continue # skips loading data of this type

        data_name = columnNames[i]
        if len(error_connection) is 0:
            variables[data_name] = sc.Variable([sc.Dim.Row],
                                               values=ws.column(i))
        elif data_name in error_connection:
            # This data has error availble
            error_name = error_connection[data_name]
            error_index = columnNames.index(error_name)
            if (columnTypes[error_index] in blacklist_variance_types
               or columnTypes[i] in blacklist_variance_types):
                raise RuntimeError(
                    "Data with variance can not have type string. \n"
                    + "Data:     " + str(data_name) + "\n"
                    + "Variance: " + str(error_name) + "\n"
                    "Supply error_connection using keyword argument.")

            variance = np.array(ws.column(error_name))**2
            variables[data_name] = sc.Variable([sc.Dim.Row],
                                               values=np.array(ws.column(i)),
                                               variances=variance)
        elif data_name not in error_connection.values():
            # This data is not an error for another dataset, and has no error
            variables[data_name] = sc.Variable([sc.Dim.Row],
                                               values=ws.column(i))

    return sc.Dataset(variables) # Return scipp dataset built from the variables

def load(filename="",
         load_pulse_times=True,
         instrument_filename=None,
         error_connection=None,
         **kwargs):
    """
    Wrapper function to provide a load method for a Nexus file, hiding mantid
    specific code from the scipp interface. All other keyword arguments not
    specified in the parameters below are passed on to the mantid.Load
    function.

    Example of use:

      from scipp.neutron import load
      d = sc.Dataset()
      d["sample"] = load(filename='PG3_4844_event.nxs', \
                         BankName='bank184', load_pulse_times=True)

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
        from mantid.api import EventType
    except ImportError as e:
        raise ImportError(
            "Mantid Python API was not found, please install Mantid framework "
            "as detailed in the installation instructions (https://scipp."
            "readthedocs.io/en/latest/getting-started/installation.html)"
        ) from e

    ws = mantid.Load(filename, **kwargs)
    if instrument_filename is not None:
        mantid.LoadInstrument(ws,
                              FileName=instrument_filename,
                              RewriteSpectraMap=True)
    if ws.id() == 'Workspace2D':
        return convert_Workspace2D_to_dataset(ws)
    if ws.id() == 'EventWorkspace':
        return convert_EventWorkspace_to_dataset(ws, load_pulse_times,
                                                 EventType)
    if ws.id() == 'TableWorkspace':
        return convert_TableWorkspace_to_dataset(ws, error_connection)
    raise RuntimeError('Unsupported workspace type')
