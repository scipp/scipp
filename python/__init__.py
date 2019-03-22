from .dataset import *
from .dataset_plotting import plot, plot_1d, plot_image, plot_sliceviewer, plot_waterfall
from .table import table
# Following fix for problem described here https://stackoverflow.com/questions/21784641/installation-issue-with-matplotlib-python
import platform
if platform.system() == 'Darwin':
    import matplotlib
    # Switch backends
    matplotlib.use('TkAgg')
