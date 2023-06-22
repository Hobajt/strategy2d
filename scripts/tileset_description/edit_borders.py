import argparse, json, os
import numpy as np
import cv2

win_tilemap_name = 'tilemap'
win_editor_name = 'editor'

viz_width = 640
editor_size = 256
bs = 2

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
    
    tilemap = cv2.imread(desc['texture_filepath'], cv2.IMREAD_UNCHANGED)
    c,r = desc['tile_count']
    w,h = desc['tile_size']
    o = desc['offset']
    ow,oh = w+o, h+o

    i2c = lambda i : (i // c, i % c)

    bordercodes = np.zeros(r*c, dtype=np.uint8)
    if 'border_types' in desc:
        for code,indices in desc['border_types'].items():
            for idx in indices:
                bordercodes[idx] = code

    ctx = argparse.Namespace( 
        running = True,
        selection = -1,
        border = [0]*4,
        tilemap_view = False,
        cloning_mode = False,
        walls_view = False,
        hide_corners_viz = False,
    )

    ############################################

    def stop():
        ctx.running = False
    
    def display():
        if ctx.tilemap_view:
            img = np.zeros((r, c), dtype=np.uint8)
            for y in range(r):
                for x in range(c):
                    img[y,x] = 255 * int(bordercodes[y*c+x] != 0)
        else:
            img = tilemap
        cv2.imshow(win_tilemap_name, img)

        s = bs*2+1
        editor = np.zeros((h+2*bs, w+2*bs, 4), dtype=np.uint8)
        clr = [86, 245, 112,255]
        if ctx.selection >= 0:
            y,x = i2c(ctx.selection)
            editor[bs:-bs,bs:-bs] = tilemap[oh*y:oh*y+h,ow*x:ow*x+w]

            code = get_current_code()
            if code != bordercodes[ctx.selection]:
                clr = [99, 214, 255, 255]

        if not ctx.hide_corners_viz:
            if not ctx.walls_view:
                editor[0:s,0:s] = [*[255 if ctx.border[0] else 64]*3, 255]
                editor[0:s,-s:] = [*[255 if ctx.border[1] else 64]*3, 255]
                editor[-s:,0:s] = [*[255 if ctx.border[2] else 64]*3, 255]
                editor[-s:,-s:] = [*[255 if ctx.border[3] else 64]*3, 255]
            else:
                editor[0:s,:] = [*[255 if ctx.border[0] else 64]*3, 255]
                editor[:,-s:] = [*[255 if ctx.border[1] else 64]*3, 255]
                editor[-s:,:] = [*[255 if ctx.border[2] else 64]*3, 255]
                editor[:,0:s] = [*[255 if ctx.border[3] else 64]*3, 255]

        indicator = np.zeros((bs*2, w+2*bs, 4), dtype=np.uint8)
        indicator[...,:] = clr

        cv2.imshow(win_editor_name, np.vstack((editor, indicator)))
    
    def decompose_code(code):
        return [*[int(x) for x in format(code, '#010b')[-4:][::-1]]]

    def get_current_code():
        return 1*ctx.border[0] + 2*ctx.border[1] + 4*ctx.border[2] + 8*ctx.border[3]

    def tilemap_callback(evt, x, y, flags, params):
        x, y = x // ow, y // oh
        idx = y*c+x
        if evt == cv2.EVENT_LBUTTONDOWN:
            if not ctx.cloning_mode:
                ctx.selection = idx
                ctx.border = decompose_code(bordercodes[idx])
                print(f"SELECTED TILE [{y},{x}] (idx={idx}) (code: {ctx.border} ({bordercodes[idx]}))")
                display()
            else:
                cy,cx = i2c(ctx.selection)
                bordercodes[idx] = bordercodes[ctx.selection]
                print(f"CLONEM FROM [{cy},{cx}] TO [{y}, {x}] (code: {ctx.border} ({bordercodes[idx]}))")
        
    def editor_callback(evt, x, y, flags, params):
        cy,cx = (h+2*bs)//2, (w+2*bs)//2

        if not ctx.walls_view:
            idx = (y//cy)*2+(x//cx)
        else:
            l = [h+2*bs-y, x, y, w+2*bs-x]
            idx = max(list(enumerate(l)), key=lambda x: x[1])[0]   #== argmax

        if evt == cv2.EVENT_LBUTTONDOWN and idx >= 0 and idx < 4:
            ctx.border[idx] = 1 - ctx.border[idx]
            display()
    
    def toggle_tilemap_view():
        ctx.tilemap_view = not ctx.tilemap_view
        print(f"TILEMAP MODE: {'ON' if ctx.tilemap_view else 'OFF'}")
    
    def toggle_cloning():
        ctx.cloning_mode = not ctx.cloning_mode
        print(f"CLONING MODE: {'ON' if ctx.cloning_mode else 'OFF'}")
    
    def toggle_walls_view():
        ctx.walls_view = not ctx.walls_view
        print(f"WALLS VIEW: {'ON' if ctx.walls_view else 'OFF'}")
    
    def toggle_corners_viz():
        ctx.hide_corners_viz = not ctx.hide_corners_viz
    
    def write():
        borders = {}
        for idx, code in enumerate(bordercodes):
            if code == 0:
                continue

            code = int(code)
            if code not in borders:
                borders[code] = []
            
            borders[code].append(idx)

        desc['border_types'] = borders

        with open(args.filepath, 'w') as f:
            json.dump(desc, f)
        print(f"CHANGES WRITTEN TO '{args.filepath}'...")

    def invert_corners():
        if ctx.selection < 0:
            return
        ctx.border = [1-x for x in ctx.border]
        save_corners()

    def save_corners():
        if ctx.selection < 0:
            return
        bordercodes[ctx.selection] = get_current_code()
        y,x = i2c(ctx.selection)
        print(f"Writing code {bordercodes[ctx.selection]} onto [{y}, {x}]")
        display()

    def erase_corners():
        if ctx.selection < 0:
            return
        bordercodes[ctx.selection] = 0
        ctx.border = [0,0,0,0]
        display()
    
    ############################################

    cv2.namedWindow(win_tilemap_name, cv2.WINDOW_NORMAL)
    cv2.resizeWindow(win_tilemap_name, (viz_width, int(viz_width * r/c)))
    cv2.setMouseCallback(win_tilemap_name, tilemap_callback)

    cv2.namedWindow(win_editor_name, cv2.WINDOW_NORMAL)
    cv2.resizeWindow(win_editor_name, (editor_size, int(editor_size * (w+4*bs)/(w+2*bs))))
    cv2.setMouseCallback(win_editor_name, editor_callback)

    handlers = {
        ord('q'): stop,
        ord('w'): write,
        ord('r'): save_corners,
        ord('t'): erase_corners,
        ord('i'): invert_corners,
        ord('g'): toggle_tilemap_view,
        ord('d'): toggle_cloning,
        ord('b'): toggle_walls_view,
        ord('f'): toggle_corners_viz,
    }

    while ctx.running:
        display()
        key = cv2.waitKey(0)
        if key in handlers:
            handlers[key]()

if __name__ == "__main__":
    main(parse_args())
