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

#include "config.h"
#include "SecurityLabelImpl.h"

#include "AtomicString.h"
#include "HashMap.h"
#include "Vector.h"
#include "WTFThreadData.h"
#include <algorithm>
#include <wtf/text/StringBuilder.h>

namespace WTF {

class SecurityLabelTable {
public:
    typedef HashMap<RefPtr<StringImpl>, SecurityLabelImpl*> Table;
    static SecurityLabelTable* create()
    {
        SecurityLabelTable* table = new SecurityLabelTable;
        
        WTFThreadData& data = wtfThreadData();
        data.m_securityLabelTable = table;
        data.m_securityLabelTableDestructor = SecurityLabelTable::destroy;
        
        return table;
    }
    
    Table& table()
    {
        return m_table;
    }
    
private:
    static void destroy(SecurityLabelTable* table)
    {
        delete table;
    }
    
    Table m_table;
};

static inline SecurityLabelTable::Table& labelTable()
{
    // Once possible we should make this non-lazy (constructed in WTFThreadData's constructor).
    SecurityLabelTable* table = wtfThreadData().securityLabelTable();
    if (UNLIKELY(!table))
        table = SecurityLabelTable::create();
    return table->table();
}

static inline PassRefPtr<SecurityLabelImpl> lookupOrCreateForDescriptor(PassRefPtr<StringImpl> prpDescriptor)
{
    RefPtr<StringImpl> descriptor = prpDescriptor;

    SecurityLabelImpl* impl = labelTable().get(descriptor);
    if (impl)
        return impl;

    RefPtr<SecurityLabelImpl> newImpl = SecurityLabelImpl::create(descriptor);
    labelTable().add(descriptor, newImpl.get());
    return newImpl;
}

static inline PassRefPtr<SecurityLabelImpl> lookupOrCreateForTag(const SecurityTag& tag) 
{
    StringBuilder builder;
    const UChar *chars = reinterpret_cast<const UChar*>(&tag);
    builder.append(chars, WTF_CHARACTERS_PER_SECURITY_TAG);
    AtomicString newDescriptor = builder.toAtomicString();
    ASSERT((double)tag == *reinterpret_cast<const double*>(newDescriptor.impl()->characters16()));
    return lookupOrCreateForDescriptor(newDescriptor.string().impl());
}

SecurityLabelImpl::~SecurityLabelImpl()
{
    labelTable().remove(m_descriptor);
}

PassRefPtr<SecurityLabelImpl> SecurityLabelImpl::add(PassRefPtr<SecurityLabelImpl> prpImpl, const SecurityTag& tag)
{
    RefPtr<SecurityLabelImpl> impl = prpImpl;
    if (!impl)
        return lookupOrCreateForTag(tag);

    if (impl->hasTag(tag))
        return impl;


    RefPtr<StringImpl> descriptor = impl->m_tagTransitionTable.get(tag);
    if (descriptor)
        return lookupOrCreateForDescriptor(descriptor);

    descriptor = impl->m_descriptor;
    Vector<SecurityTag> tags;

    // copy the tags into a vector and sort them
    ASSERT(!(descriptor->length() % WTF_CHARACTERS_PER_SECURITY_TAG));
    size_t length = descriptor->length() / WTF_CHARACTERS_PER_SECURITY_TAG;
    const SecurityTag* descriptorTags = reinterpret_cast<const SecurityTag*>(descriptor->characters16());
    for (size_t i = 0; i < length; i++)
        tags.append(descriptorTags[i]);
    tags.append(tag);
    std::sort(tags.begin(), tags.end());
    
    StringBuilder builder;
    for (size_t i = 0; i < tags.size(); i++) {
        UChar* chars = reinterpret_cast<UChar*>(&tags[i]);
        builder.append(chars, WTF_CHARACTERS_PER_SECURITY_TAG);
    }
    
    AtomicString newDescriptor = builder.toAtomicString();
    impl->m_tagTransitionTable.add(tag, newDescriptor.impl());
    return lookupOrCreateForDescriptor(newDescriptor.impl());
}

PassRefPtr<SecurityLabelImpl> SecurityLabelImpl::combine(PassRefPtr<SecurityLabelImpl> prpImpl, const PassRefPtr<SecurityLabelImpl>& prpOther)
{
    RefPtr<SecurityLabelImpl> impl = prpImpl;
    RefPtr<SecurityLabelImpl> other = prpOther;
    if (!impl)
        return other;
    
    if (impl->hasLabel(other))
        return impl;

    RefPtr<StringImpl> descriptor = impl->m_transitionTable.get(other->m_descriptor);
    if (descriptor)
        return lookupOrCreateForDescriptor(descriptor);

    descriptor = impl->m_descriptor;
    RefPtr<StringImpl> descriptor2 = other->m_descriptor;
    Vector<SecurityTag> tags;
    
    // copy the tags into a vector and sort them
    ASSERT(!(descriptor->length() % WTF_CHARACTERS_PER_SECURITY_TAG));
    ASSERT(!(descriptor2->length() % WTF_CHARACTERS_PER_SECURITY_TAG));

    size_t length = descriptor->length() / WTF_CHARACTERS_PER_SECURITY_TAG;
    const SecurityTag* descriptorTags = reinterpret_cast<const SecurityTag*>(descriptor->characters16());
    for (size_t i = 0; i < length; i++)
        tags.append(descriptorTags[i]);

    length = descriptor2->length() / WTF_CHARACTERS_PER_SECURITY_TAG;
    descriptorTags = reinterpret_cast<const SecurityTag*>(descriptor2->characters16());
    for (size_t i = 0; i < length; i++)
        tags.append(descriptorTags[i]);
    std::sort(tags.begin(), tags.end());
    std::unique(tags.begin(), tags.end());
    
    StringBuilder builder;
    for (size_t i = 0; i < tags.size(); i++) {
        UChar* chars = reinterpret_cast<UChar*>(&tags[i]);
        builder.append(chars, WTF_CHARACTERS_PER_SECURITY_TAG);
    }
    
    AtomicString newDescriptor = builder.toAtomicString();
    impl->m_transitionTable.add(other->m_descriptor, newDescriptor.impl());
    return lookupOrCreateForDescriptor(newDescriptor.impl());
}

void SecurityLabelImpl::createSet()
{
    RefPtr<StringImpl> descriptor = m_descriptor;
    size_t length = descriptor->length() / (sizeof(SecurityTag) / sizeof(UChar));
    const SecurityTag* descriptorTags = reinterpret_cast<const SecurityTag*>(descriptor->characters16());
    for (size_t i = 0; i < length; i++)
        m_tagSet.add(descriptorTags[i]);
    m_setCreated = true;
}

} // namespace WTF
