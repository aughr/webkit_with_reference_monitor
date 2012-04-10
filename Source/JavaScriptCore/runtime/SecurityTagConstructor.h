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

#ifndef SecurityTagConstructor_h
#define SecurityTagConstructor_h

#include "InternalFunction.h"

namespace JSC {

    class SecurityTagPrototype;

    class SecurityTagConstructor : public InternalFunction {
    public:
        typedef InternalFunction Base;

        static SecurityTagConstructor* create(ExecState* exec, JSGlobalObject* globalObject, Structure* structure, SecurityTagPrototype* securityTagPrototype)
        {
            SecurityTagConstructor* constructor = new (NotNull, allocateCell<SecurityTagConstructor>(*exec->heap())) SecurityTagConstructor(globalObject, structure);
            constructor->finishCreation(exec, securityTagPrototype);
            return constructor;
        }

        static const ClassInfo s_info;

        static Structure* createStructure(JSGlobalData& globalData, JSGlobalObject* globalObject, JSValue proto) 
        { 
            return Structure::create(globalData, globalObject, proto, TypeInfo(ObjectType, StructureFlags), &s_info); 
        }

    protected:
        void finishCreation(ExecState*, SecurityTagPrototype*);

    private:
        SecurityTagConstructor(JSGlobalObject*, Structure*);
        static ConstructType getConstructData(JSCell*, ConstructData&);
        static CallType getCallData(JSCell*, CallData&);
    };

} // namespace JSC

#endif // SecurityTagConstructor_h
