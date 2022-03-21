#include <metal_stdlib>
using namespace metal;

struct VertInOut {
    float4 pos[[position]];
    float2 texcoord[[user(texturecoord)]];
};

struct FragmentShaderArguments {
    texture2d<float> texture[[id(0)]];
};

vertex VertInOut vertexShader(constant float4 *pos[[buffer(0)]],constant packed_float2  *texcoord[[buffer(1)]],uint vid[[vertex_id]]) {
    VertInOut outVert;
    outVert.pos = pos[vid];
    outVert.texcoord = float2(texcoord[vid][0],1.0-texcoord[vid][1]);
    return outVert;
}

fragment float4 fragmentShader(VertInOut inFrag[[stage_in]],constant FragmentShaderArguments &args[[buffer(0)]]) {
    constexpr sampler sampler(address::clamp_to_edge,filter::linear);
    constexpr float f = 1.0/255.0;
    
    float2 xy = args.texture.sample(sampler,inFrag.texcoord).rg;
    
    unsigned short x = (unsigned short)(65535.0*xy.x);
    unsigned short y = (unsigned short)(65535.0*xy.y);
    
    return float4(
        (y&0xFF)*f,
        ((x>>8)&0xFF)*f,
        (x&0xFF)*f,
        ((y>>8)&0xFF)*f
    );
}
