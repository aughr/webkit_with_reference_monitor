/*
 *  Copyright (C) 2012 Andrew Bloomgarden. All rights reserved.
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
#include "JSLabeledValue.h"

#include "BooleanConstructor.h"
#include "BooleanPrototype.h"
#include "Error.h"
#include "ExceptionHelpers.h"
#include "GetterSetter.h"
#include "JSGlobalObject.h"
#include "JSFunction.h"
#include "JSNotAnObject.h"
#include "NumberObject.h"
#include <wtf/MathExtras.h>
#include <wtf/StringExtras.h>

namespace JSC {

    const ClassInfo JSLabeledValue::s_info = { "LabeledValue", 0, 0, 0, CREATE_METHOD_TABLE(JSLabeledValue) };
    
    void JSLabeledValue::visitChildren(JSCell* cell, SlotVisitor& visitor)
    {
        JSLabeledValue* thisObject = jsCast<JSLabeledValue*>(cell);
        Base::visitChildren(thisObject, visitor);
        if (thisObject->m_value.isCell()) {
            JSCell *valueCell = thisObject->m_value.asCell();
            valueCell->methodTable()->visitChildren(valueCell, visitor);
        }
    }

    JSValue JSLabeledValue::toPrimitive(ExecState*, PreferredPrimitiveType) const
    {
        return this;
    }

    bool JSLabeledValue::getPrimitiveNumber(ExecState* exec, double& number, JSValue& result) const
    {
        return m_value.getPrimitiveNumber(exec, number, result);
    }

    bool JSLabeledValue::toBoolean(ExecState* exec) const
    {
        return m_value.toBoolean(exec);
    }

    double JSLabeledValue::toNumber(ExecState* exec) const
    {
        return m_value.toNumber(exec);
    }

    JSString* JSLabeledValue::toString(ExecState* exec) const
    {
        JSString *result = m_value.toString(exec);
        result->mergeSecurityLabel(exec, securityLabel());
        return result;
    }

    JSObject* JSLabeledValue::toObject(ExecState* exec, JSGlobalObject* globalObject) const
    {
        return m_value.toObject(exec, globalObject);
    }

} // namespace JSC
