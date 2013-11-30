import random

from pymonocle import Monocle, keycodes


class Globe(object):
    def __init__(self, x_max, y_max, sprite):
        self.sprite = sprite
        self.x_max = x_max - sprite.w
        self.y_max = y_max - sprite.h

        self.x = random.randint(0, self.x_max)
        self.y = random.randint(0, self.y_max)
        self.dx = random.randint(1, 5) * random.choice([-1, 1])
        self.dy = random.randint(1, 5) * random.choice([-1, 1])
        self.frame = random.choice(range(sprite.nframes))

    def update(self):
        self.x += self.dx
        self.y += self.dy

        if self.x < 0:
            self.x = 0
            self.dx = -self.dx
        if self.x > self.x_max:
            self.x = self.x_max
            self.dx = -self.dx
        if self.y < 0:
            self.y = 0
            self.dy = -self.dy
        if self.y > self.y_max:
            self.y = self.y_max
            self.dy = -self.dy

        self.frame = (self.frame + 1) % 30

    def draw(self):
        self.sprite.draw(self.x, self.y, self.frame)


class EarthBall(object):
    def __init__(self):
        self.mncl = Monocle()

    def instructions(self):
        insts = self.mncl.data_resource("instructions")
        if (insts is not self.mncl.NULL and
                insts.tag == self.mncl.MNCL_DATA_ARRAY):
            print "Instructions:"
            for i in range(insts.value.array.size):
                val = insts.value.array.data[i]
                if (val is not self.mncl.NULL and
                        val.tag == self.mncl.MNCL_DATA_STRING):
                    print "    %s" % self.mncl.to_string(val.value.string)
                else:
                    print "    (invalid datum)"
        else:
            print "No instructions available"

    def update(self):
        for globe in self.globes:
            globe.update()

    def render(self):
        self.mncl.draw_rect(256, 112, 256, 256, 128, 128, 128)
        for globe in self.globes:
            globe.draw()

    def run(self):

        self.mncl.init()
        self.mncl.config_video("Earthball Demo", 768, 480, 0, 0)
        self.mncl.add_resource_zipfile("bin/earthball-res.zip")

        self.mncl.load_resmap("earthball.json")
        self.earth = self.mncl.sprite_resource("earth")
        self.sfx = self.mncl.sfx_resource("sfx")
        self.mncl.play_music_resource("bgm", 2000)

        self.globes = [Globe(768, 480, self.earth) for _ in xrange(16)]

        done = False
        countdown = 0
        music_on = True
        bg = 0
        music_volume = 128

        self.instructions()
        self.mncl.hide_mouse_in_fullscreen(True)

        while not done:
            e = self.mncl.pop_global_event()
            if e.type == self.mncl.MNCL_EVENT_QUIT:
                done = True

            elif e.type == self.mncl.MNCL_EVENT_KEYDOWN:
                if e.value.key == keycodes.SDLK_ESCAPE:
                    self.mncl.fade_out_music(1000)
                    countdown = 100

                elif e.value.key == keycodes.SDLK_b:
                    bg = (bg + 1) % 4
                    self.mncl.set_clear_color(
                        128 if bg == 1 else 0,
                        128 if bg == 2 else 0,
                        128 if bg == 3 else 0)

                elif e.value.key == keycodes.SDLK_p:
                    music_on = not music_on
                    if music_on:
                        self.mncl.resume_music()
                    else:
                        self.mncl.pause_music()

                elif e.value.key == keycodes.SDLK_t:
                    print "Fullscreen does strange things, ignored."
                    # print "Toggled fullscreen to: %s" % (
                    #     self.mncl.toggle_fullscreen(),)
                    # print "Fullscreen reported as: %s" % (
                    #     self.mncl.is_fullscreen(),)

                elif e.value.key == keycodes.SDLK_UP:
                    music_volume = min(music_volume + 16, 128)
                    self.mncl.music_volume(music_volume)
                    print "Music volume now: %d" % (music_volume,)

                elif e.value.key == keycodes.SDLK_DOWN:
                    music_volume = max(music_volume - 16, 0)
                    self.mncl.music_volume(music_volume)
                    print "Music volume now: %d" % (music_volume,)

                elif e.value.key == keycodes.SDLK_SPACE:
                    self.mncl.play_sfx(self.sfx, 128)

                elif e.value.key == keycodes.SDLK_LSHIFT:
                    self.mncl.play_sfx(self.sfx, 64)

            elif e.type == self.mncl.MNCL_EVENT_UPDATE:
                self.update()
                if countdown > 0:
                    countdown -= 1
                    if countdown == 0:
                        done = True

            elif e.type == self.mncl.MNCL_EVENT_RENDER:
                self.render()

        self.mncl.stop_music()
        self.mncl.uninit()


if __name__ == '__main__':
    EarthBall().run()
