import numpy as np
import cv2
import argparse, os
from copy import deepcopy

fill_clr = [83,103,141,0]

row_order = [0,1,3,5,7, 2,4,6,8, 9]

def parse_args():
    args = argparse.ArgumentParser()
    args.add_argument('filepath', type=str, help='Path to the input spritesheet.')
    args.add_argument('-o', '--outpath', type=str, default='output.png', help='Path where to write the result.')

    args.add_argument('-r', type=int, required=True)
    args.add_argument('-c', type=int, required=True)

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

def create_grid(bb, r, c, sprite_count):
    grid = []

    tmp = sorted(bb, key = lambda x : x[0])

    i = 0
    for y in range(r):
        row = tmp[:c]
        row = sorted(row, key = lambda x : x[1])
        if i + len(row) > sprite_count:
            row = row[:sprite_count-i]
        grid.append(row)
        tmp = tmp[c:]
        i += len(row)

    return grid

def get_row_range(img, grid, idx):
    row = grid[idx]
    m,M = 0, img.shape[0]

    if idx > 0:
        m = max([yM for ym,xm,yM,xM in grid[idx-1]])+1
    
    if idx < len(grid)-1:
        M = min([ym for ym,xm,yM,xM in grid[idx+1]])

    g = max([yM for ym,xm,yM,xM in grid[idx]])+1
    
    return m,M,g

def main(args):
    if not os.path.exists(args.filepath):
        print("Spritesheet path is invalid.")
        exit(1)
    img = cv2.imread(args.filepath, cv2.IMREAD_UNCHANGED)
    img = bgr2rgb(img)

    h,w = img.shape[:2]

    bb = find_bounding_boxes(img, fill_clr)

    grid = create_grid(bb, args.r, args.c, args.r * args.c)

    rg = []
    for idx in range(len(grid)):
        rg.append(get_row_range(img, grid, idx))
    print(rg)
    
    grid_new = []
    pos = 0
    for idx in row_order:
        row = []
        m,M,g = rg[idx]
        for ym,xm,yM,xM in grid[idx]:
            row.append((ym-m+pos, xm, yM-m+pos, xM,))
        grid_new.append(row)
        pos += g-m
    
    res = np.zeros((pos, *img.shape[1:]), dtype=img.dtype)
    res[...,:] = fill_clr

    for dst,src in enumerate(row_order):
        for src_bb, dst_bb in zip(grid[src], grid_new[dst]):
            ym,xm,yM,xM = src_bb
            patch = img[ym:yM,xm:xM]
            ym,xm,yM,xM = dst_bb
            res[ym:yM,xm:xM] = patch
    
    res = bgr2rgb(res)
    cv2.imwrite(args.outpath, res)


if __name__ == "__main__":
    main(parse_args())
