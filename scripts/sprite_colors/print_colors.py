import numpy as np
import cv2
import argparse, os

'''Print unique colors in an image.'''

def parse_args():
    args = argparse.ArgumentParser()
    args.add_argument('filepath', type=str, help='Path to the input spritesheet.')
    return args.parse_args()

def main(args):
    if not os.path.exists(args.filepath):
        print("Spritesheet path is invalid.")
        exit(1)
    img = cv2.imread(args.filepath, cv2.IMREAD_UNCHANGED)
    img = img.reshape((-1, img.shape[-1]))
    
    clrs = np.unique(img, axis=0)
    print(clrs)
    print("color count:", clrs.shape[0])
    

if __name__ == "__main__":
    main(parse_args())

