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

#ifndef SecurityTagPrototype_h
#define SecurityTagPrototype_h

#include "SecurityTagObject.h"

namespace JSC {

class SecurityTagPrototype : public SecurityTagObject {
public:
    typedef SecurityTagObject Base;
    
    static SecurityTagPrototype* create(ExecState* exec, JSGlobalObject* globalObject, Structure* structure)
    {
        SecurityTagPrototype* prototype = new (NotNull, allocateCell<SecurityTagPrototype>(*exec->heap())) SecurityTagPrototype(exec, structure);
        prototype->finishCreation(exec, globalObject);
        return prototype;
    }
    
    static const ClassInfo s_info;
    
    static Structure* createStructure(JSGlobalData& globalData, JSGlobalObject* globalObject, JSValue prototype)
    {
        return Structure::create(globalData, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info);
    }
    
protected:
    void finishCreation(ExecState*, JSGlobalObject*);
    
private:
    SecurityTagPrototype(ExecState*, Structure*);
};

} // namespace JSC

#endif // SecurityTagPrototype_h
