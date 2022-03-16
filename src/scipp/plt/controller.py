# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from ..units import one
from .pipeline import Pipeline
import numpy as np
from dataclasses import dataclass
from typing import Callable
from functools import partial


class Controller:
    """
    Controller class plots.
    """
    def __init__(
        self,
        # dims,
        # vmin=None,
        # vmax=None,
        # norm=None,
        # scale=None,
        # widgets=None,
        models,
        view,
        # preprocessors=None
    ):
        # self._dims = dims
        # self.widgets = widgets
        self._models = models
        self._view = view
        # self.preprocessors = preprocessors
        self._pipelines = {key: Pipeline() for key in self._models}

        # self.vmin = vmin
        # self.vmax = vmax
        # self.norm = norm if norm is not None else "linear"

        # # self.scale = {dim: "linear" for dim in self._dims}
        # if scale is not None:
        #     for dim, item in scale.items():
        #         self.scale[dim] = item

    def add_pipeline_step(self, step, key=None):
        if key is None:
            for pipeline in self._pipelines.values():
                pipeline.append(step)
            step.register_callback(self._run_all_pipelines)
        else:
            self._pipelines[key].append(step)
            step.register_callback(partial(self._run_pipeline, key=key))

    def render(self):
        """
        """
        self._run_all_pipelines()

    def _run_all_pipelines(self):
        for key in self._pipelines:
            self._run_pipeline(key, draw=False)
        self._view.draw()

    def _run_pipeline(self, key, draw=True):
        new_values = self._pipelines[key].run(self._models[key])
        self._view.update(new_values, key=key, draw=draw)

    def toggle_mask(self):
        pass