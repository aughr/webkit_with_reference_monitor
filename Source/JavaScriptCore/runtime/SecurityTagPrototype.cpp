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
#include "SecurityTagObject.h"
#include <wtf/Assertions.h>

namespace JSC {

static EncodedJSValue JSC_HOST_CALL securityTagProtoFuncToString(ExecState*);
static EncodedJSValue JSC_HOST_CALL securityTagProtoFuncAddTo(ExecState*);
static EncodedJSValue JSC_HOST_CALL securityTagProtoFuncIsOn(ExecState*);

}

namespace JSC {

const ClassInfo SecurityTagPrototype::s_info = { "SecurityTag", &SecurityTagObject::s_info, 0, 0, CREATE_METHOD_TABLE(SecurityTagPrototype) };

ASSERT_CLASS_FITS_IN_CELL(SecurityTagPrototype);

SecurityTagPrototype::SecurityTagPrototype(ExecState* exec, Structure* structure)
: SecurityTagObject(exec->globalData(), structure)
{
}

void SecurityTagPrototype::finishCreation(ExecState* exec, JSGlobalObject* globalObject)
{
    Base::finishCreation(exec->globalData());
    
    ASSERT(inherits(&s_info));
    
    JSFunction* toStringFunction = JSFunction::create(exec, globalObject, 0, exec->propertyNames().toString, securityTagProtoFuncToString);
    putDirectWithoutTransition(exec->globalData(), exec->propertyNames().toString, toStringFunction, DontEnum | DontDelete | ReadOnly);
    
    JSFunction* addToFunction = JSFunction::create(exec, globalObject, 1, Identifier(exec, "addTo"), securityTagProtoFuncAddTo);
    putDirectWithoutTransition(exec->globalData(), Identifier(exec, "addTo"), addToFunction, DontEnum | DontDelete | ReadOnly);
    
    JSFunction* isOnFunction = JSFunction::create(exec, globalObject, 1, Identifier(exec, "inOn"), securityTagProtoFuncIsOn);
    putDirectWithoutTransition(exec->globalData(), Identifier(exec, "isOn"), isOnFunction, DontEnum | DontDelete | ReadOnly);
}

// ------------------------------ Functions ---------------------------

EncodedJSValue JSC_HOST_CALL securityTagProtoFuncToString(ExecState* exec)
{
    return JSValue::encode(jsString(exec, "SecurityTag"));
}

EncodedJSValue JSC_HOST_CALL securityTagProtoFuncAddTo(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&SecurityTagObject::s_info))
        return throwVMTypeError(exec);

    SecurityTagObject* thisObject = asSecurityTagObject(thisValue);
    JSValue argument = exec->argument(0);
    ASSERT(!thisObject->labelForTag().isNull());
    ASSERT(thisObject->labelForTag().hasTag(thisObject->tag()));
    return JSValue::encode(argument, exec, thisObject->labelForTag());
}

EncodedJSValue JSC_HOST_CALL securityTagProtoFuncIsOn(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&SecurityTagObject::s_info))
        return throwVMTypeError(exec);

    SecurityTagObject* thisObject = asSecurityTagObject(thisValue);
    JSValue argument = exec->argument(0);
    if (argument.isCell()) {
        bool result = argument.asCell()->securityLabel().hasTag(thisObject->tag());
        return JSValue::encode(jsBoolean(result));
    }
    return JSValue::encode(jsBoolean(false));
}

} // namespace JSC
