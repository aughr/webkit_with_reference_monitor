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
#include "JSObject.h"
#include "JSGlobalObject.h"
#include <wtf/Assertions.h>

namespace JSC {
    
    static EncodedJSValue JSC_HOST_CALL securityLabelProtoFuncToString(ExecState*);
    
}

namespace JSC {
    
    const ClassInfo SecurityLabelPrototype::s_info = { "SecurityLabel", &JSNonFinalObject::s_info, 0, 0, CREATE_METHOD_TABLE(SecurityLabelPrototype) };
    
    ASSERT_CLASS_FITS_IN_CELL(SecurityLabelPrototype);
    
    SecurityLabelPrototype::SecurityLabelPrototype(ExecState* exec, Structure* structure)
    : JSNonFinalObject(exec->globalData(), structure)
    {
    }
    
    void SecurityLabelPrototype::finishCreation(ExecState* exec, JSGlobalObject* globalObject)
    {
        Base::finishCreation(exec->globalData());

        ASSERT(inherits(&s_info));

        JSFunction* toStringFunction = JSFunction::create(exec, globalObject, 0, exec->propertyNames().toString, securityLabelProtoFuncToString);
        putDirectWithoutTransition(exec->globalData(), exec->propertyNames().toString, toStringFunction, DontEnum | DontDelete | ReadOnly);
    }
    
    // ------------------------------ Functions ---------------------------
    
    EncodedJSValue JSC_HOST_CALL securityLabelProtoFuncToString(ExecState* exec)
    {
        return JSValue::encode(jsString(exec, "SecurityLabel"));
    }

} // namespace JSC
