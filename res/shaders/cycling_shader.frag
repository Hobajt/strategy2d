#version 450 core

layout(location = 0) out vec4 FragColor;
layout(location = 1) out uvec4 ObjectInfo;

in vec4 color;
in vec2 texCoords;
in flat uint textureID;
in flat float alphaFromTexture;
in flat float paletteIdx;
in flat uvec4 objectInfo;

#define MAX_TEXTURES 8
uniform sampler2D textures[MAX_TEXTURES];

in vec3 fragPos;

uniform sampler2D colorPalette;
uniform vec2 centeringOffset = vec2(0,0);

void main() {
    uint tID = textureID;
    vec4 tColor = vec4(1.0);
    switch(tID) {
        case 0: tColor = texture(textures[0], texCoords); break;
        case 1: tColor = texture(textures[1], texCoords); break;
        case 2: tColor = texture(textures[2], texCoords); break;
        case 3: tColor = texture(textures[3], texCoords); break;
        case 4: tColor = texture(textures[4], texCoords); break;
        case 5: tColor = texture(textures[5], texCoords); break;
        case 6: tColor = texture(textures[6], texCoords); break;
        case 7: tColor = texture(textures[7], texCoords); break;
    }

    //added condition cuz blending apparently doesn't work when depth testing is enabled
    if(tColor.a < 0.8 || (alphaFromTexture > 0 && tColor.r < 0.05))
        discard;
    
    //not sure if this is branchless, gonna have to look it up
    float useCycling = float(paletteIdx >= 0.0 && tColor.a == 0.8);
    vec2 idx = centeringOffset + vec2((tColor.r-0.2)*5/4, paletteIdx);
    vec4 cColor = texture(colorPalette, idx);

    vec4 out_color;
    out_color = (1 - alphaFromTexture) * (color * tColor) + alphaFromTexture * color * vec4(1.0, 1.0, 1.0, tColor.r);
    out_color = (1 - useCycling) * out_color + useCycling * cColor;

    ObjectInfo = objectInfo;
    FragColor = out_color;
}
