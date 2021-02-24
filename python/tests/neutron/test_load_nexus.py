from nexus_helpers import (InMemoryNexusFileBuilder, EventData,
                           in_memory_hdf5_file_with_no_nxentry)
import scipp as sc
import numpy as np
import pytest


def test_load_nexus_raises_exception_if_no_NXentry_in_file():
    with in_memory_hdf5_file_with_no_nxentry() as nexus_file:
        with pytest.raises(RuntimeError):
            sc.neutron.load_nexus(nexus_file)


def test_load_nexus_loads_data_from_single_nxevent_data_group():
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

    counts_on_detectors = loaded_data.bins.sum()
    # No detector_numbers dataset in file so expect detector-id to be
    # binned from the min to the max detector-id recorded in event_id
    # dataset: 2 on det 1, 1 on det 2, 2 on det 3
    expected_counts = np.array([2, 1, 2])
    assert np.allclose(counts_on_detectors.data.values, expected_counts)
    expected_detector_ids = np.array([1, 2, 3])
    assert np.allclose(loaded_data.coords['detector-id'].values,
                       expected_detector_ids)

    # Expect time of flight to match the values in the
    # event_time_offset dataset
    # May be reordered due to binning (hence np.sort)
    assert np.allclose(
        np.sort(
            loaded_data.bins.concatenate(
                'detector-id').values.coords['tof'].values),
        np.sort(event_time_offsets))


def test_load_nexus_loads_data_from_multiple_nxevent_data_groups():
    pulse_times = np.array([
        1600766730000000000, 1600766731000000000, 1600766732000000000,
        1600766733000000000
    ])
    event_data_1 = EventData(
        event_id=np.array([1, 2, 3, 1, 3]),
        event_time_offset=np.array([456, 743, 347, 345, 632]),
        event_time_zero=pulse_times,
        event_index=np.array([0, 3, 3, 5]),
    )
    event_data_2 = EventData(
        event_id=np.array([4, 5, 6, 4, 6]),
        event_time_offset=np.array([456, 743, 347, 345, 632]),
        event_time_zero=pulse_times,
        event_index=np.array([0, 3, 3, 5]),
    )

    builder = InMemoryNexusFileBuilder()
    builder.add_event_data(event_data_1)
    builder.add_event_data(event_data_2)

    with builder.file() as nexus_file:
        loaded_data = sc.neutron.load_nexus(nexus_file)

    # TODO test expected tof values?
    #  test numbers of events in id bins is as expected

    # TODO provide detector_numbers in NXdetector parent and
    #  check they are used for the bins

    print(loaded_data)
