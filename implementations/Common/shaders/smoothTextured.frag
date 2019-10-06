#version 450

#include "fog.glsl"

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec4 inTexcoord;
layout(location = 2) in vec4 inPosition;

layout(location = 0) out vec4 outColor;

layout(set=0, binding=0) uniform sampler Sampler0;
layout(set=5, binding=0) uniform texture2D Texture0;

void main()
{
    outColor = applyFog(inColor*textureProj(sampler2D(Texture0, Sampler0), inTexcoord), inPosition.xyz);
}
