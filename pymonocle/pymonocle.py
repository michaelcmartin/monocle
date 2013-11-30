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

    NULL = ffi.NULL
    to_string = ffi.string

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

    def acquire_raw(self, resource_name):
        return _mncl.mncl_acquire_raw(resource_name)

    def release_raw(self, raw):
        return _mncl.mncl_release_raw(raw)

    # Accessors and decoders

    def raw_size(self, raw):
        return _mncl.mncl_raw_size(raw)

    def raw_s8(self, raw, offset):
        return _mncl.mncl_raw_s8(raw, offset)

    def raw_u8(self, raw, offset):
        return _mncl.mncl_raw_u8(raw, offset)

    def raw_s16le(self, raw, offset):
        return _mncl.mncl_raw_s16le(raw, offset)

    def raw_s16be(self, raw, offset):
        return _mncl.mncl_raw_s16be(raw, offset)

    def raw_u16le(self, raw, offset):
        return _mncl.mncl_raw_u16le(raw, offset)

    def raw_u16be(self, raw, offset):
        return _mncl.mncl_raw_u16be(raw, offset)

    def raw_s32le(self, raw, offset):
        return _mncl.mncl_raw_s32le(raw, offset)

    def raw_s32be(self, raw, offset):
        return _mncl.mncl_raw_s32be(raw, offset)

    def raw_u32le(self, raw, offset):
        return _mncl.mncl_raw_u32le(raw, offset)

    def raw_u32be(self, raw, offset):
        return _mncl.mncl_raw_u32be(raw, offset)

    def raw_s64le(self, raw, offset):
        return _mncl.mncl_raw_s64le(raw, offset)

    def raw_s64be(self, raw, offset):
        return _mncl.mncl_raw_s64be(raw, offset)

    def raw_u64le(self, raw, offset):
        return _mncl.mncl_raw_u64le(raw, offset)

    def raw_u64be(self, raw, offset):
        return _mncl.mncl_raw_u64be(raw, offset)

    def raw_f32le(self, raw, offset):
        return _mncl.mncl_raw_f32le(raw, offset)

    def raw_f32be(self, raw, offset):
        return _mncl.mncl_raw_f32be(raw, offset)

    def raw_f64le(self, raw, offset):
        return _mncl.mncl_raw_f64le(raw, offset)

    def raw_f64be(self, raw, offset):
        return _mncl.mncl_raw_f64be(raw, offset)

    # Key-value map component

    def alloc_kv(self, deleter):
        return _mncl.mncl_alloc_kv(deleter)

    def free_kv(self, kv):
        return _mncl.mncl_free_kv(kv)

    def kv_insert(self, kv, key, value):
        return _mncl.mncl_kv_insert(kv, key, value)

    def kv_delete(self, kv, key):
        return _mncl.mncl_kv_delete(kv, key)

    def kv_foreach(self, kv, fn, user):
        return _mncl.mncl_kv_foreach(kv, fn, user)

    # Semi-structured data component

    MNCL_DATA_NULL = _mncl.MNCL_DATA_NULL
    MNCL_DATA_BOOLEAN = _mncl.MNCL_DATA_BOOLEAN
    MNCL_DATA_NUMBER = _mncl.MNCL_DATA_NUMBER
    MNCL_DATA_STRING = _mncl.MNCL_DATA_STRING
    MNCL_DATA_ARRAY = _mncl.MNCL_DATA_ARRAY
    MNCL_DATA_OBJECT = _mncl.MNCL_DATA_OBJECT

    def parse_data(self, data, size):
        return _mncl.mncl_parse_data(data, size)

    def data_clone(self, src):
        return _mncl.mncl_data_clone(src)

    def free_data(self, mncl_data):
        return _mncl.mncl_free_data(mncl_data)

    def data_error(self):
        return _mncl.mncl_data_error()

    def data_lookup(self, map, key):
        return _mncl.mncl_data_lookup(map, key)

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

    def alloc_spritesheet(self, resource_name):
        return _mncl.mncl_alloc_spritesheet(resource_name)

    def free_spritesheet(self, spritesheet):
        return _mncl.mncl_free_spritesheet(spritesheet)

    def draw_from_spritesheet(self, spritesheet, x, y, my_x, my_y, my_w, my_h):
        return _mncl.mncl_draw_from_spritesheet(
            spritesheet, x, y, my_x, my_y, my_w, my_h)

    # Music Component

    def play_music_file(self, pathname, fade_in_ms):
        return _mncl.mncl_play_music_file(pathname, fade_in_ms)

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

    def alloc_sfx(self, resource_name):
        return _mncl.mncl_alloc_sfx(resource_name)

    def free_sfx(self, sfx):
        return _mncl.mncl_free_sfx(sfx)

    def play_sfx(self, sfx, volume):
        return _mncl.mncl_play_sfx(sfx, volume)

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

    # Sprite component. Everything but mncl_draw_sprite may eventually be
    # internalized.

    def alloc_sprite(self, nframes):
        return _mncl.mncl_alloc_sprite(nframes)

    def free_sprite(self, sprite):
        return _mncl.mncl_free_sprite(sprite)

    def draw_sprite(self, s, x, y, frame):
        return _mncl.mncl_draw_sprite(s, x, y, frame)

    # Resource component

    def load_resmap(self, path):
        return _mncl.mncl_load_resmap(path)

    def unload_resmap(self, path):
        return _mncl.mncl_unload_resmap(path)

    def unload_all_resources(self):
        return _mncl.mncl_unload_all_resources()

    def raw_resource(self, resource):
        return _mncl.mncl_raw_resource(resource)

    def spritesheet_resource(self, resource):
        return _mncl.mncl_spritesheet_resource(resource)

    def sprite_resource(self, resource):
        return _mncl.mncl_sprite_resource(resource)

    def sfx_resource(self, resource):
        return _mncl.mncl_sfx_resource(resource)

    def data_resource(self, resource):
        return _mncl.mncl_data_resource(resource)

    def play_music_resource(self, resource, fade_in_ms):
        return _mncl.mncl_play_music_resource(resource, fade_in_ms)


class Keycodes(object):
    def __getattr__(self, value):
        return getattr(_mncl, value)


keycodes = Keycodes()
