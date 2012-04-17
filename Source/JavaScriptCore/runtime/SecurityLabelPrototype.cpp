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
#include "SecurityLabelPrototype.h"

#include "BigInteger.h"
#include "Error.h"
#include "JSFunction.h"
#include "JSGlobalObject.h"
#include <wtf/Assertions.h>

namespace JSC {
    
    static EncodedJSValue JSC_HOST_CALL securityLabelProtoFuncToString(ExecState*);
    
}

#include "SecurityLabelPrototype.lut.h"

namespace JSC {
    
    const ClassInfo SecurityLabelPrototype::s_info = { "SecurityLabel", &SecurityLabelObject::s_info, 0, ExecState::securityLabelPrototypeTable, CREATE_METHOD_TABLE(SecurityLabelPrototype) };
    
    /* Source for SecurityLabelPrototype.lut.h
     @begin securityLabelPrototypeTable
     toString          securityLabelProtoFuncToString         DontEnum|Function 0
     @end
     */
    
    ASSERT_CLASS_FITS_IN_CELL(SecurityLabelPrototype);
    
    SecurityLabelPrototype::SecurityLabelPrototype(ExecState* exec, Structure* structure)
    : SecurityLabelObject(exec->globalData(), structure)
    {
    }
    
    void SecurityLabelPrototype::finishCreation(ExecState* exec, JSGlobalObject*)
    {
        Base::finishCreation(exec->globalData());
        
        ASSERT(inherits(&s_info));
    }
    
    bool SecurityLabelPrototype::getOwnPropertySlot(JSCell* cell, ExecState* exec, const Identifier& propertyName, PropertySlot &slot)
    {
        return getStaticFunctionSlot<SecurityLabelObject>(exec, ExecState::securityLabelPrototypeTable(exec), jsCast<SecurityLabelPrototype*>(cell), propertyName, slot);
    }
    
    bool SecurityLabelPrototype::getOwnPropertyDescriptor(JSObject* object, ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
    {
        return getStaticFunctionDescriptor<SecurityLabelObject>(exec, ExecState::securityLabelPrototypeTable(exec), jsCast<SecurityLabelPrototype*>(object), propertyName, descriptor);
    }
    
    // ------------------------------ Functions ---------------------------
    
    EncodedJSValue JSC_HOST_CALL securityLabelProtoFuncToString(ExecState* exec)
    {
        return JSValue::encode(jsString(exec, "SecurityLabel"));
    }

} // namespace JSC
