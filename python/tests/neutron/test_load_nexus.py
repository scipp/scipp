from nexus_helpers import (
    InMemoryNexusFileBuilder,
    EventData,
    Detector,
    Log,
    in_memory_hdf5_file_with_two_nxentry,
)
import scipp as sc
import numpy as np
import pytest


def test_load_nexus_raises_exception_if_multiple_NXentry_in_file():
    with in_memory_hdf5_file_with_two_nxentry() as nexus_file:
        with pytest.raises(RuntimeError):
            sc.neutron.load_nexus(nexus_file)


def test_load_nexus_no_exception_if_single_NXentry_in_file():
    builder = InMemoryNexusFileBuilder()
    with builder.file() as nexus_file:
        assert sc.neutron.load_nexus(nexus_file) is None


def test_load_nexus_no_exception_if_single_NXentry_found_below_root():
    with in_memory_hdf5_file_with_two_nxentry() as nexus_file:
        # There are 2 NXentry in the file, but root is used
        # to specify which to load data from
        assert sc.neutron.load_nexus(nexus_file, root='/entry_1') is None


def test_load_nexus_loads_data_from_single_event_data_group():
    event_time_offsets = np.array([456, 743, 347, 345, 632])
    event_data = EventData(
        event_id=np.array([1, 2, 3, 1, 3]),
        event_time_offset=event_time_offsets,
        event_time_zero=np.array([
            1600766730000000000, 1600766731000000000, 1600766732000000000,
            1600766733000000000
        ]),
        event_index=np.array([0, 3, 3, 5]),
    )

    builder = InMemoryNexusFileBuilder()
    builder.add_event_data(event_data)

    with builder.file() as nexus_file:
        loaded_data = sc.neutron.load_nexus(nexus_file)

    # Expect time of flight to match the values in the
    # event_time_offset dataset
    # May be reordered due to binning (hence np.sort)
    assert np.allclose(
        np.sort(
            loaded_data.bins.concatenate(
                'detector-id').values.coords['tof'].values),
        np.sort(event_time_offsets))

    counts_on_detectors = loaded_data.bins.sum()
    # No detector_number dataset in file so expect detector-id to be
    # binned from the min to the max detector-id recorded in event_id
    # dataset: 2 on det 1, 1 on det 2, 2 on det 3
    expected_counts = np.array([2, 1, 2])
    assert np.allclose(counts_on_detectors.data.values, expected_counts)
    expected_detector_ids = np.array([1, 2, 3])
    assert np.allclose(loaded_data.coords['detector-id'].values,
                       expected_detector_ids)


def test_load_nexus_loads_data_from_multiple_event_data_groups():
    pulse_times = np.array([
        1600766730000000000, 1600766731000000000, 1600766732000000000,
        1600766733000000000
    ])
    event_time_offsets_1 = np.array([456, 743, 347, 345, 632])
    event_data_1 = EventData(
        event_id=np.array([1, 2, 3, 1, 3]),
        event_time_offset=event_time_offsets_1,
        event_time_zero=pulse_times,
        event_index=np.array([0, 3, 3, 5]),
    )
    detector_1_ids = np.array([0, 1, 2, 3])
    event_time_offsets_2 = np.array([682, 237, 941, 162, 52])
    event_data_2 = EventData(
        event_id=np.array([4, 5, 6, 4, 6]),
        event_time_offset=event_time_offsets_2,
        event_time_zero=pulse_times,
        event_index=np.array([0, 3, 3, 5]),
    )
    detector_2_ids = np.array([4, 5, 6, 7])

    builder = InMemoryNexusFileBuilder()
    builder.add_detector(Detector(detector_1_ids, event_data_1))
    builder.add_detector(Detector(detector_2_ids, event_data_2))

    with builder.file() as nexus_file:
        loaded_data = sc.neutron.load_nexus(nexus_file)

    # Expect time of flight to match the values in the
    # event_time_offset datasets
    # May be reordered due to binning (hence np.sort)
    assert np.allclose(
        np.sort(
            loaded_data.bins.concatenate(
                'detector-id').values.coords['tof'].values),
        np.sort(np.concatenate((event_time_offsets_1, event_time_offsets_2))))

    counts_on_detectors = loaded_data.bins.sum()
    # There are detector_number datasets in the NXdetector for each
    # NXevent_data, these are used for detector-id binning
    expected_counts = np.array([0, 2, 1, 2, 2, 1, 2, 0])
    assert np.allclose(counts_on_detectors.data.values, expected_counts)
    expected_detector_ids = np.concatenate((detector_1_ids, detector_2_ids))
    assert np.allclose(loaded_data.coords['detector-id'].values,
                       expected_detector_ids)


