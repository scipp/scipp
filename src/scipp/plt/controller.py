# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .. import DataArray
from .pipeline import Pipeline, Step
from .view import View

from functools import partial
from typing import Dict


class Controller:
    """
    Controller class plots.
    """
    def __init__(self, models: Dict[str, DataArray], view: View):
        self._models = models
        self._view = view
        self._pipelines = {key: Pipeline() for key in self._models}

    def add_pipeline_step(self, step: Step, key: str = None):
        if key is None:
            for pipeline in self._pipelines.values():
                pipeline.append(step)
            step.register_callback(self._run_all_pipelines)
        else:
            self._pipelines[key].append(step)
            step.register_callback(partial(self._run_pipeline, key=key))

    def render(self):
        self._run_all_pipelines()
        self._view.render()

    def _run_all_pipelines(self):
        for key in self._pipelines:
            self._run_pipeline(key, draw=False)
        self._view.draw()

    def _run_pipeline(self, key: str, draw: bool = True):
        new_values = self._pipelines[key].run(self._models[key])
        self._view.update(new_values, key=key, draw=draw)
