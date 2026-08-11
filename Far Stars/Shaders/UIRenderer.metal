//
//  UIRenderer.metal
//  1st game
//
//  Created by Luke Hickey on 9/14/24.
//

#include <metal_stdlib>
using namespace metal;

#import "MainShaderTypes.h"
#import "UIShaderTypes.h"
//#import "Fonts/Fonts.h"

typedef struct
{
    float4 position   [[position]];
    float2 texCoord;
} ColorInOut;

typedef struct
{
    float2 position;
    float2 texCoord;
} Vertex2d;


vertex ColorInOut flatVertexShader
(
    const device GUIVertex *vertices [[buffer(0)]],
    const device GUIVertexUniforms *vUniforms [[buffer(1)]],
    const device GUITextureUniforms *tUniforms [[buffer(3)]],
    const device float *aspectRatio [[buffer(2)]],
    const device float *scrollOffest [[buffer(4)]],
    uint vertexID [[vertex_id]],
    uint instanceID [[instance_id]]
)
{
    // Declare commenly used variables to optimise memory access
    GUIVertex currentVertex = vertices[instanceID];
    float2 vertexUniform = vUniforms[currentVertex.uniformVertexIndex].sizes;
    GUITextureUniforms textureUniform = tUniforms[currentVertex.uniformTextureIndex];
    float aspect = (*aspectRatio);
    
    // The positions of each vertex with a clockwise square:
    //  -1,  1
    //   1,  1
    //  -1, -1
    //   1, -1
    
    // vertexID & 1      1 if the vertexID is odd, which sets x to the larger x value
    // vertexID >> 1     1 if the vertexID is more than 1, which sets the y to the larger y value
    
    // Location of the individual vertex based off of winding
    float4 pos = float4(currentVertex.position + float2((vertexID & 1) * vertexUniform.x, (vertexID >> 1) * vertexUniform.y), 1.0, 1.0);
    
    pos.y += *scrollOffest;
    
    // Adjust the positions based off of aspect ratio (1.6 is the screen aspect ratio)
    pos.x *= ((aspect < AspectRatio) ? 1/AspectRatio : (1/aspect));
    pos.y *= ((aspect > AspectRatio) ? 1 : aspect/AspectRatio);
    
    // Return the final variable with calculated positions and texture coords
    return (ColorInOut) {
        pos, // Vertex location
        textureUniform.texCoords + float2((vertexID & 1) * textureUniform.texSizes.x, (vertexID >> 1) * textureUniform.texSizes.y) // Texture coordinate location
        // upside down
    };
}


fragment half4 colorMapFragmentShader
(
    ColorInOut      in                 [[stage_in]],
    texture2d<half> colorMap           [[texture(0)]],
    half4           forwardOpaqueColor [[color(0), raster_order_group(0)]]
 )
{
    constexpr sampler colorSampler(mip_filter::none);

    half4 colorSample = half4(colorMap.sample(colorSampler, (in.texCoord.xy)));
    colorSample.xyz = colorSample.xyz + (1.0h - colorSample.w) * forwardOpaqueColor.xyz;

    return colorSample;
}


vertex float4 simpleVert
(
    const device float2 *vertices [[buffer(0)]],
    const device float *aspectRatio [[buffer(1)]],
    uint vertexID [[vertex_id]],
    uint instanceID [[instance_id]]
)
{
    float aspect = (*aspectRatio);
    
    // Location of the individual vertex based off of winding
    float4 pos = float4(vertices[instanceID*4+vertexID], 1.0, 1.0);
    
    // Adjust the positions based off of aspect ratio (1.6 is the screen aspect ratio)
    pos.x *= ((aspect < AspectRatio) ? 1/AspectRatio : (1/aspect));
    pos.y *= ((aspect > AspectRatio) ? 1 : aspect/AspectRatio);
    
    // Return the final variable with calculated positions and texture coords
    return pos;
}

