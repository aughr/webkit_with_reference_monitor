/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2002 Apple Computer, Inc.
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
 *  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 *
 */

#ifndef KJS_SCOPE_CHAIN_H
#define KJS_SCOPE_CHAIN_H

namespace KJS {

    class NoRefScopeChain;
    class ObjectImp;
    
    class ScopeChainNode {
    public:
        ScopeChainNode(ScopeChainNode *n, ObjectImp *o);

        ScopeChainNode *next;
        ObjectImp *object;
        short nodeAndObjectRefCount;
        short nodeOnlyRefCount;
    };

    class ScopeChain {
        friend class NoRefScopeChain;
    public:
        ScopeChain() : _node(0) { }
        ~ScopeChain() { deref(); }

        ScopeChain(const ScopeChain &c) : _node(c._node)
            { if (_node) ++_node->nodeAndObjectRefCount; }
        ScopeChain(const NoRefScopeChain &);
        ScopeChain &operator=(const ScopeChain &);

        bool isEmpty() const { return !_node; }
        ObjectImp *top() const { return _node->object; }

        void clear() { deref(); _node = 0; }
        void push(ObjectImp *);
        void pop();
        
    private:
        ScopeChainNode *_node;
        
        void deref() { if (_node && --_node->nodeAndObjectRefCount == 0) release(); }
        void ref() const;
        
        void release();
    };

    class NoRefScopeChain {
        friend class ScopeChain;
    public:
        NoRefScopeChain() : _node(0) { }
        ~NoRefScopeChain() { deref(); }

        NoRefScopeChain &operator=(const ScopeChain &c);

        void mark();

    private:
        ScopeChainNode *_node;
        
        void deref() { if (_node && --_node->nodeOnlyRefCount == 0) release(); }
        void ref() const;

        void release();

        NoRefScopeChain(const NoRefScopeChain &);
        NoRefScopeChain &operator=(const NoRefScopeChain &);
    };

}; // namespace KJS

#endif // KJS_SCOPE_CHAIN_H
