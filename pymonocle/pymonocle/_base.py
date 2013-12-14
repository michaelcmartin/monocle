from pymonocle._monocle import ffi, mncl
from pymonocle.constants import (
    DATA_NULL, DATA_BOOLEAN, DATA_NUMBER, DATA_STRING, DATA_ARRAY, DATA_OBJECT)


# These things will hopefully go away once the wrapping is complete.

NULL = ffi.NULL


def get_pyobj(void_star):
    return ffi.from_handle(void_star)


# Meta Component

def init():
    return mncl.mncl_init()


def uninit():
    return mncl.mncl_uninit()


# Raw Data Component

def add_resource_directory(pathname):
    return mncl.mncl_add_resource_directory(pathname)


def add_resource_zipfile(pathname):
    return mncl.mncl_add_resource_zipfile(pathname)


def raw_resource(resource):
    mncl_raw = mncl.mncl_raw_resource(resource)
    if mncl_raw == NULL:
        # TODO: Throw exception here?
        return None
    return ffi.buffer(mncl_raw.data, mncl_raw.size)


# Semi-structured data component

def data_resource(resource):
    return _parse_monocle_data(mncl.mncl_data_resource(resource))


def _parse_monocle_data(mncl_data):
    if mncl_data == NULL or mncl_data.tag == DATA_NULL:
        return None
    if mncl_data.tag == DATA_BOOLEAN:
        return mncl_data.value.boolean
    if mncl_data.tag == DATA_NUMBER:
        return mncl_data.value.number
    if mncl_data.tag == DATA_STRING:
        return ffi.string(mncl_data.value.string)
    if mncl_data.tag == DATA_ARRAY:
        value = []
        for i in range(mncl_data.value.array.size):
            value.append(_parse_monocle_data(mncl_data.value.array.data[i]))
        return value
    if mncl_data.tag == DATA_OBJECT:
        data_dict = {}
        handle = ffi.new_handle(data_dict)
        mncl.mncl_kv_foreach(
            mncl_data.value.object, _parse_kv_value_fn, handle)
        return data_dict
    raise ValueError("Unknown MNCL_DATA_TYPE: %s" % mncl_data.tag)


@ffi.callback("MNCL_KV_VALUE_FN")
def _parse_kv_value_fn(key, value, handle):
    data_dict = ffi.from_handle(handle)
    data_dict[ffi.string(key)] = _parse_monocle_data(
        ffi.cast("MNCL_DATA*", value))


# Framebuffer component

def config_video(title, width, height, fullscreen, flags):
    return mncl.mncl_config_video(title, width, height, fullscreen, flags)


def is_fullscreen():
    return mncl.mncl_is_fullscreen()


def toggle_fullscreen():
    return mncl.mncl_toggle_fullscreen()


def set_clear_color(r, g, b):
    return mncl.mncl_set_clear_color(r, g, b)


def hide_mouse_in_fullscreen(val):
    return mncl.mncl_hide_mouse_in_fullscreen(val)


def begin_frame():
    return mncl.mncl_begin_frame()


def end_frame():
    return mncl.mncl_end_frame()


def draw_rect(x, y, w, h, r, g, b):
    return mncl.mncl_draw_rect(x, y, w, h, r, g, b)


# Spritesheet Component

def spritesheet_resource(resource):
    # TODO: Wrap this in a higher-level object?
    return mncl.mncl_spritesheet_resource(resource)


def draw_from_spritesheet(spritesheet, x, y, my_x, my_y, my_w, my_h):
    # TODO: Make this a method on the higher-level object mentioned above?
    return mncl.mncl_draw_from_spritesheet(
        spritesheet, x, y, my_x, my_y, my_w, my_h)


# Music Component

def play_music_resource(resource, fade_in_ms):
    return mncl.mncl_play_music_resource(resource, fade_in_ms)


def stop_music():
    return mncl.mncl_stop_music()


def fade_out_music(ms):
    return mncl.mncl_fade_out_music(ms)


def music_volume(volume):
    return mncl.mncl_music_volume(volume)


def pause_music():
    return mncl.mncl_pause_music()


def resume_music():
    return mncl.mncl_resume_music()


# SFX Component

def sfx_resource(resource):
    # TODO: Wrap this in a higher-level object?
    return mncl.mncl_sfx_resource(resource)


def play_sfx(sfx, volume):
    # TODO: Make this a method on the higher-level object mentioned above?
    return mncl.mncl_play_sfx(sfx, volume)


# Sprite component

def sprite_resource(resource):
    return MonocleSprite(mncl.mncl_sprite_resource(resource))


class MonocleSprite(object):
    def __init__(self, mncl_sprite):
        self._mncl_sprite = mncl_sprite

    def draw(self, x, y, frame):
        mncl.mncl_draw_sprite(self._mncl_sprite, x, y, frame)


# Event component

def pop_global_event():
    # TODO: Wrap this in a higher-level object.
    return mncl.mncl_pop_global_event()


def event_type(evt):
    return mncl.mncl_event_type(evt)

# int mncl_event_subscriber(MNCL_EVENT *evt);
# int mncl_event_key(MNCL_EVENT *evt);
# int mncl_event_mouse_x(MNCL_EVENT *evt);
# int mncl_event_mouse_y(MNCL_EVENT *evt);
# int mncl_event_mouse_button(MNCL_EVENT *evt);
# int mncl_event_joy_stick(MNCL_EVENT *evt);
# int mncl_event_joy_index(MNCL_EVENT *evt);
# int mncl_event_joy_value(MNCL_EVENT *evt);


# Resource component

def load_resmap(path):
    return mncl.mncl_load_resmap(path)


def unload_resmap(path):
    return mncl.mncl_unload_resmap(path)


def unload_all_resources():
    return mncl.mncl_unload_all_resources()
