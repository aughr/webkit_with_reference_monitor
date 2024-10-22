/*
 * Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "Operations.h"

#include "Error.h"
#include "JSLabeledValue.h"
#include "JSObject.h"
#include "JSString.h"
#include <math.h>
#include <stdio.h>
#include <wtf/MathExtras.h>

namespace JSC {

bool JSValue::equalSlowCase(ExecState* exec, JSValue v1, JSValue v2)
{
    return equalSlowCaseInline(exec, v1.unwrappedValue(), v2.unwrappedValue());
}

bool JSValue::strictEqualSlowCase(ExecState* exec, JSValue v1, JSValue v2)
{
    return strictEqualSlowCaseInline(exec, v1, v2);
}

NEVER_INLINE JSValue jsAddSlowCase(CallFrame* callFrame, JSValue v1, JSValue v2)
{
    // exception for the Date exception in defaultValue()
    JSValue p1 = v1.toPrimitive(callFrame);
    JSValue p2 = v2.toPrimitive(callFrame);
    
    SecurityLabel label;
    label.merge(p1.securityLabel());
    label.merge(p2.securityLabel());
    
    JSValue result;

    if (p1.isString())
        result = jsString(callFrame, asString(p1), p2.toString(callFrame));

    else if (p2.isString())
        result = jsString(callFrame, p1.toString(callFrame), asString(p2));

    else
        result = jsNumber(p1.toNumber(callFrame) + p2.toNumber(callFrame));
    
    result = result.mergeSecurityLabel(callFrame, label);
    return result;
}

JSValue jsTypeStringForValue(CallFrame* callFrame, JSValue v)
{
    JSGlobalData& globalData = callFrame->globalData();
    if (v.isUndefined())
        return globalData.smallStrings.undefinedString(&globalData);
    if (v.isBoolean())
        return globalData.smallStrings.booleanString(&globalData);
    if (v.isNumber())
        return globalData.smallStrings.numberString(&globalData);
    if (v.isLabeledValue()) {
        const JSLabeledValue* labeledValue = static_cast<const JSLabeledValue*>(v.asCell());
        return jsTypeStringForValue(callFrame, labeledValue->value()).mergeSecurityLabel(callFrame, labeledValue->securityLabel());
    }
    if (v.isString())
        return globalData.smallStrings.stringString(&globalData);
    if (v.isObject()) {
        // Return "undefined" for objects that should be treated
        // as null when doing comparisons.
        if (asObject(v)->structure()->typeInfo().masqueradesAsUndefined())
            return globalData.smallStrings.undefinedString(&globalData);
        CallData callData;
        JSObject* object = asObject(v);
        if (object->methodTable()->getCallData(object, callData) != CallTypeNone)
            return globalData.smallStrings.functionString(&globalData);
    }
    return globalData.smallStrings.objectString(&globalData);
}

bool jsIsObjectType(JSValue v)
{
    if (!v.isCell())
        return v.isNull();

    JSType type = v.asCell()->structure()->typeInfo().type();
    if (type == NumberType || type == StringType || type == LabeledType)
        return false;
    if (type >= ObjectType) {
        if (asObject(v)->structure()->typeInfo().masqueradesAsUndefined())
            return false;
        CallData callData;
        JSObject* object = asObject(v);
        if (object->methodTable()->getCallData(object, callData) != CallTypeNone)
            return false;
    }
    return true;
}

bool jsIsFunctionType(JSValue v)
{
    if (v.isObject()) {
        CallData callData;
        JSObject* object = asObject(v);
        if (object->methodTable()->getCallData(object, callData) != CallTypeNone)
            return true;
    }
    return false;
}

} // namespace JSC
