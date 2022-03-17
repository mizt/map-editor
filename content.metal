#import <metal_stdlib>
#import "Types.h"

using namespace metal;

struct VertInOut {
    float4 pos[[position]];
    float2 texcoord[[user(texturecoord)]];
};

struct FragmentShaderArguments {
    texture2d<float> texture[[id(0)]];
    texture2d<float> map[[id(1)]];
    device unsigned int *type[[id(2)]];
    device float2 *resolution[[id(3)]];
};

vertex VertInOut vertexShader(constant float4 *pos[[buffer(0)]], constant packed_float2 *texcoord[[buffer(1)]], uint vid[[vertex_id]]) {
    VertInOut outVert;
    outVert.pos = pos[vid];
    outVert.texcoord = float2(texcoord[vid][0],1.0-texcoord[vid][1]);
    return outVert;
}

fragment float4 fragmentShader(VertInOut inFrag[[stage_in]], constant FragmentShaderArguments &args[[buffer(0)]]) {
    constexpr sampler sampler(address::clamp_to_edge, filter::linear);
    
    if(args.type[0]==Type::RGB) {
        
        float4 map = args.map.sample(sampler,inFrag.texcoord);
        
        float x = ((((int(map.a*255.0))<<8|(int(map.b*255.0)))-32767.0)*0.25)/(args.resolution[0].x-1.0);
        float y = ((((int(map.g*255.0))<<8|(int(map.r*255.0)))-32767.0)*0.25)/(args.resolution[0].y-1.0);
        
        return float4(args.texture.sample(sampler,float2(x,y)));
    }
    else {
        return float4(args.map.sample(sampler,inFrag.texcoord));
    }
    
}
