import numpy as np
import cv2
import argparse, os

def parse_args():
    args = argparse.ArgumentParser()
    args.add_argument('filepath', type=str, help='Path to the input spritesheet.')
    
    args.add_argument('-ym', type=int, default=-1, help='y-min coordinate of the cutout.')
    args.add_argument('-xm', type=int, default=-1, help='x-min coordinate of the cutout.')
    args.add_argument('-yM', type=int, default=-1, help='y-max coordinate of the cutout.')
    args.add_argument('-xM', type=int, default=-1, help='x-max coordinate of the cutout.')

    args.add_argument('-d', '--dont_display', action='store_true', help="Skip displaying of the detected sprites.")

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
    img = cv2.imread(args.filepath, cv2.IMREAD_UNCHANGED)
    img = bgr2rgb(img)

    args.ym = args.ym if args.ym >= 0 else 0
    args.xm = args.xm if args.xm >= 0 else 0
    args.yM = args.yM if args.yM >= 0 else img.shape[0]
    args.xM = args.xM if args.xM >= 0 else img.shape[1]
    
    img = remove_noise(img)
    
    cv2.namedWindow('res', cv2.WINDOW_NORMAL)
    cv2.resizeWindow('res', (int(img.shape[1] * 1.2), int(img.shape[0]*1.2)))

    bb = find_bounding_boxes(img)

    res = img
    if not args.dont_display:
        res = viz_bounding_boxes(img, bb)
        cv2.imshow('res', res)
        cv2.waitKey(0)
    
    bb = merge_overlapping_boxes(bb)
    res = viz_bounding_boxes(bgr2rgb(res), bb, clr = [0,0,255,255])
    cv2.imshow('res', res)
    cv2.waitKey(0)

    res = np.array(res)
    np.save(args.outpath, bb)
    print(f"saved as '{args.outpath}'")

if __name__ == "__main__":
    main(parse_args())
