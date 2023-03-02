# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

import scipp as sc


def import_and_call():
    from scipp.plotting import plot

    plot(sc.data.table_xyz(10))


def test_plot_import():
    """
    This test came up when importing the plot function twice would lead to a mix-up
    between the plot function and the plot module/file, making the second call to
    plot() raise an error saying the 'plot' module was not callable. The fix was to
    rename the file that contains the plot function.
    """
    import_and_call()
    import_and_call()
