# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .. import DataArray
from .filters import Workflow, Filter
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
        self._workflows = {key: Workflow() for key in self._models}

    def add_filter(self, filt: Filter, key: str = None):
        if key is None:
            for workflow in self._workflows.values():
                workflow.append(filt)
            filt.register_callback(self._run_all_workflows)
        else:
            self._workflows[key].append(filt)
            filt.register_callback(partial(self._run_workflow, key=key))

    def render(self):
        self._run_all_workflows()
        self._view.render()

    def _run_all_workflows(self):
        for key in self._workflows:
            self._run_workflow(key, draw=False)
        self._view.draw()

    def _run_workflow(self, key: str, draw: bool = True):
        new_values = self._workflows[key].run(self._models[key])
        self._view.update(new_values, key=key, draw=draw)
