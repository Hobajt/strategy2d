import argparse, json, os
import numpy as np
import cv2

win_tilemap_name = 'tilemap'
win_borders_name = 'borders'
win_selection_name = 'selection'

viz_width = 640
ratio = int(33 * 1.5)
borders_width = 16*ratio
bs = 2

selection_size = 256

tile_names = [
    "GROUND1",
    "GROUND2",
    "MUD1",
    "MUD2",
    "WALL_BROKEN",
    "ROCK_BROKEN",
    "TREES_FELLED",
    "WATER",
    "ROCK",
    "WALL_HU",
    "WALL_HU_DAMAGED",
    "WALL_OC",
    "WALL_OC_DAMAGED",
    "TREES"
]

############################################

def parse_args():
    args = argparse.ArgumentParser()

    args.add_argument('filepath', type=str, help="Path to a JSON file with the descriptions.")    

    return args.parse_args()

def main(args):
    if not os.path.exists(args.filepath):
        print(f"File '{args.filepath}' doesn't exist.")
        exit(1)
    with open(args.filepath, 'r') as f:
        desc = json.load(f)

    if not os.path.exists(desc['texture_filepath']):
        print(f"Texture '{desc['texture_filepath']}' not found.")
        exit(1)

    if 'tile_types' not in desc:
        print(f"No 'tile_types' entry in the JSON. Use edit_types.py first.")
        exit(1)
    if 'border_types' not in desc or len(desc['border_types'].items()) < 1:
        print("No 'border_types' entry in JSON. Use edit_borders.py first.")
        exit(1)
    
    tilemap = cv2.imread(desc['texture_filepath'], cv2.IMREAD_UNCHANGED)
    c,r = desc['tile_count']
    w,h = desc['tile_size']
    o = desc['offset']
    ow,oh = w+o, h+o
    
    tm = np.zeros((tilemap.shape[0]+1, tilemap.shape[1]+1, tilemap.shape[2]), dtype=tilemap.dtype)
    tm[:-1,:-1] = tilemap
    tilemap = tm

    i2c = lambda i : (i // c, i % c)

    bordercodes = np.zeros(r*c, dtype=np.uint8)
    for code,indices in desc['border_types'].items():
        for idx in indices:
            bordercodes[idx] = code
    
    tiletypes = np.full((r, c), -1, dtype=np.int32)
    tiletypes = tiletypes.flatten()
    for tileTypeIdx, tileRanges in enumerate(desc['tile_types']):
        for s,e in tileRanges:
            tiletypes[s:e] = tileTypeIdx
    tiletypes = tiletypes.reshape((r, c))

    samples_img = cv2.imread('sample_borders.png', cv2.IMREAD_UNCHANGED)
    samples = []
    for i in range(16):
        sp = np.zeros((33, 33, samples_img.shape[-1]), dtype=samples_img.dtype)
        sp[:-1,:-1] = samples_img[32*i:32*i+32]
        samples.append(sp)

    ctx = argparse.Namespace( 
        running = True,
        selection = -1,
        type = 0
    )

    ############################################

    def stop():
        ctx.running = False
    
    def display():
        cv2.imshow(win_tilemap_name, tilemap)

        # borders = [[] for _ in range(16)]
        borders = [[x] for x in samples]
        for y in range(r):
            for x in range(c):
                if tiletypes[y,x] == ctx.type:
                    code = bordercodes[y*c+x]
                    borders[code].append(tilemap[oh*y:oh*y+oh,ow*x:ow*x+ow])
        
        y,x = 0,0
        empty = np.zeros_like(tilemap[oh*y:oh*y+oh,ow*x:ow*x+ow])
        l = max([len(x) for x in borders])
        for b in borders:
            bl = len(b)
            for _ in range(bl, l):
                b.append(empty)

        borders = np.vstack([np.hstack(x) for x in borders])
        cv2.imshow(win_borders_name, borders)
        cv2.resizeWindow(win_borders_name, (ratio * l, borders_width))

        if ctx.selection >= 0:
            y,x = i2c(ctx.selection)
            selection = tilemap[oh*y:oh*y+h,ow*x:ow*x+w]
        else:
            selection = np.zeros((h,w,tilemap.shape[-1]), dtype=np.uint8)
        cv2.imshow(win_selection_name, selection)
    
    def tilemap_callback(evt, x, y, flags, params):
        x, y = x // ow, y // oh
        idx = y*c+x
        if evt == cv2.EVENT_LBUTTONDOWN:
            ctx.selection = idx
            display()
    
    def decompose_code(code):
        return [*[int(x) for x in format(code, '#010b')[-4:][::-1]]]

    def compose_code(border):
        return 1*border[0] + 2*border[1] + 4*border[2] + 8*border[3]

    def prev_type():
        ctx.type = ctx.type-1 if ctx.type > 0 else 0
        print(f"Showing borders for '{tile_names[ctx.type]}'")
    
    def next_type():
        ctx.type = ctx.type+1 if ctx.type < (len(tile_names)-1) else (len(tile_names)-1)
        print(f"Showing borders for '{tile_names[ctx.type]}'")
    
    ############################################

    cv2.namedWindow(win_tilemap_name, cv2.WINDOW_NORMAL)
    cv2.resizeWindow(win_tilemap_name, (viz_width, int(viz_width * r/c)))
    cv2.setMouseCallback(win_tilemap_name, tilemap_callback)

    cv2.namedWindow(win_borders_name, cv2.WINDOW_NORMAL)
    cv2.resizeWindow(win_borders_name, (borders_width, int(borders_width * (w+4*bs)/(w+2*bs))))
    # cv2.setMouseCallback(win_borders_name, editor_callback)

    cv2.namedWindow(win_selection_name, cv2.WINDOW_NORMAL)
    cv2.resizeWindow(win_selection_name, (selection_size, selection_size))

    handlers = {
        ord('q'): stop,
        ord('a'): prev_type,
        ord('d'): next_type,
    }

    print(f"Showing borders for '{tile_names[ctx.type]}'")
    while ctx.running:
        display()
        key = cv2.waitKey(0)
        if key in handlers:
            handlers[key]()

if __name__ == "__main__":
    main(parse_args())
