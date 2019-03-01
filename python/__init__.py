from .dataset_plotting import *
# from .dataset import *
from .xarray_compat import as_xarray
# Following fix for problem described here https://stackoverflow.com/questions/21784641/installation-issue-with-matplotlib-python
import platform
if platform.system() == 'Darwin':
    import matplotlib
    # Switch backends
    matplotlib.use('TkAgg')
