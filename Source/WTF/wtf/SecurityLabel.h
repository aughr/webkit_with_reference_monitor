/*
 * Copyright (C) 2012 Andrew Bloomgarden. All rights reserved.
 * Copyright (C) 2008 David Levin <levin@chromium.org>
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

#ifndef WTF_SecurityLabel_h
#define WTF_SecurityLabel_h

#include <wtf/RefCounted.h>
#include <wtf/CurrentTime.h>
#include <wtf/SecurityLabelImpl.h>
#include <wtf/SecurityTag.h>

namespace WTF {
    class SecurityLabel {
    public:
        SecurityLabel() : m_impl(0) {}

        bool isNull() const { return m_impl == NULL; }
        SecurityLabelImpl* get() const { return m_impl.get(); }

        WTF_EXPORT_PRIVATE void add(const SecurityTag& tag);        
        WTF_EXPORT_PRIVATE bool hasTag(const SecurityTag& tag) const;
        WTF_EXPORT_PRIVATE bool hasLabel(const SecurityLabel& other) const;
        WTF_EXPORT_PRIVATE void merge(const SecurityLabel& other);
    private:
        void duplicateOrInit();

        RefPtr<SecurityLabelImpl> m_impl;
    };
}

using WTF::SecurityLabel;

#endif
