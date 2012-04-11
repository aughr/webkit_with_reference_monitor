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
#include <wtf/SecurityLabel.h>

namespace WTF {
    PassRefPtr<SecurityLabelImpl> SecurityLabelImpl::combine(const RefPtr<SecurityLabelImpl>& other) {
        SecurityTagSet& otherSet = other->m_tagSet;
        RefPtr<SecurityLabelImpl> result = new SecurityLabelImpl();
        result->m_tagSet = m_tagSet;
        for (SecurityTagSet::iterator it = otherSet.begin(); it != otherSet.end(); ++it) {
            result->m_tagSet.add(*it);
        }
        return result.release();
    }
    
    PassRefPtr<SecurityLabelImpl> SecurityLabelImpl::duplicate() {
        RefPtr<SecurityLabelImpl> result = new SecurityLabelImpl();
        result->m_tagSet = m_tagSet;
        return result.release();
    }
    
    void SecurityLabel::add(const SecurityTag& tag) {
        if (hasTag(tag))
            return;
        
        duplicateOrInit();
        m_impl->add(tag);
    }
    
    bool SecurityLabel::hasTag(const SecurityTag& tag) const {
        return !isNull() && m_impl->hasTag(tag);
    }
    
    void SecurityLabel::merge(const SecurityLabel& other) {
        if (other.isNull())
            return;
        
        if (isNull())
            m_impl = other.m_impl;
        else
            m_impl = m_impl->combine(other.m_impl);
    }
    
    void SecurityLabel::duplicateOrInit() {
        if (isNull())
            m_impl = SecurityLabelImpl::create();
        else {
            m_impl = m_impl->duplicate();
            
        }                
    }
}
