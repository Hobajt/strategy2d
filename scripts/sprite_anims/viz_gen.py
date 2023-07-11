import numpy as np
import cv2
import argparse, os

def parse_args():
    args = argparse.ArgumentParser()
    args.add_argument('filepath', type=str, help='Path to the input spritesheet.')
    args.add_argument('-b', '--bbpath', type=str, default='res.npy', help='Path to the input spritesheet.')
    args.add_argument('-o', '--outpath', default='res.npy', type=str, help='where to store the resulting bounding boxes (as np array)')

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

def find_bounding_boxes(img):
    bb = []

    mask = (img[...,-1] == 0)
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
    return bgr2rgb(res)

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

def remove_noise(img):
    img[img[...,-1] == 0] = [0,0,0,0]
    return img

def centroid(ym, xm, yM, xM):
    return [(ym+yM)/2, (xm+xM)/2]

def merge_overlapping_boxes(bb):
    print("Box count:", len(bb))

    res = [bb[0]]

    boxes_overlap = lambda a,b : (a[0] <= b[2]) and (a[2] >= b[0]) and (a[1] <= b[3]) and (a[3] >= b[1])
    merge_boxes = lambda a,b : [min(b1,b2) if i < 2 else max(b1,b2) for i,(b1,b2) in enumerate(zip(a,b))]

    for i in range(1, len(bb)):
        merged = False
        for j in range(len(res)):
            if boxes_overlap(bb[i], res[j]):
                res[j] = merge_boxes(res[j], bb[i])
                merged = True
                break
        if not merged:
            res.append(bb[i])

    print("Box count after merge:", len(res))
    return res

def main(args):
    if not os.path.exists(args.filepath):
        print("Spritesheet path is invalid.")
        exit(1)
    
    if not os.path.exists(args.bbpath):
        print("BB path is invalid.")
        exit(1)
    img = cv2.imread(args.filepath, cv2.IMREAD_UNCHANGED)

    bb = np.load(args.bbpath)
    print(bb.shape)
    
    img = remove_noise(img)
    
    cv2.namedWindow('res', cv2.WINDOW_NORMAL)

    ctx = argparse.Namespace(
        running = True,
        idx = 0,
        scale = 1,
        fixed_size = True
    )

    def terminate():
        ctx.running = False

    def display():
        print(f"BB[{ctx.idx}]: {bb[ctx.idx]}")
        ym,xm,yM,xM = bb[ctx.idx]
        patch = img[ym:yM+1, xm:xM+1]
        if not ctx.fixed_size:
            cv2.resizeWindow('res', (int(patch.shape[1] * ctx.scale), int(patch.shape[0] * ctx.scale)))
        else:
            cv2.resizeWindow('res', (256,256))
        cv2.imshow('res', patch)

    def bb_next():
        ctx.idx = (ctx.idx+1) if (ctx.idx+1) < bb.shape[0] else bb.shape[0]
    
    def bb_prev():
        ctx.idx = (ctx.idx-1) if ctx.idx > 0 else 0
    
    def write():
        np.save(args.outpath, bb)
        print(f"Written to '{args.outpath}'...")

    def ym_dec():
        bb[ctx.idx][0] -= 1

    def ym_inc():
        bb[ctx.idx][0] += 1

    def yM_dec():
        bb[ctx.idx][2] -= 1

    def yM_inc():
        bb[ctx.idx][2] += 1
    
    def xm_dec():
        bb[ctx.idx][1] -= 1

    def xm_inc():
        bb[ctx.idx][1] += 1

    def xM_dec():
        bb[ctx.idx][3] -= 1

    def xM_inc():
        bb[ctx.idx][3] += 1
    
    def toggle_size():
        ctx.fixed_size = not ctx.fixed_size

    handlers = {
        ord('q'): terminate,
        ord('d'): bb_next,
        ord('a'): bb_prev,
        ord('w'): write,

        ord('i'): ym_dec,
        ord('I'): yM_dec,
        ord('k'): ym_inc,
        ord('K'): yM_inc,
        ord('j'): xm_dec,
        ord('J'): xM_dec,
        ord('l'): xm_inc,
        ord('L'): xM_inc,

        ord('f'): toggle_size
    }

    while ctx.running:
        display()
        key = cv2.waitKey(0)
        if key in handlers:
            handlers[key]()

if __name__ == "__main__":
    main(parse_args())