fragment half4 invertFragment
(
    half4           forwardOpaqueColor [[color(0)]]
 )
{
    return half4((1-forwardOpaqueColor.xyz),1.0);
}



typedef struct
{
    float4 position [[position]];
    float2 texCoord;
    half3 color;
} InOut;


vertex InOut textVertex
(
    const device uint8_t *str [[buffer(0)]],
    const device vector_float2 *letterTexCoords [[buffer(1)]],
    const device vector_float2 *letterSpacing [[buffer(2)]],
    const device float *letterSizes [[buffer(3)]],
    const device truncatedTextPerameters *fontSizesBuffer [[buffer(4)]],
    const device vector_float2 *loadedTextSize [[buffer(5)]],
    const device float *aspectRatio [[buffer(6)]],
    const device int *numTextParams [[buffer(7)]],
    const device float *scrollOffset [[buffer(8)]],
    uint vertexID [[vertex_id]],
    uint instanceID [[instance_id]]
    
)
{
    
    
    
    // Aspect ratios
    float aspect = *aspectRatio;
    
    uint8_t ltr = str[instanceID];
    float2 coords = letterSpacing[instanceID];
    
    int numParams = *numTextParams;
    
    half3 color = half3(0);
    float fontSize = 0.0;
    {
        int i = 0;
        for(; i<numParams; ++i){
            color = (fontSizesBuffer[i].startsAt<instanceID) ? half3(fontSizesBuffer[i].color) : color;
            fontSize = (fontSizesBuffer[i].startsAt<instanceID) ? fontSizesBuffer[i].fontSize : fontSize;
        }
    }
    
    // Location of the individual vertex based off of winding
    float4 pos = float4(0.0, 0.0, 1.0, 1.0);
    
    //= { float4(coords + float2((vertexID & 1) * abs(dividing[ltr].letterSize) * fontSize, (vertexID >> 1) * fontSize), 1.0, 1.0)} ;
    pos.x = coords.x + (vertexID & 1) * abs(letterSizes[ltr]) * fontSize;
    pos.y = coords.y + (vertexID >> 1) * fontSize;
    
    pos.y += *scrollOffset;
    
    // Adjust the positions based off of aspect ratio (1.6 is the screen aspect ratio)
    pos.x *= ((aspect < AspectRatio) ? 1/AspectRatio : (1/aspect));
    pos.y *= ((aspect > AspectRatio) ? 1 : aspect/AspectRatio);
    
    float2 texCoords = letterTexCoords[ltr].xy;
    texCoords.x += (vertexID & 1) ? (loadedTextSize->x) * abs(letterSizes[ltr]) : 0;
    texCoords.y += (vertexID >> 1) ? 0 : (loadedTextSize->y);
    
    
    // Return the final variable with calculated positions
    return (InOut) {
        pos, // Vertex location
        texCoords, // Texture coordinate location
        color
    };
}

