/*
 *  Copyright (C) 2012 Andrew Bloomgarden. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "SecurityTagObject.h"

#include "JSGlobalObject.h"
#include "SecurityTagPrototype.h"

namespace JSC {
    
    ASSERT_CLASS_FITS_IN_CELL(SecurityTagObject);
    ASSERT_HAS_TRIVIAL_DESTRUCTOR(SecurityTagObject);
    
    const ClassInfo SecurityTagObject::s_info = { "SecurityTag", &JSNonFinalObject::s_info, 0, 0, CREATE_METHOD_TABLE(SecurityTagObject) };
    
    SecurityTagObject::SecurityTagObject(JSGlobalData& globalData, Structure* structure, WTF::SecurityTag tag)
    : JSNonFinalObject(globalData, structure)
    , m_tag(tag)
    {
    }
    
    void SecurityTagObject::finishCreation(JSGlobalData& globalData)
    {
        Base::finishCreation(globalData);
        ASSERT(inherits(&s_info));
    }
    
    SecurityTagObject* constructSecurityTag(ExecState* exec, JSGlobalObject* globalObject)
    {
        SecurityTagObject* object = SecurityTagObject::create(exec->globalData(), globalObject->securityTagStructure());
        return object;
    }

    SecurityTagObject* constructSecurityTag(ExecState* exec, JSGlobalObject* globalObject, WTF::SecurityTag tag)
    {
        SecurityTagObject* object = SecurityTagObject::create(exec->globalData(), globalObject->securityTagStructure(), tag);
        return object;
    }
    
} // namespace JSC
