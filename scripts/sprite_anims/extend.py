import numpy as np
import cv2
import argparse, os

#fill color to override the source rectangle position, BGR format
fill_clr = [141,103,83,0]

def parse_args():
    args = argparse.ArgumentParser()
    args.add_argument('filepath', type=str, help='Path to the input spritesheet.')
    args.add_argument('-o', '--outpath', type=str, default='output.png', help='Path where to write the result.')
    
    args.add_argument('-aw', '--width', type=int, default=0, help='How many pixels to add in width.')
    args.add_argument('-ah', '--height', type=int, default=0, help='How many pixels to add in height.')
    args.add_argument('-ax', '--left', type=int, default=0, help='How many pixels to add in width.')
    args.add_argument('-ay', '--top', type=int, default=0, help='How many pixels to add in height.')


    return args.parse_args()

def main(args):
    if not os.path.exists(args.filepath):
        print("Spritesheet path is invalid.")
        exit(1)
    img = cv2.imread(args.filepath, cv2.IMREAD_UNCHANGED)

    res = np.zeros((img.shape[0]+args.height+args.top,img.shape[1]+args.width+args.left,img.shape[-1]), dtype=img.dtype)
    for i in range(img.shape[-1]):
        res[...,i] = fill_clr[i]
    res[args.top:args.top+img.shape[0],args.left:args.left+img.shape[1]] = img
    print(img.shape)

    cv2.imwrite(args.outpath, res)
    print("Done.")

if __name__ == "__main__":
    main(parse_args())
