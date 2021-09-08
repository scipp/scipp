from .. import typing
from ..utils import name_with_unit, value_to_string
from ..core import arange, to_unit, Unit
import enum
import numpy as np


class LabelFormatter:
    """
    Format ticks by replacing coord values with label values.
    """
    def __init__(self, labels, coord):
        self.label_values = labels.values
        self.coord_values = coord.values

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

    def formatter(self, val, pos):
        return "(" + ",".join([
            value_to_string(item, precision=2) for item in self.array_values[int(val)]
        ]) + ")" if (int(val) >= 0 and int(val) < self.size) else ""


class StringFormatter:
    """
    Format string ticks: find closest string in coordinate array.
    """
    def __init__(self, array_values, size):
        self.array_values = array_values
        self.size = size

    def formatter(self, val, pos):
        return self.array_values[int(val)] if (int(val) >= 0
                                               and int(val) < self.size) else ""


class DateFormatter:
    """
    Format datetime ticks: adjust the time precision and update
    offset according to the currently displayed range.
    """
    def __init__(self, offset, dim):
        self.offset = offset
        self.dim = dim
        self.indicators = []

    def formatter(self, val, pos, axis=None, get_axis_bounds=None, set_axis_label=None):
        d = (self.offset + (int(val) * self.offset.unit)).value
        dt = str(d)
        if pos is None:  # Return full string, not split into label + offset
            return dt
        trim = 0
        bounds = get_axis_bounds(axis)
        diff = (bounds[1] - bounds[0]) * self.offset.unit
        label = self.dim
        if pos == 0:
            self.indicators.clear()

        date_min = str((int(bounds[0]) * self.offset.unit + self.offset).value)
        date_max = str((int(bounds[1]) * self.offset.unit + self.offset).value)

        check_transition = True
        check_time = True
        check_ms = False

        if (diff < to_unit(4 * Unit('us'), diff.unit)).value:
            # label: 2017-01-13T12:15:45.123, tick: 456.789 us
            label += r" [$\mu$s]"
            string = str(float("{}.{}".format(dt[23:26], dt[26:])))
            check_ms = True
        elif (diff < to_unit(4 * Unit('ms'), diff.unit)).value:
            # label: 2017-01-13T12:15:45, tick: 123.456 ms
            label += " [ms]"
            string = str(float("{}.{}".format(dt[20:23], dt[23:26])))
        elif (diff < to_unit(4 * Unit('s'), diff.unit)).value:
            # label: 2017-01-13T12:15:45, tick: 123 ms
            label += " [ms]"
            string = str(float(dt[20:23]))
        elif (diff < to_unit(4 * Unit('min'), diff.unit)).value:
            # label: 2017-01-13, tick: 12:15:45.1
            string = dt[11:21]
            check_time = False
        elif (diff < to_unit(4 * Unit('h'), diff.unit)).value:
            # label: 2017-01-13, tick: 12:15:45
            string = dt[11:19]
            check_time = False
        elif (diff < to_unit(4 * Unit('d'), diff.unit)).value:
            # label: 2017-01-13, tick: 12:15
            string = dt[11:16]
            check_time = False
        elif (diff < to_unit(6 * Unit('month'), diff.unit)).value:
            # label: 2017-01-13, tick: 12h
            string = dt[11:13] + 'h'
            check_time = False
        elif (diff < to_unit(4 * Unit('year'), diff.unit)).value:
            # label: 2017, tick: 01-13
            string = dt[:10]
            check_transition = False
        elif (diff < to_unit(100 * Unit('year'), diff.unit)).value:
            # tick: 2017-01
            string = dt[:7]
            check_transition = False
        else:
            # tick: 2017
            string = dt[:4]
            check_transition = False

        if check_transition:
            string, trim = self.check_for_transition(pos, string, date_min, date_max,
                                                     dt, check_time, check_ms)

        if pos == 1:
            if trim > 0:
                label += f" ({dt[:trim]})"
            set_axis_label(axis, label)
        return string

    def check_for_transition(self, pos, string, date_min, date_max, dt, check_time,
                             check_ms):
        """
        Function that checks for transitions between years, months, days etc..
        adds an additional row of information below the tick labels if
        required, to indicate the year on each side of the transition.
        """
        different_date = False
        different_time = False

        trim = 0
        date_start = 0
        time_start = 11
        date_end = 10
        time_end = 0

        different_date = date_min[:date_end] != date_max[:date_end]

        if check_time:
            if check_ms:
                time_end = 23
            else:
                time_end = 19
            different_time = date_min[time_start:time_end] != date_max[
                time_start:time_end]

        if (not different_date) and (not different_time):
            trim = max(date_end, time_end)
        else:
            if different_date and different_time:
                start = date_start
                end = time_end
            elif different_date:
                start = date_start
                end = date_end
            elif different_time:
                start = time_start
                end = time_end
                trim = time_start - 1
            if pos != 0 and (dt[start:end] not in self.indicators):
                string += '\n' + dt[start:end]
                self.indicators.append(dt[start:end])

        return string, trim


class Kind(enum.Enum):
    """
    Small enum listing the special cases for the axis tick formatters.
    """
    vector = enum.auto()
    string = enum.auto()
    datetime = enum.auto()
    other = enum.auto()


def _dtype_kind(var):
    if typing.has_vector_type(var):
        return Kind.vector
    elif typing.has_string_type(var):
        return Kind.string
    elif typing.has_datetime_type(var):
        return Kind.datetime
    else:
        return Kind.other


def _get_or_make_coord(array, dim):
    if dim in array.meta:
        coord = array.meta[dim]
        if _dtype_kind(coord) not in [Kind.vector, Kind.string]:
            return coord
    return arange(dim=dim, start=0, stop=array.sizes[dim])


def make_formatter(array, key):
    """
    Get dimensions from requested axis.
    Also return axes tick formatters and locators.
    """

    # Create some default axis tick formatter, depending on linear or log
    # scaling.
    formatter = {"linear": None, "log": None, "custom_locator": False}

    labels = None
    dim = key
    if key in array.meta:
        labels = array.meta[key]
        dim = key if key in labels.dims else labels.dims[-1]
        kind = _dtype_kind(labels)
        if kind == Kind.vector:
            form = VectorFormatter(labels.values, array.sizes[dim]).formatter
            formatter["custom_locator"] = True
        elif kind == Kind.string:
            form = StringFormatter(labels.values, array.sizes[dim]).formatter
            formatter["custom_locator"] = True
        elif kind == Kind.datetime:
            coord = _get_or_make_coord(array, dim)
            form = DateFormatter(offset=coord.min(), dim=key).formatter
            formatter["need_callbacks"] = True
        elif dim is not key:
            coord = _get_or_make_coord(array, dim)
            form = LabelFormatter(labels, coord).formatter
        else:
            form = None
        formatter.update({"linear": form, "log": form})
    formatter['unit'] = name_with_unit(var=labels, name="")
    formatter['label'] = name_with_unit(var=labels, name=key)

    return formatter
