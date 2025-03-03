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

/* Included as first to check that we *really* don't need the StridedArrayView
   header for definition of pixels(). We actually need, but just for the
   arrayCast() template, which is forward-declared. */
#include "Magnum/Image.h"

#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/String.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once dataProperties() std::pair is gone */

#include "Magnum/ImageView.h"
#include "Magnum/PixelFormat.h"
#include "Magnum/Math/Color.h"

namespace Magnum { namespace Test { namespace {

struct ImageTest: TestSuite::Tester {
    explicit ImageTest();

    void constructGeneric();
    void constructGenericPlaceholder();
    void constructImplementationSpecific();
    void constructImplementationSpecificPlaceholder();
    void constructCompressedGeneric();
    void constructCompressedGenericPlaceholder();
    void constructCompressedImplementationSpecific();

    void constructUnknownImplementationSpecificPixelSize();
    void constructInvalidPixelSize();
    void constructInvalidSize();
    void constructInvalidCubeMap();
    void constructCompressedInvalidSize();
    void constructCompressedInvalidCubeMap();

    void constructCopy();
    void constructCopyCompressed();

    void constructMoveGeneric();
    void constructMoveImplementationSpecific();
    void constructMoveCompressedGeneric();
    void constructMoveCompressedImplementationSpecific();

    template<class T> void toViewGeneric();
    template<class T> void toViewImplementationSpecific();
    template<class T> void toViewCompressedGeneric();
    template<class T> void toViewCompressedImplementationSpecific();

    void data();
    void dataCompressed();
    void dataRvalue();
    void dataRvalueCompressed();

    void dataProperties();
    void dataPropertiesCompressed();

    void release();
    void releaseCompressed();

    void pixels1D();
    void pixels2D();
    void pixels3D();
};

template<class> struct MutabilityTraits;
template<> struct MutabilityTraits<const char> {
    typedef const Image2D ImageType;
    typedef const CompressedImage2D CompressedImageType;

    static const char* name() { return "ImageView"; }
};
template<> struct MutabilityTraits<char> {
    typedef Image2D ImageType;
    typedef CompressedImage2D CompressedImageType;

    static const char* name() { return "MutableImageView"; }
};

ImageTest::ImageTest() {
    addTests({&ImageTest::constructGeneric,
              &ImageTest::constructGenericPlaceholder,
              &ImageTest::constructImplementationSpecific,
              &ImageTest::constructImplementationSpecificPlaceholder,
              &ImageTest::constructCompressedGeneric,
              &ImageTest::constructCompressedGenericPlaceholder,
              &ImageTest::constructCompressedImplementationSpecific,

              &ImageTest::constructUnknownImplementationSpecificPixelSize,
              &ImageTest::constructInvalidPixelSize,
              &ImageTest::constructInvalidSize,
              &ImageTest::constructInvalidCubeMap,
              &ImageTest::constructCompressedInvalidSize,
              &ImageTest::constructCompressedInvalidCubeMap,

              &ImageTest::constructCopy,
              &ImageTest::constructCopyCompressed,

              &ImageTest::constructMoveGeneric,
              &ImageTest::constructMoveImplementationSpecific,
              &ImageTest::constructMoveCompressedGeneric,
              &ImageTest::constructMoveCompressedImplementationSpecific,

              &ImageTest::toViewGeneric<const char>,
              &ImageTest::toViewGeneric<char>,
              &ImageTest::toViewImplementationSpecific<const char>,
              &ImageTest::toViewImplementationSpecific<char>,
              &ImageTest::toViewCompressedGeneric<const char>,
              &ImageTest::toViewCompressedGeneric<char>,
              &ImageTest::toViewCompressedImplementationSpecific<const char>,
              &ImageTest::toViewCompressedImplementationSpecific<char>,

              &ImageTest::data,
              &ImageTest::dataCompressed,
              &ImageTest::dataRvalue,
              &ImageTest::dataRvalueCompressed,

              &ImageTest::dataProperties,
              &ImageTest::dataPropertiesCompressed,

              &ImageTest::release,
              &ImageTest::releaseCompressed,

              &ImageTest::pixels1D,
              &ImageTest::pixels2D,
              &ImageTest::pixels3D});
}

namespace GL {
    enum class PixelFormat { RGB = 666 };
    enum class PixelType { UnsignedShort = 1337 };
    /* Clang -Wmissing-prototypes warns otherwise, even though this is in an
       anonymous namespace */
    UnsignedInt pixelFormatSize(PixelFormat, PixelType);
    UnsignedInt pixelFormatSize(PixelFormat format, PixelType type) {
        CORRADE_INTERNAL_ASSERT(format == PixelFormat::RGB);
        CORRADE_INTERNAL_ASSERT(type == PixelType::UnsignedShort);
        #ifdef CORRADE_NO_ASSERT
        static_cast<void>(format);
        static_cast<void>(type);
        #endif
        return 6;
    }

