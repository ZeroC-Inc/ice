//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_VALUE_H
#define ICE_VALUE_H

#include "Config.h"
#include "ValueF.h"
#include "SlicedDataF.h"

namespace Ice
{
    class OutputStream;
    class InputStream;

    /**
     * The base class for instances of Slice-defined classes.
     * \headerfile Ice/Ice.h
     */
    class ICE_API Value
    {
    public:
        // There is no copy constructor, move constructor, copy-assignment operator or move-assignment operator
        // to prevent accidental slicing.
        Value() = default;
        Value(Value&&) = delete;
        virtual ~Value() = default;

        Value& operator=(const Value&) = delete;
        Value& operator=(Value&&) = delete;

        /**
         * The Ice run time invokes this method prior to marshaling an object's data members. This allows a subclass
         * to override this method in order to validate its data members.
         */
        virtual void ice_preMarshal();

        /**
         * The Ice run time invokes this method after unmarshaling an object's data members. This allows a
         * subclass to override this method in order to perform additional initialization.
         */
        virtual void ice_postUnmarshal();

        /**
         * Obtains the Slice type ID of the most-derived class supported by this object.
         * @return The type ID.
         */
        virtual std::string ice_id() const;

        /**
         * Obtains the Slice type ID of this type.
         * @return The return value is always "Ice::Object".
         */
        static std::string_view ice_staticId() noexcept;

        /**
         * Creates a shallow polymorphic copy of this instance.
         * @return The cloned value.
         */
        ValuePtr ice_clone() const { return _iceCloneImpl(); }

        /**
         * Obtains the sliced data associated with this instance.
         * @return The sliced data if the value has a preserved-slice base class and has been sliced during
         * unmarshaling of the value, nil otherwise.
         */
        SlicedDataPtr ice_getSlicedData() const;

        /// \cond STREAM
        virtual void _iceWrite(Ice::OutputStream*) const;
        virtual void _iceRead(Ice::InputStream*);
        /// \endcond

    protected:
        /// \cond INTERNAL
        Value(const Value&) = default; // for clone

        // Helper class that allows derived classes to clone "this" even though the copy constructor is protected.
        template<class T> struct CloneEnabler : public T
        {
            CloneEnabler(const T& other) : T(other) {}
            static std::shared_ptr<T> clone(const T& other) { return std::make_shared<CloneEnabler>(other); }
        };

        virtual ValuePtr _iceCloneImpl() const;
        /// \endcond

        /// \cond STREAM
        virtual void _iceWriteImpl(Ice::OutputStream*) const {}
        virtual void _iceReadImpl(Ice::InputStream*) {}
        /// \endcond

    private:
        SlicedDataPtr _slicedData;
    };
}

#endif
