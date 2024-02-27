//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_VALUE_FACTORY_H
#define ICE_VALUE_FACTORY_H

#include <Ice/Config.h>
#include <functional>
#include <memory>

namespace Ice
{

    class Value;

    /**
     * Create a new value for a given value type. The type is the absolute Slice type id, i.e., the id relative to the
     * unnamed top-level Slice module. For example, the absolute Slice type id for an interface <code>Bar</code> in
     * the module <code>Foo</code> is <code>"::Foo::Bar"</code>.
     * Note that the leading "<code>::</code>" is required.
     * @param type The value type.
     * @return The value created for the given type, or nil if the factory is unable to create the value.
     */
    using ValueFactoryFunc = ::std::function<::std::shared_ptr<Value>(std::string_view type)>;

    /**
     * A factory for values. Value factories are used in several places, such as when Ice receives a class instance.
     * Value factories must be implemented by the application writer and registered with the communicator. \headerfile
     * Ice/Ice.h
     */
    class ICE_API ValueFactory
    {
    public:
        ValueFactory() = default;
        virtual ~ValueFactory();
        ValueFactory(const ValueFactory&) = default;
        ValueFactory& operator=(const ValueFactory&) = default;

        /**
         * Create a new value for a given value type. The type is the absolute Slice type id, i.e., the id relative to
         * the unnamed top-level Slice module. For example, the absolute Slice type id for an interface <code>Bar</code>
         * in the module <code>Foo</code> is <code>"::Foo::Bar"</code>. Note that the leading "<code>::</code>" is
         * required.
         * @param type The value type.
         * @return The value created for the given type, or nil if the factory is unable to create the value.
         */
        virtual std::shared_ptr<Value> create(std::string_view type) = 0;
    };

    using ValueFactoryPtr = std::shared_ptr<ValueFactory>;

    /**
     * A value factory manager maintains a collection of value factories. An application can supply a custom
     * implementation during communicator initialization, otherwise Ice provides a default implementation.
     * @see ValueFactory
     * \headerfile Ice/Ice.h
     */
    class ICE_CLASS(ICE_API) ValueFactoryManager
    {
    public:
        ICE_MEMBER(ICE_API) virtual ~ValueFactoryManager();

        /**
         * Add a value factory. Attempting to add a factory with an id for which a factory is already registered throws
         * AlreadyRegisteredException.
         * When unmarshaling an Ice value, the Ice run time reads the most-derived type id off the wire and attempts to
         * create an instance of the type using a factory. If no instance is created, either because no factory was
         * found, or because all factories returned nil, the behavior of the Ice run time depends on the format with
         * which the value was marshaled: If the value uses the "sliced" format, Ice ascends the class hierarchy until
         * it finds a type that is recognized by a factory, or it reaches the least-derived type. If no factory is found
         * that can create an instance, the run time throws NoValueFactoryException. If the value uses the "compact"
         * format, Ice immediately raises NoValueFactoryException. The following order is used to locate a factory for a
         * type: <ol> <li>The Ice run-time looks for a factory registered specifically for the type.</li> <li>If no
         * instance has been created, the Ice run-time looks for the default factory, which is registered with an empty
         * type id.</li> <li>If no instance has been created by any of the preceding steps, the Ice run-time looks for a
         * factory that may have been statically generated by the language mapping for non-abstract classes.</li>
         * </ol>
         * @param factory The factory to add.
         * @param id The type id for which the factory can create instances, or an empty string for the default factory.
         */
        virtual void add(ValueFactoryFunc factory, std::string_view id) = 0;

        /**
         * Add a value factory. Attempting to add a factory with an id for which a factory is already registered throws
         * AlreadyRegisteredException.
         * When unmarshaling an Ice value, the Ice run time reads the most-derived type id off the wire and attempts to
         * create an instance of the type using a factory. If no instance is created, either because no factory was
         * found, or because all factories returned nil, the behavior of the Ice run time depends on the format with
         * which the value was marshaled: If the value uses the "sliced" format, Ice ascends the class hierarchy until
         * it finds a type that is recognized by a factory, or it reaches the least-derived type. If no factory is found
         * that can create an instance, the run time throws NoValueFactoryException. If the value uses the "compact"
         * format, Ice immediately raises NoValueFactoryException. The following order is used to locate a factory for a
         * type: <ol> <li>The Ice run-time looks for a factory registered specifically for the type.</li> <li>If no
         * instance has been created, the Ice run-time looks for the default factory, which is registered with an empty
         * type id.</li> <li>If no instance has been created by any of the preceding steps, the Ice run-time looks for a
         * factory that may have been statically generated by the language mapping for non-abstract classes.</li>
         * </ol>
         * @param factory The factory to add.
         * @param id The type id for which the factory can create instances, or an empty string for the default factory.
         */
        virtual void add(ValueFactoryPtr factory, std::string_view id) = 0;

        /**
         * Find an value factory registered with this communicator.
         * @param id The type id for which the factory can create instances, or an empty string for the default factory.
         * @return The value factory, or null if no value factory was found for the given id.
         */
        virtual ::Ice::ValueFactoryFunc find(std::string_view id) const noexcept = 0;
    };

    using ValueFactoryManagerPtr = ::std::shared_ptr<ValueFactoryManager>;

}

#endif
