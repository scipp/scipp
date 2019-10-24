# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Mads Bertelsen

import numpy as np

from ..._scipp import core as sc
from ...compat.mantid import convert_TableWorkspace_to_dataset
from .. import exceptions


def load_calibration(filename, mantid_LoadDiffCal_args={}):
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
      input = {"InstrumentName": "WISH"}
      cal = loadCal('cal_file.cal', mantid_LoadDiffCal_args=input')

    Note that this function requires mantid to be installed and available in
    the same Python environment as scipp.

    :param str filename: The name of the cal file to be loaded.
    :param dict mantid_LoadDiffCal_args : Dictionary with arguments for the
                                          LoadDiffCal Mantid algorithm.
                                          Currently InstrumentName is required.
    :raises: If the InstrumentName given in mantid_LoadDiffCal_args is not
             valid.
    :return: A Dataset containing the calibration data and grouping.
    :rtype: Dataset
    """

    try:
        import mantid.simpleapi as mantid
    except ImportError as e:
        raise exceptions.MantidNotFoundError from e

    if "WorkspaceName" not in mantid_LoadDiffCal_args:
        mantid_LoadDiffCal_args["WorkspaceName"] = "cal_output"

    output = mantid.LoadDiffCal(Filename=filename, **mantid_LoadDiffCal_args)

    cal_ws = output.OutputCalWorkspace
    cal_data = convert_TableWorkspace_to_dataset(cal_ws)

    # Modify units of cal_data
    cal_data["difc"].unit = sc.units.us/sc.units.angstrom
    cal_data["difa"].unit = sc.units.us/sc.units.angstrom/sc.units.angstrom
    cal_data["tzero"].unit = sc.units.us

    # Mask data not used, but is loaded by LoadDiffCal
    # output.OutputMaskWorkspace)

    # Apply group information to dataset by matching detector id's to group nr
    group_ws = output.OutputGroupingWorkspace
    group_map = {}  # dict from detectorID to group number
    for i in range(group_ws.getNumberHistograms()):
        group_map[group_ws.getDetector(i).getID()] = group_ws.readY(i)[0]

    # Create list with same ordering as in the cal_data dataset
    group_list = np.array([group_map[detid] for detid in cal_data["detid"].values])

    cal_data["group"] = sc.Variable([sc.Dim.Row], values=group_list)

    cal_data.rename_dims({sc.Dim.Row: sc.Dim.Detector})
    cal_data.coords[sc.Dim.Detector] = cal_data['detid'].data
    del cal_data['detid']

    # Delete generated mantid workspaces
    base_name = mantid_LoadDiffCal_args["WorkspaceName"]
    mantid.mtd.remove(base_name + "_cal")
    mantid.mtd.remove(base_name + "_group")
    mantid.mtd.remove(base_name + "_mask")

    return cal_data
