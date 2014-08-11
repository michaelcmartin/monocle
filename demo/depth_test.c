#include <stdlib.h>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "monocle.h"

void
update(MNCL_OBJECT *obj)
{
    intptr_t state = (intptr_t)obj->user_data;
    if (state == 1) {
        obj->dx -= 1;
        if (obj->dx == 0) {
            mncl_object_set_depth(obj, 1000);
        }
    } else {
        obj->dx += 1;
        if (obj->dx == 0) {
            mncl_object_set_depth(obj, 5000);
        }
    }
    if (obj->dx < -10) {
        state = 2;
    }
    if (obj->dx > 10) {
        state = 1;
    }
    obj->user_data = (void *)state;
}

void
render(MNCL_OBJECT *obj)
{
    int v = obj->dx < 0 ? 192 : 64;
    mncl_draw_rect (obj->x - 32, obj->y - 32, 64, 64, v, v, v);
}

int
main(int argc, char **argv)
{
    int done;

    mncl_init();
    mncl_config_video("Depth Test Demo", 768, 480, 0, 0);

    mncl_add_resource_zipfile("earthball-res.zip");
    mncl_load_resmap("earthball.json");

    MNCL_OBJECT *left = mncl_create_object (300, 200, "custom");
    MNCL_OBJECT *right = mncl_create_object (400, 200, "custom");
    left->dx = 0;
    right->dx = 0;
    left->user_data = (void *)(intptr_t)(2);
    right->user_data = (void *)(intptr_t)(1);

    done = 0;

    while (!done) {
        MNCL_EVENT *e = mncl_pop_global_event();
        switch(e->type) {
        case MNCL_EVENT_QUIT:
            done = 1;
            break;
        case MNCL_EVENT_KEYDOWN:
            switch (e->value.key) {
            case SDLK_ESCAPE:
                done = 1;
                break;
            default:
                break;
            }
            break;
        case MNCL_EVENT_PRERENDER:
            if (e->value.self) {
                update(e->value.self);
            }
            break;
        case MNCL_EVENT_RENDER:
            if (e->value.self) {
                render(e->value.self);
            }
            break;
        default:
            break;
        }
    }

    mncl_uninit();
    return 0;
}
