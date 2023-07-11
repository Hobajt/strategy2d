import numpy as np
import cv2
import argparse, os
from copy import deepcopy

fill_clr = [83,103,141,0]

def parse_args():
    args = argparse.ArgumentParser()
    args.add_argument('filepath', type=str, help='Path to the input spritesheet.')
    args.add_argument('-o', '--outpath', type=str, default='output.png', help='Path where to write the result.')

    return args.parse_args()

def bgr2rgb(img):
    img = np.array(img)
    b = np.array(img[...,0])
    img[...,0] = img[...,2]
    img[...,2] = b
    return img

def flood_fill(arr, visited, y, x, bb):
    box = [y,x,y,x]

    def ff_inner(y,x, stack):
        if y < 0 or x < 0 or y >= arr.shape[0] or x >= arr.shape[1]:
            return
        if visited[y,x]:
            return
        visited[y,x] = True

        if not arr[y,x]:
            ym,xm,yM,xM = box
            box[0],box[2] = min(ym,y), max(yM,y)
            box[1],box[3] = min(xm,x), max(xM,x)

            stack.append([y-1,x])
            stack.append([y+1,x])
            stack.append([y,x-1])
            stack.append([y,x+1])

    stack = [[y,x]]

    i = 0
    while i < len(stack):
        ff_inner(*stack[i], stack)
        i += 1
    
    if box != [y,x,y,x]:
        bb.append(box)

def find_bounding_boxes(img, bg_clr):
    bb = []
    mask = np.all(img == bg_clr, axis=-1)
    visited = np.zeros_like(mask)
    for y in range(mask.shape[0]):
        for x in range(mask.shape[1]):
            flood_fill(mask, visited, y, x, bb)
    return bb

def viz_bounding_boxes(img, bb, clr = [255,0,0,255]):
    res = np.array(img)
    for b in bb:
        x,y = b[1],b[0]
        w,h = b[3]-b[1]+1, b[2]-b[0]+1
        r = (x,y,w,h,)
        cv2.rectangle(res, r, clr, thickness=1)
    cv2.imshow('img', bgr2rgb(res))

