# Monocle #
## A Minimal, Native Console-Like library based on SDL ##

Monocle is a platform for writing 2D sprite-based games that is designed to have as simple as possible of an API while still enabling works of great sophistication. It draws inspiration from YoYoGames's _Game Maker_ systems, as well as from the core engines behind the game projects I've worked on over the years (predominantly _The Ur-Quan Masters_ and _Sable_).

The Monocle Library is built in layers. Each layer is designed to operate with a well-defined C interface, to make embedding parts of it in other languages as straightforward as possible. If a layer does not mesh well with the target language, a cut may be made at a lower level.

This spec does not currently discuss user input. For now, user input is handled by the client code interacting directly with SDL. This does rather restrict the languages Monocle can work with easily to C and C++, but that's the initial goal, so here we go. There are presently hazy plans to have a callback-based API to do event handling and thus abstract away input as well, but that hasn't been nailed down yet.

Since Monocle largely deals with the details of managing and deploying various resources, and its API is C-based, there are many paired methods of the form acquire/release. It is expected that C++-based clients will use RAII techniques with thin wrappers to handle those; Monocle itself will probably provide some. Due to the general horror that is C++ function exports, these classes will only be expected to see use in cases where Monocle is directly incorporated into the executable instead of linked against as a DLL.

# Layer 0 #

Layer 0 functionality is mostly direct interaction with the system as a whole.  The two currently specified components are the Raw Data component and the frame buffer.

## Raw Data Component ##

This is a virtual filesystem. Clients of this library send filenames to it and it gives them back an in-memory copy of that file. The client code does not have to worry about whether it was loaded directly from a file, decompressed from a zip file, or anything else.

