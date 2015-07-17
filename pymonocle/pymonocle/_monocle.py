import os.path

from cffi import FFI
ffi = FFI()


base_dir = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))


def read_cdef(filename):
    return open(os.path.join(os.path.dirname(__file__), filename)).read()


def rel_mncl(dirname):
    return os.path.join(base_dir, dirname)


ffi.cdef('\n'.join([
    read_cdef('monocle.cdef'),
    read_cdef('sdl2_constants.cdef'),
]))


_VERIFY_CODE = """
#include <SDL.h>
#include <SDL_keycode.h>
#include "monocle.h"
"""


mncl = ffi.verify(
    _VERIFY_CODE, library_dirs=[
        rel_mncl('bin'),
        '/usr/local/lib',
        '/usr/lib'],
    include_dirs=[
        rel_mncl('include'),
        '/usr/include/SDL2',
        '/usr/local/include/SDL2'],
    libraries=['monocle', 'SDL2', 'SDL2_image', 'SDL2_mixer'])


_remembered_objects = set()


remember_object = _remembered_objects.add
forget_object = _remembered_objects.discard
