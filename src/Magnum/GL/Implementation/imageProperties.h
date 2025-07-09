#ifndef Magnum_GL_Implementation_imageProperties_h
#define Magnum_GL_Implementation_imageProperties_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025
              Vladimír Vondruš <mosra@centrum.cz>

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

#include "Magnum/Implementation/ImageProperties.h"

namespace Magnum { namespace GL { namespace Implementation {

/* Used in compressed image upload functions. Unlike what common sense and
   various robustness extensions would imply, where the size is the memory
   range occupied by the data given various pixel storage parameters, it's
   instead expected to stupidly be just the image dimensions (*not* row length
   etc) in whole blocks. Which has absolutely NO RELATION to the actual memory
   and thus is completely useless for enforcing any memory security in the
   driver, it's only there to bully users. My suspicion is that whoever did
   ARB_compressed_texture_pixel_storage (which makes skip, row length etc.
   possible for compressed formats) didn't bother thinking about what the
   existing parameter is for, just left it unchanged, and nobody else in the
   commitee bothered either.

   In case the block size properties aren't set, the actual image data size is
   used as a backup, which might still be correct in most cases. */
template<class T> std::size_t occupiedCompressedImageDataSize(const T& image, std::size_t dataSize) {
    return image.storage().compressedBlockSize().product() && image.storage().compressedBlockDataSize()
        ? Magnum::Implementation::compressedImageDataOffsetSizeFor(image, image.size()).second : dataSize;
}

}}}

#endif
