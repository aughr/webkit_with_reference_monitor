/*
 * This file is part of the internal font implementation.  It should not be included by anyone other than
 * FontMac.cpp, FontWin.cpp and Font.cpp.
 *
 * Copyright (C) 2006, 2007, 2008 Apple Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef FontPlatformData_H
#define FontPlatformData_H

#include "StringImpl.h"

#if PLATFORM(CAIRO)
#include <cairo-win32.h>
#endif

typedef struct HFONT__* HFONT;
typedef struct CGFont* CGFontRef;

namespace WebCore {

class FontDescription;

class FontPlatformData
{
public:
    class Deleted {};

    // Used for deleted values in the font cache's hash tables.
    FontPlatformData(Deleted)
        : m_font((HFONT)-1)
#if PLATFORM(CG)
        , m_cgFont(0)
#elif PLATFORM(CAIRO)
        , m_fontFace(0)
#endif
        , m_size(0)
        , m_syntheticBold(false)
        , m_syntheticOblique(false)
        , m_useGDI(false)
    {
    }

    FontPlatformData()
        : m_font(0)
#if PLATFORM(CG)
        , m_cgFont(0)
#elif PLATFORM(CAIRO)
        , m_fontFace(0)
#endif
        , m_size(0)
        , m_syntheticBold(false)
        , m_syntheticOblique(false)
        , m_useGDI(false)
    {
    }

    FontPlatformData(HFONT, float size, bool bold, bool oblique, bool useGDI);
    FontPlatformData(float size, bool bold, bool oblique);

#if PLATFORM(CG)
    FontPlatformData(CGFontRef, float size, bool bold, bool oblique);
#elif PLATFORM(CAIRO)
    FontPlatformData(cairo_font_face_t*, float size, bool bold, bool oblique);
#endif
    ~FontPlatformData();

    void platformDataInit(HFONT font, float size, HDC hdc, WCHAR* faceName);

    HFONT hfont() const { return m_font; }
#if PLATFORM(CG)
    CGFontRef cgFont() const { return m_cgFont; }
#elif PLATFORM(CAIRO)
    void setFont(cairo_t* ft) const;
    cairo_font_face_t* fontFace() const { return m_fontFace; }
    cairo_scaled_font_t* scaledFont() const { return m_scaledFont; }
#endif

    float size() const { return m_size; }
    void setSize(float size) { m_size = size; }
    bool syntheticBold() const { return m_syntheticBold; }
    bool syntheticOblique() const { return m_syntheticOblique; }
    bool useGDI() const { return m_useGDI; }

    unsigned hash() const
    {
        return StringImpl::computeHash((UChar*)(&m_font), sizeof(HFONT) / sizeof(UChar));
    }

    bool operator==(const FontPlatformData& other) const
    { 
        return m_font == other.m_font &&
#if PLATFORM(CG)
               m_cgFont == other.m_cgFont &&
#elif PLATFORM(CAIRO)
               m_fontFace == other.m_fontFace &&
               m_scaledFont == other.m_scaledFont &&
#endif
               m_size == other.m_size &&
               m_syntheticBold == other.m_syntheticBold && m_syntheticOblique == other.m_syntheticOblique &&
               m_useGDI == other.m_useGDI;
    }

private:
    HFONT m_font;
#if PLATFORM(CG)
    CGFontRef m_cgFont;
#elif PLATFORM(CAIRO)
    cairo_font_face_t* m_fontFace;
    cairo_scaled_font_t* m_scaledFont;
#endif

    float m_size;
    bool m_syntheticBold;
    bool m_syntheticOblique;
    bool m_useGDI;
};

}

#endif
