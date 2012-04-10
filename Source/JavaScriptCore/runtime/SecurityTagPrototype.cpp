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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 *
 */

#include "config.h"
#include "SecurityTagPrototype.h"

#include "BigInteger.h"
#include "Error.h"
#include "JSFunction.h"
#include "JSGlobalObject.h"
#include <wtf/Assertions.h>
#include <wtf/CurrentTime.h>

namespace JSC {
    
    static EncodedJSValue JSC_HOST_CALL securityTagProtoFuncToString(ExecState*);
    
}

#include "SecurityTagPrototype.lut.h"

namespace JSC {
    
    const ClassInfo SecurityTagPrototype::s_info = { "SecurityTag", &SecurityTagObject::s_info, 0, ExecState::securityTagPrototypeTable, CREATE_METHOD_TABLE(SecurityTagPrototype) };
    
    /* Source for SecurityTagPrototype.lut.h
     @begin securityTagPrototypeTable
     toString          securityTagProtoFuncToString         DontEnum|Function 0
     @end
     */
    
    ASSERT_CLASS_FITS_IN_CELL(SecurityTagPrototype);
    ASSERT_HAS_TRIVIAL_DESTRUCTOR(SecurityTagPrototype);
    
    SecurityTagPrototype::SecurityTagPrototype(ExecState* exec, Structure* structure)
    : SecurityTagObject(exec->globalData(), structure)
    {
    }
    
    void SecurityTagPrototype::finishCreation(ExecState* exec, JSGlobalObject*)
    {
        Base::finishCreation(exec->globalData());
        setInternalValue(exec->globalData(), jsNumber(0));
        
        ASSERT(inherits(&s_info));
    }
    
    bool SecurityTagPrototype::getOwnPropertySlot(JSCell* cell, ExecState* exec, const Identifier& propertyName, PropertySlot &slot)
    {
        return getStaticFunctionSlot<SecurityTagObject>(exec, ExecState::securityTagPrototypeTable(exec), jsCast<SecurityTagPrototype*>(cell), propertyName, slot);
    }
    
    bool SecurityTagPrototype::getOwnPropertyDescriptor(JSObject* object, ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
    {
        return getStaticFunctionDescriptor<SecurityTagObject>(exec, ExecState::securityTagPrototypeTable(exec), jsCast<SecurityTagPrototype*>(object), propertyName, descriptor);
    }
    
    // ------------------------------ Functions ---------------------------
    
    EncodedJSValue JSC_HOST_CALL securityTagProtoFuncToString(ExecState* exec)
    {
        return JSValue::encode(jsString(exec, "SecurityTag"));
    }

} // namespace JSC
