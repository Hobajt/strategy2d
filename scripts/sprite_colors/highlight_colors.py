import numpy as np
import cv2
import argparse, os

'''Highlights selected colors in an image (overrides them with magenta).'''

#in BGR!!!
colors = np.array([
    [204,72,12,255],
    [160,40,4,255],
    [116,20,0,255],
    [76,4,0,255]
])
new_clr = [255,0,255,255]

scale = 3

def parse_args():
    args = argparse.ArgumentParser()

    args.add_argument('filepath', type=str, help='Path to the input spritesheet.')
    return args.parse_args()

def main(args):
    if not os.path.exists(args.filepath):
        print("Spritesheet path is invalid.")
        exit(1)
    img = cv2.imread(args.filepath, cv2.IMREAD_UNCHANGED)
    
    def replace_colors(img, clr_org, clr_new):
        for i in range(len(clr_org)):
            t = np.all((img == clr_org[i]), axis=-1)
            img[t] = clr_new[i]
    
    new_colors = np.array([new_clr for x in range(colors.shape[0])])
    replace_colors(img, colors, new_colors)

    cv2.namedWindow('image', cv2.WINDOW_NORMAL)
    cv2.resizeWindow('image', (int(img.shape[1]*scale), int(img.shape[0]*scale)))
    cv2.imshow('image', img)
    cv2.waitKey(0)

if __name__ == "__main__":
    main(parse_args())

