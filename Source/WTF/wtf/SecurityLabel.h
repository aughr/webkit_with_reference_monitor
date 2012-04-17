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

#ifndef WTF_SecurityLabelImpl_h
#define WTF_SecurityLabelImpl_h

#include <wtf/HashSet.h>
#include <wtf/RefCounted.h>
#include <wtf/CurrentTime.h>

namespace WTF {
    
    struct SecurityTag {
    public:
        SecurityTag() : m_impl(monotonicallyIncreasingTime()) {}
        inline operator double() const {
            return m_impl;
        }
    private:
        double m_impl;
    };
    
    template<> struct DefaultHash<SecurityTag> { typedef FloatHash<double> Hash; };
    template<> struct HashTraits<SecurityTag> : FloatHashTraits<double> { };

    class SecurityLabelImpl : public RefCounted<SecurityLabelImpl> {
    private:
        typedef HashSet<SecurityTag> SecurityTagSet;
    public:
        void add(const SecurityTag& tag);

        bool hasTag(const SecurityTag& tag) const;

        PassRefPtr<SecurityLabelImpl> combine(const RefPtr<SecurityLabelImpl>& other);
        PassRefPtr<SecurityLabelImpl> duplicate();

        static inline PassRefPtr<SecurityLabelImpl> create()
        {
            return adoptRef(new SecurityLabelImpl());
        }

    private:
        SecurityLabelImpl() {}
        SecurityLabelImpl(SecurityTagSet set) : m_tagSet(set) {}
        static inline PassRefPtr<SecurityLabelImpl> create(SecurityTagSet set)
        {
            return adoptRef(new SecurityLabelImpl(set));
        }
    private:
        SecurityTagSet m_tagSet;
    };

    class SecurityLabel {
    public:
        SecurityLabel() : m_impl(0) {}

        bool isNull() const { return m_impl == NULL; }

        void add(const SecurityTag& tag);        
        bool hasTag(const SecurityTag& tag) const;
        WTF_EXPORT_PRIVATE void merge(const SecurityLabel& other);
    private:
        void duplicateOrInit();

        RefPtr<SecurityLabelImpl> m_impl;
    };
    
    inline void SecurityLabelImpl::add(const SecurityTag& tag) {
        m_tagSet.add(tag); 
    }
    
    inline bool SecurityLabelImpl::hasTag(const SecurityTag& tag) const {
        return m_tagSet.contains(tag);
    }
}

using WTF::SecurityLabel;

#endif
