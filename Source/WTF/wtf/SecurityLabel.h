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
        StringImpl* descriptor() const { return m_impl ? m_impl->descriptor() : StringImpl::empty(); }

        void add(const SecurityTag& tag);        
        bool hasTag(const SecurityTag& tag) const;
        bool hasLabel(const SecurityLabel& other) const;
        void merge(const SecurityLabel& other);
    private:
        RefPtr<SecurityLabelImpl> m_impl;
    };
    
    inline void SecurityLabel::add(const SecurityTag& tag) {
        if (hasTag(tag))
            return;
        
        m_impl = SecurityLabelImpl::add(m_impl, tag);
    }
    
    inline bool SecurityLabel::hasTag(const SecurityTag& tag) const {
        return !isNull() && m_impl->hasTag(tag);
    }
    
    inline bool SecurityLabel::hasLabel(const SecurityLabel& other) const {
        if (isNull() || other.isNull())
            return other.isNull();
        return m_impl->hasLabel(other.m_impl);
    }
    
    inline void SecurityLabel::merge(const SecurityLabel& other) {
        if (other.isNull() || m_impl == other.m_impl)
            return;
        
        if (isNull())
            m_impl = other.m_impl;
        else
            m_impl = SecurityLabelImpl::combine(m_impl, other.m_impl);
    }
}

using WTF::SecurityLabel;

#include <wtf/text/WTFString.h>
#endif
