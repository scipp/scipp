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

