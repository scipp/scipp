# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
"""
Convert RHESSI's xray flare event list to a Scipp HDF5 file.

Downloads the event list if necessary.
Output is written to ./data/hessi_flares.h5
"""

from __future__ import annotations

import re
from datetime import date, datetime, time, timedelta
from pathlib import Path

import numpy as np
import pooch

import scipp as sc

# IMPORTANT
# NASA have updated the flare list.
# It is now more heavily filtered which makes some steps in the tutorial obsolete.
# Also, the parser in this script no longer works because there are some lines
# where the total counts fill their entire column and there is no space to split by.
# This could be fixed by using that the columns each have a fixed width.
#
# The old file was apparently removed from the public server.
#
# The following code converts the output file of this script
# to the one used by the tutorial.
"""
import scipp as sc

old = sc.io.load_hdf5("rhessi_flares-old.h5")
print(old)
new = sc.DataGroup(
    flares=old,
    non_solar=old.attrs.pop("non_solar"),
    **{key: old.attrs.pop(key).value
       for key in ("detector_efficiency", "citation", "description", "url")},
)
new['flares'].coords['max_energy'] = new['flares'].attrs.pop('max_energy')
new['flares'].coords['min_energy'] = new['flares'].attrs.pop('min_energy')
new.save_hdf5("rhessi_flares.h5")
"""

DATA_DIR = Path(__file__).parent / "data"


def parse_month(m) -> int:
    return [
        "Jan",
        "Feb",
        "Mar",
        "Apr",
        "May",
        "Jun",
        "Jul",
        "Aug",
        "Sep",
        "Oct",
        "Nov",
        "Dec",
    ].index(m) + 1


def parse_date(d) -> date:
    day, month, year = d.split("-")
    return date(day=int(day), month=parse_month(month), year=int(year))


def parse_time(t) -> time:
    return time.fromisoformat(t)


def parse_datetimes(d, start, peak, end) -> dict[str, np.datetime64 | int]:
    d = parse_date(d)
    start = parse_time(start)
    peak = parse_time(peak)
    end = parse_time(end)

    start_datetime = datetime.combine(d, start)
    peak_date = d + timedelta(days=1) if peak < start else d
    peak_datetime = datetime.combine(peak_date, peak)
    end_date = d + timedelta(days=1) if end < start else d
    end_datetime = datetime.combine(end_date, end)

    return {
        "peak_time": np.datetime64(int(peak_datetime.timestamp()), "s"),
        "start_time": np.datetime64(int(start_datetime.timestamp()), "s"),
        "end_time": np.datetime64(int(end_datetime.timestamp()), "s"),
        "duration": int((end_datetime - start_datetime).total_seconds()),
    }


def flare_list_file():
    registry = pooch.create(
        path=DATA_DIR / "pooch",
        base_url="https://hesperia.gsfc.nasa.gov/hessidata/dbase/",
        registry={"hessi_flare_list.txt": "md5:89392347dbd0d954e21fe06c9c54c0dd"},
    )
    return open(registry.fetch("hessi_flare_list.txt"), "r")


def get_quality(flags: list) -> int:
    pattern = re.compile(r"Q(\d)")
    for flag in flags:
        if match := pattern.match(flag):
            return int(match[1])
    return -1


FLAGS = (
    "a0",
    "a1",
    "a2",
    "a3",
    "An",
    "DF",
    "DR",
    "ED",
    "EE",
    "ES",
    "FE",
    "FR",
    "FS",
    "GD",
    "GE",
    "GS",
    "MR",
    "NS",
    "PE",
    "PS",
    "Pn",
    "Qn",
    "SD",
    "SE",
    "SS",
)


def parse_line(line):
    fields = [c for c in line.split(" ") if c]
    times = parse_datetimes(*fields[1:5])
    flags = fields[13:]
    eclipsed = "ED" in flags or "EE" in flags or "ES" in flags
    non_solar = "NS" in flags
    quality = get_quality(flags)

    return {
        "flare_id": int(fields[0]),
        "peak_time": times["peak_time"],
        "start_time": times["start_time"],
        "end_time": times["end_time"],
        "total_counts": float(fields[7]),
        "energy_range": list(map(float, fields[8].split("-"))),
        "x": float(fields[9]),
        "y": float(fields[10]),
        "radial": float(fields[11]),
        "eclipsed": eclipsed,
        "non_solar": non_solar,
        "quality": quality,
        **{name: name in flags for name in FLAGS},
    }


