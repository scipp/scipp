from .tools import date2cal
from .._utils import value_to_string
from .._scipp.core import to_unit, Unit
import numpy as np
import re
import os


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
        boundary = None
        # os.write(1, "got to here 1\n".encode())
        bounds = self.interface["get_view_axis_bounds"](self.dim)
        diff = (bounds[1] - bounds[0]) * self.offset.unit
        # os.write(1, "got to here 2\n".encode())
        # os.write(1, (str(bounds) + '\n').encode())
        label = self.dim
        # os.write(1, "got to here 3\n".encode())
        # os.write(1, ("a " + str(bounds[0] * offset.unit) + '\n').encode())
        # os.write(1, ("b " + str(int(bounds[0]) * offset.unit + offset) +
        #              '\n').encode())
        # os.write(1, ("c " + str(
        #     (int(bounds[0]) * offset.unit + offset).value) +
        #              '\n').encode())
        if pos == 0:
            self.indicators.clear()

        date_min = date2cal(
            (int(bounds[0]) * self.offset.unit + self.offset).value)
        # os.write(1, "got to here 4\n".encode())
        date_max = date2cal(
            (int(bounds[1]) * self.offset.unit + self.offset).value)
        if (diff < to_unit(4 * Unit('us'), diff.unit)).value:
            # label: 2017-01-13T12:15:45.123, tick: 456.789 us
            # trim = 23
            label += r" [$\mu$s]"
            string = str(float("{}.{}".format(dt[23:26], dt[26:])))
            string, trim = self.check_for_transition(pos, string, date_min,
                                                     date_max, date, dt, 7,
                                                     'ms')
        elif (diff < to_unit(4 * Unit('ms'), diff.unit)).value:
            # label: 2017-01-13T12:15:45, tick: 123.456 ms
            # trim = 19
            label += " [ms]"
            string = str(float("{}.{}".format(dt[20:23], dt[23:26])))
            string, trim = self.check_for_transition(pos, string, date_min,
                                                     date_max, date, dt, 6,
                                                     's')

        elif (diff < to_unit(4 * Unit('s'), diff.unit)).value:
            # label: 2017-01-13T12:15, tick: 45.123 s
            # trim = 16
            label += " [s]"
            string = str(float(dt[17:23]))
            string, trim = self.check_for_transition(pos, string, date_min,
                                                     date_max, date, dt, 5,
                                                     'm')

        elif (diff < to_unit(4 * Unit('min'), diff.unit)).value:
            # label: 2017-01-13, tick: 12:15:45
            # trim = 10
            string = dt[14:21]
            string, trim = self.check_for_transition(pos, string, date_min,
                                                     date_max, date, dt, 4)
        elif (diff < to_unit(4 * Unit('h'), diff.unit)).value:
            # label: 2017-01-13, tick: 12:15:45
            # trim = 10
            string = dt[11:19]
            string, trim = self.check_for_transition(pos, string, date_min,
                                                     date_max, date, dt, 3)
        elif (diff < to_unit(4 * Unit('d'), diff.unit)).value:
            # label: 2017-01, tick: 13 12:15
            # trim = 7
            string = dt[8:16].replace('T', ' ')

            # # Find non-equal year and month
            # tick_indicator = False
            # splits = [4, 7, 10, 13, 16, 19]
            # ind = None
            # not_equal = np.ravel(np.where(date_min[:2] != date_max[:2]))
            # if len(not_equal) > 0:
            #     ind = np.amin(not_equal)
            #     if pos == 1:
            #         tick_indicator = True
            #     if (date[0] == date_max[0]) and (not self.indicator_set):
            #         tick_indicator = True
            #         self.indicator_set = True
            #     if tick_indicator:
            #         string += "\n{}".format(dt[:splits[ind]])
            # else:
            #     trim = 7

            string, trim = self.check_for_transition(pos, string, date_min,
                                                     date_max, date, dt, 2)

            # if date_min[0] != date_max[0]:
            #     if pos == 1:
            #         tick_indicator = True
            #     if (date[0] == date_max[0]) and (not self.indicator_set):
            #         tick_indicator = True
            #         self.indicator_set = True
            #     if tick_indicator:
            #         string += "\n{}".format(date[0])

        elif (diff < to_unit(6 * Unit('month'), diff.unit)).value:
            # label: 2017, tick: 01-13
            string = dt[5:13].replace('T', ' ') + 'h'
            # tick_indicator = False
            # if date_min[0] != date_max[0]:
            #     if pos == 1:
            #         tick_indicator = True
            #     if (date[0] == date_max[0]) and (not self.indicator_set):
            #         tick_indicator = True
            #         self.indicator_set = True
            #     if tick_indicator:
            #         string += "\n{}".format(date[0])
            # else:
            #     trim = 4
            string, trim = self.check_for_transition(pos, string, date_min,
                                                     date_max, date, dt, 1)

        elif (diff < to_unit(4 * Unit('year'), diff.unit)).value:
            string = dt[5:10]
            string, trim = self.check_for_transition(pos, string, date_min,
                                                     date_max, date, dt, 1)
        elif (diff < to_unit(100 * Unit('year'), diff.unit)).value:
            # tick: 2017-01
            string = dt[:7]
        else:
            # tick: 2017
            string = dt[:4]

        # os.write(1, ('got to here 46 {} {}\n'.format(string, trim)).encode())

        if pos == 1:
            # offstring = dt[:trim]
            if trim > 0:
                # offstring = "+" + offstring
                label += f" ({dt[:trim]})"
            # os.write(1, ('got to here 47\n').encode())
            # if boundary is not None:
            #     # os.write(1, ('got to here 47.5\n').encode())
            #     # os.write(1, (str(self.interface) + '\n').encode())
            #     self.interface["set_view_axis_offset"](dim,
            #                                            boundary["text"],
            #                                            boundary["loc"])
            #     # os.write(1, ('got to here 48\n').encode())
            # else:
            # self.interface["set_view_axis_offset"](self.dim, offstring)
            self.interface["set_view_axis_label"](self.dim, label)
        return string

    # return formatter

    def check_for_transition(self,
                             pos,
                             string,
                             date_min,
                             date_max,
                             date,
                             dt,
                             max_ind,
                             suffix=None):
        # Find non-equal year and month
        # tick_indicator = False
        splits = [0, 4, 7, 10, 13, 16, 19, 23]
        ind = None
        trim = 0
        # Find first year/month/day etc.. that differs
        not_equal = np.ravel(
            np.where(date_min[:max_ind] != date_max[:max_ind]))
        # os.write(1, "{}\n".format(date_min[:max_ind]).encode())
        # os.write(1, "{}\n".format(date_max[:max_ind]).encode())
        # os.write(1, "{}\n".format(not_equal).encode())
        if len(not_equal) > 0:
            ind = np.amin(not_equal)
            # return string, trim
            # os.write(1, "{} {} {}\n".format(ind, date[ind], pos).encode())
            if pos != 0 and (date[ind] not in self.indicators):
                # return string, trim
                # if pos == 1:
                #     tick_indicator = True
                # if (date[ind] == date_max[ind]) and (date[ind]
                #                                      not in self.indicators):
                # tick_indicator = True
                self.indicators.append(date[ind])
                # if tick_indicator:
                # ind_m1 =
                transition_marker = dt[splits[ind]:splits[max_ind]].strip(
                    '-').strip(':').strip('.')
                transition_marker = re.sub(r'(T)(\d\d)', r' \2h',
                                           transition_marker)
                # if transition_marker[0] == 'T':
                #     transition_marker = transition_marker[1:] + 'h'
                if suffix is not None:
                    transition_marker += suffix
                string += f"\n{transition_marker}"
                # if ind > 0:
                trim = splits[ind]
        else:
            trim = splits[max_ind]
        return string, trim


# 2017 - 01 - 31
# 2017 - 02 - 03