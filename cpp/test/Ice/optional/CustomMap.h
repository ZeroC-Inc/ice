//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef CUSTOM_MAP_H
#define CUSTOM_MAP_H

#include <IceUtil/Config.h>
#include <unordered_map>

namespace Test
{
    template<typename K, typename V> class CustomMap : public std::unordered_map<K, V>
    {
    };
}

#endif
