# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

import matplotlib
import matplotlib.pyplot as plt
import pytest


@pytest.fixture(autouse=True)
def reset_mpl_defaults():
    matplotlib.rcdefaults()
    matplotlib.use('Agg')


@pytest.fixture(autouse=True)
def close_figures():
    """
    Force closing all figures after each test case.
    Otherwise, the figures consume a lot of memory and matplotlib complains.
    """
    yield
    for fig in map(plt.figure, plt.get_fignums()):
        plt.close(fig)
