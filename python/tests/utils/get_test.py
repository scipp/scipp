import scipp.utils as su


class NoItemsKeysValues:
    def __init__(self, **items):
        self._dict = items


class OnlyItems(NoItemsKeysValues):
    def items(self):
        return self._dict.items()


class OnlyKeysValues(NoItemsKeysValues):
    def keys(self):
        return self._dict.keys()

    def values(self):
        return self._dict.values()


def test_dict_get():
    pydict = dict(a=1, b=2., c='3')
    assert su.get(pydict, 'a') == pydict['a']
    assert su.get(pydict, 'b') == pydict.get('b')
    assert su.get(pydict, 'c', 3) == pydict.get('c', 'c')
    assert su.get(pydict, 'd') is None
    assert su.get(pydict, 'e', 'e') == pydict.get('e', 'e')


def test_items_obj_get():
    pydict = dict(a=1, b=2., c='3')
    obj = OnlyItems(**pydict)
    assert su.get(obj, 'a') == pydict['a']
    assert su.get(obj, 'b') == pydict.get('b')
    assert su.get(obj, 'c', 3) == pydict.get('c', 'c')
    assert su.get(obj, 'd') is None
    assert su.get(obj, 'e', 'e') == pydict.get('e', 'e')


def test_keysvalues_obj_get():
    pydict = dict(a=1, b=2., c='3')
    obj = OnlyKeysValues(**pydict)
    assert su.get(obj, 'a') == pydict['a']
    assert su.get(obj, 'b') == pydict.get('b')
    assert su.get(obj, 'c', 3) == pydict.get('c', 'c')
    assert su.get(obj, 'd') is None
    assert su.get(obj, 'e', 'e') == pydict.get('e', 'e')


def test_neither_obj_get():
    pydict = dict(a=1, b=2., c='3')
    obj = NoItemsKeysValues(**pydict)
    assert su.get(obj, 'a') is None
    assert su.get(obj, 'b') is None
    assert su.get(obj, 'c', 3) == 3
    assert su.get(obj, 'd') is None
    assert su.get(obj, 'e', 'e') == 'e'
