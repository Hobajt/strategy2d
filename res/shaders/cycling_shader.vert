#version 450 core

layout(location = 0) in vec3  aPosition;
layout(location = 1) in vec4  aColor;
layout(location = 2) in vec2  aTexCoords;
layout(location = 3) in uint  aTextureID;
layout(location = 4) in float aAlphaFromTexture;
layout(location = 5) in float aPaletteIdx;
layout(location = 6) in uvec4 aObjectInfo;

out vec4 color;
out vec2 texCoords;
out flat uint textureID;
out flat float alphaFromTexture;
out flat float paletteIdx;
out flat uvec4 objectInfo;

out vec3 fragPos;

void main() {
    gl_Position       = vec4(aPosition, 1.0);
    color             = aColor;
    texCoords         = aTexCoords;
    textureID         = aTextureID;
    alphaFromTexture  = aAlphaFromTexture;
    paletteIdx        = aPaletteIdx;
    objectInfo        = aObjectInfo;

    fragPos           = aPosition;
}
