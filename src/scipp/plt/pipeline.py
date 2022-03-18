# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)


class Pipeline(list):
    pass

    def run(self, model):
        out = model
        for step in self:
            out = step(out)
        return out
