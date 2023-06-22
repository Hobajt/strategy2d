import argparse
import json

def parse_args():
    args = argparse.ArgumentParser()

    args.add_argument('--json', type=str, required=True, help="Path to a output JSON file (where should the results be stored).")
    args.add_argument('--texture', type=str, required=True, help='Path to the tileset texture.')

    args.add_argument('-tw', type=int, default=32, help='Tile width (in pixels)')
    args.add_argument('-th', type=int, default=32, help='Tile height (in pixels)')
    args.add_argument('--offset', type=int, default=1, help='Empty space between individual tiles (in pixels)')

    args.add_argument('-r', type=int, required=True, help='Number of rows in the tilemap spritesheet.')
    args.add_argument('-c', type=int, required=True, help='Number of cols in the tilemap spritesheet.')

    return args.parse_args()

def main(args):
    res = {}

    res['texture_filepath'] = args.texture
    res['tile_count'] = [args.c, args.r]
    res['tile_size'] = [args.tw, args.th]
    res['offset'] = args.offset

    res = json.dumps(res, indent=4)
    with open(args.json, 'w') as f:
        f.write(res)
    
    print(f"Descriptions written to '{args.json}'...")

if __name__ == "__main__":
    main(parse_args())