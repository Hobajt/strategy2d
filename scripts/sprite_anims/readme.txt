How to create sprite animation annotations:
    - annotation descriptions:
        - size - size of a single frame in pixels (all frames have the same size)
        - offset - initial-offset from the top-left corner of the image (defines where [0,0] is in pts)
        - frames.line_length, frames.line_count
            - groups together multiple sprites (under one sprite object)
            - for animations, this only works for different orientations of the same graphics
        - frames.offset - step-offset between individual frames (applies when switching orientations)
        - pts:
            - actual frames definitions
            - every entry here means one frame
            - values are offset from the initial offset
        
        - so in general
            - length of "pts" defines how many frames does the animation have
            - line_length or line_count defines how many orientations does the animation have (which one depends on if it's stored vertically or horizontally)