    enum class CompressedPixelFormat { RGBS3tcDxt1 = 21 };
}

namespace Vk {
    enum class PixelFormat { R32G32B32F = 42 };
    /* Clang -Wmissing-prototypes warns otherwise, even though this is in an
       anonymous namespace */
    UnsignedInt pixelFormatSize(PixelFormat);
    UnsignedInt pixelFormatSize(PixelFormat format) {
        CORRADE_INTERNAL_ASSERT(format == PixelFormat::R32G32B32F);
        #ifdef CORRADE_NO_ASSERT
        static_cast<void>(format);
        #endif
        return 12;
    }
}

void ImageTest::constructGeneric() {
    {
        auto data = new char[4*4];
        Image2D a{PixelFormat::RGBA8Unorm, {1, 3}, Containers::Array<char>{data, 4*4}, ImageFlag2D::Array};

        CORRADE_COMPARE(a.flags(), ImageFlag2D::Array);
        CORRADE_COMPARE(a.storage().alignment(), 4);
        CORRADE_COMPARE(a.format(), PixelFormat::RGBA8Unorm);
        CORRADE_COMPARE(a.formatExtra(), 0);
        CORRADE_COMPARE(a.pixelSize(), 4);
        CORRADE_COMPARE(a.size(), (Vector2i{1, 3}));
        CORRADE_COMPARE(a.data(), static_cast<const void*>(data));
        CORRADE_COMPARE(a.data().size(), 4*4);
    } {
        auto data = new char[3*2];
        Image2D a{PixelStorage{}.setAlignment(1),
            PixelFormat::R16UI, {1, 3}, Containers::Array<char>{data, 3*2}, ImageFlag2D::Array};

        CORRADE_COMPARE(a.flags(), ImageFlag2D::Array);
        CORRADE_COMPARE(a.storage().alignment(), 1);
        CORRADE_COMPARE(a.format(), PixelFormat::R16UI);
        CORRADE_COMPARE(a.formatExtra(), 0);
        CORRADE_COMPARE(a.pixelSize(), 2);
        CORRADE_COMPARE(a.size(), (Vector2i{1, 3}));
        CORRADE_COMPARE(a.data(), static_cast<const void*>(data));
        CORRADE_COMPARE(a.data().size(), 3*2);
    }
}

void ImageTest::constructGenericPlaceholder() {
    {
        Image2D a{PixelFormat::RG32F};

        CORRADE_COMPARE(a.flags(), ImageFlags2D{});
        CORRADE_COMPARE(a.storage().alignment(), 4);
        CORRADE_COMPARE(a.format(), PixelFormat::RG32F);
        CORRADE_COMPARE(a.formatExtra(), 0);
        CORRADE_COMPARE(a.pixelSize(), 8);
        CORRADE_COMPARE(a.size(), Vector2i{});
        CORRADE_COMPARE(a.data(), static_cast<const void*>(nullptr));
    } {
        Image2D a{
            PixelStorage{}
                /* Even with skip it shouldn't assert on data size */
                .setSkip({1, 0, 0})
                .setAlignment(1),
            PixelFormat::RGB16F};

        CORRADE_COMPARE(a.flags(), ImageFlags2D{});
        CORRADE_COMPARE(a.storage().skip(), (Vector3i{1, 0, 0}));
        CORRADE_COMPARE(a.storage().alignment(), 1);
        CORRADE_COMPARE(a.format(), PixelFormat::RGB16F);
        CORRADE_COMPARE(a.formatExtra(), 0);
        CORRADE_COMPARE(a.pixelSize(), 6);
        CORRADE_COMPARE(a.size(), Vector2i{});
        CORRADE_COMPARE(a.data(), static_cast<const void*>(nullptr));
    }
}

void ImageTest::constructImplementationSpecific() {
    /* Single format */
    {
        auto data = new char[3*12];
        Image2D a{Vk::PixelFormat::R32G32B32F, {1, 3}, Containers::Array<char>{data, 3*12}, ImageFlag2D::Array};

        CORRADE_COMPARE(a.flags(), ImageFlag2D::Array);
        CORRADE_COMPARE(a.storage().alignment(), 4);
        CORRADE_COMPARE(a.format(), pixelFormatWrap(Vk::PixelFormat::R32G32B32F));
        CORRADE_COMPARE(a.formatExtra(), 0);
        CORRADE_COMPARE(a.pixelSize(), 12);
        CORRADE_COMPARE(a.size(), (Vector2i{1, 3}));
        CORRADE_COMPARE(a.data(), static_cast<const void*>(data));
        CORRADE_COMPARE(a.data().size(), 3*12);
    } {
        auto data = new char[3*12];
        Image2D a{PixelStorage{}.setAlignment(1),
            Vk::PixelFormat::R32G32B32F, {1, 3}, Containers::Array<char>{data, 3*12}, ImageFlag2D::Array};

        CORRADE_COMPARE(a.flags(), ImageFlag2D::Array);
        CORRADE_COMPARE(a.storage().alignment(), 1);
        CORRADE_COMPARE(a.format(), pixelFormatWrap(Vk::PixelFormat::R32G32B32F));
        CORRADE_COMPARE(a.formatExtra(), 0);
        CORRADE_COMPARE(a.pixelSize(), 12);
        CORRADE_COMPARE(a.size(), (Vector2i{1, 3}));
        CORRADE_COMPARE(a.data(), static_cast<const void*>(data));
        CORRADE_COMPARE(a.data().size(), 3*12);
    }

    /* Format + extra */
    {
        auto data = new char[3*8];
        Image2D a{GL::PixelFormat::RGB, GL::PixelType::UnsignedShort, {1, 3}, Containers::Array<char>{data, 3*8}, ImageFlag2D::Array};

        CORRADE_COMPARE(a.flags(), ImageFlag2D::Array);
        CORRADE_COMPARE(a.storage().alignment(), 4);
        CORRADE_COMPARE(a.format(), pixelFormatWrap(GL::PixelFormat::RGB));
        CORRADE_COMPARE(a.formatExtra(), UnsignedInt(GL::PixelType::UnsignedShort));
        CORRADE_COMPARE(a.pixelSize(), 6);
        CORRADE_COMPARE(a.size(), (Vector2i{1, 3}));
        CORRADE_COMPARE(a.data(), static_cast<const void*>(data));
        CORRADE_COMPARE(a.data().size(), 3*8);
    } {
        auto data = new char[3*6];
        Image2D a{PixelStorage{}.setAlignment(1),
            GL::PixelFormat::RGB, GL::PixelType::UnsignedShort, {1, 3}, Containers::Array<char>{data, 3*6}, ImageFlag2D::Array};

        CORRADE_COMPARE(a.flags(), ImageFlag2D::Array);
        CORRADE_COMPARE(a.format(), pixelFormatWrap(GL::PixelFormat::RGB));
        CORRADE_COMPARE(a.formatExtra(), UnsignedInt(GL::PixelType::UnsignedShort));
        CORRADE_COMPARE(a.pixelSize(), 6);
        CORRADE_COMPARE(a.size(), (Vector2i{1, 3}));
        CORRADE_COMPARE(a.data(), static_cast<const void*>(data));
        CORRADE_COMPARE(a.data().size(), 3*6);
    }

    /* Manual pixel size */
    {
        auto data = new char[3*6];
        Image2D a{PixelStorage{}.setAlignment(1), 666, 1337, 6, {1, 3}, Containers::Array<char>{data, 3*6}, ImageFlag2D::Array};

        CORRADE_COMPARE(a.flags(), ImageFlag2D::Array);
        CORRADE_COMPARE(a.storage().alignment(), 1);
        CORRADE_COMPARE(a.format(), pixelFormatWrap(GL::PixelFormat::RGB));
        CORRADE_COMPARE(a.formatExtra(), UnsignedInt(GL::PixelType::UnsignedShort));
        CORRADE_COMPARE(a.pixelSize(), 6);
        CORRADE_COMPARE(a.size(), (Vector2i{1, 3}));
        CORRADE_COMPARE(a.data(), static_cast<const void*>(data));
        CORRADE_COMPARE(a.data().size(), 3*6);
    }
}

void ImageTest::constructImplementationSpecificPlaceholder() {
    /* Single format */
    {
        Image2D a{Vk::PixelFormat::R32G32B32F};

        CORRADE_COMPARE(a.flags(), ImageFlags2D{});
        CORRADE_COMPARE(a.storage().alignment(), 4);
        CORRADE_COMPARE(a.format(), pixelFormatWrap(Vk::PixelFormat::R32G32B32F));
        CORRADE_COMPARE(a.formatExtra(), 0);
        CORRADE_COMPARE(a.pixelSize(), 12);
        CORRADE_COMPARE(a.size(), Vector2i{});
        CORRADE_COMPARE(a.data(), static_cast<const void*>(nullptr));
    } {
        Image2D a{
            PixelStorage{}
                /* Even with skip it shouldn't assert on data size */
                .setSkip({1, 0, 0})
                .setAlignment(1),
            Vk::PixelFormat::R32G32B32F};

        CORRADE_COMPARE(a.flags(), ImageFlags2D{});
        CORRADE_COMPARE(a.storage().skip(), (Vector3i{1, 0, 0}));
        CORRADE_COMPARE(a.storage().alignment(), 1);
        CORRADE_COMPARE(a.format(), pixelFormatWrap(Vk::PixelFormat::R32G32B32F));
        CORRADE_COMPARE(a.formatExtra(), 0);
        CORRADE_COMPARE(a.pixelSize(), 12);
        CORRADE_COMPARE(a.size(), Vector2i{});
        CORRADE_COMPARE(a.data(), static_cast<const void*>(nullptr));
    }

    /* Format + extra */
    {
        Image2D a{GL::PixelFormat::RGB, GL::PixelType::UnsignedShort};

        CORRADE_COMPARE(a.flags(), ImageFlags2D{});
        CORRADE_COMPARE(a.storage().alignment(), 4);
        CORRADE_COMPARE(a.format(), pixelFormatWrap(GL::PixelFormat::RGB));
        CORRADE_COMPARE(a.formatExtra(), UnsignedInt(GL::PixelType::UnsignedShort));
        CORRADE_COMPARE(a.pixelSize(), 6);
        CORRADE_COMPARE(a.size(), Vector2i{});
        CORRADE_COMPARE(a.data(), static_cast<const void*>(nullptr));
    } {
        Image2D a{
            PixelStorage{}
                /* Even with skip it shouldn't assert on data size */
                .setSkip({1, 0, 0})
                .setAlignment(1),
            GL::PixelFormat::RGB, GL::PixelType::UnsignedShort};

        CORRADE_COMPARE(a.flags(), ImageFlags2D{});
        CORRADE_COMPARE(a.storage().skip(), (Vector3i{1, 0, 0}));
        CORRADE_COMPARE(a.storage().alignment(), 1);
        CORRADE_COMPARE(a.format(), pixelFormatWrap(GL::PixelFormat::RGB));
        CORRADE_COMPARE(a.formatExtra(), UnsignedInt(GL::PixelType::UnsignedShort));
        CORRADE_COMPARE(a.pixelSize(), 6);
        CORRADE_COMPARE(a.size(), Vector2i{});
        CORRADE_COMPARE(a.data(), static_cast<const void*>(nullptr));
    }

    /* Manual pixel size */
    {
        Image2D a{PixelStorage{}
            /* Even with skip it shouldn't assert on data size */
            .setSkip({1, 0, 0})
            .setAlignment(1), 666, 1337, 6};

        CORRADE_COMPARE(a.flags(), ImageFlags2D{});
        CORRADE_COMPARE(a.storage().skip(), (Vector3i{1, 0, 0}));
        CORRADE_COMPARE(a.storage().alignment(), 1);
        CORRADE_COMPARE(a.format(), pixelFormatWrap(GL::PixelFormat::RGB));
        CORRADE_COMPARE(a.formatExtra(), UnsignedInt(GL::PixelType::UnsignedShort));
        CORRADE_COMPARE(a.pixelSize(), 6);
        CORRADE_COMPARE(a.size(), Vector2i{});
        CORRADE_COMPARE(a.data(), static_cast<const void*>(nullptr));
    }
}

void ImageTest::constructCompressedGeneric() {
    {
        auto data = new char[8];
        CompressedImage2D a{CompressedPixelFormat::Bc1RGBAUnorm, {4, 4},
            Containers::Array<char>{data, 8}, ImageFlag2D::Array};

        CORRADE_COMPARE(a.flags(), ImageFlag2D::Array);
        CORRADE_COMPARE(a.storage().compressedBlockSize(), Vector3i{0});
        CORRADE_COMPARE(a.format(), CompressedPixelFormat::Bc1RGBAUnorm);
        CORRADE_COMPARE(a.size(), (Vector2i{4, 4}));
        CORRADE_COMPARE(a.data(), static_cast<const void*>(data));
        CORRADE_COMPARE(a.data().size(), 8);
    } {
        auto data = new char[8];
        CompressedImage2D a{CompressedPixelStorage{}.setCompressedBlockSize(Vector3i{4}),
            CompressedPixelFormat::Bc1RGBAUnorm, {4, 4},
            Containers::Array<char>{data, 8}, ImageFlag2D::Array};

        CORRADE_COMPARE(a.flags(), ImageFlag2D::Array);
        CORRADE_COMPARE(a.storage().compressedBlockSize(), Vector3i{4});
        CORRADE_COMPARE(a.format(), CompressedPixelFormat::Bc1RGBAUnorm);
        CORRADE_COMPARE(a.size(), Vector2i(4, 4));
        CORRADE_COMPARE(a.data(), static_cast<const void*>(data));
        CORRADE_COMPARE(a.data().size(), 8);
    }
}

void ImageTest::constructCompressedGenericPlaceholder() {
    {
        CompressedImage2D a;

        CORRADE_COMPARE(a.flags(), ImageFlags2D{});
        CORRADE_COMPARE(a.storage().compressedBlockSize(), Vector3i{0});
        CORRADE_COMPARE(a.format(), CompressedPixelFormat{});
        CORRADE_COMPARE(a.size(), Vector2i{});
        CORRADE_COMPARE(a.data(), static_cast<const void*>(nullptr));
    } {
        CompressedImage2D a{
            CompressedPixelStorage{}
                /* Even with skip it shouldn't assert on data size */
                .setSkip({1, 0, 0})
                .setCompressedBlockSize(Vector3i{4})};

        CORRADE_COMPARE(a.flags(), ImageFlags2D{});
        CORRADE_COMPARE(a.storage().skip(), (Vector3i{1, 0, 0}));
        CORRADE_COMPARE(a.storage().compressedBlockSize(), Vector3i{4});
        CORRADE_COMPARE(a.format(), CompressedPixelFormat{});
        CORRADE_COMPARE(a.size(), Vector2i{});
        CORRADE_COMPARE(a.data(), static_cast<const void*>(nullptr));
    }
}

void ImageTest::constructCompressedImplementationSpecific() {
    /* Format with autodetection */
    {
        auto data = new char[8];
        CompressedImage2D a{GL::CompressedPixelFormat::RGBS3tcDxt1, {4, 4},
            Containers::Array<char>{data, 8}, ImageFlag2D::Array};

        CORRADE_COMPARE(a.flags(), ImageFlag2D::Array);
        CORRADE_COMPARE(a.storage().compressedBlockSize(), Vector3i{0});
        CORRADE_COMPARE(a.format(), compressedPixelFormatWrap(GL::CompressedPixelFormat::RGBS3tcDxt1));
        CORRADE_COMPARE(a.size(), (Vector2i{4, 4}));
        CORRADE_COMPARE(a.data(), static_cast<const void*>(data));
        CORRADE_COMPARE(a.data().size(), 8);
    } {
        auto data = new char[8];
        CompressedImage2D a{CompressedPixelStorage{}.setCompressedBlockSize(Vector3i{4}),
            GL::CompressedPixelFormat::RGBS3tcDxt1, {4, 4},
            Containers::Array<char>{data, 8}, ImageFlag2D::Array};

        CORRADE_COMPARE(a.flags(), ImageFlag2D::Array);
        CORRADE_COMPARE(a.storage().compressedBlockSize(), Vector3i{4});
        CORRADE_COMPARE(a.format(), compressedPixelFormatWrap(GL::CompressedPixelFormat::RGBS3tcDxt1));
        CORRADE_COMPARE(a.size(), (Vector2i{4, 4}));
        CORRADE_COMPARE(a.data(), static_cast<const void*>(data));
        CORRADE_COMPARE(a.data().size(), 8);
    }

    /* Manual properties not implemented yet */
}

void ImageTest::constructUnknownImplementationSpecificPixelSize() {
    CORRADE_SKIP_IF_NO_ASSERT();

    Containers::String out;
    Error redirectError{&out};
    Image2D{pixelFormatWrap(0x666), {1, 1}, Containers::Array<char>{NoInit, 1}};
    Image2D{pixelFormatWrap(0x777)};
    CORRADE_COMPARE_AS(out,
        "Image: can't determine size of an implementation-specific pixel format 0x666, pass it explicitly\n"
        /* The next messages are printed because it cannot exit the
           construction from the middle of the member initializer list. It does
           so with non-graceful asserts tho and just one message is printed. */
        "pixelFormatSize(): can't determine size of an implementation-specific format 0x666\n"
        "Image: expected pixel size to be non-zero and less than 256 but got 0\n"

        "Image: can't determine size of an implementation-specific pixel format 0x777, pass it explicitly\n"
        "pixelFormatSize(): can't determine size of an implementation-specific format 0x777\n"
        "Image: expected pixel size to be non-zero and less than 256 but got 0\n",
        TestSuite::Compare::String);
}

void ImageTest::constructInvalidPixelSize() {
    CORRADE_SKIP_IF_NO_ASSERT();

    Containers::String out;
    Error redirectError{&out};
    Image2D{PixelStorage{}, 666, 0, 0, {}, nullptr};
    Image2D{PixelStorage{}, 666, 0, 256, {}, nullptr};
    Image2D{PixelStorage{}, 666, 0, 0};
    Image2D{PixelStorage{}, 666, 0, 256};
    CORRADE_COMPARE_AS(out,
        "Image: expected pixel size to be non-zero and less than 256 but got 0\n"
        "Image: expected pixel size to be non-zero and less than 256 but got 256\n"
        "Image: expected pixel size to be non-zero and less than 256 but got 0\n"
        "Image: expected pixel size to be non-zero and less than 256 but got 256\n",
        TestSuite::Compare::String);
}

void ImageTest::constructInvalidSize() {
    CORRADE_SKIP_IF_NO_ASSERT();

    Containers::String out;
    Error redirectError{&out};

    /* Doesn't consider alignment */
    Image2D{PixelFormat::RGB8Unorm, {1, 3}, Containers::Array<char>{3*3}};
    CORRADE_COMPARE(out, "Image: data too small, got 9 but expected at least 12 bytes\n");
}

void ImageTest::constructInvalidCubeMap() {
    CORRADE_SKIP_IF_NO_ASSERT();

    Containers::String out;
    Error redirectError{&out};
    Image3D{PixelFormat::RGBA8Unorm, {3, 3, 5}, Containers::Array<char>{3*3*5*4}, ImageFlag3D::CubeMap};
    Image3D{PixelFormat::RGBA8Unorm, {3, 4, 6}, Containers::Array<char>{3*4*6*4}, ImageFlag3D::CubeMap};
    Image3D{PixelFormat::RGBA8Unorm, {3, 3, 17}, Containers::Array<char>{3*3*17*4}, ImageFlag3D::CubeMap |ImageFlag3D::Array};
    Image3D{PixelFormat::RGBA8Unorm, {4, 3, 18}, Containers::Array<char>{4*3*18*4}, ImageFlag3D::CubeMap |ImageFlag3D::Array};
    CORRADE_COMPARE(out,
        "Image: expected exactly 6 faces for a cube map, got 5\n"
        "Image: expected square faces for a cube map, got {3, 4}\n"
        "Image: expected a multiple of 6 faces for a cube map array, got 17\n"
        "Image: expected square faces for a cube map, got {4, 3}\n");
}

void ImageTest::constructCompressedInvalidSize() {
    CORRADE_SKIP_IF_NO_ASSERT();

    CORRADE_EXPECT_FAIL("Size checking for compressed image data is not implemented yet.");

    /* Too small for given format */
    {
        Containers::String out;
        Error redirectError{&out};
        CompressedImage2D{CompressedPixelFormat::Bc2RGBAUnorm, {4, 4}, Containers::Array<char>{15}};
        CORRADE_COMPARE(out, "CompressedImage: data too small, got 15 but expected at least 16 bytes\n");

    /* Size should be rounded up even if the image size is not full block */
    } {
        Containers::String out;
        Error redirectError{&out};
        CompressedImage2D{CompressedPixelFormat::Bc2RGBAUnorm, {2, 2}, Containers::Array<char>{15}};
        CORRADE_COMPARE(out, "CompressedImage: data too small, got 15 but expected at least 16 bytes\n");
    }
}

void ImageTest::constructCompressedInvalidCubeMap() {
    CORRADE_SKIP_IF_NO_ASSERT();

    Containers::String out;
    Error redirectError{&out};
    CompressedImage3D{CompressedPixelFormat::Bc1RGBAUnorm, {3, 3, 5}, Containers::Array<char>{8*5}, ImageFlag3D::CubeMap};
    CompressedImage3D{CompressedPixelFormat::Bc1RGBAUnorm, {3, 4, 6}, Containers::Array<char>{8*6}, ImageFlag3D::CubeMap};
    CompressedImage3D{CompressedPixelFormat::Bc1RGBAUnorm, {3, 3, 17}, Containers::Array<char>{8*17}, ImageFlag3D::CubeMap |ImageFlag3D::Array};
    CompressedImage3D{CompressedPixelFormat::Bc1RGBAUnorm, {4, 3, 18}, Containers::Array<char>{8*18}, ImageFlag3D::CubeMap |ImageFlag3D::Array};
    CORRADE_COMPARE(out,
        "CompressedImage: expected exactly 6 faces for a cube map, got 5\n"
        "CompressedImage: expected square faces for a cube map, got {3, 4}\n"
        "CompressedImage: expected a multiple of 6 faces for a cube map array, got 17\n"
        "CompressedImage: expected square faces for a cube map, got {4, 3}\n");
}

void ImageTest::constructCopy() {
    CORRADE_VERIFY(!std::is_copy_constructible<Image2D>{});
    CORRADE_VERIFY(!std::is_copy_assignable<Image2D>{});
}

void ImageTest::constructCopyCompressed() {
    CORRADE_VERIFY(!std::is_copy_constructible<CompressedImage2D>{});
    CORRADE_VERIFY(!std::is_copy_assignable<CompressedImage2D>{});
}

void ImageTest::constructMoveGeneric() {
    auto data = new char[3*16];
    Image2D a{PixelStorage{}.setAlignment(1),
        PixelFormat::RGBA32F, {1, 3}, Containers::Array<char>{data, 3*16}, ImageFlag2D::Array};
    Image2D b(Utility::move(a));

    CORRADE_COMPARE(a.data(), static_cast<const void*>(nullptr));
    CORRADE_COMPARE(a.size(), Vector2i{});

    CORRADE_COMPARE(b.flags(), ImageFlag2D::Array);
    CORRADE_COMPARE(b.storage().alignment(), 1);
    CORRADE_COMPARE(b.format(), PixelFormat::RGBA32F);
    CORRADE_COMPARE(b.formatExtra(), 0);
    CORRADE_COMPARE(b.pixelSize(), 16);
    CORRADE_COMPARE(b.size(), (Vector2i{1, 3}));
    CORRADE_COMPARE(b.data(), static_cast<const void*>(data));
    CORRADE_COMPARE(b.data().size(), 3*16);

    auto data2 = new char[24];
    Image2D c{PixelFormat::R8I, {2, 6}, Containers::Array<char>{data2, 24}};
    c = Utility::move(b);

    CORRADE_COMPARE(b.data(), static_cast<const void*>(data2));
    CORRADE_COMPARE(b.size(), (Vector2i{2, 6}));

    CORRADE_COMPARE(c.flags(), ImageFlag2D::Array);
    CORRADE_COMPARE(c.storage().alignment(), 1);
    CORRADE_COMPARE(c.format(), PixelFormat::RGBA32F);
    CORRADE_COMPARE(c.formatExtra(), 0);
    CORRADE_COMPARE(c.pixelSize(), 16);
    CORRADE_COMPARE(c.size(), (Vector2i{1, 3}));
    CORRADE_COMPARE(c.data(), static_cast<const void*>(data));
    CORRADE_COMPARE(c.data().size(), 3*16);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<Image2D>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<Image2D>::value);
}

void ImageTest::constructMoveImplementationSpecific() {
    auto data = new char[3*6];
    Image2D a{PixelStorage{}.setAlignment(1),
        GL::PixelFormat::RGB, GL::PixelType::UnsignedShort, {1, 3}, Containers::Array<char>{data, 3*6}, ImageFlag2D::Array};
    Image2D b(Utility::move(a));

    CORRADE_COMPARE(a.data(), static_cast<const void*>(nullptr));
    CORRADE_COMPARE(a.size(), Vector2i{});

    CORRADE_COMPARE(b.flags(), ImageFlag2D::Array);
    CORRADE_COMPARE(b.storage().alignment(), 1);
    CORRADE_COMPARE(b.format(), pixelFormatWrap(GL::PixelFormat::RGB));
    CORRADE_COMPARE(b.formatExtra(), 1337);
    CORRADE_COMPARE(b.pixelSize(), 6);
    CORRADE_COMPARE(b.size(), (Vector2i{1, 3}));
    CORRADE_COMPARE(b.data(), static_cast<const void*>(data));
    CORRADE_COMPARE(b.data().size(), 3*6);

    auto data2 = new char[12*4*2];
    Image2D c{PixelStorage{},
        1, 2, 8, {2, 6}, Containers::Array<char>{data2, 12*4*2}};
    c = Utility::move(b);

    CORRADE_COMPARE(b.data(), static_cast<const void*>(data2));
    CORRADE_COMPARE(b.size(), Vector2i(2, 6));

    CORRADE_COMPARE(c.flags(), ImageFlag2D::Array);
    CORRADE_COMPARE(c.storage().alignment(), 1);
    CORRADE_COMPARE(c.format(), pixelFormatWrap(GL::PixelFormat::RGB));
    CORRADE_COMPARE(c.formatExtra(), 1337);
    CORRADE_COMPARE(c.pixelSize(), 6);
    CORRADE_COMPARE(c.size(), (Vector2i{1, 3}));
    CORRADE_COMPARE(c.data(), static_cast<const void*>(data));
    CORRADE_COMPARE(c.data().size(), 3*6);
}

void ImageTest::constructMoveCompressedGeneric() {
    auto data = new char[8];
    CompressedImage2D a{
        CompressedPixelStorage{}.setCompressedBlockSize(Vector3i{4}),
        CompressedPixelFormat::Bc3RGBAUnorm, {4, 4}, Containers::Array<char>{data, 8}, ImageFlag2D::Array};
    CompressedImage2D b{Utility::move(a)};

    CORRADE_COMPARE(a.data(), static_cast<const void*>(nullptr));
    CORRADE_COMPARE(a.size(), Vector2i{});

    CORRADE_COMPARE(b.flags(), ImageFlag2D::Array);
    CORRADE_COMPARE(b.storage().compressedBlockSize(), Vector3i{4});
    CORRADE_COMPARE(b.format(), CompressedPixelFormat::Bc3RGBAUnorm);
    CORRADE_COMPARE(b.size(), (Vector2i{4, 4}));
    CORRADE_COMPARE(b.data(), static_cast<const void*>(data));
    CORRADE_COMPARE(b.data().size(), 8);

    auto data2 = new char[16];
    CompressedImage2D c{CompressedPixelFormat::Bc1RGBAUnorm, {8, 4}, Containers::Array<char>{data2, 16}};
    c = Utility::move(b);

    CORRADE_COMPARE(b.data(), static_cast<const void*>(data2));
    CORRADE_COMPARE(b.size(), (Vector2i{8, 4}));

    CORRADE_COMPARE(c.flags(), ImageFlag2D::Array);
    CORRADE_COMPARE(c.storage().compressedBlockSize(), Vector3i{4});
    CORRADE_COMPARE(c.format(), CompressedPixelFormat::Bc3RGBAUnorm);
    CORRADE_COMPARE(c.size(), (Vector2i{4, 4}));
    CORRADE_COMPARE(c.data(), static_cast<const void*>(data));
    CORRADE_COMPARE(c.data().size(), 8);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<CompressedImage2D>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<CompressedImage2D>::value);
}

void ImageTest::constructMoveCompressedImplementationSpecific() {
    auto data = new char[8];
    CompressedImage2D a{
        CompressedPixelStorage{}.setCompressedBlockSize(Vector3i{4}),
        GL::CompressedPixelFormat::RGBS3tcDxt1, {4, 4}, Containers::Array<char>{data, 8}, ImageFlag2D::Array};
    CompressedImage2D b{Utility::move(a)};

    CORRADE_COMPARE(a.data(), static_cast<const void*>(nullptr));
    CORRADE_COMPARE(a.size(), Vector2i{});

    CORRADE_COMPARE(b.flags(), ImageFlag2D::Array);
    CORRADE_COMPARE(b.storage().compressedBlockSize(), Vector3i{4});
    CORRADE_COMPARE(b.format(), compressedPixelFormatWrap(GL::CompressedPixelFormat::RGBS3tcDxt1));
    CORRADE_COMPARE(b.size(), (Vector2i{4, 4}));
    CORRADE_COMPARE(b.data(), static_cast<const void*>(data));
    CORRADE_COMPARE(b.data().size(), 8);

    auto data2 = new char[16];
    CompressedImage2D c{CompressedPixelFormat::Bc2RGBAUnorm, {8, 4}, Containers::Array<char>{data2, 16}};
    c = Utility::move(b);

    CORRADE_COMPARE(b.data(), static_cast<const void*>(data2));
    CORRADE_COMPARE(b.size(), (Vector2i{8, 4}));

    CORRADE_COMPARE(c.flags(), ImageFlag2D::Array);
    CORRADE_COMPARE(c.storage().compressedBlockSize(), Vector3i{4});
    CORRADE_COMPARE(c.format(), compressedPixelFormatWrap(GL::CompressedPixelFormat::RGBS3tcDxt1));
    CORRADE_COMPARE(c.size(), (Vector2i{4, 4}));
    CORRADE_COMPARE(c.data(), static_cast<const void*>(data));
    CORRADE_COMPARE(c.data().size(), 8);
}

template<class T> void ImageTest::toViewGeneric() {
    setTestCaseTemplateName(MutabilityTraits<T>::name());

    auto data = new char[3*4];
    typename MutabilityTraits<T>::ImageType a{PixelStorage{}.setAlignment(1),
        PixelFormat::RG16I, {1, 3}, Containers::Array<char>{data, 3*4}, ImageFlag2D::Array};
    ImageView<2, T> b = a;

    CORRADE_COMPARE(b.flags(), ImageFlag2D::Array);
    CORRADE_COMPARE(b.storage().alignment(), 1);
    CORRADE_COMPARE(b.format(), PixelFormat::RG16I);
    CORRADE_COMPARE(b.formatExtra(), 0);
    CORRADE_COMPARE(b.pixelSize(), 4);
    CORRADE_COMPARE(b.size(), (Vector2i{1, 3}));
    CORRADE_COMPARE(b.data(), static_cast<const void*>(data));
}

template<class T> void ImageTest::toViewImplementationSpecific() {
    setTestCaseTemplateName(MutabilityTraits<T>::name());

    auto data = new char[3*6];
    typename MutabilityTraits<T>::ImageType a{PixelStorage{}.setAlignment(1),
        GL::PixelFormat::RGB, GL::PixelType::UnsignedShort, {1, 3}, Containers::Array<char>{data, 3*6}, ImageFlag2D::Array};
    ImageView<2, T> b = a;

    CORRADE_COMPARE(b.flags(), ImageFlag2D::Array);
    CORRADE_COMPARE(b.storage().alignment(), 1);
    CORRADE_COMPARE(b.format(), pixelFormatWrap(GL::PixelFormat::RGB));
    CORRADE_COMPARE(b.formatExtra(), 1337);
    CORRADE_COMPARE(b.pixelSize(), 6);
    CORRADE_COMPARE(b.size(), (Vector2i{1, 3}));
    CORRADE_COMPARE(b.data(), static_cast<const void*>(data));
}

template<class T> void ImageTest::toViewCompressedGeneric() {
    setTestCaseTemplateName(MutabilityTraits<T>::name());

    auto data = new char[8];
    typename MutabilityTraits<T>::CompressedImageType a{
        CompressedPixelStorage{}.setCompressedBlockSize(Vector3i{4}),
        CompressedPixelFormat::Bc1RGBUnorm, {4, 4}, Containers::Array<char>{data, 8}, ImageFlag2D::Array};
    CompressedImageView<2, T> b = a;

    CORRADE_COMPARE(b.flags(), ImageFlag2D::Array);
    CORRADE_COMPARE(b.storage().compressedBlockSize(), Vector3i{4});
    CORRADE_COMPARE(b.format(), CompressedPixelFormat::Bc1RGBUnorm);
    CORRADE_COMPARE(b.size(), (Vector2i{4, 4}));
    CORRADE_COMPARE(b.data(), static_cast<const void*>(data));
    CORRADE_COMPARE(b.data().size(), 8);
}

template<class T> void ImageTest::toViewCompressedImplementationSpecific() {
    setTestCaseTemplateName(MutabilityTraits<T>::name());

    auto data = new char[8];
    typename MutabilityTraits<T>::CompressedImageType a{
        CompressedPixelStorage{}.setCompressedBlockSize(Vector3i{4}),
        GL::CompressedPixelFormat::RGBS3tcDxt1, {4, 4}, Containers::Array<char>{data, 8}, ImageFlag2D::Array};
    CompressedImageView<2, T> b = a;

    CORRADE_COMPARE(b.flags(), ImageFlag2D::Array);
    CORRADE_COMPARE(b.storage().compressedBlockSize(), Vector3i{4});
    CORRADE_COMPARE(b.format(), compressedPixelFormatWrap(GL::CompressedPixelFormat::RGBS3tcDxt1));
    CORRADE_COMPARE(b.size(), (Vector2i{4, 4}));
    CORRADE_COMPARE(b.data(), static_cast<const void*>(data));
    CORRADE_COMPARE(b.data().size(), 8);
}

void ImageTest::data() {
    auto data = new char[4*4];
    Image2D a{PixelFormat::RGBA8Unorm, {1, 3}, Containers::Array<char>{data, 4*4}};
    const Image2D& ca = a;
    CORRADE_COMPARE(a.data(), static_cast<const void*>(data));
    CORRADE_COMPARE(ca.data(), static_cast<const void*>(data));
}

void ImageTest::dataCompressed() {
    auto data = new char[8];
    CompressedImage2D a{CompressedPixelFormat::Bc1RGBAUnorm, {4, 4},
        Containers::Array<char>{data, 8}};
    const CompressedImage2D& ca = a;
    CORRADE_COMPARE(a.data(), static_cast<const void*>(data));
    CORRADE_COMPARE(ca.data(), static_cast<const void*>(data));
}

void ImageTest::dataRvalue() {
    auto data = new char[4*4];
    Containers::Array<char> released = Image2D{PixelFormat::RGBA8Unorm, {1, 3},
        Containers::Array<char>{data, 4*4}}.data();
    CORRADE_COMPARE(released.data(), static_cast<const void*>(data));
}

void ImageTest::dataRvalueCompressed() {
    auto data = new char[8];
    Containers::Array<char> released = CompressedImage2D{
        CompressedPixelFormat::Bc1RGBAUnorm, {4, 4},
        Containers::Array<char>{data, 8}}.data();
    CORRADE_COMPARE(released.data(), static_cast<const void*>(data));
}

void ImageTest::dataProperties() {
    Image3D image{
        PixelStorage{}
            .setAlignment(8)
            .setSkip({3, 2, 1}),
        PixelFormat::R8Unorm, {2, 4, 6},
        Containers::Array<char>{224}};
    CORRADE_COMPARE(image.dataProperties(),
        (std::pair<Math::Vector3<std::size_t>, Math::Vector3<std::size_t>>{{3, 16, 32}, {8, 4, 6}}));
}

void ImageTest::dataPropertiesCompressed() {
    /* Yes, I know, this is totally bogus and doesn't match the BC1 format */
    CompressedImage3D image{
        CompressedPixelStorage{}
            .setCompressedBlockSize({3, 4, 5})
            .setCompressedBlockDataSize(16)
            .setImageHeight(12)
            .setSkip({5, 8, 11}),
        CompressedPixelFormat::Bc1RGBAUnorm, {2, 8, 11},
        Containers::Array<char>{1}};
    CORRADE_COMPARE(image.dataProperties(),
        (std::pair<Math::Vector3<std::size_t>, Math::Vector3<std::size_t>>{{2*16, 2*16, 9*16}, {1, 3, 3}}));
}

void ImageTest::release() {
    char data[] = {'c', 'a', 'f', 'e'};
    Image2D a(PixelFormat::RGBA8Unorm, {1, 1}, Containers::Array<char>{data, 4});
    const char* const pointer = a.release().release();

    CORRADE_COMPARE(pointer, data);
    CORRADE_COMPARE(a.data(), static_cast<const void*>(nullptr));
    CORRADE_COMPARE(a.size(), Vector2i());
}

void ImageTest::releaseCompressed() {
    char data[8];
    CompressedImage2D a{CompressedPixelFormat::Bc1RGBAUnorm, {4, 4}, Containers::Array<char>{data, 8}};
    const char* const pointer = a.release().release();

    CORRADE_COMPARE(pointer, data);
    CORRADE_COMPARE(a.data(), static_cast<const void*>(nullptr));
    CORRADE_COMPARE(a.size(), Vector2i());
}

void ImageTest::pixels1D() {
    Image1D image{
        PixelStorage{}
            .setAlignment(1) /** @todo alignment 4 expects 17 bytes. what */
            .setSkip({3, 0, 0}),
        PixelFormat::RGB8Unorm, 2,
        Containers::Array<char>{InPlaceInit, {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 4, 5, 6, 7, 8
        }}};
    const Image1D& cimage = image;

    {
        Containers::StridedArrayView1D<Color3ub> pixels = image.pixels<Color3ub>();
        CORRADE_COMPARE(pixels.size(), 2);
        CORRADE_COMPARE(pixels.stride(), 3);
        CORRADE_COMPARE(pixels.data(), image.data() + 3*3);
        CORRADE_COMPARE(pixels[0], (Color3ub{3, 4, 5}));
        CORRADE_COMPARE(pixels[1], (Color3ub{6, 7, 8}));
    } {
        Containers::StridedArrayView1D<const Color3ub> pixels = cimage.pixels<Color3ub>();
        CORRADE_COMPARE(pixels.size(), 2);
        CORRADE_COMPARE(pixels.stride(), 3);
        CORRADE_COMPARE(pixels.data(), cimage.data() + 3*3);
        CORRADE_COMPARE(pixels[0], (Color3ub{3, 4, 5}));
        CORRADE_COMPARE(pixels[1], (Color3ub{6, 7, 8}));
    }
}

void ImageTest::pixels2D() {
    Image2D image{
        PixelStorage{}
            .setAlignment(4)
            .setSkip({3, 2, 0})
            .setRowLength(6),
        PixelFormat::RGB8Unorm, {2, 4},
        Containers::Array<char>{InPlaceInit, {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 4, 5, 6, 7, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 4, 5, 6, 7, 8, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0,
        }}};
    const Image2D& cimage = image;

    {
        Containers::StridedArrayView2D<Color3ub> pixels = image.pixels<Color3ub>();
        CORRADE_COMPARE(pixels.size(), (Containers::Size2D{4, 2}));
        CORRADE_COMPARE(pixels.stride(), (Containers::Stride2D{20, 3}));
        CORRADE_COMPARE(pixels.data(), image.data() + 2*20 + 3*3);
        CORRADE_COMPARE(pixels[3][0], (Color3ub{4, 5, 6}));
        CORRADE_COMPARE(pixels[3][1], (Color3ub{7, 8, 9}));
    } {
        Containers::StridedArrayView2D<const Color3ub> pixels = cimage.pixels<Color3ub>();
        CORRADE_COMPARE(pixels.size(), (Containers::Size2D{4, 2}));
        CORRADE_COMPARE(pixels.stride(), (Containers::Stride2D{20, 3}));
        CORRADE_COMPARE(pixels.data(), cimage.data() + 2*20 + 3*3);
        CORRADE_COMPARE(pixels[3][0], (Color3ub{4, 5, 6}));
        CORRADE_COMPARE(pixels[3][1], (Color3ub{7, 8, 9}));
    }
}

void ImageTest::pixels3D() {
    Image3D image{
        PixelStorage{}
            .setAlignment(4)
            .setSkip({3, 2, 1})
            .setRowLength(6)
            .setImageHeight(7),
        PixelFormat::RGB8Unorm, {2, 4, 3},
        Containers::Array<char>{InPlaceInit, {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 4, 5, 6, 7, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 4, 5, 6, 7, 8, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 6, 5, 4, 3, 2, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 7, 6, 5, 4, 3, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 8, 7, 6, 5, 4, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 5, 6, 1, 2, 3, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 6, 7, 2, 3, 4, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 7, 8, 3, 4, 5, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 8, 9, 4, 5, 6, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        }}};
    const Image3D& cimage = image;

    {
        Containers::StridedArrayView3D<Color3ub> pixels = image.pixels<Color3ub>();
        CORRADE_COMPARE(pixels.size(), (Containers::Size3D{3, 4, 2}));
        CORRADE_COMPARE(pixels.stride(), (Containers::Stride3D{140, 20, 3}));
        CORRADE_COMPARE(pixels.data(), image.data() + 140 + 2*20 + 3*3);
        CORRADE_COMPARE(pixels[1][3][0], (Color3ub{9, 8, 7}));
        CORRADE_COMPARE(pixels[1][3][1], (Color3ub{6, 5, 4}));
    } {
        Containers::StridedArrayView3D<const Color3ub> pixels = cimage.pixels<Color3ub>();
        CORRADE_COMPARE(pixels.size(), (Containers::Size3D{3, 4, 2}));
        CORRADE_COMPARE(pixels.stride(), (Containers::Stride3D{140, 20, 3}));
        CORRADE_COMPARE(pixels.data(), cimage.data() + 140 + 2*20 + 3*3);
        CORRADE_COMPARE(pixels[1][3][0], (Color3ub{9, 8, 7}));
        CORRADE_COMPARE(pixels[1][3][1], (Color3ub{6, 5, 4}));
    }
}

}}}

CORRADE_TEST_MAIN(Magnum::Test::ImageTest)
