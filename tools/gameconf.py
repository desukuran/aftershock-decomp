#!/usr/bin/env python3
"""Which game the tools work on, and its settings (games/<game>/game.ini).

The game is $GAME (a folder name under games/) if set, otherwise the games/
folder the current directory is in:

  cd games/madness && ../../tools/progress.py
  GAME=madness tools/progress.py

  gameconf.py [key]     print the game's folder, or one of its settings
"""
import configparser
import os
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GAMES = os.path.join(REPO, 'games')


def games():
    return sorted(g for g in os.listdir(GAMES) if os.path.exists(os.path.join(GAMES, g, 'game.ini')))


def _find():
    name = os.environ.get('GAME')
    if name:
        path = os.path.join(GAMES, name)
        if not os.path.exists(os.path.join(path, 'game.ini')):
            sys.exit('GAME=%s: no games/%s/game.ini (games: %s)' % (name, name, ', '.join(games())))
        return path
    cwd = os.path.realpath(os.getcwd())
    rel = os.path.relpath(cwd, os.path.realpath(GAMES))
    if not rel.startswith('..') and rel != '.':
        return os.path.join(GAMES, rel.split(os.sep)[0])
    sys.exit('no game chosen: run from games/<game>/ or set GAME (games: %s)' % ', '.join(games()))


def ranges(text):
    return [tuple(int(v, 16) for v in r.split('-')) for r in text.split()]


def config(root):
    """A game folder's game.ini settings."""
    c = configparser.ConfigParser()
    c.read(os.path.join(root, 'game.ini'))
    return c['game']


ROOT = _find()                                   # the game's folder
NAME = os.path.basename(ROOT)
CONFIG = config(ROOT)

TITLE = CONFIG['title']
ORIGINAL = os.path.join(ROOT, 'original', 'default.xbe')
APP_RANGES = ranges(CONFIG['app_ranges'])
GAME = APP_RANGES[0]
RDATA = ranges(CONFIG['rdata'])[0]
D3D_TEXTURESTATE = int(CONFIG['d3d_texturestate'], 16)
D3D_RENDERSTATE = int(CONFIG['d3d_renderstate'], 16)

if __name__ == '__main__':
    print(CONFIG[sys.argv[1]] if len(sys.argv) > 1 else ROOT)
