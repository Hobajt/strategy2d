import numpy as np
import cv2
import argparse, os

'''
Computes sprite width/height, initial & step offsets and row coordinates - for sprite JSON descriptions.
Can only do this for groups of sprites, that are in a rectangular grid (r*c sprites in a rectangle, doesn't really work when there's incomplete row).
    - ym, xm, yM, xM - identifies a rectangular cutout from the image, where all the relevant sprites are (values are image coordinates)
        - include empty spaces in these values (ie. 2 rows -> use coord of where the 3rd row starts as y-max (not just where the 2nd ends))
'''

#background color in the spritesheet (BGRA)
bg_clr = [83,103,141,0]

def parse_args():
    args = argparse.ArgumentParser()
    args.add_argument('filepath', type=str, help='Path to the input spritesheet.')
    
    args.add_argument('ym', type=int, help='y-min coordinate of the cutout.')
    args.add_argument('xm', type=int, help='x-min coordinate of the cutout.')
    args.add_argument('yM', type=int, help='y-max coordinate of the cutout.')
    args.add_argument('xM', type=int, help='x-max coordinate of the cutout.')

    args.add_argument('-r', '--rows', type=int, required=True, help='How many rows of sprites are in the cutout.')
    args.add_argument('-c', '--cols', type=int, required=True, help='How many cols of sprites are in the cutout.')

    args.add_argument('--dont_display', action='store_true', help="Skip displaying of the detected sprites.")

    args.add_argument('--keep_row_logic', action='store_true', help='')

    args.add_argument('-s', '--sprite_count', type=int, default=0, help="There's not rows*cols sprites in the cutout.")


    args = args.parse_args()
    args.sprite_count = args.sprite_count if args.sprite_count > 0 else (args.rows*args.cols)
    return args

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

    # print(bb, mask.sum(), np.prod(img.shape[:2]))
    return bb

def viz_bounding_boxes(img, bb, clr = [255,0,0,255]):
    res = np.array(img)
    for b in bb:
        x,y = b[1],b[0]
        w,h = b[3]-b[1]+1, b[2]-b[0]+1
        r = (x,y,w,h,)
        cv2.rectangle(res, r, clr, thickness=1)

    cv2.namedWindow('res', cv2.WINDOW_NORMAL)
    cv2.resizeWindow('res', (res.shape[1] * 2, res.shape[0]*2))
    cv2.imshow('res', bgr2rgb(res))
    cv2.waitKey(0)

def viz_boxes_by_row(img, grid):
    clrs = [[255,0,0,255],[0,255,0,255],[0,0,255,255],[255,0,255,255]]
    for i,row in enumerate(grid):
        clr = clrs[i] if i < len(clrs) else [*[int(x) for x in np.random.randint(255, size=3)], 255]
        print(clr)
        viz_bounding_boxes(img, row, clr)

def viz_centroids(img, centers, clr = [255,0,255,255]):
    res = np.array(img)

    for row in centers:
        for y,x in row:
            cv2.circle(res, (int(x), int(y)), 2, clr, thickness=-1)

    cv2.namedWindow('res', cv2.WINDOW_NORMAL)
    cv2.resizeWindow('res', (res.shape[1] * 2, res.shape[0]*2))
    cv2.imshow('res', bgr2rgb(res))
    cv2.waitKey(0)

def centroid(ym, xm, yM, xM):
    return [(ym+yM)/2, (xm+xM)/2]

def main(args):
    if not os.path.exists(args.filepath):
        print("Spritesheet path is invalid.")
        exit(1)
    img = cv2.imread(args.filepath, cv2.IMREAD_UNCHANGED)
    img = bgr2rgb(img)
    
    cut = [args.ym, args.xm, args.yM, args.xM]
    r,c = args.rows, args.cols

    img = img[cut[0]:cut[2],cut[1]:cut[3]]

    def get_step(c1,c2):
        (y1,x1),(y2,x2) = c1,c2
        return (y2-y1)/(r-1) if (r > 1) else 0,(x2-x1)/(c-1) if (c > 1) else 0

    def c_interp(c1,c2,i):
        y1,x1 = c1
        sy,sx = get_step(c1,c2)
        iy = i if args.keep_row_logic else i/(c if (c > 1) else 1)
        return y1+sy*iy, x1+sx*i

    def create_grid(bb, sprite_count):
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

    bb = find_bounding_boxes(img, bg_clr)
    if not args.dont_display:
        viz_bounding_boxes(img, bb)
    print(f"Bounding boxes found: {len(bb)}")

    grid = create_grid(bb, args.sprite_count)
    if not args.dont_display:
        viz_boxes_by_row(img, grid)

    b_c = [(centroid(*row[0]),centroid(*row[-1]),) for row in grid]
    centers = [[c_interp(*row,i) for i in range(c)] for row in b_c]
    if not args.dont_display:
        viz_centroids(img, centers)

    ########## obtaining values along the x-axis ##########
    x_step = [get_step(c1,c2)[1] for c1,c2 in b_c]
    if args.sprite_count != r*c:
        x_step = x_step[:-1]
    x_step = int(round(sum(x_step)/len(x_step)))
    print("x-step:", x_step)

    x_lim = list(zip(*[(c1[1],c2[1]) for c1,c2 in b_c]))
    x_lim = [int(round(sum(x)/len(x))) for x in x_lim]
    print("x-limits:", x_lim)

    dx = 0
    for y in range(r):
        for x in range(c):
            if y*c+x >= args.sprite_count:
                break
            x_c = x_lim[0] + x_step*x
            ym,xm,yM,xM = grid[y][x]
            dx = max(dx, x_c-xm, xM-x_c)
    print("x-edge distance:", dx)

    w = 2*dx+1
    print("width:", w)

    ox = x_step-w
    print("x-step offset:", ox)

    ix = cut[1] + x_lim[0] - dx
    print('x-initial offset:', ix)
    print('-'*20)


    ########## obtaining values along the y-axis ##########
    centers = [[centroid(*x)[0] for x in row] for row in grid]
    y_coord = [int(round(sum(x)/len(x))) for x in centers]
    print('y-pos per row:', [x+cut[0] for x in y_coord])

    dy = 0
    for y in range(r):
        for x in range(c):
            if y*c+x >= args.sprite_count:
                break
            y_c = centers[y][x]
            ym,xm,yM,xM = grid[y][x]
            dy = max(dy, y_c-ym, yM-y_c)
    dy = int(round(dy))
    print('y-edge distance:', dy)

    h = 2*dy+1
    print('height:', h)

    oy = 0
    print('y-step offset:', oy)

    iy = cut[0] + y_coord[0] - dy
    print('y-initial offset:', iy)
    print('-'*20)


    y_coord = [cut[0]+y-dy-iy for y in y_coord]

    print("COPY/PASTE:")
    print("vec =", [
        [w,h],
        [ox,oy],
        [ix,iy],
        y_coord,
        [r,c]
    ])

if __name__ == "__main__":
    main(parse_args())
