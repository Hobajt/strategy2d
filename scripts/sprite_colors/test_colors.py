import cv2
import numpy as np
import argparse, os

'''
For validation that modify_colors.py worked.
    - LMB into the samples image to select color
    - should properly color the sprites in the spritesheet
    - RMB to show the original (without any palette changes)
    - doesn't override anything, just for validation
'''

#colors that are assigned from modify_colors.py
custom_colors = np.array([[0,0,51*i,204] for i in range(1,5)])
bg_clr = [83,103,141,0]

def parse_args():
    args = argparse.ArgumentParser()

    args.add_argument('filepath', type=str, help='Path to the spritesheet for testing.')
    args.add_argument('--samples_path', type=str, default='color_samples.png', help='Path to the image with sample colors..')

    args.add_argument('-a', '--keep_alpha', action='store_true', help='Keeps the background caused by alpha channel.')

    return args.parse_args()

def main(args):
    if not os.path.exists(args.samples_path):
        print("Color samples image is missing.")
        exit(1)
    if not os.path.exists(args.filepath):
        print("Spritesheet path is invalid.")
        exit(1)

    colors_img = cv2.imread(args.samples_path, cv2.IMREAD_UNCHANGED)
    img = cv2.imread(args.filepath, cv2.IMREAD_UNCHANGED)

    #params of the sample image
    w,h = 76,69
    r,c = 2,4
    ow,oh = w+1,h+1
    yp,xp,step = 60,22,10
    scale = 3

    colors = []

    #retrieve color palettes from the sample image
    for y in range(r):
        for x in range(c):
            patch = colors_img[1+y*oh:1+y*oh+h,1+x*ow:1+x*ow+w]
            palette = [patch[yp,xp+i*step] for i in range(4)]
            colors.append(palette)

    def replace_colors(patch, clr_org, clr_new):
        for i in range(len(clr_org)):
            t = np.all((patch == clr_org[i]), axis=-1)
            patch[t] = clr_new[i]

    def display(res):
        res = np.array(res)
        if not args.keep_alpha:
            res[res[...,-1] == 0] = bg_clr
        cv2.imshow('image', res)

    def callback(evt, x, y, flags, params):

        x,y = (x-1)//ow, (y-1)//oh
        idx = y*c+x

        if evt == cv2.EVENT_LBUTTONDOWN:
            res = np.array(img)
            print(f"COLORS: {np.array(colors[idx])[0]}")
            replace_colors(res, custom_colors, colors[idx])
            display(res)
        elif evt == cv2.EVENT_RBUTTONDOWN:
            display(img)

    cv2.namedWindow('image', cv2.WINDOW_NORMAL)
    cv2.resizeWindow('image', (int(img.shape[1]*scale), int(img.shape[0]*scale)))
    display(img)

    cv2.namedWindow('samples', cv2.WINDOW_NORMAL)
    cv2.resizeWindow('samples', (int(colors_img.shape[1]*scale), int(colors_img.shape[0]*scale)))
    cv2.setMouseCallback('samples', callback)
    cv2.imshow('samples', colors_img)

    cv2.waitKey(0)
    print("Done.")

if __name__ == "__main__":
    main(parse_args())
