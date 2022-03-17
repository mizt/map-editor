#include <metal_stdlib>
using namespace metal;

constexpr sampler linear(filter::linear,coord::pixel,address::clamp_to_edge);

kernel void processimage(
    texture2d<float,access::sample> src[[texture(0)]],
    texture2d<float,access::write> dst[[texture(1)]],
    constant float2 &resolution[[buffer(0)]],
    constant float2 &cursor[[buffer(1)]],
    constant float2 &prevCursor[[buffer(2)]],
    uint2 gid[[thread_position_in_grid]]) {
    
    float2 uv = (float2(gid)+float2(0.5,0.5))/resolution;
    
    float2 aspect = float2(1.0,resolution[1]/resolution[0]);
    
    float radius=0.55;
    
    float2 cursorOffset = prevCursor - cursor;
    
    float mask = smoothstep(radius,0.0,distance((uv*2.0-1.0)*aspect,((cursor/float2(resolution))*2.0-1.0)*aspect));

    float2 dir = cursorOffset/float2(resolution)*mask*0.5;
    float2 uv2 = uv - dir;
    
    float2 rg = src.sample(linear,uv2*float2(resolution)).rg;
        
    dst.write(float4(rg,0.0,0.0),gid);
        
}
