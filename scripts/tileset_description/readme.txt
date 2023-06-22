python scripts that simplify generating descriptions (JSONs) for tileset spritesheets
scripts are interactive & require numpy & python-opencv to run

1) run generate.py
    - generates initial JSON file with preset values

2) run edit_types.py
    - interactive script for setting up tile types
        - mouse clicks (LMB,RMB) to select tiles in the tilemap
        - '+'/'-' - to change tileID
        - space to apply tileID to selected tiles
        - 'x' to unset tileIDs from selected tiles
        - 'w' to write changes to file
    - script should populate 'tile_types' entry in the JSON

3) run edit_borders.py
    - interactive script for corner types labeling
    - select tiles in the tilemap (LMB) for editing
    - click on corners in the editor window
        - mark corner white if it's corner type is different than the tile type
            - ie. mud + grass transition -> mark grass corners with white (bcs it's a mud tile in the end)
        - 'r' to save corner setup for this tile
        - 't' to erase (sets to 0)
        - 'i' to invert & save current tile
        - 'f' to toggle corners overlay (to see the entire tile)
    - 'g' to toggle tilemap view (shows what tiles have non-zero corner type assigned)
    - 'd' to toggle cloning mode
        - when enabled, clicking tiles will copy current tile's corner setup
    - 'h' to toggle 'wall view' - editor will work with borders instead of corners
    - 'w' to write changes into the file

4) borders_check.py
    - for border type validation
    - should display all tiles of given tile type sorted by assigned border types
        - 'a'/'d' to move between tile types
99) move JSON to it's place override "texture_filepath"
