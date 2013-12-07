import os.path

from cffi import FFI
ffi = FFI()


def read_cdef(filename):
    return open(os.path.join(os.path.dirname(__file__), filename)).read()


ffi.cdef('\n'.join([
    read_cdef('monocle.cdef'),
    read_cdef('sdl2_constants.cdef'),
]))


_VERIFY_CODE = """
#include <SDL.h>
#include <SDL_keycode.h>
#include "monocle.h"
"""


_mncl = ffi.verify(
    _VERIFY_CODE, library_dirs=['bin', '/usr/local/lib'],
    include_dirs=['include', '/usr/local/include/SDL2'],
    libraries=['monocle', 'SDL2', 'SDL2_image', 'SDL2_mixer'])


class Monocle(object):
    NULL = ffi.NULL  # TODO: Make this unnecessary.

    # Meta Component

    def init(self):
        return _mncl.mncl_init()

    def uninit(self):
        return _mncl.mncl_uninit()

    # Raw Data Component

    def add_resource_directory(self, pathname):
        return _mncl.mncl_add_resource_directory(pathname)

    def add_resource_zipfile(self, pathname):
        return _mncl.mncl_add_resource_zipfile(pathname)

    def raw_resource(self, resource):
        mncl_raw = _mncl.mncl_raw_resource(resource)
        if mncl_raw == ffi.NULL:
            # TODO: Throw exception here?
            return None
        return ffi.buffer(mncl_raw.data, mncl_raw.size)

    # Semi-structured data component

    def data_resource(self, resource):
        return MonocleData(_mncl.mncl_data_resource(resource))

    # Framebuffer component

    def config_video(self, title, width, height, fullscreen, flags):
        return _mncl.mncl_config_video(title, width, height, fullscreen, flags)

    def is_fullscreen(self):
        return _mncl.mncl_is_fullscreen()

    def toggle_fullscreen(self):
        return _mncl.mncl_toggle_fullscreen()

    def set_clear_color(self, r, g, b):
        return _mncl.mncl_set_clear_color(r, g, b)

    def hide_mouse_in_fullscreen(self, val):
        return _mncl.mncl_hide_mouse_in_fullscreen(val)

    def begin_frame(self):
        return _mncl.mncl_begin_frame()

    def end_frame(self):
        return _mncl.mncl_end_frame()

    def draw_rect(self, x, y, w, h, r, g, b):
        return _mncl.mncl_draw_rect(x, y, w, h, r, g, b)

    # Spritesheet Component

    def spritesheet_resource(self, resource):
        # TODO: Wrap this in a higher-level object?
        return _mncl.mncl_spritesheet_resource(resource)

    def draw_from_spritesheet(self, spritesheet, x, y, my_x, my_y, my_w, my_h):
        # TODO: Make this a method on the higher-level object mentioned above?
        return _mncl.mncl_draw_from_spritesheet(
            spritesheet, x, y, my_x, my_y, my_w, my_h)

    # Music Component

    def play_music_resource(self, resource, fade_in_ms):
        return _mncl.mncl_play_music_resource(resource, fade_in_ms)

    def stop_music(self):
        return _mncl.mncl_stop_music()

    def fade_out_music(self, ms):
        return _mncl.mncl_fade_out_music(ms)

    def music_volume(self, volume):
        return _mncl.mncl_music_volume(volume)

    def pause_music(self):
        return _mncl.mncl_pause_music()

    def resume_music(self):
        return _mncl.mncl_resume_music()

    # SFX Component

    def sfx_resource(self, resource):
        # TODO: Wrap this in a higher-level object?
        return _mncl.mncl_sfx_resource(resource)

    def play_sfx(self, sfx, volume):
        # TODO: Make this a method on the higher-level object mentioned above?
        return _mncl.mncl_play_sfx(sfx, volume)

    # Sprite component

    def sprite_resource(self, resource):
        return MonocleSprite(_mncl.mncl_sprite_resource(resource))

    # Object component

    def create_object(self):
        return MonocleObject(_mncl.mncl_create_object())

    # Event component

    MNCL_EVENT_QUIT = _mncl.MNCL_EVENT_QUIT
    MNCL_EVENT_PREUPDATE = _mncl.MNCL_EVENT_PREUPDATE
    MNCL_EVENT_KEYDOWN = _mncl.MNCL_EVENT_KEYDOWN
    MNCL_EVENT_KEYUP = _mncl.MNCL_EVENT_KEYUP
    MNCL_EVENT_MOUSEMOVE = _mncl.MNCL_EVENT_MOUSEMOVE
    MNCL_EVENT_MOUSEBUTTONDOWN = _mncl.MNCL_EVENT_MOUSEBUTTONDOWN
    MNCL_EVENT_MOUSEBUTTONUP = _mncl.MNCL_EVENT_MOUSEBUTTONUP
    MNCL_EVENT_JOYAXISMOVE = _mncl.MNCL_EVENT_JOYAXISMOVE
    MNCL_EVENT_JOYBUTTONDOWN = _mncl.MNCL_EVENT_JOYBUTTONDOWN
    MNCL_EVENT_JOYBUTTONUP = _mncl.MNCL_EVENT_JOYBUTTONUP
    MNCL_EVENT_JOYHATMOVE = _mncl.MNCL_EVENT_JOYHATMOVE
    MNCL_EVENT_UPDATE = _mncl.MNCL_EVENT_UPDATE
    MNCL_EVENT_COLLISION = _mncl.MNCL_EVENT_COLLISION
    MNCL_EVENT_POSTUPDATE = _mncl.MNCL_EVENT_POSTUPDATE
    MNCL_EVENT_PRERENDER = _mncl.MNCL_EVENT_PRERENDER
    MNCL_EVENT_RENDER = _mncl.MNCL_EVENT_RENDER
    MNCL_EVENT_POSTRENDER = _mncl.MNCL_EVENT_POSTRENDER
    MNCL_EVENT_FRAMEBOUNDARY = _mncl.MNCL_EVENT_FRAMEBOUNDARY
    MNCL_NUM_EVENTS = _mncl.MNCL_NUM_EVENTS

    def pop_global_event(self):
        # TODO: Wrap this in a higher-level object.
        return _mncl.mncl_pop_global_event()

    def event_type(self, evt):
        return _mncl.mncl_event_type(evt)

    # int mncl_event_subscriber(MNCL_EVENT *evt);
    # int mncl_event_key(MNCL_EVENT *evt);
    # int mncl_event_mouse_x(MNCL_EVENT *evt);
    # int mncl_event_mouse_y(MNCL_EVENT *evt);
    # int mncl_event_mouse_button(MNCL_EVENT *evt);
    # int mncl_event_joy_stick(MNCL_EVENT *evt);
    # int mncl_event_joy_index(MNCL_EVENT *evt);
    # int mncl_event_joy_value(MNCL_EVENT *evt);

    # Resource component

    def load_resmap(self, path):
        return _mncl.mncl_load_resmap(path)

    def unload_resmap(self, path):
        return _mncl.mncl_unload_resmap(path)

    def unload_all_resources(self):
        return _mncl.mncl_unload_all_resources()


