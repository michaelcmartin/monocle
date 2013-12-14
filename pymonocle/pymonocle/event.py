from pymonocle._monocle import ffi, mncl

from pymonocle.constants import (
    EVENT_QUIT, EVENT_INIT, EVENT_PREINPUT, EVENT_KEYDOWN, EVENT_KEYUP,
    EVENT_MOUSEMOVE, EVENT_MOUSEBUTTONDOWN, EVENT_MOUSEBUTTONUP,
    EVENT_JOYAXISMOVE, EVENT_JOYBUTTONDOWN, EVENT_JOYBUTTONUP,
    EVENT_JOYHATMOVE, EVENT_PREPHYSICS, EVENT_COLLISION, EVENT_PRERENDER,
    EVENT_RENDER, EVENT_POSTRENDER)


def pop_global_event():
    return MonocleEvent(mncl.mncl_pop_global_event())


class MonocleEvent(object):
    def __init__(self, mncl_event):
        self._mncl_event = mncl_event
        self.type = mncl.mncl_event_type(self._mncl_event)
        event_value = self._mncl_event.value

        if self.type in (EVENT_INIT, EVENT_QUIT, EVENT_POSTRENDER):
            # These events have no content.
            pass

        elif self.type in (EVENT_PREINPUT, EVENT_PREPHYSICS, EVENT_PRERENDER,
                           EVENT_RENDER):
            self.game_object = None
            if event_value.self != ffi.NULL:
                self.game_object = ffi.from_handle(event_value.self.user_data)

        elif self.type in (EVENT_KEYDOWN, EVENT_KEYUP):
            self.key = event_value.key

        elif self.type == EVENT_MOUSEMOVE:
            self.x = event_value.mousemove.x
            self.y = event_value.mousemove.y

        elif self.type in (EVENT_MOUSEBUTTONDOWN, EVENT_MOUSEBUTTONUP):
            self.button = event_value.mousebutton

        elif self.type == EVENT_JOYAXISMOVE:
            self.joystick = event_value.joystick.stick
            self.axis = event_value.joystick.index
            self.value = event_value.joystick.value

        elif self.type in (EVENT_JOYBUTTONDOWN, EVENT_JOYBUTTONUP):
            self.joystick = event_value.joystick.stick
            self.button = event_value.joystick.index

        elif self.type == EVENT_JOYHATMOVE:
            self.joystick = event_value.joystick.stick
            self.hat = event_value.joystick.index
            self.value = event_value.joystick.value

        elif self.type == EVENT_COLLISION:
            raise NotImplementedError("Monocle doesn't have these yet.")

        else:
            raise ValueError("Unknown event type: %s" % (self.type,))
