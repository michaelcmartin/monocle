import random

import pymonocle
from pymonocle.constants import (
    keycodes, EVENT_QUIT, EVENT_KEYDOWN, EVENT_PRERENDER, EVENT_RENDER)
from pymonocle.event import pop_global_event
from pymonocle.gameobject import MonocleObject


class Globe(MonocleObject):
    def __init__(self, x_max, y_max):
        width = height = 64
        nframes = 30
        self.x_max = x_max - width
        self.y_max = y_max - height

        super(Globe, self).__init__(
            random.randint(0, self.x_max),
            random.randint(0, self.y_max),
            'earth')

        self.f = random.choice(range(nframes))
        self.dx = random.randint(1, 5) * random.choice([-1, 1])
        self.dy = random.randint(1, 5) * random.choice([-1, 1])
        self.df = 1

    def update(self):
        # Monocle actually does the movement. We just set sensible boundary
        # conditions here.
        newx = self.x + self.dx
        newy = self.y + self.dy

        if newx < 0:
            self.x = 0
            self.dx = -self.dx
        elif newx > self.x_max:
            self.x = self.x_max
            self.dx = -self.dx

        if newy < 0:
            self.y = 0
            self.dy = -self.dy
        elif newy > self.y_max:
            self.y = self.y_max
            self.dy = -self.dy


class EarthBall(object):

    def instructions(self):
        insts = pymonocle.data_resource("instructions")
        if insts:
            print "Instructions:"
            for line in insts:
                print "    %s" % line
        else:
            print "No instructions available"

    def update(self):
        pass
        # for globe in self.globes:
        #     globe.update()

    def render(self):
        pymonocle.draw_rect(256, 112, 256, 256, 128, 128, 128)

    def run(self):

        pymonocle.init()
        pymonocle.config_video("Earthball Demo", 768, 480, 0, 0)
        pymonocle.add_resource_zipfile("bin/earthball-res.zip")

        pymonocle.load_resmap("earthball.json")
        self.sfx = pymonocle.sfx_resource("sfx")
        pymonocle.play_music_resource("bgm", 2000)

        self.globes = [Globe(768, 480) for _ in xrange(16)]

        done = False
        countdown = 0
        music_on = True
        bg = 0
        music_volume = 128

        self.instructions()
        pymonocle.hide_mouse_in_fullscreen(True)

        while not done:
            e = pop_global_event()
            if e.type == EVENT_QUIT:
                done = True

            elif e.type == EVENT_KEYDOWN:
                if e.key == keycodes.ESCAPE:
                    pymonocle.fade_out_music(1000)
                    countdown = 100

                elif e.key == keycodes.b:
                    bg = (bg + 1) % 4
                    pymonocle.set_clear_color(
                        128 if bg == 1 else 0,
                        128 if bg == 2 else 0,
                        128 if bg == 3 else 0)

                elif e.key == keycodes.p:
                    music_on = not music_on
                    if music_on:
                        pymonocle.resume_music()
                    else:
                        pymonocle.pause_music()

                elif e.key == keycodes.t:
                    print "Fullscreen does strange things, ignored."
                    # print "Toggled fullscreen to: %s" % (
                    #     mncl.toggle_fullscreen(),)
                    # print "Fullscreen reported as: %s" % (
                    #     mncl.is_fullscreen(),)

                elif e.key == keycodes.UP:
                    music_volume = min(music_volume + 16, 128)
                    pymonocle.music_volume(music_volume)
                    print "Music volume now: %d" % (music_volume,)

                elif e.key == keycodes.DOWN:
                    music_volume = max(music_volume - 16, 0)
                    pymonocle.music_volume(music_volume)
                    print "Music volume now: %d" % (music_volume,)

                elif e.key == keycodes.SPACE:
                    pymonocle.play_sfx(self.sfx, 128)

                elif e.key == keycodes.LSHIFT:
                    pymonocle.play_sfx(self.sfx, 64)

            elif e.type == EVENT_PRERENDER:
                # NOTE: We get one of these events for each object and one for
                # a NULL object which is for global updates.
                if e.game_object is None:
                    if countdown > 0:
                        countdown -= 1
                        if countdown == 0:
                            done = True
                else:
                    e.game_object.update()

            elif e.type == EVENT_RENDER:
                # NOTE: We get one of these events for each object and one for
                # a NULL object which is for global updates.
                if e.game_object is None:
                    self.render()

        pymonocle.stop_music()
        pymonocle.uninit()


if __name__ == '__main__':
    EarthBall().run()