def main(args):
    if not os.path.exists(args.filepath):
        print("Spritesheet path is invalid.")
        exit(1)
    img = cv2.imread(args.filepath, cv2.IMREAD_UNCHANGED)
    img = bgr2rgb(img)

    h,w = img.shape[:2]

    bb = find_bounding_boxes(img, fill_clr)
    bb_new = deepcopy(bb)

    ctx = argparse.Namespace(
        running = True,
        scale = 1,
        show_orig = False,
        outline_orig = False,
        show_outline = True,
        h = h,
        w = w,
        idx = -1
    )

    def terminate():
        ctx.running = False
    
    def display():
        # viz_bounding_boxes(img, bb)
        if not ctx.show_orig:
            res = np.zeros((ctx.h, ctx.w, img.shape[-1]), dtype=img.dtype)
            res[...,:] = fill_clr

            for old,new in zip(bb, bb_new):
                ym,xm,yM,xM = old
                patch = img[ym:yM+1, xm:xM+1]
                ym,xm,yM,xM = new
                res[ym:yM+1,xm:xM+1] = patch
        else:
            res = np.array(img)
        
        if ctx.idx > 0 and ctx.show_outline:
            ym,xm,yM,xM = bb[ctx.idx] if ctx.outline_orig else bb_new[ctx.idx]
            y,x,ys,xs = ym,xm,yM-ym,xM-xm
            cv2.rectangle(res, (x,y,xs+1,ys+1,), [255,0,0,255], thickness=1)
        
        res = bgr2rgb(res)
        cv2.imshow('img', res)
        return res

    
    def mouse_callback(evt, x, y, flags, params):

        if evt == cv2.EVENT_LBUTTONDOWN:
            ctx.idx = -1
            for i,(ym,xm,yM,xM) in enumerate(bb):
                if y >= ym and y <= yM and x >= xm and x <= xM:
                    ctx.idx = i
                    print(f"Selected BB[{i}] {bb[i]} {bb_new[i]}")
                    break
            
            if ctx.idx < 0:
                print("No BB selected")
        
            display()
    
    def write():
        a,b,c = ctx.show_orig, ctx.outline_orig, ctx.show_outline
        ctx.show_orig, ctx.outline_orig, ctx.show_outline = False, False, False

        res = display()
        cv2.imwrite(args.outpath, res)
        print(f"Current layout written to '{args.outpath}'...")

        ctx.show_orig, ctx.outline_orig, ctx.show_outline = a,b,c

    def toggle_view():
        ctx.show_orig = not ctx.show_orig

    def toggle_outline():
        ctx.outline_orig = not ctx.outline_orig

    def scale_inc():
        ctx.scale += 0.1
        cv2.resizeWindow('img', (int(img.shape[1]*ctx.scale),int(img.shape[0]*ctx.scale),))
    
    def scale_dec():
        ctx.scale = (ctx.scale-0.1) if (ctx.scale-0.1) > 0.1 else 0.1
        cv2.resizeWindow('img', (int(img.shape[1]*ctx.scale),int(img.shape[0]*ctx.scale),))

    def width_inc():
        ctx.w += 1
        print(f"CURRENT SIZE: [{ctx.h}, {ctx.w}], ORIG SIZE: [{h}, {w}]")
    
    def width_dec():
        ctx.w = (ctx.w-1) if (ctx.w-1) > 0 else 1
        print(f"CURRENT SIZE: [{ctx.h}, {ctx.w}], ORIG SIZE: [{h}, {w}]")
    
    def height_inc():
        ctx.h += 1
        print(f"CURRENT SIZE: [{ctx.h}, {ctx.w}], ORIG SIZE: [{h}, {w}]")
    
    def height_dec():
        ctx.h = (ctx.h-1) if (ctx.h-1) > 0 else 1
        print(f"CURRENT SIZE: [{ctx.h}, {ctx.w}], ORIG SIZE: [{h}, {w}]")
    
    def move_y(n):
        if ctx.idx < 0:
            return
        bb_new[ctx.idx][0] += n
        bb_new[ctx.idx][2] += n
        print(f"BB[{ctx.idx}] {bb_new[ctx.idx]}")
    
    def move_x(n):
        if ctx.idx < 0:
            return
        bb_new[ctx.idx][1] += n
        bb_new[ctx.idx][3] += n
        print(f"BB[{ctx.idx}] {bb_new[ctx.idx]}")

    def move_up():
        move_y(-1)

    def move_UP():
        move_y(-10)
    
    def move_down():
        move_y(1)
    
    def move_DOWN():
        move_y(10)
    
    def move_left():
        move_x(-1)
    
    def move_LEFT():
        move_x(-10)
    
    def move_right():
        move_x(1)
    
    def move_RIGHT():
        move_x(10)


    cv2.namedWindow('img', cv2.WINDOW_NORMAL)
    cv2.resizeWindow('img', (int(img.shape[1]*ctx.scale),int(img.shape[0]*ctx.scale),))
    cv2.setMouseCallback('img', mouse_callback)

    handlers = {
        ord('q'): terminate,
        ord('o'): scale_dec,
        ord('p'): scale_inc,
        ord('r'): write,

        ord('t'): toggle_view,
        ord('g'): toggle_outline,

        ord('i'): height_inc,
        ord('k'): height_dec,
        ord('l'): width_inc,
        ord('j'): width_dec,

        ord('w'): move_up,
        ord('s'): move_down,
        ord('a'): move_left,
        ord('d'): move_right,
        ord('W'): move_UP,
        ord('S'): move_DOWN,
        ord('A'): move_LEFT,
        ord('D'): move_RIGHT,
    }

    while ctx.running:
        display()

        key = cv2.waitKey(0)
        if key in handlers:
            handlers[key]()
        


if __name__ == "__main__":
    main(parse_args())
