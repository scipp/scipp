from .tools import date2cal
from .._utils import value_to_string
from .._scipp.core import to_unit, Unit
import numpy as np
import re


class LabelFormatter:
    """
    Format ticks by replacing coord values with label values.
    """
    def __init__(self, label_values, coord_values):
        self.label_values = label_values
        self.coord_values = coord_values

    def formatter(self, val, pos):
        return value_to_string(self.label_values[np.abs(self.coord_values -
                                                        val).argmin()])


class VectorFormatter:
    """
    Format vector output for ticks: return 3 components as a string.
    """
    def __init__(self, array_values, size):
        self.array_values = array_values
        self.size = size

    def format(self, val, pos):
        return "(" + ",".join([
            value_to_string(item, precision=2)
            for item in self.array_values[int(val)]
        ]) + ")" if (int(val) >= 0 and int(val) < self.size) else ""


class StringFormatter:
    """
    Format string ticks: find closest string in coordinate array.
    """
    def __init__(self, array_values, size):
        self.array_values = array_values
        self.size = size

    def formatter(self, val, pos):
        return self.array_values[int(val)] if (
            int(val) >= 0 and int(val) < self.size) else ""


class DateFormatter:
    """
    Format datetime ticks: adjust the time precision and update
    offset according to the currently displayed range.
    """
    def __init__(self, offset, dim, interface):
        self.offset = offset
        self.dim = dim
        self.interface = interface
        self.indicators = []

    def formatter(self, val, pos):
        d = (self.offset + (int(val) * self.offset.unit)).value
        dt = str(d)
        date = date2cal(d)
        trim = 0
        bounds = self.interface["get_view_axis_bounds"](self.dim)
        diff = (bounds[1] - bounds[0]) * self.offset.unit
        label = self.dim
        if pos == 0:
            self.indicators.clear()

        date_min = date2cal(
            (int(bounds[0]) * self.offset.unit + self.offset).value)
        date_max = date2cal(
            (int(bounds[1]) * self.offset.unit + self.offset).value)

        suffix = None
        level = None

        if (diff < to_unit(4 * Unit('us'), diff.unit)).value:
            # label: 2017-01-13T12:15:45.123, tick: 456.789 us
            label += r" [$\mu$s]"
            string = str(float("{}.{}".format(dt[23:26], dt[26:])))
            level = 7
            suffix = 'ms'
        elif (diff < to_unit(4 * Unit('ms'), diff.unit)).value:
            # label: 2017-01-13T12:15:45, tick: 123.456 ms
            label += " [ms]"
            string = str(float("{}.{}".format(dt[20:23], dt[23:26])))
            level = 6
            suffix = 's'
        elif (diff < to_unit(4 * Unit('s'), diff.unit)).value:
            # label: 2017-01-13T12:15, tick: 45.123 s
            label += " [s]"
            string = str(float(dt[17:23]))
            level = 5
            suffix = 'm'
        elif (diff < to_unit(4 * Unit('min'), diff.unit)).value:
            # label: 2017-01-13T12, tick: 15:45.1
            string = dt[14:21]
            level = 4
        elif (diff < to_unit(4 * Unit('h'), diff.unit)).value:
            # label: 2017-01-13, tick: 12:15:45
            string = dt[11:19]
            level = 3
        elif (diff < to_unit(4 * Unit('d'), diff.unit)).value:
            # label: 2017-01, tick: 13 12:15
            string = dt[8:16].replace('T', ' ')
            level = 2
        elif (diff < to_unit(6 * Unit('month'), diff.unit)).value:
            # label: 2017, tick: 01-13 12h
            string = dt[5:13].replace('T', ' ') + 'h'
            level = 1
        elif (diff < to_unit(4 * Unit('year'), diff.unit)).value:
            # label: 2017, tick: 01-13
            string = dt[5:10]
            level = 1
        elif (diff < to_unit(100 * Unit('year'), diff.unit)).value:
            # tick: 2017-01
            string = dt[:7]
        else:
            # tick: 2017
            string = dt[:4]

        if level is not None:
            string, trim = self.check_for_transition(pos, string, date_min,
                                                     date_max, date, dt, level,
                                                     suffix)

        if pos == 1:
            if trim > 0:
                label += f" ({dt[:trim]})"
            self.interface["set_view_axis_label"](self.dim, label)
        return string

    def check_for_transition(self,
                             pos,
                             string,
                             date_min,
                             date_max,
                             date,
                             dt,
                             max_ind,
                             suffix=None):
        """
        Function that checks for transitions between years, months, days etc..
        adds an additional row of information below the tick labels if
        required, to indicate the year on each side of the transition.
        """
        splits = [0, 4, 7, 10, 13, 16, 19, 23]
        ind = None
        trim = 0
        # Find first year/month/day etc.. that differs
        not_equal = np.ravel(
            np.where(date_min[:max_ind] != date_max[:max_ind]))
        if len(not_equal) > 0:
            ind = np.amin(not_equal)
            if pos != 0 and (date[ind] not in self.indicators):
                self.indicators.append(date[ind])
                transition_marker = dt[splits[ind]:splits[max_ind]].strip(
                    '-').strip(':').strip('.')
                transition_marker = re.sub(r'(T)(\d\d)', r' \2h',
                                           transition_marker)
                if suffix is not None:
                    transition_marker += suffix
                string += f"\n{transition_marker}"
                trim = splits[ind]
        else:
            trim = splits[max_ind]
        return string, trim
