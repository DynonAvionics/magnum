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

#if defined(INSTANCED_OBJECT_ID) && !defined(GL_ES) && !defined(NEW_GLSL)
#extension GL_EXT_gpu_shader4: require
#endif

#if defined(UNIFORM_BUFFERS) && defined(TEXTURE_ARRAYS) && !defined(GL_ES)
#extension GL_ARB_shader_bit_encoding: require
#endif

#ifdef MULTI_DRAW
#ifndef GL_ES
#extension GL_ARB_shader_draw_parameters: require
#else /* covers WebGL as well */
#extension GL_ANGLE_multi_draw: require
#endif
#endif

#ifndef NEW_GLSL
#define in attribute
#define out varying
#endif

#ifndef RUNTIME_CONST
#define const
#endif

/* Uniforms */

/* This one is for both classic and UBOs, as it's usually set globally instead
   of changing per-draw */
#ifdef EXPLICIT_UNIFORM_LOCATION
layout(location = 0)
#endif
uniform lowp vec2 viewportSize; /* defaults to zero */

#ifndef UNIFORM_BUFFERS
#ifdef EXPLICIT_UNIFORM_LOCATION
layout(location = 1)
#endif
#ifdef TWO_DIMENSIONS
uniform highp mat3 transformationProjectionMatrix
    #ifndef GL_ES
    = mat3(1.0)
    #endif
    ;
#elif defined(THREE_DIMENSIONS)
uniform highp mat4 transformationProjectionMatrix
    #ifndef GL_ES
    = mat4(1.0)
    #endif
    ;
#else
#error
#endif

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
layout(location = 4)
#endif
uniform mediump float miterLimit
    #ifndef GL_ES
    = 4.0
    #endif
    ;

/* Uniform buffers */

#else
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

layout(std140
    #ifdef EXPLICIT_BINDING
    , binding = 1
    #endif
) uniform TransformationProjection {
    highp
        #ifdef TWO_DIMENSIONS
        /* Can't be a mat3 because of ANGLE, see DrawUniform in Phong.vert for
           details */
        mat3x4
        #elif defined(THREE_DIMENSIONS)
        mat4
        #else
        #error
        #endif
    transformationProjectionMatrices[DRAW_COUNT];
};
#endif

/* Inputs */

/* The third component in 2D (or fourth in 3D) is a point marker. Its sign
   denotes the direction (above the line / below the line) in which it extends
   to form a quad, its absolute value is then a combination bits indicating the
   following:

    -   line begin, in which case the second point of the segment is stored in
        the nextPosition input, and the neighboring segment point (if any) is
        stored in previousPosition
    -   line end, in which case the second point of the segment is stored in
        the previousPosition input, and the neighboring segment point (if any)
        is stored in nextPosition
    -   line cap, in which case the neighboring segment point should be ignored
        and instead a line cap formed; and conversely if not set the
        neighboring segment points are meant to be used to form a line join

   The sign / extension direction could theoretically be inferred from
   `gl_VertexID & 1`, but doing so makes it very tied to the actual vertex order
   which may not be desirable. An alternative scenario considered was using
   NaNs in previousPosition / nextPosition to mark caps and then, if those
   aren't NaNs, it's a join, and the begin/end is implicit based on
   `gl_VertexID % 2`. But while that removed the need for a dedicated extra
   input, it made it impossible to alias the position attribute with
   previousPosition/nextPosition, leading to a much higher memory use. */
// TODO move the last paragraph elsewhere? "GET A BLOG GODDAMIT" etc
// TODO orientation of the "both" cap?
#define POINT_MARKER_BEGIN_MASK 1u
#define POINT_MARKER_END_MASK 2u // TODO use this for zero-length lines (points)
#define POINT_MARKER_CAP_MASK 4u
#ifdef EXPLICIT_ATTRIB_LOCATION
layout(location = LINE_POSITION_ATTRIBUTE_LOCATION)
#endif
#ifdef TWO_DIMENSIONS
in highp vec3 positionPointMarkerSign;
#define position positionPointMarkerSign.xy
#define pointMarkerSign positionPointMarkerSign.z
#elif defined(THREE_DIMENSIONS)
in highp vec4 positionPointMarkerSign;
#define position positionPointMarkerSign.xyz
#define pointMarkerSign positionPointMarkerSign.w
#else
#error
#endif

#ifdef EXPLICIT_ATTRIB_LOCATION
layout(location = LINE_PREVIOUS_POSITION_ATTRIBUTE_LOCATION)
#endif
#ifdef TWO_DIMENSIONS
in highp vec2 previousPosition;
#elif defined(THREE_DIMENSIONS)
in highp vec3 previousPosition;
#else
#error
#endif