class MonocleData(object):
    def __init__(self, mncl_data):
        self._mncl_data = mncl_data
        if self._mncl_data == ffi.NULL:
            self.value = None
        else:
            self._parse_value()

    def _parse_value(self):
        data = self._mncl_data
        if data.tag == _mncl.MNCL_DATA_BOOLEAN:
            self.value = data.value.boolean
        elif data.tag == _mncl.MNCL_DATA_NUMBER:
            self.value = data.value.number
        elif data.tag == _mncl.MNCL_DATA_STRING:
            self.value = ffi.string(data.value.string)
        elif data.tag == _mncl.MNCL_DATA_ARRAY:
            self.value = []
            for i in range(data.value.array.size):
                self.value.append(MonocleData(data.value.array.data[i]))
        elif data.tag == _mncl.MNCL_DATA_OBJECT:
            raise NotImplementedError("Sorry, I haven't built this yet.")


class MonocleSprite(object):
    def __init__(self, mncl_sprite):
        self._mncl_sprite = mncl_sprite

    def draw(self, x, y, frame):
        _mncl.mncl_draw_sprite(self._mncl_sprite, x, y, frame)


class MonocleObject(object):
    def __init__(self, mncl_object):
        self._mncl_object = mncl_object

    def set_sprite(self, sprite):
        # TODO: Wrap this properly.
        self._mncl_object.sprite = sprite._mncl_sprite


def _add_mncl_obj_prop(name):
    prop = property(
        lambda self: getattr(self._mncl_object, name),
        lambda self, value: setattr(self._mncl_object, name, value))
    setattr(MonocleObject, name, prop)


for name in ['x', 'y', 'f', 'dx', 'dy', 'df']:
    _add_mncl_obj_prop(name)


class Keycodes(object):
    def __getattr__(self, value):
        return getattr(_mncl, value)


keycodes = Keycodes()
