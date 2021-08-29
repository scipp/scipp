# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
from scippbuildtools.sphinxconf import *  # noqa: F403

project = u'scipp'

nbsphinx_prolog = nbsphinx_prolog.replace("XXXX", "scipp")  # noqa: F405

html_logo = "_static/logo-large-v4.png"
