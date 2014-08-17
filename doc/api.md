# Monocle #
## A Minimal, Native Console-Like library based on SDL ##

Monocle is a platform for writing 2D sprite-based games that is designed to have as simple as possible of an API while still enabling works of great sophistication. It draws inspiration from YoYoGames's _Game Maker_ systems, as well as from the core engines behind the game projects I've worked on over the years (predominantly _The Ur-Quan Masters_ and _Sable_).

# Getting Started #

The first thing you need to do is set up the library and the screen you'll be ultimately drawing to.

```C
void mncl_init(void);
void mncl_uninit(void);
```

These two calls should bracket all usage of the Monocle library. They're listed with the framebuffer component both for convenience and because the raw data component doesn't happen to need them.

```C
int mncl_config_video (title, width, height, fullscreen, reserved);
```

Set up the screen. Pretty self-explanatory, other than that "reserved" is an integer and must be zero. I'm expecting to need to have a flags argument in the future, but for now there aren't any.

```C
int mncl_is_fullscreen(void);
int mncl_toggle_fullscreen(void);
```

Self-explanatory. Both return nonzero if the (new) state of the screen is fullscreen. Depending on the OS, attempting to toggle fullscreen may fail.

```C
void mncl_set_clear_color(unsigned char r, unsigned char g, unsigned char b);
```

