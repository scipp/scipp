# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Mads Bertelsen

import numpy as np

from ..._scipp import core as sc
from ...compat.mantid import run_mantid_alg
from ...compat.mantid import convert_TableWorkspace_to_dataset


def load_calibration(filename, mantid_args={}):
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
      cal = loadCal('cal_file.cal', mantid_args=input')

    Note that this function requires mantid to be installed and available in
    the same Python environment as scipp.

    :param str filename: The name of the cal file to be loaded.
    :param dict mantid_args : Dictionary with arguments for the
                              LoadDiffCal Mantid algorithm.
                              Currently InstrumentName is required.
    :raises: If the InstrumentName given in mantid_args is not
             valid.
    :return: A Dataset containing the calibration data and grouping.
    :rtype: Dataset
    """

    if "WorkspaceName" not in mantid_args:
        mantid_args["WorkspaceName"] = "cal_output"

    with run_mantid_alg('LoadDiffCal', Filename=filename,
                        **mantid_args) as output:
        cal_ws = output.OutputCalWorkspace
        cal_data = convert_TableWorkspace_to_dataset(cal_ws)

        # Modify units of cal_data
        cal_data["difc"].unit = sc.units.us / sc.units.angstrom
        cal_data[
            "difa"].unit = sc.units.us / sc.units.angstrom / sc.units.angstrom
        cal_data["tzero"].unit = sc.units.us

        # Note that despite masking and grouping stored in separate workspaces,
        # there is no need to handle potentially mismatching ordering: All
        # workspaces have been created by the same algorithm, which should
        # guarantee ordering.
        mask_ws = output.OutputMaskWorkspace
        group_ws = output.OutputGroupingWorkspace
        rows = mask_ws.getNumberHistograms()
        mask = np.fromiter((mask_ws.readY(i)[0] for i in range(rows)),
                           count=rows,
                           dtype=np.bool)
        group = np.fromiter((group_ws.readY(i)[0] for i in range(rows)),
                            count=rows,
                            dtype=np.int32)

        # This is deliberately not stored as a mask since that would make
        # subsequent handling, e.g., with groupby, more complicated. The mask
        # is conceptually not masking rows in this table, i.e., it is not
        # marking invalid rows, but rather describes masking for other data.
        cal_data["mask"] = sc.Variable(['row'], values=mask)
        cal_data["group"] = sc.Variable(['row'], values=group)

        cal_data.rename_dims({'row': 'detector'})
        cal_data.coords['detector'] = sc.Variable(
            ['detector'], values=cal_data['detid'].values.astype(np.int32))
        del cal_data['detid']

        return cal_data
