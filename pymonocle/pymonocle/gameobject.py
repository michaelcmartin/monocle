from pymonocle._monocle import ffi, mncl, remember_object


class MonocleObject(object):
    def __init__(self, x, y, kind):
        self._mncl_object = mncl.mncl_create_object(x, y, kind)
        handle = ffi.new_handle(self)
        remember_object(handle)
        self._mncl_object.user_data = handle


def _add_mncl_obj_prop(name):
    prop = property(
        lambda self: getattr(self._mncl_object, name),
        lambda self, value: setattr(self._mncl_object, name, value))
    setattr(MonocleObject, name, prop)


for name in ['x', 'y', 'f', 'dx', 'dy', 'df']:
    _add_mncl_obj_prop(name)
