import numpy as np
import cv2
import argparse, os

'''
Script to modify spritesheet colors, so that it can be used for color cycling in the game.
Overrides specified faction color with custom values.
Resulting image is written into a new file.

Color changes:
    - alpha channel is thresholded & only has 3 values - [0, 204, 255]
        - a=204 (0.2) informs the shader to use color cycling
    - red channel is used as index into the palette
    
'''

#values that are written instead of the original faction colors
custom_colors = np.array([[0,0,51*i,204] for i in range(1,5)])

#custom starting colors to replace (when specified in args), color in BGR
custom_source_colors = np.array([
    [76,4,0,255],
    [116,20,0,255],
    [160,40,4,255],
    [204,72,12,255],
])

def parse_args():
    args = argparse.ArgumentParser()

    args.add_argument('filepath', type=str, help='Path to the input spritesheet.')
    args.add_argument('--outpath', type=str, default='output.png', help='Where to write the result.')

    args.add_argument('--samples_path', type=str, default='color_samples.png', help='Path to the image with sample colors..')
    args.add_argument('-c', '--clr_idx', type=int, default=0, choices=list(range(8)), help="Index into samples image, which color is used in the spritesheet (which color to replace).")
    args.add_argument('-d', '--different_clrs', action='store_true', help="Don't use source colors from sample image, use values specified in the script instead.")

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
    r,c = 2,4
    w,h = 76,69
    yp,xp,step = 60,22,10
    ow,oh = w+1,h+1

    #retrieve color palettes from the sample image
    colors = []
    for y in range(r):
        for x in range(c):
            patch = colors_img[1+y*oh:1+y*oh+h,1+x*ow:1+x*ow+w]
            clrs = [patch[yp,xp+i*step] for i in range(4)]
            colors.append(np.array(clrs))
    
    def replace_colors(img, clr_org, clr_new):
        for i in range(len(clr_org)):
            t = np.all((img == clr_org[i]), axis=-1)
            img[t] = clr_new[i]
    
    #alpha channel thresholding
    t = (img[...,-1] > 50)
    img[t,-1] = 255
    print(np.unique(img[...,-1]))

    src_clrs = custom_source_colors if args.different_clrs else colors[args.clr_idx]

    #update the colors
    replace_colors(img, src_clrs, custom_colors)

    uq = np.unique(img[...,-1])
    print(uq)
    if len(uq) != 3:
        print("No colors were replaced! (try different clr_idx)")

    cv2.imwrite(args.outpath, img)
    print("Done.")

if __name__ == "__main__":
    main(parse_args())