def test_load_nexus_loads_data_from_single_log_with_no_units():
    values = np.array([1, 2, 3])
    times = np.array([4, 5, 6])
    name = "test_log"
    builder = InMemoryNexusFileBuilder()
    builder.add_log(Log(name, values, times))

    with builder.file() as nexus_file:
        loaded_data = sc.neutron.load_nexus(nexus_file)

    # Expect a sc.Dataset with log names as keys
    assert np.allclose(loaded_data[name].data.values.values, values)
    assert np.allclose(loaded_data[name].data.values.coords['time'], times)


def test_load_nexus_loads_data_from_single_log_with_units():
    values = np.array([1.1, 2.2, 3.3])
    times = np.array([4.4, 5.5, 6.6])
    name = "test_log"
    builder = InMemoryNexusFileBuilder()
    builder.add_log(Log(name, values, times, value_units="m", time_units="s"))

    with builder.file() as nexus_file:
        loaded_data = sc.neutron.load_nexus(nexus_file)

    # Expect a sc.Dataset with log names as keys
    assert np.allclose(loaded_data[name].data.values.values, values)
    assert np.allclose(loaded_data[name].data.values.coords['time'], times)
    assert loaded_data[name].data.values.unit == sc.units.m
    assert loaded_data[name].data.values.coords['time'].unit == sc.units.s


def test_load_nexus_loads_data_from_multiple_logs():
    builder = InMemoryNexusFileBuilder()
    log_1 = Log("test_log", np.array([1.1, 2.2, 3.3]),
                np.array([4.4, 5.5, 6.6]))
    log_2 = Log("test_log_2", np.array([123, 253, 756]),
                np.array([246, 1235, 2369]))
    builder.add_log(log_1)
    builder.add_log(log_2)

    with builder.file() as nexus_file:
        loaded_data = sc.neutron.load_nexus(nexus_file)

    # Expect a sc.Dataset with log names as keys
    assert np.allclose(loaded_data[log_1.name].data.values.values, log_1.value)
    assert np.allclose(loaded_data[log_1.name].data.values.coords['time'],
                       log_1.time)
    assert np.allclose(loaded_data[log_2.name].data.values.values, log_2.value)
    assert np.allclose(loaded_data[log_2.name].data.values.coords['time'],
                       log_2.time)


def test_load_nexus_skips_multidimensional_log():
    # Loading NXlogs with more than 1 dimension is not yet implemented
    # We need to come up with a sensible approach to labelling the dimensions

    multidim_values = np.array([[1, 2, 3], [1, 2, 3]])
    name = "test_log"
    builder = InMemoryNexusFileBuilder()
    builder.add_log(Log(name, multidim_values, np.array([4, 5, 6])))

    with builder.file() as nexus_file:
        loaded_data = sc.neutron.load_nexus(nexus_file)

    assert loaded_data is None


def test_load_nexus_loads_data_from_non_timeseries_log():
    values = np.array([1.1, 2.2, 3.3])
    name = "test_log"
    builder = InMemoryNexusFileBuilder()
    builder.add_log(Log(name, values))

    with builder.file() as nexus_file:
        loaded_data = sc.neutron.load_nexus(nexus_file)

    assert np.allclose(loaded_data[name].data.values.values, values)


def test_load_instrument_name():
    name = "INSTR"
    builder = InMemoryNexusFileBuilder()
    builder.add_instrument(name)

    with builder.file() as nexus_file:
        loaded_data = sc.neutron.load_nexus(nexus_file)

    assert loaded_data['instrument-name'].values == name


# TODO test
#  - pixel positions
#  - sample and source position
#  - duplicate log name (parent name prefixing)
#  - log data and event data in single file (everything)
#  - other sample info?
#  Remove and make tickets for any remaining todos (time offsets)
