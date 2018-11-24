#!/usr/bin/env python

import sys

if len(sys.argv) != 3:
    print("Usage: layoutgen.py LAYOUTCFG HEADEROUT")

layoutcfg = sys.argv[1];
headerout = sys.argv[2];
outf = open(headerout, mode="w")

MODIFIERS = ("LSHIFT", "RSHIFT", "LCTRL", "RCTRL", "LGUI", "RGUI", "LALT", "RALT")

LAYER_MAX = 64
layer = 0
n_cols = 0
n_rows = 0
outf.write("{")
for lineno, line in enumerate(open(layoutcfg)):
    l = line.strip()
    if len(l) == 0:
        continue
    elif (l[0] == '#'):
        continue
    elif len(l) > 2 and l.startswith("|-"):
        layer += 1
        if (layer > LAYER_MAX - 1):
            print("Error: too many layers")
        n_rows = 0
        outf.write("\n},")
        outf.write("\n{")
        continue

    tokens_all = l.split("|")
    if len(tokens_all) < 3:
        print("Error: syntax error @ line", lineno)
        exit(1)
    tokens = tokens_all[1:-1]
    n_cols = len(tokens)
    n_rows += 1

    lineout = "\n  {"
    for tokenno, token in enumerate(tokens):
        tok = token.strip()
        elem = "{"
        if len(tok) == 0:
            elem += "KeyState::UP      , {:11}".format("0")
        elif len(tok) < 4:
            print("Error: token '", tok, "' #", tokenno, " too short @ line", lineno, )
            exit(1)
        else:
            if tok.startswith("SW_"):
                elem += "KeyState::KEY_DOWN, {:11}".format("KEY_" + tok)
            elif tok.startswith("FN_L"):
                elem += "KeyState::MOD_DOWN, {:11}".format("FUN_LOFFS|" + tok[4:])
            elif tok.startswith("FN_B"):
                elem += "KeyState::MOD_DOWN, {:11}".format("FUN_LBASE|" + tok[4:])
            elif tok in MODIFIERS:
                elem += "KeyState::MOD_DOWN, {:11}".format("MOD_" + tok)
            else:
                print("Error: unexpected token:'", tok, "' #", tokenno, " @ line", lineno, )
                exit(1)
        elem += "},"
        lineout += elem
    lineout += "},"
    outf.write(lineout)
outf.write("\n}")

# Fill out remaining layers with empty rows
for _ in range(LAYER_MAX - layer - 1):
    outf.write("\n,{")
    for _ in range(n_rows):
        outf.write("\n  {")
        for _ in range(n_cols):
            outf.write("{KeyState::UP,0}, ")
        outf.write("},")
    outf.write("\n}")

outf.write("\n")
