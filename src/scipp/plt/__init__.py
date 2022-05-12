# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

# flake8: noqa

import matplotlib.pyplot as plt

plt.ioff()

from .plot import Plot
from .graph import Node, Graph
from .figure import Figure
from . import widgets
