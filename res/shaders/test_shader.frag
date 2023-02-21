#version 450 core

layout(location = 0) out vec4 FragColor;
layout(location = 1) out uvec4 ObjectInfo;

in vec4 color;
in vec2 texCoords;
in flat uint textureID;
in flat float alphaFromTexture;
in flat uvec4 objectInfo;

#define MAX_TEXTURES 8
uniform sampler2D textures[MAX_TEXTURES];

in vec3 fragPos;

uniform mat3 M;
uniform float zScale;
uniform float zoom_log;

//remove later on
uniform int PCF_samples = 10;
uniform bool PCF;

uniform bool shadows = true;
uniform int shadowmapIdx = 7;

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
    if(tColor.a < 0.75 || (alphaFromTexture > 0 && tColor.r < 0.05))
        discard;

    ObjectInfo = objectInfo;
    FragColor = (1 - alphaFromTexture) * (color * tColor) + alphaFromTexture * color * vec4(1.0, 1.0, 1.0, tColor.r);
    // FragColor = vec4(vec3(tColor.r > 0.25), tColor.r);

    if(shadows) {
        // vec3 fragPos_ls = M * fragPos;
        vec3 fragPos_ls = M * fragPos;
        vec3 projCoords = fragPos_ls.xyz;
        projCoords      = projCoords * 0.5 + 0.5;

        float depthClosest = texture(textures[shadowmapIdx], projCoords.xy).r * 2.0 - 1.0;
        float depthCurrent = projCoords.z * 2.0 - 1.0;

        //adaptively compute bias based on current zoom
        float bias = max(0.12, (0.1 + (0.5 * -2.0*(zoom_log+0.8)))) * zScale;
        float shadow = 0.0;

        if(!PCF) {
            float v = abs(depthCurrent - bias - depthClosest)/zScale;
            v = min(v/8, 1.0);
            // v = 0.2;
            shadow = (depthCurrent - bias) > depthClosest ? v : 1.0;
        }
        else {
            vec2 texelSize = 1.0 / textureSize(textures[shadowmapIdx], 0);
            for(int y = -PCF_samples; y <= PCF_samples; ++y) {
                for(int x = -PCF_samples; x <= PCF_samples; ++x) {
                    vec2 coords = projCoords.xy + vec2(x,y) * texelSize;
                    float depth = texture(textures[shadowmapIdx], coords).r * 2.0 - 1.0;
                    shadow += ((depthCurrent - bias) > depth) ? 0.4 : 1.0;
                }
            }
            shadow = shadow / ((2*PCF_samples+1)*(2*PCF_samples+1));
        }

        //makes shadows slightly lighter with increasing distance from the occluding point
        shadow = min(shadow + ((abs(depthCurrent - bias - depthClosest)/zScale)/80), 1.0);
        FragColor = vec4(vec3(FragColor.rgb) * shadow, FragColor.a);
    }
}
