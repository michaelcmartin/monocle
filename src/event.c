#include "monocle.h"
#include "monocle_internal.h"
#include <SDL.h>

/* Event destructuring functions */
MNCL_EVENT_TYPE
mncl_event_type(MNCL_EVENT *evt)
{
    return evt->type;
}

int
mncl_event_key(MNCL_EVENT *evt)
{
    return evt->value.key;
}

int
mncl_event_mouse_x(MNCL_EVENT *evt)
{
    return evt->value.mousemove.x;
}

int
mncl_event_mouse_y(MNCL_EVENT *evt)
{
    return evt->value.mousemove.y;
}

int
mncl_event_mouse_button(MNCL_EVENT *evt)
{
    return evt->value.mousebutton;
}

int
mncl_event_joy_stick(MNCL_EVENT *evt)
{
    return evt->value.joystick.stick;
}

int
mncl_event_joy_index(MNCL_EVENT *evt)
{
    return evt->value.joystick.index;
}

int
mncl_event_joy_value(MNCL_EVENT *evt)
{
    return evt->value.joystick.value;
}

static Uint32 target_time = 0;
static MNCL_EVENT current_global_event = { MNCL_EVENT_INIT, { 0 } };

MNCL_EVENT *
mncl_pop_global_event(void)
{
    if (current_global_event.type == MNCL_EVENT_QUIT) {
        return &current_global_event;
    }
    switch (current_global_event.type) {
    case MNCL_EVENT_QUIT:
        /* Once we've quit, we stay quit */
        break;
    case MNCL_EVENT_PREINPUT:
        if (!current_global_event.value.self) {
            current_global_event.value.self = object_begin(MNCL_EVENT_PREINPUT);
        } else {
            current_global_event.value.self = object_next();
        }
        if (current_global_event.value.self) {
            /* The NULL case was handled by the FRAMEBOUNDARY
             * event. If we have an object awaiting a PREUPDATE
             * message we return it here. */
            break;
        }
        /* Otherwise, we've completed our iteration and are done with
         * pre-update objects. Fall through to input. */
    case MNCL_EVENT_KEYDOWN:
    case MNCL_EVENT_KEYUP:
    case MNCL_EVENT_MOUSEMOVE:
    case MNCL_EVENT_MOUSEBUTTONDOWN:
    case MNCL_EVENT_MOUSEBUTTONUP:
    case MNCL_EVENT_JOYAXISMOVE:
    case MNCL_EVENT_JOYBUTTONDOWN:
    case MNCL_EVENT_JOYBUTTONUP:
    case MNCL_EVENT_JOYHATMOVE:
        /* If we finished pre-update or are in the middle of pumping
         * input events, then we're still pumping the SDL event
         * stream. */
        while (1) {
            SDL_Event e;
            if (SDL_PollEvent(&e)) {
                switch(e.type) {
                case SDL_QUIT:
                    current_global_event.type = MNCL_EVENT_QUIT;
                    return &current_global_event;
                    break;
                case SDL_KEYDOWN:
                    current_global_event.type = MNCL_EVENT_KEYDOWN;
                    current_global_event.value.key = e.key.keysym.sym;
                    return &current_global_event;
                case SDL_KEYUP:
                    current_global_event.type = MNCL_EVENT_KEYUP;
                    current_global_event.value.key = e.key.keysym.sym;
                    return &current_global_event;
                case SDL_MOUSEMOTION:
                    current_global_event.type = MNCL_EVENT_MOUSEMOVE;
                    current_global_event.value.mousemove.x = e.motion.x;
                    current_global_event.value.mousemove.y = e.motion.y;
                    return &current_global_event;
                case SDL_MOUSEBUTTONDOWN:
                    current_global_event.type = MNCL_EVENT_MOUSEBUTTONDOWN;
                    current_global_event.value.mousebutton = e.button.button;
                    return &current_global_event;
                case SDL_MOUSEBUTTONUP:
                    current_global_event.type = MNCL_EVENT_MOUSEBUTTONUP;
                    current_global_event.value.mousebutton = e.button.button;
                    return &current_global_event;
                case SDL_JOYAXISMOTION:
                    current_global_event.type = MNCL_EVENT_JOYAXISMOVE;
                    current_global_event.value.joystick.stick = e.jaxis.which;
                    current_global_event.value.joystick.index = e.jaxis.axis;
                    current_global_event.value.joystick.value = e.jaxis.value;
                    return &current_global_event;
                case SDL_JOYBUTTONDOWN:
                    current_global_event.type = MNCL_EVENT_JOYBUTTONDOWN;
                    current_global_event.value.joystick.stick = e.jbutton.which;
                    current_global_event.value.joystick.index = e.jbutton.button;
                    return &current_global_event;
                case SDL_JOYBUTTONUP:
                    current_global_event.type = MNCL_EVENT_JOYBUTTONUP;
                    current_global_event.value.joystick.stick = e.jbutton.which;
                    current_global_event.value.joystick.index = e.jbutton.button;
                    return &current_global_event;
                case SDL_JOYHATMOTION:
                    current_global_event.type = MNCL_EVENT_JOYHATMOVE;
                    current_global_event.value.joystick.stick = e.jhat.which;
                    current_global_event.value.joystick.index = e.jhat.hat;
                    current_global_event.value.joystick.value = e.jhat.value;
                    return &current_global_event;
                default:
                    /* An SDL event we don't really care about */
                    break;
                }
            } else {
                /* No events left! */
                current_global_event.type = MNCL_EVENT_PREPHYSICS;
                current_global_event.value.self = NULL;
                break;
            }
        }
        break;
    case MNCL_EVENT_PREPHYSICS:
        if (!current_global_event.value.self) {
            current_global_event.value.self = object_begin(MNCL_EVENT_PREPHYSICS);
        } else {
            current_global_event.value.self = object_next();
        }
        if (!current_global_event.value.self) {
            default_update_all_objects();
            /* TODO: find collisions now that the update has happened */
            current_global_event.type = MNCL_EVENT_PRERENDER;
        }
        break;
    case MNCL_EVENT_PRERENDER:
        if (!current_global_event.value.self) {
            current_global_event.value.self = object_begin(MNCL_EVENT_PRERENDER);
        } else {
            current_global_event.value.self = object_next();
        }
        if (!current_global_event.value.self) {
            mncl_begin_frame();
            current_global_event.type = MNCL_EVENT_RENDER;
        }
        break;
    case MNCL_EVENT_RENDER:
        /* TODO: This should be part of a scene sort */
        if (!current_global_event.value.self) {
            current_global_event.value.self = object_begin(MNCL_EVENT_RENDER);
        } else {
            current_global_event.value.self = object_next();
        }
        if (!current_global_event.value.self) {
            /* TODO: This is a flagrant stopgap */
            default_render_all_objects();
            current_global_event.type = MNCL_EVENT_POSTRENDER;
        }
        break;
    case MNCL_EVENT_POSTRENDER:
    {
        Uint32 current_time;
            
        mncl_end_frame(); /* Should only happen the first time */
        current_global_event.type = MNCL_EVENT_PREINPUT;
        current_time = SDL_GetTicks();
        if (current_time < target_time) {
            Uint32 wait_time = target_time - current_time;
            /* Don't choke for more than a second if things freak out */
            /* Better solution is to not choke for more than a frame. */
            if (wait_time > 0 && wait_time < 1000) {
                SDL_Delay(wait_time);
            }
        }
        /* "40" should be based on a framerate setter */
        target_time = SDL_GetTicks() + 40;
        break;
    }
    case MNCL_EVENT_INIT:
        current_global_event.type = MNCL_EVENT_PREINPUT;
        current_global_event.value.self = NULL;
        break;
    default:
        break;
    }
    return &current_global_event;
}


