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

#include <wtf/CurrentTime.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/RefCounted.h>
#include <wtf/SecurityTag.h>
#include <wtf/text/StringImpl.h>

#define WTF_CHARACTERS_PER_SECURITY_TAG (sizeof(SecurityTag) / sizeof(UChar))

namespace WTF {

template<> struct DefaultHash<SecurityTag> {
    typedef FloatHash<double> Hash;
};
template<> struct HashTraits<SecurityTag> : FloatHashTraits<double> { };

class SecurityLabelImpl : public RefCounted<SecurityLabelImpl> {
private:
    typedef HashSet<SecurityTag> SecurityTagSet;
public:
    StringImpl* descriptor() const { return m_descriptor.get(); }
    bool hasTag(const SecurityTag&);
    bool hasLabel(const PassRefPtr<SecurityLabelImpl>& other);
    
    WTF_EXPORT_PRIVATE static PassRefPtr<SecurityLabelImpl> add(PassRefPtr<SecurityLabelImpl>, const SecurityTag&);
    WTF_EXPORT_PRIVATE static PassRefPtr<SecurityLabelImpl> combine(PassRefPtr<SecurityLabelImpl>, const PassRefPtr<SecurityLabelImpl>& other);
    
    static PassRefPtr<SecurityLabelImpl> create()
    {
        return adoptRef(new SecurityLabelImpl());
    }
    static PassRefPtr<SecurityLabelImpl> create(PassRefPtr<StringImpl> descriptor)
    {
        return adoptRef(new SecurityLabelImpl(descriptor));
    }
    
    WTF_EXPORT_PRIVATE ~SecurityLabelImpl();
private:
    SecurityLabelImpl() : m_setCreated(false) { }
    SecurityLabelImpl(PassRefPtr<StringImpl> descriptor) : m_descriptor(descriptor), m_setCreated(false) { }

    WTF_EXPORT_PRIVATE void createSet();

    RefPtr<StringImpl> m_descriptor;
    SecurityTagSet m_tagSet;
    HashMap<RefPtr<StringImpl>, RefPtr<StringImpl> > m_transitionTable;
    HashMap<SecurityTag, RefPtr<StringImpl> > m_tagTransitionTable;
    bool m_setCreated;
    friend class SecurityLabelTable;
};

inline bool SecurityLabelImpl::hasTag(const SecurityTag& tag)
{
    if (UNLIKELY(!m_setCreated))
        createSet();

    return m_tagSet.contains(tag);
}

inline bool SecurityLabelImpl::hasLabel(const PassRefPtr<SecurityLabelImpl>& other)
{
    if (UNLIKELY(!m_setCreated))
        createSet();
    
    RefPtr<StringImpl> descriptor = other->m_descriptor;
    size_t length = descriptor->length() / WTF_CHARACTERS_PER_SECURITY_TAG;
    const SecurityTag* descriptorTags = reinterpret_cast<const SecurityTag*>(descriptor->characters16());
    for (size_t i = 0; i < length; i++)
        if (!m_tagSet.contains(descriptorTags[i]))
            return false;
    return true;
}

}

#endif
