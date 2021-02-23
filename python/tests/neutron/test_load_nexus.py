from nexus_helpers import InMemoryNexusFileBuilder, EventData
import scipp as sc
import numpy as np


def test_load_nexus_loads_data_from_single_nxevent_data_group():
    event_data = EventData(
        event_id=np.array([1, 2, 3, 1, 3]),
        event_time_offset=np.array([456, 743, 347, 345, 632]),
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
    # TODO test expected tof values?
    #  test numbers of events in id bins is as expected
    print(loaded_data)


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
    print(loaded_data)
