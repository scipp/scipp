from .scippy import *
from .table import table
try:
    from .plot import plot
except ImportError:
    print("Warning: the plotting module for Scippy was not imported. Check that"
          "plotly is installed on your system. You can still use Scippy "
          "without its plotting functionality enabled.")
