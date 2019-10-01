# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock, Neil Vaytet

import scipp as sc


def loadCal(Filename="", InstrumentName="",
            InstrumentFilename="",
            GroupFilename="",
            TofMin=None, TofMax=None):
    """
    Function that loads calibration files using the Mantid algorithm
    LoadDiffCal. This algorithm produces up to three workspaces, a
    TableWorkspace containing conversion factors between TOF and d, a
    GroupingWorkspace with detector groups and a MaskWorkspace for masking.
    The information from the TableWorkspace and GroupingWorkspace is converted
    to a Scipp dataset and returned, while the MaskWorkspace is ignored for
    now. Only the keyword paramters Filename and InstrumentName are mandatory.

    Example of use:

      from scipp.neutron.diffraction import load
      cal = loadCal(Filename='cal_file.cal', InstrumentName='WISH')

    Note that this function requires mantid to be installed and available in
    the same Python environment as scipp.

    :param str Filename: The name of the cal file to be loaded.
    :param str InstrumentName: Name of Mantid instrument to use.
    :param str InstrumentFilename: If specified, over-write the instrument
                                    definition in the final Dataset with the
                                    geometry contained in the file.
    :param str GroupFilename: If specified, over-writes the grouping from
                               CallFileName.
    :param float TofMin: Minimum time of flight (default 0)
    :param float TofMax: Maximum time of flight
    :raises: If the Mantid workspace type returned by the Mantid loader is not
             either EventWorkspace or Workspace2D.
    :return: A Dataset containing the calibration data and grouping.
    :rtype: Dataset
    """

    try:
        import mantid.simpleapi as mantid
    except ImportError as e:
        raise ImportError(
            "Mantid Python API was not found, please install Mantid framework "
            "as detailed in the installation instructions (https://scipp."
            "readthedocs.io/en/latest/getting-started/installation.html)"
        ) from e

    output = mantid.LoadDiffCal(Filename=Filename,
                                InstrumentName=InstrumentName,
                                WorkspaceName="cal_output",
                                InstrumentFilename=InstrumentFilename,
                                GroupFilename="",
                                TofMin=TofMin, TofMax=TofMax)

    cal_ws = output.OutputCalWorkspace
    cal_data = sc.compat.mantid.convert_TableWorkspace_to_dataset(cal_ws)

    # Modify units of cal_data
    cal_data["difc"].unit = sc.units.us/sc.units.angstrom
    # Currently unsupported unit on python 3.6
    # cal_data["difa"].unit = sc.units.us/sc.units.angstrom/sc.units.angstrom
    cal_data["tzero"].unit = sc.units.us

    # Mask data not used yet
    # output.OutputMaskWorkspace)

    group_ws = output.OutputGroupingWorkspace

    group_list = []
    for i in range(group_ws.getNumberHistograms()):
        group_list.append(group_ws.readY(i))

    group_var = sc.Variable([sc.Dim.Row], values=group_list)

    cal_data["group"] = group_var

    return cal_data
