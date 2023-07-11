import numpy as np
import cv2
import argparse, os


bg_clr = [141,103,83,0]

replace_clr = [0,0,0,0]

def parse_args():
    args = argparse.ArgumentParser()
    args.add_argument('filepath', type=str, help='Path to the input spritesheet.')
    args.add_argument('-o', '--outpath', type=str, default='output.png', help='Path where to write the result.')
    return args.parse_args()

def main(args):
    if not os.path.exists(args.filepath):
        print("Spritesheet path is invalid.")
        exit(1)
    img = cv2.imread(args.filepath, cv2.IMREAD_UNCHANGED)

    img[np.all(img == replace_clr, axis=-1)] = bg_clr

    cv2.imwrite(args.outpath, img)

if __name__ == "__main__":
    main(parse_args())