#ifdef EXPLICIT_ATTRIB_LOCATION
layout(location = LINE_NEXT_POSITION_ATTRIBUTE_LOCATION)
#endif
#ifdef TWO_DIMENSIONS
in highp vec2 nextPosition;
#elif defined(THREE_DIMENSIONS)
in highp vec3 nextPosition;
#else
#error
#endif

#ifdef VERTEX_COLOR
#ifdef EXPLICIT_ATTRIB_LOCATION
layout(location = COLOR_ATTRIBUTE_LOCATION)
#endif
in lowp vec4 vertexColor;
#endif

#ifdef INSTANCED_OBJECT_ID
#ifdef EXPLICIT_ATTRIB_LOCATION
layout(location = OBJECT_ID_ATTRIBUTE_LOCATION)
#endif
in highp uint instanceObjectId;
#endif

#ifdef INSTANCED_TRANSFORMATION
#ifdef EXPLICIT_ATTRIB_LOCATION
layout(location = TRANSFORMATION_MATRIX_ATTRIBUTE_LOCATION)
#endif
#ifdef TWO_DIMENSIONS
in highp mat3 instancedTransformationMatrix;
#elif defined(THREE_DIMENSIONS)
in highp mat4 instancedTransformationMatrix;
#else
#error
#endif
#endif

/* Outputs */

// TODO document, maybe join together?
out highp vec2 centerDistanceSigned;
out highp float halfSegmentLength;
out lowp float hasCap;

#ifdef VERTEX_COLOR
out lowp vec4 interpolatedVertexColor;
#endif

#ifdef INSTANCED_OBJECT_ID
flat out highp uint interpolatedInstanceObjectId;
#endif

#ifdef MULTI_DRAW
flat out highp uint drawId;
#endif

/* Same as Math::Vector2::perpendicular() */
vec2 perpendicular(vec2 a) {
    return vec2(-a.y, a.x);
}

