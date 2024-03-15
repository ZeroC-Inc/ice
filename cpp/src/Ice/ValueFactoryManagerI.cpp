//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "ValueFactoryManagerI.h"
#include "Ice/LocalException.h"

using namespace std;
using namespace Ice;
using namespace IceInternal;

void
IceInternal::ValueFactoryManagerI::add(ValueFactoryFunc factoryFunc, string_view id)
{
    lock_guard lock(_mutex);

    if ((_factoryFuncMapHint != _factoryFuncMap.end() && _factoryFuncMapHint->first == id) ||
        _factoryFuncMap.find(id) != _factoryFuncMap.end())
    {
        throw AlreadyRegisteredException(__FILE__, __LINE__, "value factory", string{id});
    }

    _factoryFuncMapHint = _factoryFuncMap.insert(_factoryFuncMapHint, make_pair(string{id}, factoryFunc));
}

ValueFactoryFunc
IceInternal::ValueFactoryManagerI::find(string_view id) const noexcept
{
    lock_guard lock(_mutex);

    FactoryFuncMap& factoryFuncMap = const_cast<FactoryFuncMap&>(_factoryFuncMap);

    FactoryFuncMap::iterator p = factoryFuncMap.end();
    if (_factoryFuncMapHint != factoryFuncMap.end())
    {
        if (_factoryFuncMapHint->first == id)
        {
            p = _factoryFuncMapHint;
        }
    }

    if (p == factoryFuncMap.end())
    {
        p = factoryFuncMap.find(id);
    }

    if (p != factoryFuncMap.end())
    {
        _factoryFuncMapHint = p;
        return p->second;
    }
    else
    {
        return nullptr;
    }
}

IceInternal::ValueFactoryManagerI::ValueFactoryManagerI() : _factoryFuncMapHint(_factoryFuncMap.end()) {}
