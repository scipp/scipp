from .dataset import *
from .table import table
try:
    from .dataset_plotting import plot, plot_1d, plot_image, plot_sliceviewer, plot_waterfall
except ImportError:
    print("Warning: the plotting module for Dataset was not imported. Check "
          "that plotly is installed on your system. You can still use Dataset "
          "without its plotting functionality enabled.")