Sets the color to clear the screen to at the beginning of every frame. The clear color defaults to black (#000000).

```C
void mncl_hide_mouse_in_fullscreen(int val);
```

Sets a boolean value regarding whether or not the mouse cursor should be invisible when in fullscreen mode. Defaults to false (that is, the mouse is not hidden).

# Objects #

Objects are the primary units of logic and agency in the Monocle system. Each object has:

 - A _location_ represented as two floating point numbers (x, y).
 - A _depth_ that places it in the scene. Objects with a lower depth are more forward in the scene (that is, they are rendered later).
 - A _canonical sprite_. This may define the way it will be drawn if it is visible and does not have custom rendering routines. It also defines the hitbox of the object if the object is one that participates in collisions.
 - A _current frame_ that indicates which frame from the canonical sprite should be drawn. It is a floating point number so that animation speeds may conveniently be less than 1 per frame.
 - _Speeds_ for both location and animation frame.
 - A _kind_, about which more below. Kinds define defaults for all of these values (except for location) and also define how the object will interact with the rest of the game world.
 - Optionally, arbitrary _user data_ which is supplied by the library client and is bundled unchanged. This can be supplemental game-specific data such as hit points or acceleration, or, for a language like C++, a pointer to its own wrapper class for tighter binding of the Monocle event loop into an object-oriented framework.

```C
MNCL_OBJECT *mncl_create_object(float x, float y, const char *kind);
```

Creates an object of the designated kind at the designated location. Speed, depth, frame, and sprite are set to the default as specified by the kind, and the user data is initialized to `NULL`. All of these values except depth may be changed after this call by editing the fields in the `MNCL_OBJECT`. Depth is kept in a hidden field that is part of the allocation unit. To change depth, use the following function:

```C
void mncl_object_set_depth(MNCL_OBJECT *object, int depth);
```

Changing the depth of an object requires the scene graph to be resorted, so you cannot change it simply by assigning a field. Call this function to do so, instead.

## Kinds ##

Kinds are an interesting part of the Monocle system; they are part of every object, but client code never deals with them directly. They are specified in the resource map, and named during object creation, but then they exist only implicitly.

### Resource Map specification ###

```JSON
{
    "kind": { "coin": { "dx": -5, "dy": 0,
                        "frame": 3,
                        "frame-speed": 1,
                        "depth": 1000,
                        "sprite": "coin",
                        "traits": ["pickup"]
                        "collisions": ["player"] },
              "player": { "sprite": "player",
                          "traits": ["player", "render", "pre-render"],
                          "collisions": ["hazard", "pickup"] } }
}
```

Generally, the only things you really need to specify here are the traits and collisions if any. The collisions key specifies which collisions you will participate in as the collider. The traits key specifies which collisions you will participate in as the collidee. In the example above, there will be two collision events created: one for when the player collides with the coin (on the "pickup" trait) and one for the coin colliding with the player (on the "player" trait). If the coin kind did not have a collisions key, then only the first event would be generated on collision.

```C
unsigned int mncl_get_trait(const char *trait);
```

When an object participates in a collision event, the reason the collision was relevant is reported in two fields: one is a character string identifying the trait participating in the collision, and the other is a small integer that encodes the same data. That integer is not necessarily constant across runs, because traits are only added when you load a resmap that defines a kind tha refers to that trait. However, once loaded in any given run, traits are immutable and immortal. This function lets you turn the string version of the traits (which _are_ immutable across runs) into the internal ID number for this run. This may make dispatching on collision events easier.

# Events #

Internally, Monocle has an event loop that interacts with both the user and the intrinsic game physics. Each frame goes through a series of events, which are reported to the client once per frame. In addition, object traits will cause the various phases generate callbacks for each callback, appropriately. 

Events are specified with a constant identified what kind of event this is, and then an optional data structure that carries ancillary data. The exact data structures are listed below; a summary of the event types is presented here for convenience.

 - **Update.** Types: `MNCL_EVENT_PREINPUT`, `MNCL_EVENT_PREPHYSICS`, and `MNCL_EVENT_PRERENDER`. These are for updating game logic. Pre-input happens before input is read; pre-physics happens before objects are do their default updates and before collision detection, and pre-render happens after collisions and just before drawing starts. Objects with the traits "pre-input", "pre-physics", or "pre-render", respectively, will receive events of the corresponding types.
 - **Collision.** Types: `MNCL_EVENT_COLLISION`. Objects have traits and also list traits with which they collide. 
 - **Render.** Types: `MNCL_EVENT_RENDER`, and `MNCL_EVENT_POSTRENDER`. These are the callbacks that signal the client that it is time to draw things. The "generic" version of these events fires first, Conceptually, rendering at the global scale is for background images, render at the object scale is for the sprites, and postrender is for the HUD and GUI. Objects cannot receive postrender events. Objects will receive render events if they do not have the "invisible" trait and do have the "render" trait. If they have neither of these traits, a default rendering routine will run which draws their current sprite frame at their current location.
 - **Keyboard events.** Types: `MNCL_EVENT_KEYDOWN`, `MNCL_EVENT_KEYUP`. Data field: `key`. User pressed or released the key identified by the symbol specified by `key`. These symbol constants are compatible with the `SDL_keysym` enumeration for now, but may get a rename later so that SDL headers need not be directly included.
 - **Mouse move events.** Type: `MNCL_EVENT_MOUSEMOVE`. Data field: `mousemove`. When the mouse is moved within the window, a series of these event are fired. the `x` and `y` subfields give the mouse location in screen coordinates.
 - **Mouse button events.** Types: `MNCL_EVENT_MOUSEBUTTONDOWN`, `MNCL_EVENT_MOUSEBUTTONUP`. Data field: `mousebutton`. The value is which button was pressed or released.
 - **Joystick move events.** Type: `MNCL_EVENT_JOYAXISMOVE`. Data field: `joystick`. When an analog joystick is moved, the axis it moved on gets a new value. This usually needs to be calibrated, both to get a good idea of where the edges are but also to get a good idea of where the dead zone should be. The `stick` field identifies the joystick, the `index` field of the data indicates which axis moved, and the `value` field indicates the new location of the stick on that axis. If a stick is moved diagonally, multiple events will fire. *Due to SDL using the older DirectInput library for joystick input, the LT and RT buttons on an XBox 360 controller will register as axis moves and generally misbehave. Avoid use of these buttons if possible.*
 - **Joystick button events.** Types: `MNCL_EVENT_JOYBUTTONDOWN`, `MNCL_EVENT_JOYBUTTONUP`. Data field: `joystick`. The `stick` is the joystick, the `index` is the button that has been pressed or released.
 - **Joystick hat events.** Types: `MNCL_EVENT_JOYHATMOVE`. Data field: `joystick`. The `stick` is the joystick, the `index` is the hat number, and the `value` is the new hat orientation. Orientation constants currently track SDL's. *D-Pads often present as Hats.*

## Frame Sequence ##

 - `MNCL_EVENT_PREINPUT` - it first fires with a NULL object each frame, and then will fire once for each object with the "pre-input" trait. The object being processed is in the `self` field of the union.
 - All input events, if any: `MNCL_EVENT_KEYDOWN`, `MNCL_EVENT_KEYUP`, `MNCL_EVENT_MOUSEMOVE`, `MNCL_EVENT_MOUSEBUTTONDOWN`, `MNCL_EVENT_MOUSEBUTTONUP`, `MNCL_EVENT_JOYAXISMOVE`, `MNCL_EVENT_JOYBUTTONDOWN`, `MNCL_EVENT_JOYBUTTONUP`, and `MNCL_EVENT_JOYHATMOVE`. The relevant input data is in the `key`, `mousemove`, `mousebutton`, or `joystick` fields as appropriate.
 - `MNCL_EVENT_PREPHYSICS` - it first fires with a NULL object each frame, and then will fire once for each object with the "pre-physics" trait. The object being processed is in the `self` field of the union.
 - All objects have their _x_, _y_, and _f_ values updated by _dx_, _dy_, and _df_.
 - `MNCL_EVENT_COLLISION` - This fires only if collisions happen. One collision event will be fired for each triple of `(object1, object2, trait)` for which `object1` and `object2` overlap, `object1` collides with `trait`, and `object2` has `trait`. Information about the collision and why it happened will be in the `collision` field.
 - `MNCL_EVENT_PRERENDER` - it first fires with a NULL object each frame, and then will fire once for each object with the "pre-render" trait. The object being processed is in the `self` field of the union. This is normally where frame-logic updates happen after reacting to collisions.
 - `MNCL_EVENT_RENDER` - it first fires with a NULL object each frame, and then will fire once for each object with the "render" trait. The object being processed is in the `self` field of the union. This is normally where frame-logic updates happen after reacting to collisions. _It is only safe to actually call drawing functions while processing a RENDER or POSTRENDER event._ The expectation is that RENDER on no object will draw the backgrounds, then RENDER on the objects will draw the objects. If "render" is not set and "invisible" is also not set, then instead of firing an event, the object will simply draw itself based on its values of _x_, _y_, and _f_.
 - `MNCL_EVENT_POSTRENDER` - This fires exactly once each frame, and is for final drawing at the global context. It is most likely going to be used to do things like draw the score if you do not have gamemaster objects to do this for you.

After that, it goes back to `MNCL_EVENT_PREINPUT`.  If at any point the function `mncl_post_quit` is called, or if the user or OS closes the game window, then all subsequent events will be `MNCL_EVENT_QUIT`. You are expected to transition to graceful shutdown upon receiving this event. (It is nevertheless permitted to transition to a shutdown at any point without receiving this event; `mncl_uninit` and friends will all do the right thing as long as you also stop pumping the event loop.)

Objects that are created during one phase in the frame sequence will not actually appear until the next phase. Objects that are destroyed will stop appearing in events immediately but their resources will not be collected until the beginning of the next phase. It is up to you to clean up the userdata before telling Monocle to destroy the object.

### Data Structures ###

```C

struct MNCL_COLLISION {
    MNCL_OBJECT *self, *other; 
    const char *trait; 
    int trait_id;
};

struct MNCL_EVENT {
    MNCL_EVENT_TYPE type;
    union {
        int key;
        struct { int x, y; } mousemove;
        int mousebutton;
        struct { int stick, index, value; } joystick;
        MNCL_OBJECT *self;
        MNCL_COLLISION collision;
    } value;
};
```

The event structure. This is a "tagged union" - the type field indicates which member of the union to check.

### Functions ###

```C
MNCL_EVENT *mncl_pop_global_event(void);
```

These are the event loop accessor functions. The pointer returned by `mncl_pop_global_event` is *owned by Monocle*, and the client should neither free it, edit it, nor permit it to live too long. This pointer will be invalidated by the next call to `mncl_pop_global_event`. Attempting to pop a Quit event has no effect.

# Resources #

Actual game assets are generally considered to be "resources". They are stored in a directory or a zip file and game-necessary metadata about them is stored in a JSON format within the directory or zip file. 

This functions as a virtual filesystem indexed by game-sensible names instead of hard-coded filenames.  The client asks for resources of a specific type with a certain ID, and gets back an appropriate data structure. The client code does not have to worry about whether it was loaded directly from a file, decompressed from a zip file, or anything else.

This component can be thought of as a drastically simplified version of the PhysicsFS system (http://icculus.org/physfs/), and indeed it would not be difficult to extend this library to wrap PhysicsFS in all its glory.

For now, however, all you need to link in is zlib.

```C
int mncl_add_resource_directory(const char *pathname);
int mncl_add_resource_zipfile(const char *pathname);
```

These add directories or zip files to the front of the component's search path. They use the name "resource" here because the client will generally be using these functions alongside the Resource Component in layer 2. The argument names the directory or zip file to use.

Both functions return true on success, or false on failure.

Because they add to the *front* of the search path, resource locations added later override stuff added earlier. So, the protocol is to add core data first, and then add-ons.

```C
void mncl_load_resmap(const char *path);
void mncl_unload_resmap(const char *path);
void mncl_unload_all_resources(void);
```

These routines load resource specifications out of your search path. These are JSON files with a general format that works like this:

```JSON
{
    "type1": { "type1_resource1": { "key": "value1" },
               "type1_resource2": { "key": "value2" } },
    "type2": { "type2_resource1": [ "information for this type" ] }
}
```

At the moment, the types of things you specify are `raw`, `data`, `spritesheet`, `sprite`, `font`, `sfx`, `music`, and `kind`. When you load a resource map, all the things it describes (except music and kinds) are mapped directly into memory and stay there until the map is unloaded. If you have very large raw data items or sound effects, it is best to put them in separate maps.  "Kinds," which define the properties of game objects, are immortal once loaded.

For a complete example of a full resource map, see the `demo/resources/earthball.json` file.

```C
MNCL_RAW *mncl_raw_resource(const char *resource);
MNCL_DATA *mncl_data_resource(const char *resource);
MNCL_SPRITESHEET *mncl_spritesheet_resource(const char *resource);
MNCL_SFX *mncl_sfx_resource(const char *resource);
MNCL_SPRITE *mncl_sprite_resource(const char *resource);
MNCL_FONT *mncl_font_resource(const char *resource);
```

These functions grant access to the loaded resources from the resource pool. These pointers will be invalidated if you call `mncl_unload_resmap` on the map that defined them, so be careful if you are manually managing maps.

## Raw resources ##

Raw resources are just big chunks of bytes with a size marker. They're used to store arbitrary data that you might need for project-specific purposes.

### Resource map format ###

Raw resources are specified purely by filename:

```JSON
{
    "raw": { "id1": "filename1",
             "id2": "filename2",
             ...
             "idn": "filenamen" }
}
```

Filenames are relative to the base of the zip file or directory mounted by `mncl_add_resource_*`.

### Data types ###

```C
struct MNCL_RAW {
    unsigned char *data;
    unsigned int size;
};
```

The raw data itself. The data pointer is allocated on the heap and is "owned" by the structure; clients should not free it themselves.

(These structures are actually defined with typedefs in the usual manner, so one will declare it in prototypes and such with `MNCL_RAW *raw`, not `struct MNCL_RAW *raw`.)

### Functions ###

```C
MNCL_RAW *mncl_acquire_raw(const char *resource_name);
void mncl_release_raw(MNCL_RAW *raw);
```

Pull some raw data out of the search path, identified by the resource name. These are basically pathnames relative to the directories or zipfiles in question. If the resource does not exist, acquire returns NULL.

Any acquired raw data must be later released or memory will leak. That said, under the hood this is a reference-counted implementation, so acquiring the same resource twice will return the same structure, and that structure will not be deallocated until it is released twice.

```C
int mncl_raw_size(MNCL_RAW *raw);

char mncl_raw_s8(MNCL_RAW *raw, int offset);
unsigned char mncl_raw_u8(MNCL_RAW *raw, int offset);
short mncl_raw_s16le(MNCL_RAW *raw, int offset);
short mncl_raw_s16be(MNCL_RAW *raw, int offset);
unsigned short mncl_raw_u16le(MNCL_RAW *raw, int offset);
unsigned short mncl_raw_u16be(MNCL_RAW *raw, int offset);
int mncl_raw_s32le(MNCL_RAW *raw, int offset);
int mncl_raw_s32be(MNCL_RAW *raw, int offset);
unsigned int mncl_raw_u32le(MNCL_RAW *raw, int offset);
unsigned int mncl_raw_u32be(MNCL_RAW *raw, int offset);
long mncl_raw_s64le(MNCL_RAW *raw, int offset);
long mncl_raw_s64be(MNCL_RAW *raw, int offset);
unsigned long mncl_raw_u64le(MNCL_RAW *raw, int offset);
unsigned long mncl_raw_u64be(MNCL_RAW *raw, int offset);
float mncl_raw_f32le(MNCL_RAW *raw, int offset);
float mncl_raw_f32be(MNCL_RAW *raw, int offset);
double mncl_raw_f64le(MNCL_RAW *raw, int offset);
double mncl_raw_f64be(MNCL_RAW *raw, int offset);
```

These are accessors, that allow programmatic access to the contents of a raw data block. You can query the size without needing to represent the struct inside a linked language, or you can pull primitive data types out of the raw bytes in a platform-independent way. If you have some kind of custom binary resource, you'll be using these a lot. The codes at the end of each function name are:

* s/u/f for signed/unsigned/floating point
* A number indicating the number of bits to read
* le/be for little-endian or big-endian

## Music ##

This and the SFX component wrap SDL2_Mixer, which is super finicky and likes to crash if you look at it funny. This wrapper makes that go away.

Music files can be .ogg or any of the old tracker formats that MikMod understands (.mod, .s3m, .xm, .it).

### Resource Map format ###

Music resources are basically identical to raw resources in that they simply name a file. Unlike other resources that specify filenames, these files are not loaded until the music is played, and are unloaded once the music is stopped.

### Functions ###

```C
void mncl_play_music(const char *resource_name, int fade_in_ms);
void mncl_fade_out_music(int ms);
void mncl_stop_music(void);
void mncl_music_volume(int volume);
void mncl_pause_music(void);
void mncl_resume_music(void);
```

Pretty self-explanatory, other than that volumes can range from 0 to 128, and that fading out the music doesn't actually *stop* it; you can set the volume back up afterwards and it'll still be there.

## Sound Effects ##

Sound effects are .wav files. Monocle will allow up to four sound effects to play simultaneously.

At the moment, Monocle's sound backend is the SDL2_mixer library. This library is a little finicky about what kinds of .wav files it will consume; you will basically have to ensure that it gets either 11025, 22050, or 44100 Hz WAV files or the playback may fail.

### Resource Map format ###

Like Music, these are simple filenames. They are stored under the `"sfx"` key in the resource dictionary.

```C
void mncl_play_sfx(MNCL_SFX *sfx, int volume);
```

Volumes again range from 0 to 128.

## Spritesheets ##

A spritesheet is one or more images stored as a single unit. If you're designing for hardware acceleration, these should be thought of as roughly equivalent to a single texture you're pulling data from.

### Resource map format ###

These are also simple filenames, like raw resources. They should name image files, ideally PNG, though anything SDL2_image can read should be fine.

### Data Structures ###

```C
struct MNCL_SPRITESHEET;
```

It's opaque. Clients will treat pointers to this as magic handles.

### Functions ###

```C
void mncl_draw_from_spritesheet(MNCL_SPRITESHEET *spritesheet,
                                int x, int y,
                                int my_x, int my_y,
                                int my_w, int my_h);
```

Draw an image from the sprite sheet at (x, y) on the screen. The last four arguments specify the rectangle from the spritesheet to draw. It is only safe to call this function when processing a `MNCL_RENDER` event.

```C
void mncl_draw_rect(int x, int y, int w, int h, 
                    unsigned char r, unsigned char g, unsigned char b,
```

Draw a filled rectangle at (x, y) of width (w, h) of color (r, g, b). Colors are bytes. *Yeah, it's not really a spritesheet function, but it's closer to that than anything else.*

Like `mncl_draw_from_spritesheet`, it is only safe to call this when processing a `MNCL_RENDER` event.

## Sprites ##

To be written. Our first case where the resource map has to specify a bunch of stuff.

## Fonts ##

To be written.

## Kinds ##

While these are technically a resource type, we covered them in detail up in the section on Objects.

## Semi-structured Data ##

Monocle uses JSON internally to represent semistructured data such as resource assignments. This functionality is also offered to clients to allow for resource files to include project-specific data.

### Resource map format ###

The data element is a dictionary that maps keys to arbitrary JSON objects. These can be anything at all that is legal JSON. When you collect it with `mncl_data_resource` you will get a generic JSON object that you can inspect or further destructure.

```C
typedef enum {
    MNCL_DATA_NULL,
    MNCL_DATA_BOOLEAN,
    MNCL_DATA_NUMBER,
    MNCL_DATA_STRING,
    MNCL_DATA_ARRAY,
    MNCL_DATA_OBJECT
} MNCL_DATA_TYPE;

typedef struct mncl_data_value_ {
    MNCL_DATA_TYPE tag;
    union {
        int boolean;
        double number;
        char *string;
        struct { int size; struct mncl_data_value_ **data; } array;
        MNCL_KV *object;
    } value;
} MNCL_DATA;
```

The `MNCL_DATA` tagged union is the core structure here; the enumeration is really there so you can determine what kind of data it is. Individual `MNCL_DATA` objects are relatively self-contained and will normally not alias other operations.

```C
MNCL_DATA *mncl_data_lookup(MNCL_DATA *map, const char *key);
```

If you have a dictionary object, you can treat the object as a key-value map (as described below), or you can use this function if you're just trying to get at some specific field.

If you want to iterate over the values of your JSON object, though, you will need to use the full power of the `MNCL_KV` type.

# Key-Value Maps #

C rather infamously doesn't provide a whole lot of structured data types. A lot of the Monocle system needs to have string-to-object map capability under the hood, so it makes sense to expose it to other C clients. It also shows up when looking at semi-structured data, as we saw.

## Data Structures ##

```C
struct MNCL_KV;
```

As is typical for Monocle, the core type is opaque. Client code will just use pointers.

```C
typedef void (*MNCL_KV_DELETER)(void *value);
typedef void (*MNCL_KV_VALUE_FN)(const char *key, void *value, void *user);
```

The key-value API uses a lot of callbacks. The deleter is a function that's roughly equivalent to `free()`, and it is used to clean up any values that go out of scope due to being replaced or deleted. (The `MNCL_KV` maps "own" their values unless you define a deleter that doesn't do anything.) Every element that goes into a `MNCL_KV` uses the same deleter.

A `MNCL_KV_VALUE_FN` is used when iterating over a map. Its semantics are defined with `mncl_kv_foreach`, below.

### Functions ###

```C
MNCL_KV *mncl_alloc_kv(MNCL_KV_DELETER deleter);
void mncl_free_kv(MNCL_KV *kv);
```

A `MNCL_KV` map "owns" its values; when you replace or delete a value, a deletion function specified by you is called with the value being replaced or deleted as its argument. (These deleters have the same general signature as `free()`.) Every element that goes into a `MNCL_KV` uses the same deleter.

```C
int mncl_kv_insert(MNCL_KV *kv, const char *key, void *value);
void *mncl_kv_find(MNCL_KV *kv, const char *key);
void mncl_kv_delete(MNCL_KV *kv, const char *key);
```

Insert, lookup, and delete are all pretty straightforward.

```C
void mncl_kv_foreach(MNCL_KV *kv, MNCL_KV_VALUE_FN fn, void *user);
```

This allows for directed iteration. For every (key, value) pair in `kv`, `mncl_kv_foreach` will call `fn(key, value, user)`. The `user` parameter is passed unmodified; think of it as a `this` pointer, or as the enclosing context of `fn`, depending on which other languages you are familiar with.

# Conclusion #

To be written. Seems like we should have one, though.
