//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICEPY_VALUE_FACTORY_MANAGER_H
#define ICEPY_VALUE_FACTORY_MANAGER_H

#include "Config.h"
#include <Ice/ValueFactory.h>

#include <map>
#include <mutex>

namespace IcePy
{
    extern PyTypeObject ValueFactoryManagerType;

    bool initValueFactoryManager(PyObject*);

    // The IcePy C++ value factory abstract base class.
    struct ValueFactory
    {
        virtual std::shared_ptr<Ice::Value> create(std::string_view) = 0;
    };
    using ValueFactoryPtr = std::shared_ptr<ValueFactory>;

    // Adapts a Python value factory to our C++ value factory.
    class CustomValueFactory final : public ValueFactory
    {
    public:
        CustomValueFactory(PyObject*);
        ~CustomValueFactory();

        std::shared_ptr<Ice::Value> create(std::string_view) final;

        PyObject* getValueFactory() const;

    private:
        PyObject* _valueFactory;
    };

    using CustomValueFactoryPtr = std::shared_ptr<CustomValueFactory>;

    // The default Python value factory as a C++ value factory.
    class DefaultValueFactory final : public ValueFactory
    {
    public:
        std::shared_ptr<Ice::Value> create(std::string_view) final;
    };
    using DefaultValueFactoryPtr = std::shared_ptr<DefaultValueFactory>;

    class ValueFactoryManager final : public Ice::ValueFactoryManager
    {
    public:
        static std::shared_ptr<ValueFactoryManager> create();
        ~ValueFactoryManager();

        void add(Ice::ValueFactory, std::string_view) final;
        Ice::ValueFactory find(std::string_view) const noexcept final;

        void add(PyObject*, std::string_view);
        PyObject* findValueFactory(std::string_view) const;

        PyObject* getObject() const;

        void destroy();

    private:
        using CustomFactoryMap = std::map<std::string, CustomValueFactoryPtr, std::less<>>;

        ValueFactoryManager();

        PyObject* _self;
        CustomFactoryMap _customFactories;
        const DefaultValueFactoryPtr _defaultFactory;

        mutable std::mutex _mutex;
    };

    using ValueFactoryManagerPtr = std::shared_ptr<ValueFactoryManager>;
}

#endif