vertex InOut cursorVertex
(
    const device vector_float2 *letterTexCoords [[buffer(0)]],
    const device vector_float2 *letterSpacing [[buffer(1)]],
    const device float *letterSizes [[buffer(2)]],
    const device truncatedTextPerameters *fontSizesBuffer [[buffer(3)]],
    const device vector_float2 *loadedTextSize [[buffer(4)]],
    const device float *originY [[buffer(5)]],
    const device float *aspectRatio [[buffer(6)]],
    uint vertexID [[vertex_id]]
    
)
{
    
    // Aspect ratios
    float aspect = *aspectRatio;

    float2 coords = letterSpacing[0];
    
    half3 color = half3(fontSizesBuffer[0].color);
    float fontSize = fontSizesBuffer[0].fontSize;
    
    coords.y = int((coords.y-(*originY)) / (fontSize*NewlineDist)) * (fontSize*NewlineDist) + (*originY);
    
    // Location of the individual vertex based off of winding
    float4 pos = float4(0.0, 0.0, 1.0, 1.0);
    
    //= { float4(coords + float2((vertexID & 1) * abs(dividing[ltr].letterSize) * fontSize, (vertexID >> 1) * fontSize), 1.0, 1.0)} ;
    pos.x = coords.x + (vertexID & 1) * abs(letterSizes[0]) * (fontSize*0.8) - 0.02 * fontSize;
    pos.y = coords.y + (vertexID >> 1) * fontSize * NewlineDist - !(vertexID >> 1) * fontSize * 0.2;
    
    // Adjust the positions based off of aspect ratio (1.6 is the screen aspect ratio)
    pos.x *= ((aspect < AspectRatio) ? 1/AspectRatio : (1/aspect));
    pos.y *= ((aspect > AspectRatio) ? 1 : aspect/AspectRatio);
    
    float2 texCoords = letterTexCoords[0].xy;
    texCoords.x += (vertexID & 1) ? (loadedTextSize->x) * abs(letterSizes[0]) : 0;
    texCoords.y += (vertexID >> 1) ? 0 : (loadedTextSize->y);
    
    
    // Return the final variable with calculated positions
    return (InOut) {
        pos, // Vertex location
        texCoords, // Texture coordinate location
        color
    };
}

fragment half4 cursorFragment
(
    InOut                     in                 [[stage_in]],
    texture2d<half> fontCatalogue [[texture(0)]],
    const device float *time [[buffer(0)]],
    half4                     forwardOpaqueColor [[color(0)]]
    
 )
{
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    
    float cycle = 2.3*sin(*time) + 0.4;
    //float cycle = 1.0;
    //return(half4(cycle));
    
    half letterTransparency = clamp(fontCatalogue.sample(textureSampler, in.texCoord.xy, level(10.0)).r * cycle,0.0,1.0);
    
    if (letterTransparency == 0) discard_fragment();
    
    return half4(mix(in.color, forwardOpaqueColor.rgb, half3(1.0-letterTransparency)), 1.0);
}




fragment half4 colorTextFragment
(
    InOut                     in                 [[stage_in]],
    texture2d<half> fontCatalogue [[texture(0)]],
    half4                     forwardOpaqueColor [[color(0)]]
 )
{
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    
    half letterTransparency = fontCatalogue.sample(textureSampler, in.texCoord.xy, level(10.0)).r;
    
    if (letterTransparency == 0) discard_fragment();

    return half4(mix(in.color, forwardOpaqueColor.rgb, half3(1.0-letterTransparency)), 1.0);
}





