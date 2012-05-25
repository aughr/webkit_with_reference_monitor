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

#ifndef JSLabeledValue_h
#define JSLabeledValue_h
#include "Structure.h"

namespace JSC {
    
    class JSString;
    class LLIntOffsetsExtractor;
    
    class JSLabeledValue : public JSCell {
    public:
        typedef JSCell Base;
        
        //static void destroy(JSCell*);

    private:
        JSLabeledValue(JSGlobalData& globalData, JSValue value)
        : JSCell(globalData, globalData.labeledValueStructure.get())
        , m_value(value)
        {
            ASSERT(value && value.isPrimitive() && !value.isString());
        }
        
    public:
        static JSLabeledValue* create(ExecState* exec, SecurityLabel label, JSValue value)
        {
            ASSERT(!label.isNull());
            JSLabeledValue* labeledValue = new (NotNull, allocateCell<JSLabeledValue>(exec->globalData().heap)) JSLabeledValue(exec->globalData(), value);
            labeledValue->finishCreation(exec->globalData());
            labeledValue->mergeSecurityLabel(exec, label);
            return labeledValue;
        }

        JSValue value() const { return m_value; }

        JSValue toPrimitive(ExecState*, PreferredPrimitiveType) const;
        JS_EXPORT_PRIVATE bool toBoolean(ExecState*) const;
        bool getPrimitiveNumber(ExecState*, double& number, JSValue&) const;
        JSObject* toObject(ExecState*, JSGlobalObject*) const;
        double toNumber(ExecState*) const;
        JSString* toString(ExecState*) const;

        static Structure* createStructure(JSGlobalData& globalData, JSGlobalObject* globalObject, JSValue proto)
        {
            return Structure::create(globalData, globalObject, proto, TypeInfo(LabeledType), &s_info);
        }

        static JS_EXPORTDATA const ClassInfo s_info;

        static void visitChildren(JSCell*, SlotVisitor&);

        static ptrdiff_t valueOffset()
        {
            return OBJECT_OFFSETOF(JSLabeledValue, m_value);
        }
        
#if USE(JSVALUE32_64)
        static ptrdiff_t valueTagOffset()
        {
            return OBJECT_OFFSETOF(JSLabeledValue, m_value) + OBJECT_OFFSETOF(JSValue, u.asBits.tag);
        }
        
        static ptrdiff_t valuePayloadOffset()
        {
            return OBJECT_OFFSETOF(JSLabeledValue, m_value) + OBJECT_OFFSETOF(JSValue, u.asBits.payload);
        }
#endif

    private:
        friend class LLIntOffsetsExtractor;

        JSValue m_value;
    };

    inline JSValue JSValue::unwrappedValue() const {
        if (isLabeledValue())
            return static_cast<const JSLabeledValue*>(asCell())->value();
        else
            return *this;
    }

} // namespace JSC

#endif // JSLabeledValue_h
