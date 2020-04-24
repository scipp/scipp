# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author David Fairbrother
from dataclasses import dataclass
from typing import Dict, Any, Union


@dataclass()
class OneDPlotKwargs(object):
    axes: Any = None
    grid: Any = None
    filename: str = None
    masks: Any = None
    mpl_axes: Any = None
    logx: bool = None
    logxy: bool = None
    logy: bool = None
    values: Any = None
    variances: Any = None


@dataclass()
class TwoDPlotKwargs(object):
    aspect: Any = None
    axes: Any = None
    cmap: Any = None
    color: Any = None
    filename: str = None
    masks: Any = None
    mpl_axes: Any = None
    log: bool = None
    logx: bool = None
    logxy: bool = None
    logy: bool = None
    values: Any = None
    variances: Any = None
    vmin: Any = None
    vmax: Any = None


@dataclass()
class ThreeDPlotKwargs(object):
    aspect: Any = None
    axes: Any = None
    cmap: Any = None
    color: Any = None
    filename: str = None
    log: bool = None
    masks: Any = None
    values: Any = None
    variances: Any = None
    vmin: Any = None
    vmax: Any = None


@dataclass
class PlotRequest(object):
    mpl_line_params: Dict
    ndims: int
    scipp_objs: Dict
    user_kwargs: Union[OneDPlotKwargs, TwoDPlotKwargs, ThreeDPlotKwargs] = None

    # Optional
    bins: Dict = None
    projection: str = None