kernel void colorTextKernel(
    const device bezierCurve *curves             [[buffer(0)]],
    const device int *dividing [[buffer(1)]],
    const device uint8_t *ratioOfSpacing             [[buffer(2)]],
    const device float *regSpacing             [[buffer(3)]],
    const device float *jumps             [[buffer(4)]],
    const device characterAtlasParams *atlasParams       [[buffer(5)]],
    texture2d<half, access::write> fontCatalogue [[texture(0)]],
    uint2 gid [[thread_position_in_grid]]
 )
{
    int hightPerLetter = atlasParams->heightPerLetter;
    int widthOfPadding = atlasParams->widthOfPadding;
    
    int row = int(gid.y/(hightPerLetter + widthOfPadding));
    
    
    
    int jump = jumps[min(row,TextNumbersLines-1)] - 1;
    
    //if(gid.y/height->height-int(gid.y/height->height)<0.005) jump = max(jump-1, 0);
    
    //uint8_t ltr = ((gid.x+jump)>height->screenWidth ||  ? 0 : ratioOfSpacing[gid.x+jump];
    uint8_t ltr = ratioOfSpacing[gid.x+jump];

    
    ltr = min(max(ltr, (uint8_t)0), (uint8_t)98);
    
    float width = (ltr > 0) ? regSpacing[ltr]-regSpacing[ltr-1]-widthOfPadding : 0;

    float posInWidth = 1-float(regSpacing[ltr]-widthOfPadding-(gid.x+jump))/width;
    float posInHeight = 1-float(int(gid.y%(hightPerLetter + widthOfPadding)))/float(hightPerLetter);
    
    float2 texCoord = float2(posInWidth, posInHeight);
    
    if(texCoord.x > 1 || texCoord.y > 1){
        ltr = 0;
    }
    
    
    int curvesStart = dividing[max(ltr-1, 0)];
    int numCurves = dividing[ltr];
    
    
    
    const float epsilon = 1e-5; // A variable to account for floating point impersition
    float2 closestPoint = float2(2); // A default value for a closest point
    int numCurvesPassed = 0; // The number of curves that the line is to the left of
    
    // Loop over each curve, and see if the pixel is to the left of it. If this happens for an odd amount of lines, it is inside a shape. If this happens an even number of times, it is outside a shape.
    for (int i = curvesStart; i<min(numCurves,1000); ++i) {
        
        // Create a local curve variable
        bezierCurve curve = curves[i];
        
        // Settle any disputes if a pixel is on the same Y value of a straight line
        if(abs(curve.coords[1] - texCoord.y) < epsilon) texCoord.y -= epsilon;
        if(abs(curve.coords[5] - texCoord.y) < epsilon) texCoord.y -= epsilon;
        
        // Compute quadratic coefficients for y(t)
        float a = (curve.coords[5] - 2.0 * curve.coords[3] + curve.coords[1]) * 2.0;
        float b = -2.0 * (curve.coords[3] - curve.coords[1]);
        
        // If the closest position and the current position are close enough to the pixel that a graphical error would occur, skip the step
        if (a!=b && abs(closestPoint.y - texCoord.y) < epsilon && abs(curve.coords[1] - texCoord.y) < epsilon)
            continue;
        
        // Set the closest point to the current curve postion if it is the closest position
        if (curve.coords[1]<closestPoint.y && 2*texCoord.y>curve.coords[1]+closestPoint.y && texCoord.x<curve.coords[0])
            closestPoint = float2(curve.coords[0],curve.coords[1]);
        
        // Solve for t using the quadratic formula
        float discriminant = b * b - 2.0 * a * (curve.coords[1] - texCoord.y);
        
        // Ensure discriminant is non-negative (clamp to avoid NaNs)
        if (discriminant < -epsilon) continue; // Invalid case (no intersection)
        
        discriminant = sqrt(discriminant);
        
        // Two potential solutions for t
        float t1 = (b + discriminant) / a;
        float t2 = (b - discriminant) / a;
        
        // Pre-calculate two potential 1 - t constants
        float one_minus_t1 = 1.0 - t1;
        float one_minus_t2 = 1.0 - t2;
        
        // Solve for x(t1) & add a curve if the solution is in the positive X direction
        //numCurvesPassed += 1;
        //numCurvesPassed += ((1 * 1 * 1 + 2.0 * 1 * 1 * 1 + 1 * 1 * 1) <= 0.5) ? 0.0 : 1 >= 0 && 1 <= 1.0;
        numCurvesPassed += (one_minus_t1 * one_minus_t1 * curve.coords[0] + 2.0 * one_minus_t1 * t1 * curve.coords[2] + t1 * t1 * curve.coords[4] <= texCoord.x) ? 0 : int(t1 >= 0 && t1 <= 1.0);
        
        // Solve for x(t2) & add a curve if the solution is in the positive X direction
        numCurvesPassed += (one_minus_t2 * one_minus_t2 * curve.coords[0] + 2.0 * one_minus_t2 * t2 * curve.coords[2] + t2 * t2 * curve.coords[4] <= texCoord.x) ? 0 : int(t2>=0 && t2<=1.0);
        
    }
    
    //write
    fontCatalogue.write(half(numCurvesPassed % 2), gid);
}