This component can be thought of as a drastically simplified version of the PhysicsFS system (http://icculus.org/physfs/), and indeed it would not be difficult to extend this component to wrap PhysicsFS in all its glory.

For now, however, all you need to link in is zlib.

### Data types ###

```C
struct MNCL_RAW {
    unsigned char *data;
    unsigned int size;
};
```

The raw data itself. It's just a big old chunk of bytes with a size marker. The data pointer is allocated on the heap and is "owned" by the structure; clients should not free it themselves.

(These structures are actually defined with typedefs in the usual manner, so one will declare it in prototypes and such with `MNCL_RAW *raw`, not `struct MNCL_RAW *raw`.)

### Functions ###

```C
int mncl_add_resource_directory(const char *pathname);
int mncl_add_resource_zipfile(const char *pathname);
```

These add directories or zip files to the front of the component's search path. They use the name "resource" here because the client will generally be using these functions alongside the Resource Component in layer 2. The argument names the directory or zip file to use.

Both functions return true on success, or false on failure.

Because they add to the *front* of the search path, resource locations added later override stuff added earlier. So, the protocol is to add core data first, and then add-ons.

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

## Framebuffer Component ##

This corresponds to the screen; it's what you draw on. It's not *necessarily* a framebuffer, but for the initial implementation it will be. A design goal is to be able to replace this with OpenGL without the client noticing. Use of this component requires you to link in SDL, SDL_mixer, and SDL_image.

### Data Structures ###

None are exported.

### Functions ###

```C
void mncl_init(void);
void mncl_uninit(void);
```

These two calls should bracket all usage of the Monocle library. They're listed with the framebuffer component both for convenience and because the raw data component doesn't happen to need them.

```C
int mncl_config_video (width, height, fullscreen, reserved);
```

Set up the screen. Pretty self-explanatory, other than that "reserved" is an integer and must be zero. That argument will be used in the future as a selector for true framebuffer vs. OpenGL implementation type, and possibly other flags as needed.

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

```C
void mncl_begin_frame(void);
void mncl_end_frame(void);
```

These two calls bracket the frame you draw. They should not be called more than once per frame. Beginning the frame clears it - the underlying implementation is allowed to use double buffering, so this keeps things clean. It will use the set clear color.

```C
void mncl_draw_rect(int x, int y, int w, int h, 
                    unsigned char r, unsigned char g, unsigned char b,
```

Draw a filled rectangle at (x, y) of width (w, h) of color (r, g, b). Colors are bytes.

## Event Dispatch Component ##

Internally, Monocle has an event loop that interacts with both the user and the intrinsic game physics. User-related events are forwarded directly to the client code, while events related to game physics will be generated by higher layers. All of these will use the same core data structures, though. User-related events, and the event loop itself, live at layer 0. Game-logic events live at layer 3.

Layer 0's event loop is global; anything that happens in the system will be presented in this API. The layer 3 event loop provides filtering, abstraction, and multicast capabilities.

### Events ###

Events are specified with a constant identified what kind of event this is, and then an optional data structure that carries ancillary data. The exact data structures are listed below; a summary of the event types is presented here for convenience.

 - **Quit.** Type: `MNCL_EVENT_QUIT`. Data field: None. The program is exiting. Either the user closed the application window, or some part of the client code called `mncl_post_quit`. If this event is ever returned, all subsequent events will be another Quit event.
 - **Update.** Types: `MNCL_EVENT_PREUPDATE`, `MNCL_EVENT_UPDATE`, and `MNCL_EVENT_POSTUPDATE`. These are for updating game logic. Pre-update happens before input is read; once game logic is in play, all update events will happen before any post-update events, and collision detection occurs between those two as well.
 - **Render.** Types: `MNCL_EVENT_PRERENDER`, `MNCL_EVENT_RENDER`, and `MNCL_EVENT_POSTRENDER`. These are the callbacks that signal the client that it is time to draw things. Conceptually, prerendering is for background images, render is for the sprites, and postrender is for the HUD and GUI.
 - **Frame Boundary.** Type: `MNCL_EVENT_FRAMEBOUNDARY`. The per-frame wait has been completed at this point. This is not likely to be an interesting event for clients, though it is where the event loop technically begins. It may be removed from the actually active API at some point, but it has its place internally.
 - **Keyboard events.** Types: `MNCL_EVENT_KEYDOWN`, `MNCL_EVENT_KEYUP`. Data field: `key`. User pressed or released the key identified by the symbol specified by `key`. These symbol constants are compatible with the `SDL_keysym` enumeration for now, but may get a rename later so that SDL headers need not be directly included.
 - **Mouse move events.** Type: `MNCL_EVENT_MOUSEMOVE`. Data field: `mousemove`. When the mouse is moved within the window, a series of these event are fired. the `x` and `y` subfields give the mouse location in screen coordinates.
 - **Mouse button events.** Types: `MNCL_EVENT_MOUSEBUTTONDOWN`, `MNCL_EVENT_MOUSEBUTTONUP`. Data field: `mousebutton`. The value is which button was pressed or released.
 - **Joystick move events.** Type: `MNCL_EVENT_JOYAXISMOVE`. Data field: `joystick`. When an analog joystick is moved, the axis it moved on gets a new value. This usually needs to be calibrated, both to get a good idea of where the edges are but also to get a good idea of where the dead zone should be. The `stick` field identifies the joystick, the `index` field of the data indicates which axis moved, and the `value` field indicates the new location of the stick on that axis. If a stick is moved diagonally, multiple events will fire. *Due to SDL using the older DirectInput library for joystick input, the LT and RT buttons on an XBox 360 controller will register as axis moves and generally misbehave. Avoid use of these buttons if possible.*
 - **Joystick button events.** Types: `MNCL_EVENT_JOYBUTTONDOWN`, `MNCL_EVENT_JOYBUTTONUP`. Data field: `joystick`. The `stick` is the joystick, the `index` is the button that has been pressed or released.
 - **Joystick hat events.** Types: `MNCL_EVENT_JOYHATMOVE`. Data field: `joystick`. The `stick` is the joystick, the `index` is the hat number, and the `value` is the new hat orientation. Orientation constants currently track SDL's. *D-Pads often present as Hats.*

### Data Structures ###

```C
typedef enum {
    MNCL_EVENT_QUIT,
    MNCL_EVENT_PREUPDATE,
    MNCL_EVENT_KEYDOWN,
    MNCL_EVENT_KEYUP,
    MNCL_EVENT_MOUSEMOVE,
    MNCL_EVENT_MOUSEBUTTONDOWN,
    MNCL_EVENT_MOUSEBUTTONUP,
    MNCL_EVENT_JOYAXISMOVE,
    MNCL_EVENT_JOYBUTTONDOWN,
    MNCL_EVENT_JOYBUTTONUP,
    MNCL_EVENT_JOYHATMOVE,
    MNCL_EVENT_UPDATE,
    MNCL_EVENT_POSTUPDATE,
    MNCL_EVENT_PRERENDER,
    MNCL_EVENT_RENDER,
    MNCL_EVENT_POSTRENDER,
    MNCL_EVENT_FRAMEBOUNDARY,
    /* ... */
    MNCL_NUM_EVENTS
} MNCL_EVENT_TYPE;
```

The event enumeration. Other events (including, at minimum, `MNCL_EVENT_COLLISION`) will be defined in later layers.

```C
struct MNCL_EVENT {
    MNCL_EVENT_TYPE type;
    int subscriber;
    union {
        int key;
        struct { int x, y; } mousemove;
        int mousebutton;
        struct { int stick, index, value; } joystick;
        /* ... */
    } value;
};
```

The event structure. This is a "tagged union" - the type field indicates which member of the union to check. Other possible values will be added in later layers.

### Functions ###

```C
MNCL_EVENT *mncl_pop_global_event(void);

MNCL_EVENT_TYPE mncl_event_type(MNCL_EVENT *evt);
int mncl_event_subscriber(MNCL_EVENT *evt);
MNCL_KEY mncl_event_key(MNCL_EVENT *evt);
int mncl_event_mouse_x(MNCL_EVENT *evt);
int mncl_event_mouse_y(MNCL_EVENT *evt);
int mncl_event_mouse_button(MNCL_EVENT *evt);
int mncl_event_joy_stick(MNCL_EVENT *evt);
int mncl_event_joy_index(MNCL_EVENT *evt);
int mncl_event_joy_value(MNCL_EVENT *evt);
```

These are the event loop accessor functions. The pointer returned by `mncl_pop_global_event` is *owned by Monocle*, and the client should neither free it, edit it, nor permit it to live too long. This pointer will be invalidated by the next call to `mncl_pop_global_event`. Attempting to pop a Quit event has no effect.

Clients written in C and C++ will probably just access the MNCL_EVENT structure directly instead of using the accessor functions.

## Key-Value Map Component ##

C rather infamously doesn't provide a whole lot of structured data types. A lot of the Monocle system needs to have string-to-object map capability under the hood, so it makes sense to expose it to other C clients.

### Data Structures ###

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

# Layer 1 #

Layer 1 lets us load and output more interesting things. These use the raw data component from level 0 internally. *However, unlike the raw data blocks, layer 1 objects are not reference-counted and interned. Each time you allocate one of these objects some allocation ends up occurring.* Reference-counting and interning of these resources happens in the resource management component, which is one level up.

## Spritesheet Component ##

A spritesheet is one or more images stored as a single unit. If you're designing for hardware acceleration, these should be thought of as roughly equivalent to a single texture you're pulling data from.

### Data Structures ###

```C
struct MNCL_SPRITESHEET;
```

It's opaque. Clients will treat pointers to this as magic handles.

### Functions ###

```C
MNCL_SPRITESHEET *mncl_alloc_spritesheet(const char *resource_name);
void mncl_free_spritesheet(MNCL_SPRITESHEET *spritesheet);
```

Lifecycle managers. These handle the interactions with layer 0 for you. Resources must correspond to .PNG files.

```C
void mncl_normalize_spritesheet(MNCL_SPRITESHEET *spritesheet);
```

Prepares the spritesheet for rendering on the screen. Since image formats don't necessarily match the most efficient format for blitting to the end user's screen, this step makes a pre-cached version that renders faster.

This happens automatically if a spritesheet is loaded after the video is configured, and if the spritesheet is managed by the resource component in layer 2, it will happen automatically all the time. As such, end users generally won't have to worry about this call.

```C
void mncl_draw_from_spritesheet(MNCL_SPRITESHEET *spritesheet,
                                int x, int y,
                                int my_x, int my_y,
                                int my_w, int my_h);
```

Draw an image from the sprite sheet at (x, y) on the screen. The last four arguments specify the rectangle from the spritesheet to draw.

## Music Component ##

This and the SFX component wrap SDL_Mixer, which is super finicky and likes to crash if you look at it funny. This wrapper makes that go away.

Music files can be .ogg or any of the old tracker formats that MikMod understands (.mod, .s3m, .xm, .it).

### Data Structures ###

None exported.

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

## SFX Component ##

Sound effects are .wav files. Monocle will allow up to four sound effects to play simultaneously.

### Data Structures ###

```C
struct MNCL_SFX;
```

Opaque datatype.

### Functions ###

```C
MNCL_SFX *mncl_alloc_sfx(const char *resource_name);
void mncl_free_sfx(MNCL_SFX *sfx);
```

Creation and destruction. Note again that these are alloc/free, not acquire/release.

```C
void mncl_play_sfx(MNCL_SFX *sfx, int volume);
```

Volumes again range from 0 to 128.

## Semi-structured Data Component ##

Monocle uses JSON internally to represent semistructured data such as resource assignments. This functionality is also offered to clients to allow for resource files to include project-specific data.

### Data Structures ###

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

### Functions ###

```C
MNCL_DATA *mncl_parse_data(const char *data, size_t size);
MNCL_DATA *mncl_data_clone (MNCL_DATA *src);
void mncl_free_data (MNCL_DATA *mncl_data);
```

These three functions create a `MNCL_DATA` from JSON text, deep copy some other `MNCL_DATA`, or free all memory associated with a `MNCL_DATA`, respectively. One should not free a data item that is part of some other data item (an array or object).

```C
const char *mncl_data_error();
```

If `mncl_parse_data` failed, it will return `NULL` and a call to this function will give some explanation as to why.

```C
MNCL_DATA *mncl_data_lookup(MNCL_DATA *map, const char *key);
```

It's kind of tedious to pick apart the union and use the `mncl_kv_*` functions to get at fields of an object, so this provides a convenient shorthand for that.

# Layer 2 #

Layer 2 comprises data that is more structured but also more human-editable. This is where stuff modders would want to fiddle with lives, and so this stuff we actually structure on disk as JSON files. That gets parsed by the systems in this layer into more useful structures. It also mediates requests to layer 1 objects so that multiple copies are not needlessly allocated.

Above this level the spec starts getting vague until I get here, but:

- **Tile Maps:** These take a sprite sheet, a tile size, and then a giant rectangle of tiles. This is how we specify the backgrounds.
- **Fonts:** We need to encode kerning info, and JSON is as good a way as any to do that.
- **String Tables:** JSON lets us make them, they're lovely things to have, let's have them in the system.
- **Sprites:** Built out of spritesheets; these include animation orders, "hotspots" (so that we can have mncl_draw_sprite specify the middle of an image, for instance), and collision specifiers.

# Layer 3 #

This is where the game logic lives. It will involve "entities", which
are a special kind of subscriber that carries some additional state
(position, velocity, hitbox, etc). These will be able to respond to
additional messages regarding the gamestate (draw, update, collision,
etc.) The subscriber ID can be used by client systems to link these
things into their own object systems.

To make this not be a tremendous pain to use, some kind of inheritance
system will be necessary; prototypal inheritance is probably best
here, since individual instances may need slight customization.