def load_txt_file():
    values = {}

    # Use to remove duplicates.
    # Way faster than searching through flare_id list for every line.
    seen = set()

    with flare_list_file() as f:
        for _ in range(7):
            f.readline()

        while line := f.readline().strip():
            entry = parse_line(line)
            if entry["flare_id"] in seen:
                continue
            seen.add(entry["flare_id"])

            for key, val in entry.items():
                values.setdefault(key, []).append(val)

    energy_range = sc.array(
        dims=["flare", "energy"], values=values.pop("energy_range"), unit="keV"
    )

    def event_array(name, unit):
        return sc.array(dims=["flare"], values=values.pop(name), unit=unit)

    return sc.DataArray(
        sc.ones(sizes={"flare": len(values["peak_time"])}),
        coords={
            "total_counts": event_array("total_counts", "count"),
            "peak_time": event_array("peak_time", "s"),
            "start_time": event_array("start_time", "s"),
            "end_time": event_array("end_time", "s"),
            "x": event_array("x", "asec"),
            "y": event_array("y", "asec"),
            "radial": event_array("radial", "asec"),
        },
        attrs={
            "min_energy": energy_range["energy", 0],
            "max_energy": energy_range["energy", 1],
            "quality": event_array("quality", None),
            **{key: event_array(key, None) for key in list(values)},
            "description": sc.scalar(
                "X-ray flares recorded by NASA's Reuven Ramaty High Energy Solar"
                " Spectroscopic Imager (RHESSI) Small Explorer"
            ),
            "url": sc.scalar(
                "https://hesperia.gsfc.nasa.gov/rhessi3/data-access/rhessi-data/flare-list/index.html"
            ),
            "citation": sc.scalar("https://doi.org/10.1023/A:1022428818870"),
        },
    )


def prefilter(da):
    da = da.copy()
    del da.coords["total_counts"]
    del da.coords["radial"]
    del da.attrs["flare_id"]
    da = da[~da.attrs.pop("eclipsed")]
    # no quality flag
    da = da[da.attrs["quality"] >= sc.index(0)]
    # only high quality
    da = da[da.attrs["quality"] < sc.index(3)]
    del da.attrs["quality"]
    # PS - Possible Solar Flare; in front detectors, but no position
    da = da[~da.attrs.pop("PS")]

    for flag in FLAGS:
        da.attrs.pop(flag, None)
    return da


def remove_events(da, rng):
    """
    Define fictitious efficiencies for the different collimators of the detector.
    Uses a 3x3 detector grid with arbitrarily chosen apertures.
    Events are removed randomly based on the efficiencies such that the data
    can be normalised using `events / efficiency`.
    """
    print("removing events based on detector efficiency")  # noqa: T201
    collimator_x = sc.array(
        dims=["x"], values=[-1000, -300, 300, 1000], unit="asec", dtype="float64"
    )
    collimator_y = sc.array(
        dims=["y"], values=[-600, -200, 200, 600], unit="asec", dtype="float64"
    )
    efficiency = sc.DataArray(
        sc.array(
            dims=["x", "y"],
            values=np.array(
                [[0.9, 0.95, 0.77], [0.98, 0.89, 0.82], [0.93, 0.94, 0.91]]
            ),
        ),
        coords={"x": collimator_x, "y": collimator_y},
    )

    da = sc.sort(da, "x")
    filtered = []
    for i in range(len(collimator_x) - 1):
        column = sc.sort(da["x", collimator_x[i] : collimator_x[i + 1]], "y")
        filtered.append(column["y", -1e8 * collimator_y.unit : collimator_y[0]])
        filtered.append(column["y", collimator_y[-1] : 1e8 * collimator_y.unit])
        for j in range(len(collimator_y) - 1):
            cell = column["y", collimator_y[j] : collimator_y[j + 1]]
            n = cell.sizes["flare"]
            selected = np.sort(
                rng.choice(
                    n, size=int(n * efficiency["x", i]["y", j].value), replace=False
                )
            )
            filtered.append(cell[selected])

    filtered.append(da["x", -1e8 * collimator_x.unit : collimator_x[0]])
    filtered.append(da["x", collimator_x[-1] : 11e8 * collimator_x.unit])

    out = sc.sort(sc.concat(filtered, "flare"), "peak_time")
    out.attrs["detector_efficiency"] = sc.scalar(efficiency)
    return out


def main():
    rng = np.random.default_rng(9274)
    da = load_txt_file()
    da = prefilter(da)
    da = remove_events(da, rng)
    da.save_hdf5(DATA_DIR / "rhessi_flares.h5")


if __name__ == "__main__":
    main()
