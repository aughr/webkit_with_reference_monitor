/*
 *  Copyright (C) 1999-2000,2003 Harri Porten (porten@kde.org)
 *  Copyright (C) 2007, 2008, 2011 Apple Inc. All rights reserved.
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 *
 */

#include "config.h"
#include "SecurityTagConstructor.h"

#include "Lookup.h"
#include "SecurityTagObject.h"
#include "SecurityTagPrototype.h"

namespace JSC {
    
    ASSERT_CLASS_FITS_IN_CELL(SecurityTagConstructor);
    
} // namespace JSC

namespace JSC {
    
    ASSERT_HAS_TRIVIAL_DESTRUCTOR(SecurityTagConstructor);
    
    const ClassInfo SecurityTagConstructor::s_info = { "Function", &InternalFunction::s_info, 0, 0, CREATE_METHOD_TABLE(SecurityTagConstructor) };
    
    SecurityTagConstructor::SecurityTagConstructor(JSGlobalObject* globalObject, Structure* structure)
    : InternalFunction(globalObject, structure) 
    {
    }
    
    void SecurityTagConstructor::finishCreation(ExecState* exec, SecurityTagPrototype* securityTagPrototype)
    {
        Base::finishCreation(exec->globalData(), Identifier(exec, securityTagPrototype->s_info.className));
        ASSERT(inherits(&s_info));
        
        // SecurityTag.Prototype
        putDirectWithoutTransition(exec->globalData(), exec->propertyNames().prototype, securityTagPrototype, DontEnum | DontDelete | ReadOnly);
        
        // no. of arguments for constructor
        putDirectWithoutTransition(exec->globalData(), exec->propertyNames().length, jsNumber(0), ReadOnly | DontEnum | DontDelete);
    }
    
    // ECMA 15.7.1
    static EncodedJSValue JSC_HOST_CALL constructWithSecurityTagConstructor(ExecState* exec)
    {
        SecurityTagObject* object = SecurityTagObject::create(exec->globalData(), asInternalFunction(exec->callee())->globalObject()->securityTagStructure());
        return JSValue::encode(object);
    }
    
    ConstructType SecurityTagConstructor::getConstructData(JSCell*, ConstructData& constructData)
    {
        constructData.native.function = constructWithSecurityTagConstructor;
        return ConstructTypeHost;
    }
    
    // ECMA 15.7.2
    static EncodedJSValue JSC_HOST_CALL callSecurityTagConstructor(ExecState* exec)
    {
        SecurityTagObject* object = SecurityTagObject::create(exec->globalData(), asInternalFunction(exec->callee())->globalObject()->securityTagStructure());
        return JSValue::encode(object);
    }
    
    CallType SecurityTagConstructor::getCallData(JSCell*, CallData& callData)
    {
        callData.native.function = callSecurityTagConstructor;
        return CallTypeHost;
    }
    
} // namespace JSC
