import argparse, json, os
import numpy as np
import cv2

win_name = 'tilemap'
viz_width = 640

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

def parse_args():
    args = argparse.ArgumentParser()

    args.add_argument('filepath', type=str, help="Path to a JSON file with the descriptions.")

    

    return args.parse_args()

norm = lambda img : (img - img.min()) / (img.max() - img.min() + 1e-4)

def main(args):
    if not os.path.exists(args.filepath):
        print(f"File '{args.filepath}' doesn't exist.")
        exit(1)
    with open(args.filepath, 'r') as f:
        desc = json.load(f)

    if not os.path.exists(desc['texture_filepath']):
        print(f"Texture '{desc['texture_filepath']}' not found.")
        exit(1)

    tilemap = cv2.imread(desc['texture_filepath'], cv2.IMREAD_UNCHANGED)
    c,r = desc['tile_count']
    w,h = desc['tile_size']
    o = desc['offset']
    ow,oh = w+o, h+o

    i2c = lambda i : (i // c, i % c)

    tiletypes = np.full((r, c), -1, dtype=np.int32)
    if 'tile_types' in desc:
        tiletypes = tiletypes.flatten()
        for tileTypeIdx, tileRanges in enumerate(desc['tile_types']):
            for s,e in tileRanges:
                tiletypes[s:e] = tileTypeIdx
        tiletypes = tiletypes.reshape((r, c))

    ctx = argparse.Namespace( 
        running = True, show_tilemap = True, 
        start=0, end=0,
        tileID = 0
    )

    def display():
        if ctx.show_tilemap:
            img = np.array(tilemap)
            for i in range(ctx.start, ctx.end, 1):
                y,x = i2c(i)
                patch = ((img[oh*y:oh*y+h,ow*x:ow*x+w] / 255.0) + (1, 0, 1, 1)) * 0.5
                img[oh*y:oh*y+h,ow*x:ow*x+w] = (patch * 255.0).astype(np.uint8)
        else:
            img = (norm(np.array(tiletypes)) * 255.0).astype(np.uint8)
        cv2.imshow(win_name, img)

    def tilemap_callback(evt, x, y, flags, params):
        x, y = x // ow, y // oh
        idx = y*c+x

        updated = False
        if evt == cv2.EVENT_LBUTTONDOWN:
            ctx.start = idx
            updated = True
        elif evt == cv2.EVENT_RBUTTONDOWN:
            ctx.end = idx+1
            updated = True
        
        if updated:
            display()

    def stop():
        ctx.running = False
    
    def toggle_viz():
        ctx.show_tilemap = not ctx.show_tilemap
    
    def tile_inc():
        ctx.tileID += 1
        if ctx.tileID >= len(tile_names):
            ctx.tileID = len(tile_names)-1
        print(f"Tile = '{tile_names[ctx.tileID]}'")
    
    def tile_dec():
        ctx.tileID -= 1
        if ctx.tileID < 0:
            ctx.tileID = 0
        print(f"Tile = '{tile_names[ctx.tileID]}'")
    
    def tile_assign():
        for i in range(ctx.start, ctx.end, 1):
            y,x = i2c(i)
            tiletypes[y,x] = ctx.tileID
        if ctx.start < ctx.end:
            print(f"SET '{tile_names[ctx.tileID]:16s}' ON <{ctx.start:4d}, {ctx.end:4d})")

    def tile_clear():
        for i in range(ctx.start, ctx.end, 1):
            y,x = i2c(i)
            tiletypes[y,x] = -1
        if ctx.start < ctx.end:
            print(f"SET '{'NONE':16s}' ON <{ctx.start:4d}, {ctx.end:4d})")
    
    def write():
        ttypes = tiletypes.flatten()
        res = [list() for _ in range(len(tile_names))]

        prev,start = -1,0
        for i in range(ttypes.size):
            if prev != ttypes[i]:
                if prev >= 0 and prev < len(tile_names):
                    res[prev].append([start, i])
                start = i
            prev = ttypes[i]

        desc['tile_types'] = res

        with open(args.filepath, 'w') as f:
            json.dump(desc, f)
        print(f"CHANGES WRITTEN TO '{args.filepath}'...")

    handlers = {
        ord('q'): stop,
        ord('t'): toggle_viz,
        ord('+'): tile_inc,
        ord('-'): tile_dec,
        ord(' '): tile_assign,
        ord('x'): tile_clear,
        ord('w'): write
    }

    cv2.namedWindow(win_name, cv2.WINDOW_NORMAL)
    cv2.resizeWindow(win_name, (viz_width, int(viz_width * r/c)))
    cv2.setMouseCallback(win_name, tilemap_callback)

    while ctx.running:
        display()

        key = cv2.waitKey(0)
        if key in handlers:
            handlers[key]()


if __name__ == "__main__":
    main(parse_args())
