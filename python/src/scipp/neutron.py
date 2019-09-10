# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet


def load(filename="",
         load_pulse_times=True,
         instrument_filename=None,
         **kwargs):
    """
    Wrapper function to provide a load method for a Nexus file, hiding mantid
    specific code from the scipp interface. A other keyword arguments not
    specified in the parameters below are passed on to the mantid.Load
    function.

    :param str filename: The name of the Nexus/HDF file to be loaded.
    :param bool load_pulse_times: Read the pulse times if True.
    :param str instrument_filename: If specified, over-write the instrument
                                    definition in the final Dataset with the
                                    geometry contained in the file.
    :raises: If the Mantid workspace type returned by the Mantid loader is not
             either EventWorkspace or Wospace2D.
    :return: A Dataset containing the neutron event/histogram data and the
             instrument geometry.
    :rtype: Dataset
    """

    import mantid.simpleapi as mantid
    from mantid.api import EventType
    from .compat import mantid as mc

    ws = mantid.Load(filename, **kwargs)
    if instrument_filename is not None:
        mantid.LoadInstrument(ws,
                              FileName=instrument_filename,
                              RewriteSpectraMap=True)
    if ws.id() == 'Workspace2D':
        return mc.convert_Workspace2D_to_dataset(ws)
    if ws.id() == 'EventWorkspace':
        return mc.convert_EventWorkspace_to_dataset(ws, load_pulse_times,
                                                    EventType)
    raise RuntimeError('Unsupported workspace type')
