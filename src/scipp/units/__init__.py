# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
"""
Predefined units.

The following predefined units are available:

{unit_list}

.. seealso::
    :py:class:`scipp.Unit` to construct other units.
"""

from .._scipp.core.units import *  # noqa

__doc__ = __doc__.format(unit_list='\n'.join(f'- {unit}' for unit in locals()
                                             if not unit.startswith('_')))
