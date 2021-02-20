# from nexus_helpers import in_memory_nexus_file_with_event_data
import scipp as sc


def test_load_nexus_loads_event_data_from_nxevent_data_group():
    data = sc.Variable(dims=['row'], values=[1, 2, 3, 4])
    begin = sc.Variable(dims=['x'], values=[0, 2], dtype=sc.dtype.int64)
    end = sc.Variable(dims=['x'], values=[2, 4], dtype=sc.dtype.int64)

    events = sc.DataArray(data=sc.bins(begin=begin,
                                       end=end,
                                       dim='row',
                                       data=data),
                          coords={
                              'x':
                              sc.Variable(dims=['row'],
                                          values=[0.1, 0.2, 0.3, 0.4]),
                              'id':
                              sc.Variable(dims=['row'], values=[1, 2, 1, 3])
                          })
    out = sc.bin(events,
                 groups=[sc.Variable(['id'], values=[1, 2, 3])],
                 erase=["x"])

    print(out)

    # with in_memory_nexus_file_with_event_data() as nexus_file:
    # loaded_data = sc.neutron.load_nexus(nexus_file)
    loaded_data = sc.neutron.load_nexus("PG3_4844_event.nxs")
    # TODO test expect dtypes, tof+id+pulse_time values
    print(loaded_data)
