#include <stdlib.h>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "monocle.h"

void
update(MNCL_OBJECT *obj)
{
    float newx = obj->x + obj->dx;
    float newy = obj->y + obj->dy;
    if (newx < 0) {
        obj->x = 0; obj->dx = -obj->dx;
    }
    if (newy < 0) {
        obj->y = 0; obj->dy = -obj->dy;
    }
    if (newx > 704) {
        obj->x = 704; obj->dx = -obj->dx;
    }
    if (newy > 416) {
        obj->y = 416; obj->dy = -obj->dy;
    }
}

int
main(int argc, char **argv)
{
    MNCL_OBJECT *left, *right;
    int done, frame, left_hit, right_hit, old_hit;
    int collision_start, collision_end;

    mncl_init();
    mncl_config_video("Collision Test", 768, 480, 0, 0);
    mncl_add_resource_zipfile("earthball-res.zip");

    mncl_load_resmap("earthball.json");

    left = mncl_create_object(100, 208, "crashy-earth");
    right = mncl_create_object(264, 208, "crashy-earth");
    left->dx = 1; left->dy = 0;
    right->dx = -1; right->dy = 0;
    frame = 0;
    left_hit = right_hit = old_hit = 0;
    collision_start = collision_end = -1;

    while (!done) {
        MNCL_EVENT *e = mncl_pop_global_event();
        switch(e->type) {
        case MNCL_EVENT_QUIT:
            done = 1;
            break;
        case MNCL_EVENT_PRERENDER:
            if (e->value.self) {
                update(e->value.self);
            } else {
                if (left_hit != right_hit) {
                    printf ("Inconsistent collision at frame #%d\n", frame);
                }
                if (left_hit && !old_hit) {
                    collision_start = frame;
                }
                if (!left_hit && old_hit) {
                    collision_end = frame;
                    done = 1;
                }
                old_hit = left_hit;
                left_hit = right_hit = 0;
                ++frame;
                if (frame > 200) {
                    done = 1;
                }
            }
            break;
        case MNCL_EVENT_COLLISION:
            if (e->value.collision.self == left) {
                left_hit = 1;
            } else if (e->value.collision.self == right) {
                right_hit = 1;
            } else {
                printf ("Impossible collision at frame #%d\n", frame);
            }
        default:
            break;
        }
    }

    if (collision_start >= 0) {
        printf ("Collision starts at frame %d (Expected: 50)\n", collision_start);
    }
    if (collision_end >= 0) {
        printf ("Collision ends at frame %d (Expected: 113)\n", collision_end);
    }
    
    mncl_uninit();
    return 0;
}
