/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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
 */

#ifndef PageClientEfl_h
#define PageClientEfl_h

#include "IntRect.h"

typedef struct _Evas_Native_Surface Evas_Native_Surface;
typedef struct _Evas_Object Evas_Object;

namespace WebCore {

class GraphicsContext3D;

class PageClientEfl {
public:
    PageClientEfl(Evas_Object* view);
    virtual ~PageClientEfl();

#if USE(ACCELERATED_COMPOSITING)
    bool createEvasObjectForAcceleratedCompositing(Evas_Native_Surface*, const IntRect&);
    GraphicsContext3D* acceleratedCompositingContext();
#endif

    Evas_Object* view() { return m_view; }

protected:
    Evas_Object* m_view;
};

} // namespace WebCore

#endif // PageClientEfl_h
