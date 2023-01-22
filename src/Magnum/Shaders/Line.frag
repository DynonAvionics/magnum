/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022 Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#if defined(OBJECT_ID) && !defined(GL_ES) && !defined(NEW_GLSL)
#extension GL_EXT_gpu_shader4: require
#endif

#ifndef NEW_GLSL
#define fragmentColor gl_FragColor
#define in varying
#endif

#ifndef RUNTIME_CONST
#define const
#endif

/* Uniforms */

#ifdef EXPLICIT_UNIFORM_LOCATION
layout(location = 2)
#endif
uniform mediump float width
    #ifndef GL_ES
    = 1.0
    #endif
    ;

#ifdef EXPLICIT_UNIFORM_LOCATION
layout(location = 3)
#endif
uniform mediump float smoothness
    #ifndef GL_ES
    = 0.0
    #endif
    ;

#ifdef EXPLICIT_UNIFORM_LOCATION
layout(location = 5)
#endif
uniform lowp vec4 backgroundColor
    #ifndef GL_ES
    = vec4(0.0)
    #endif
    ;

#ifndef UNIFORM_BUFFERS
#ifdef EXPLICIT_UNIFORM_LOCATION
layout(location = 6)
#endif
uniform lowp vec4 color
    #ifndef GL_ES
    = vec4(1.0)
    #endif
    ;

#ifdef OBJECT_ID
#ifdef EXPLICIT_UNIFORM_LOCATION
layout(location = 7)
#endif
/* mediump is just 2^10, which might not be enough, this is 2^16 */
uniform highp uint objectId; /* defaults to zero */
#endif

/* Uniform buffers */

#else
#ifndef MULTI_DRAW
#if DRAW_COUNT > 1
#ifdef EXPLICIT_UNIFORM_LOCATION
layout(location = 1)
#endif
uniform highp uint drawOffset
    #ifndef GL_ES
    = 0u
    #endif
    ;
#else
#define drawOffset 0u
#endif
#define drawId drawOffset
#endif

struct DrawUniform {
    highp uvec4 materialIdReservedObjectIdReservedReserved;
    #define draw_materialIdReserved materialIdReservedObjectIdReservedReserved.x
    #define draw_objectId materialIdReservedObjectIdReservedReserved.y
};

layout(std140
    #ifdef EXPLICIT_BINDING
    , binding = 2
    #endif
) uniform Draw {
    DrawUniform draws[DRAW_COUNT];
};

struct MaterialUniform {
    lowp vec4 color;
};

layout(std140
    #ifdef EXPLICIT_BINDING
    , binding = 4
    #endif
) uniform Material {
    MaterialUniform materials[MATERIAL_COUNT];
};
#endif

/* Inputs */

in highp vec2 centerDistanceSigned;
in highp float halfSegmentLength;
in lowp float hasCap;

#ifdef VERTEX_COLOR
in lowp vec4 interpolatedVertexColor;
#endif

#ifdef INSTANCED_OBJECT_ID
flat in highp uint interpolatedInstanceObjectId;
#endif

/* Outputs */

#ifdef NEW_GLSL
#ifdef EXPLICIT_ATTRIB_LOCATION
layout(location = COLOR_OUTPUT_ATTRIBUTE_LOCATION)
#endif
out lowp vec4 fragmentColor;
#endif
#ifdef OBJECT_ID
#ifdef EXPLICIT_ATTRIB_LOCATION
layout(location = OBJECT_ID_OUTPUT_ATTRIBUTE_LOCATION)
#endif
/* mediump is just 2^10, which might not be enough, this is 2^16 */
out highp uint fragmentObjectId;
#endif

#ifdef MULTI_DRAW
flat in highp uint drawId;
#endif

void main() {
    #ifdef UNIFORM_BUFFERS
    #ifdef OBJECT_ID
    highp const uint objectId = draws[drawId].draw_objectId;
    #endif
    #if MATERIAL_COUNT > 1
    mediump const uint materialId = draws[drawId].draw_materialIdReserved & 0xffffu;
    #else
    #define materialId 0u
    #endif
    lowp const vec4 color = materials[materialId].color;
    #endif

    // TODO better names ffs
    highp vec2 distance_ = vec2(max(abs(centerDistanceSigned.x)
        #ifdef CAP_STYLE_BUTT
        + width*0.5  // TODO wut, clean up
        #endif
        - halfSegmentLength, 0.0), abs(centerDistanceSigned.y));
    // TODO document what is this
    if(hasCap < 0.0) distance_.x = 0.0;

    #if defined(CAP_STYLE_BUTT) || defined(CAP_STYLE_SQUARE)
    highp const float distanceS = max(distance_.x, distance_.y);
    #elif defined(CAP_STYLE_ROUND)
    highp const float distanceS = length(distance_);
    #elif defined(CAP_STYLE_TRIANGLE)
    highp const float distanceS = distance_.x + distance_.y;
    #else
    #error
    #endif

    // TODO clean this up so it doesn't include width (which isn't present for butts)
    const highp float factor = smoothstep(width*0.5 - smoothness, width*0.5 + smoothness, distanceS);

//     fragmentColor = vec4(centerDistanceSigned*0.5/vec2(halfSegmentLength + width*0.5, width*0.5) + vec2(0.5), 0.0, 1.0);
//     return;

    fragmentColor = mix(
        #ifdef VERTEX_COLOR
        interpolatedVertexColor*
        #endif
        color, backgroundColor, factor);

    #ifdef OBJECT_ID
    fragmentObjectId = objectId;
    #endif
}
