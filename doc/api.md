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

These add directories or zip files to the component's search path. They use the name "resource" here because the client will generally be using these functions alongside the Resource Component in layer 2. The argument names the directory or zip file to use.

Both functions return zero on success, or an error code on failure.

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
void mncl_begin_frame(void);
void mncl_end_frame(void);
```

These two calls bracket the frame you draw. They should not be called more than once per frame. Beginning the frame clears it - the underlying implementation is allowed to use double buffering, so this keeps things clean. It will use the set clear color.

```C
void mncl_draw_rect(int x, int y, int w, int h, 
                    unsigned char r, unsigned char g, unsigned char b,
                    unsigned char a);
```

Draw a filled rectangle at (x, y) of width (w, h) of color (r, g, b) with alpha a. Colors are bytes; an alpha of 255 is opaque.

# Layer 1

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

# Layer 2 #

Layer 2 comprises data that is more structured but also more human-editable. This is where stuff modders would want to fiddle with lives, and so this stuff we actually structure on disk as JSON files. That gets parsed by the systems in this layer into more useful structures. It also mediates requests to layer 1 objects so that multiple copies are not needlessly allocated.

Above this level the spec starts getting vague until I get here, but:

- **Tile Maps:** These take a sprite sheet, a tile size, and then a giant rectangle of tiles. This is how we specify the backgrounds.
- **Fonts:** We need to encode kerning info, and JSON is as good a way as any to do that.
- **String Tables:** JSON lets us make them, they're lovely things to have, let's have them in the system.
- **Sprites:** Built out of spritesheets; these include animation orders, "hotspots" (so that we can have mncl_draw_sprite specify the middle of an image, for instance), and collision specifiers.

# Layer 3 #

This is where the game logic lives. This will probably be some kind of callback-based publish/subscribe event-handling model, which means that the code that actually dispatches stuff out based on SDL events will live here.

This will be the largest challenge, since C will want us to be exporting this as function pointers, and we will probably want to be using C++ objects in the client code. I'm going to be cross if embedding code in Scheme ends up easier than it would be in C++.

In the extreme case, Layer 3 won't even be part of Monocle at all; actually having a game engine might be the job of the client. I don't think it will come to that, though.
