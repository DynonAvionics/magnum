#ifndef Magnum_MeshTools_CompileLines_h
#define Magnum_MeshTools_CompileLines_h
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

#include <Corrade/Containers/EnumSet.hpp>

#include "Magnum/GL/GL.h"
#include "Magnum/MeshTools/visibility.h"
#include "Magnum/Trade/Trade.h"

namespace Magnum { namespace MeshTools {

// TODO disable this if TARGET_GL isn't enabled, doc that

enum class CompileLinesFlag: UnsignedInt {
    // TODO "interpret index buffer as line strips and loops", asserting if
    //  more than 2 indices share the same vertex
    // TODO overlap, just a single triangle for bevels, no bevels
    // TODO all lines are just lone segments, so no prev/next attributes
};

typedef Containers::EnumSet<CompileLinesFlag> CompileLinesFlags;

CORRADE_ENUMSET_OPERATORS(CompileLinesFlags)

MAGNUM_MESHTOOLS_EXPORT GL::Mesh compileLines(const Trade::MeshData& lineMesh, CompileLinesFlags flags = {});

}}

#endif