void main() {
    #ifdef UNIFORM_BUFFERS
    #ifdef MULTI_DRAW
    drawId = drawOffset + uint(
        #ifndef GL_ES
        gl_DrawIDARB /* Using GL_ARB_shader_draw_parameters, not GLSL 4.6 */
        #else
        gl_DrawID
        #endif
        );
    #else
    #define drawId drawOffset
    #endif

    #ifdef TWO_DIMENSIONS
    highp const mat3 transformationProjectionMatrix = mat3(transformationProjectionMatrices[drawId]);
    #elif defined(THREE_DIMENSIONS)
    highp const mat4 transformationProjectionMatrix = transformationProjectionMatrices[drawId];
    #else
    #error
    #endif
    #endif

    // TODO look at the precision qualifiers, same for *.frag
    #ifdef TWO_DIMENSIONS
    highp const vec2 transformedPosition = (transformationProjectionMatrix*
        #ifdef INSTANCED_TRANSFORMATION
        instancedTransformationMatrix*
        #endif
        vec3(position, 1.0)).xy;

    highp const vec2 transformedPreviousPosition = (transformationProjectionMatrix*
        #ifdef INSTANCED_TRANSFORMATION
        instancedTransformationMatrix*
        #endif
        vec3(previousPosition, 1.0)).xy;

    highp const vec2 transformedNextPosition = (transformationProjectionMatrix*
        #ifdef INSTANCED_TRANSFORMATION
        instancedTransformationMatrix*
        #endif
        vec3(nextPosition, 1.0)).xy;
    #elif defined(THREE_DIMENSIONS)
    // TODO
    #else
    #error
    #endif

    /* Decide about the line direction vector `d` and edge direction vector `e`
       from the `pointMarkerSign` input. Quad corners 0 and 1 come from segment
       endpoint A, are marked with the POINT_MARKER_BEGIN_MASK bit and so their
       line direction is taken from `nextPosition`, quad corners 2 and 3 come
       from B and are marked with POINT_MARKER_END_MASK and so their line
       direction is taken from `previousPosition`, with the direction being
       always from point A to point B. The edge direction is then perpendicular
       to the line direction, thus points 0 and 2 can use it as-is (+), while
       points 1 and 2 have to negate it (-).

                 ^        ^
                 e        e
                 |        |
        [+BEGIN] 0-d-->   2-d--> [+END]

                 A        B

        [-BEGIN] 1-d-->   3-d--> [-END]
                 |        |
                 e        e
                 v        v */
    highp const uint pointMarker = uint(abs(pointMarkerSign));
    // TODO handle if both begin & end, or could that be handling rotated
    //  points somehow? since it's two ways to encode, either setting both the
    //  begin and end bit, or having a zero-length line (i.e., the line could
    //  be not actually zero-length to contain the direction, but BEGIN + END
    //  set would make it zero-length
    highp const vec2 lineDirection = bool(pointMarker & POINT_MARKER_BEGIN_MASK) ?
        transformedNextPosition - transformedPosition :
        transformedPosition - transformedPreviousPosition;
    highp const float edgeSign = pointMarkerSign > 0.0 ? 1.0 : -1.0;
    highp const float neighborSign = bool(pointMarker & POINT_MARKER_BEGIN_MASK) ? -1.0 : 1.0;

    /* Line direction and its length converted from the [-1, 1] unit square to
       the screen space so we properly take aspect ratio into account. In the
       end it undoes the transformation by multiplying by 2.0/viewportSize
       again. */ // TODO avoid this somehow? i.e., pass in just aspect ratio, and prescale width by ... ugh what exactly? a 2D vector???
    highp const vec2 screenspaceLineDirection = lineDirection*viewportSize/2.0;
    highp const float screenspaceLineDirectionLength = length(screenspaceLineDirection);

    /* Normalized screenspace line and edge direction. In case of zero-sized
       lines (i.e., points) the X axis is picked as line direction instead, and
       thus Y axis for edge direction. */
    highp const vec2 screenspaceLineDirectionNormalized = screenspaceLineDirectionLength == 0.0 ? vec2(1.0, 0.0) : screenspaceLineDirection/screenspaceLineDirectionLength;
    highp const vec2 screenspaceEdgeDirectionNormalized = perpendicular(screenspaceLineDirectionNormalized);

    /* Line width includes also twice the smoothness radius, some extra padding
       on top, and is rounded to whole pixels. So for the edge distance we need
       half of it. */
    // TODO ref the paper here; actually just drop all that, smoothstep FTW
    highp const float edgeDistance = ceil(width + 2.0*smoothness)*0.5;
    #ifdef CAP_STYLE_BUTT
    // TODO sync with edgeDistance once it's changed
    highp const float capDistance = ceil(2.0*smoothness)*0.5;
    #elif defined(CAP_STYLE_SQUARE) || defined(CAP_STYLE_ROUND) || defined(CAP_STYLE_TRIANGLE)
    highp const float capDistance = edgeDistance;
    #else
    #error
    #endif
    // TODO do we actually need the unsigned capDistance/edgeDistance?
    highp const float edgeDistanceSigned = edgeDistance*edgeSign;
    highp const float capDistanceSigned = capDistance*neighborSign;

    /* Line segment half-length, passed to the fragment shader. Same for all
       four points. */
    halfSegmentLength = screenspaceLineDirectionLength*0.5;

    /* Calculate the actual endpoint parameters depending on whether we're at a
       line cap, line join bevel, line join miter etc.

        -   `screenspacePointDirection` contains screenspace direction from
            `transformedPosition` to the actual point. After undoing the
            screenspace projection the sum of the two is written to
            gl_Position.
        -   `centerDistanceSigned` contains signed distance from the edge to
            center, passed to the fragment shader. It's chosen in a way that
            interpolates to zero in the quad center, and the area where
            `all(abs(centerDistanceSigned) <= vec2(halfSegmentLength +
            capDistance, edgeDistance))` is inside the line. Edge distance in
            the Y coordinate is common for both and thus calculated here
            already.
        -   `hasCap` contains `abs(centerDistanceSigned.x)` with a sign
            positive if the point is a cap and negative if it isn't. Given
            segment endpoints A and B (and quad points 0/1 and 2/3
            corresponding to these), the following cases can happen:

            -   if both have a cap, it's a negative value in both, thus has a
                constant negative value in the fragment shader
            -   if neither have a cap, it's a positive value in both, thus has
                a constant positive value in the fragment shader
            -   if one has a cap and the other not, it's a negative value in
                one and positive in the other, interpolating to zero in the
                quad center

       In the fragment shader, `abs(centerDistanceSigned)` and `sign(hasCap)`
       is then used to perform cap rendering and antialiasing. For example,
       with a standalone line segment that has square caps on both ends, the
       value of `centerDistanceSigned` is like in the following diagram, with
       `d` being `halfSegmentLength`, `w` being `edgeDistance`, `c` being
       `capDistance`, and an extra margin for `smoothness` indicated by `s` and
       the double border:

    [-d-c-s,+w+s]                 [+d+c+s,+w+s]
           0-----------------------------2
        [-d-c,+w]------------------[+d+c,+w]
           | |                         | |      hasCap[0] = hasCap[1] = +d+c+s
        [-d-c,0]        [0,0]      [+d+c,0]
           | |                         | |      hasCap[2] = hasCap[3] = +d+c+s
        [-d-c,-w]------------------[+d+c,-w]
           1-----------------------------3
    [-d-c-s,-w-s]                 [+d+c+s,-w+s]

       With a cap only on the left side, `centerDistanceSigned` would be like
       this. Note the absence of a smoothness margin on the right side:

    [-d-c-s,+w+s]                   [+d,+w+s]
           0---------------------------2
        [-d-c,+w]-------------------[+d,+w]
           | |                         |        hasCap[0] = hasCap[1] = +d+c+s
        [-d-c,0]        [0,0]       [+d,0]
           | |                         |        hasCap[2] = hasCap[3] = -d
        [-d-c,-w]-------------------[+d,-w]
           1---------------------------3
    [-d-c-s,-w-s]                   [+d,-w-s]

    */
    centerDistanceSigned.y = edgeDistanceSigned;
    highp vec2 screenspacePointDirection;

    /* Line cap -- the quad corner 0/1/2/3 a sum of the signed cap distance
       (`cdS`) and signed edge distance vectors (`eDS`), which are formed by
       the line direction vector `d` and its perpendicular vector. Neighbor
       direction (i.e., the other input from the one used to calculate
       `lineDirection`) isn't used at all in this case.

          cDS
        0<---+----------
        |    ^
        |    | eDS
        |    |
        |    A--d-->
        |
        |
        |
        1---

       The signed center distance a sum of half segment length and the cap
       distance, multiplied by the cap sign (thus negative for points derived
       from A and positive for B). */
    if(bool(pointMarker & POINT_MARKER_CAP_MASK)) {
        screenspacePointDirection =
            screenspaceLineDirectionNormalized*capDistanceSigned +
            screenspaceEdgeDirectionNormalized*edgeDistanceSigned;

        highp const float centerDistanceUnsignedX = halfSegmentLength + capDistance;
        centerDistanceSigned.x = centerDistanceUnsignedX*neighborSign;
        hasCap = centerDistanceUnsignedX;

    /* Line join otherwise */
    } else {
        /* Neighbor direction `nd`, needed to distinguish whether this is the
           inner or outer join point. Calculated with basically an inverse of
           the logic used to calculate `lineDirection`, with the neighbor
           direction always pointing from the A/B endpoint to the other
           neighbor line endpoint:

            <--nd-0 [BEGIN]   [END] 2-nd-->

                  A                 B

            <--nd-1 [BEGIN]   [END] 3-nd-->  */
        highp const vec2 neighborDirection = bool(pointMarker & POINT_MARKER_BEGIN_MASK) ?
            transformedPreviousPosition - transformedPosition :
            transformedNextPosition - transformedPosition;
        // TODO screenspace neighbor direction!
        // TODO what does this do in presence of zero-length line segments?
        //  document that, especially important when the aliased attributes are
        //  used (forbid those except for standalone points, e.g.)
        highp const vec2 neighborDirectionNormalized = normalize(neighborDirection);

        /* Calculate the angle `α` between the edge direction vector `e` and
           the neighbor direction vector `nd` in order to decide whether this
           is an inner or outer point. It's an outer point if the angle is
           larger than 90°, i.e. when the two go in the opposite direction,
           which is when the dot product is negative:

                 ^
                 e
                 |
            -----2 α
                 |\
             B   | nd
                 |  \
            -----3   v

           */
        // TODO maybe the sign could be a part of the screenspaceEdgeDirectionNormalized already? would save a multiplication above as well
        highp const float pointAngleCos = dot(screenspaceEdgeDirectionNormalized*edgeSign, neighborDirectionNormalized);

        /* It's an outer point of a beveled joint either if bevel/circle joins
           are desired everywhere (and this is an outer point) or if we want
           miter joints and the angle between the edge direction and neighbor
           direction vectors is larger than a specified limit (where the limit
           is always more than 90°, because less than 90° are inner points). */
        const bool outerBeveledPoint = // false && // TODO
            #if defined(JOIN_STYLE_BEVEL) || defined(JOIN_STYLE_ROUND)
            pointAngleCos < 0.0
            #elif defined(JOIN_STYLE_MITER)
            pointAngleCos < -1.0 // -miterLimit // TODO
            #else
            #error
            #endif
            ;

        /* Outer point of a beveled / circular join -- although
           https://www.w3.org/TR/svg-strokes/#LineJoin doesn't define *what
           exactly* is a bevel, it's defined as "Cuts the outside edge off
           where a circle the diameter of the stroke intersects the stroke." at
           e.g. https://apike.ca/prog_svg_line_cap_join.html, which ultimately
           means the `2a` and `2b` quad endpoints are simply the edge direction
           vector `e` away from point B, in one case with the `e` calculated
           from the AB segment, and in the other from the BC segment (left
           diagram).

           A circular join then extends the 2a / 2b points in the direction of
           their respective segments so that the triangle edge is also at a distance `w` from B, with the angle `ρ` between the triangle edge
           and the `w` line also being 90° (right diagram).

           TODO move the circular join bit later? or maybe move the bevel details
           into the branch that actually computes it, and not the general
           overview

            0---   ----2a               0---   -------2a
            |          |^\              |            |  \
            |         | e -             |           | w/ρ\
            |         | |ρ  \           |          | /  _2b
            A--  ----|--B-e->2b         A--  -----|-B _- |
            |       |   |  _-|          |        |  _-   |
            |       |   _-   |          |       | _-|    |
            |      | _- |    |          |      |_-  |    |
            1--  --3    |    |          1--  --3    |    |
                   |    |    |                 |    |    |
                        C                           C */
        if(outerBeveledPoint) {
            screenspacePointDirection = screenspaceEdgeDirectionNormalized*edgeDistanceSigned;

            centerDistanceSigned.x = halfSegmentLength*neighborSign;
            /* hasCap set below */
            // TODO circular join

        /* Otherwise it's either an outer point of a miter join (basically
           points 2a and 2b from above evaluated to the same position), or the
           inner point, which is the same for bevel and mitter joins. Given
           normalized direction `d` and neighbor direction `nd`,
           `normalized(d + nd)` is the "average" direction of the two and `perpendicular(normalized(d + nd))` gives us the direction from B to
           2 (or from 3 to B):

            0---    --------+---2
            |               | α/ \
            |             w | / j \
            |               |/     \
            A--  +_-----d->-B       \
            |       -_    α/α\       \
            |          -_ /   nd      \
            |     d + nd /-_   v       \
            1----   ----3    -_ \
                         \      -+
                          \       \
                                   C

           With `2α` being the angle between `d` and `nd`, `α` appears in two
           right triangles and the following holds, `w` being the edge distance
           from above, and `j` being the length that's needed to scale
           `perpendicular(normalized(d + nd))` to get point 2:

                     |d + nd|    w               2 w
            sin(α) = -------- = ---   -->  j = --------
                      2 |d|      j             |d + nd|

           Point 3 is then just in the opposite direction; for the other side
           it's done equivalently. */
        } else {
            /* Screenspace neighbor direction and its length, calculated
               equivalently to screenspace line direction above */
            // TODO what about zero-length segments?
            highp const vec2 screenspaceNeighborDirectionNormalized = normalize(neighborDirection*viewportSize/2.0);

            highp const vec2 averageDirection = neighborSign*screenspaceLineDirectionNormalized + screenspaceNeighborDirectionNormalized;
            highp const float averageDirectionLength = length(averageDirection);
            highp const float j = 2.0*edgeDistance/averageDirectionLength;
            screenspacePointDirection = (normalize(perpendicular(averageDirection))*neighborSign*edgeSign*j);

            // TODO ex?! what is all this
            highp const float ex = sqrt(j*j - edgeDistance*edgeDistance);
            centerDistanceSigned.x = halfSegmentLength*neighborSign + ex*sign(dot(lineDirection*neighborSign, (perpendicular(averageDirection))*edgeSign));
        }

        // TODO any way to get this value unsigned? probably not, esp given
        // that the inner join might be on the negative side of the center HUH
        // but TODO then it should be positive too, no?? test!
        hasCap = -abs(centerDistanceSigned.x);
    }

    /* Undo the screenspace projection */
    highp const vec2 pointDirection = screenspacePointDirection*2.0/viewportSize;

    #ifdef TWO_DIMENSIONS
    gl_Position.xyzw = vec4(transformedPosition + pointDirection, 0.0, 1.0);
    #elif defined(THREE_DIMENSIONS)
    // TODO 3D, how to handle perspective? multiply edgeDistance with w?
    // TODO also how to handle depth?
    gl_Position = transformationProjectionMatrix*
        #ifdef INSTANCED_TRANSFORMATION
        instancedTransformationMatrix*
        #endif
        position;
    #else
    #error
    #endif

    #ifdef VERTEX_COLOR
    /* Vertex colors, if enabled */
    interpolatedVertexColor = vertexColor;
    #endif

    #ifdef INSTANCED_OBJECT_ID
    /* Instanced object ID, if enabled */
    interpolatedInstanceObjectId = instanceObjectId;
    #endif
}
