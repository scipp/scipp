# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet


class SciPlot(dict):
    """
    The SciPlot object is used as output from the plot command.
    It is a small wrapper around python dict, with a silent repr.
    The dict will contain all the plot elements as well as the slider and
    button widgets.
    More functionalities can be added in the future.
    """
    def __init__(self, *arg, **kw):
        super(SciPlot, self).__init__(*arg, **kw)

    def __repr__(self):
        return ""
