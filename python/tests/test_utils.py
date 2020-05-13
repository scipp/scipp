import scipp as sc
from scipp import utils
import numpy as np


class TestUtils:
    def setup_method(self):
        var = sc.Variable(['x'], values=np.arange(5.0))
        self._d = sc.Dataset(data={
            'a': var,
            'b': var
        },
                             masks={'mask1': var},
                             coords={'x': var},
                             attrs={'meta': var})

    def test_to_dict(self):
        pass

    def test_to_dict_round_trip(self):
        orig = self._d.copy()
        serialize = utils.to_dict(orig)
        deserialize = sc.Dataset(**serialize)
        assert orig == deserialize
        del orig['a']
        assert not orig == deserialize
