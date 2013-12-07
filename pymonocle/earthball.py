import random

from pymonocle import Monocle, keycodes


mncl = Monocle()


class Globe(object):
    def __init__(self, x_max, y_max, sprite):
        self.sprite = sprite
        width = height = 64
        nframes = 30
        self.x_max = x_max - width
        self.y_max = y_max - height

        self.obj = mncl.create_object()
        self.obj.set_sprite(sprite)
        self.obj.x = random.randint(0, self.x_max)
        self.obj.y = random.randint(0, self.y_max)
        self.obj.f = random.choice(range(nframes))
        self.obj.dx = random.randint(1, 5) * random.choice([-1, 1])
        self.obj.dy = random.randint(1, 5) * random.choice([-1, 1])
        self.obj.df = 1

    def update(self):
        # Monocle actually does the movement. We just set sensible boundary
        # conditions here.
        newx = self.obj.x + self.obj.dx
        newy = self.obj.y + self.obj.dy

        if newx < 0:
            self.obj.x = self.obj.dx
            self.obj.dx = -self.obj.dx
        elif newx > self.x_max:
            self.obj.x = self.x_max + self.obj.dx
            self.obj.dx = -self.obj.dx

        if newy < 0:
            self.obj.y = self.obj.dy
            self.obj.dy = -self.obj.dy
        elif newy > self.y_max:
            self.obj.y = self.y_max + self.obj.dy
            self.obj.dy = -self.obj.dy


class EarthBall(object):

    def instructions(self):
        insts = mncl.data_resource("instructions")
        if insts.value:
            print "Instructions:"
            for val in insts.value:
                if isinstance(val.value, basestring):
                    print "    %s" % val.value
                else:
                    print "    (invalid datum)"
        else:
            print "No instructions available"

    def update(self):
        for globe in self.globes:
            globe.update()

    def render(self):
        mncl.draw_rect(256, 112, 256, 256, 128, 128, 128)

    def run(self):

        mncl.init()
        mncl.config_video("Earthball Demo", 768, 480, 0, 0)
        mncl.add_resource_zipfile("bin/earthball-res.zip")

        mncl.load_resmap("earthball.json")
        self.earth = mncl.sprite_resource("earth")
        self.sfx = mncl.sfx_resource("sfx")
        mncl.play_music_resource("bgm", 2000)

        self.globes = [Globe(768, 480, self.earth) for _ in xrange(16)]

        done = False
        countdown = 0
        music_on = True
        bg = 0
        music_volume = 128

        self.instructions()
        mncl.hide_mouse_in_fullscreen(True)

        while not done:
            e = mncl.pop_global_event()
            if e.type == mncl.MNCL_EVENT_QUIT:
                done = True

            elif e.type == mncl.MNCL_EVENT_KEYDOWN:
                if e.value.key == keycodes.SDLK_ESCAPE:
                    mncl.fade_out_music(1000)
                    countdown = 100

                elif e.value.key == keycodes.SDLK_b:
                    bg = (bg + 1) % 4
                    mncl.set_clear_color(
                        128 if bg == 1 else 0,
                        128 if bg == 2 else 0,
                        128 if bg == 3 else 0)

                elif e.value.key == keycodes.SDLK_p:
                    music_on = not music_on
                    if music_on:
                        mncl.resume_music()
                    else:
                        mncl.pause_music()

                elif e.value.key == keycodes.SDLK_t:
                    print "Fullscreen does strange things, ignored."
                    # print "Toggled fullscreen to: %s" % (
                    #     mncl.toggle_fullscreen(),)
                    # print "Fullscreen reported as: %s" % (
                    #     mncl.is_fullscreen(),)

                elif e.value.key == keycodes.SDLK_UP:
                    music_volume = min(music_volume + 16, 128)
                    mncl.music_volume(music_volume)
                    print "Music volume now: %d" % (music_volume,)

                elif e.value.key == keycodes.SDLK_DOWN:
                    music_volume = max(music_volume - 16, 0)
                    mncl.music_volume(music_volume)
                    print "Music volume now: %d" % (music_volume,)

                elif e.value.key == keycodes.SDLK_SPACE:
                    mncl.play_sfx(self.sfx, 128)

                elif e.value.key == keycodes.SDLK_LSHIFT:
                    mncl.play_sfx(self.sfx, 64)

            elif e.type == mncl.MNCL_EVENT_UPDATE:
                self.update()
                if countdown > 0:
                    countdown -= 1
                    if countdown == 0:
                        done = True

            elif e.type == mncl.MNCL_EVENT_RENDER:
                if e.value.self == mncl.NULL:
                    self.render()

        mncl.stop_music()
        mncl.uninit()


if __name__ == '__main__':
    EarthBall().run()
