//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_BT_DBUS_H
#define ICE_BT_DBUS_H

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <cassert>
#include <sstream>

using namespace std;

namespace IceBT
{
    namespace DBus
    {

        class Exception
        {
        public:
            std::string reason;

        protected:
            Exception() {}
            Exception(const std::string& s) : reason(s) {}
        };

        //
        // Type is the base class for a hierarchy representing DBus data types.
        //
        class Type;
        using TypePtr = std::shared_ptr<Type>;

        class Type
        {
        public:
            enum Kind
            {
                KindInvalid,
                KindBoolean,
                KindByte,
                KindUint16,
                KindInt16,
                KindUint32,
                KindInt32,
                KindUint64,
                KindInt64,
                KindDouble,
                KindString,
                KindObjectPath,
                KindSignature,
                KindUnixFD,
                KindArray,
                KindVariant,
                KindStruct,
                KindDictEntry
            };

            static TypePtr getPrimitive(Kind);

            virtual Kind getKind() const = 0;
            virtual std::string getSignature() const = 0;

        protected:
            Type() {}
        };

        class ArrayType : public Type
        {
        public:
            ArrayType(const TypePtr& t) : elementType(t) {}

            virtual Kind getKind() const { return KindArray; }

            virtual std::string getSignature() const;

            TypePtr elementType;
        };
        using ArrayTypePtr = std::shared_ptr<ArrayType>;

        class VariantType : public Type
        {
        public:
            VariantType() {}

            virtual Kind getKind() const { return KindVariant; }

            virtual std::string getSignature() const;
        };
        using VariantTypePtr = std::shared_ptr<VariantType>;

        class StructType : public Type
        {
        public:
            StructType(const std::vector<TypePtr>& types) : memberTypes(types) {}

            virtual Kind getKind() const { return KindStruct; }

            virtual std::string getSignature() const;

            std::vector<TypePtr> memberTypes;
        };
        using StructTypePtr = std::shared_ptr<StructType>;

        class DictEntryType : public Type
        {
        public:
            DictEntryType(const TypePtr& k, const TypePtr& v) : keyType(k), valueType(v) {}

            virtual Kind getKind() const { return KindDictEntry; }

            virtual std::string getSignature() const;

            TypePtr keyType;
            TypePtr valueType;
        };
        using DictEntryTypePtr = std::shared_ptr<DictEntryType>;

        //
        // Value is the base class of a hierarchy representing DBus data values.
        //
        class Value;
        using ValuePtr = std::shared_ptr<Value>;

        class Value
        {
        public:
            virtual TypePtr getType() const = 0;

            virtual ValuePtr clone() const = 0;

            virtual std::string toString() const = 0;

        protected:
            virtual void print(std::ostream&) = 0;

            friend std::ostream& operator<<(std::ostream&, const ValuePtr&);
        };

        inline std::ostream& operator<<(std::ostream& ostr, const ValuePtr& v)
        {
            if (v)
            {
                v->print(ostr);
            }
            else
            {
                ostr << "nil";
            }
            return ostr;
        }

        template<typename E, Type::Kind K> class PrimitiveValue final : public Value
        {
        public:
            PrimitiveValue() : v(E()), kind(K) {}
            PrimitiveValue(const E& val) : v(val), kind(K) {}

            TypePtr getType() const final { return Type::getPrimitive(kind); }

            ValuePtr clone() const final { return make_shared<PrimitiveValue>(v); }

            std::string toString() const final
            {
                std::ostringstream out;
                out << v;
                return out.str();
            }

            E v;
            Type::Kind kind;

        protected:
            void print(std::ostream& ostr) final { ostr << v; }
        };

        using BooleanValue = PrimitiveValue<bool, Type::KindBoolean>;
        using BooleanValuePtr = std::shared_ptr<BooleanValue>;
        using ByteValue = PrimitiveValue<unsigned char, Type::KindByte>;
        using ByteValuePtr = std::shared_ptr<ByteValue>;
        using Uint16Value = PrimitiveValue<unsigned short, Type::KindUint16>;
        using Uint16ValuePtr = std::shared_ptr<Uint16Value>;
        using Int16Value = PrimitiveValue<short, Type::KindInt16>;
        using Int16ValuePtr = std::shared_ptr<Int16Value>;
        using Uint32Value = PrimitiveValue<unsigned int, Type::KindUint32>;
        using Uint32ValuePtr = std::shared_ptr<Uint32Value>;
        using Int32Value = PrimitiveValue<int, Type::KindInt32>;
        using Int32ValuePtr = std::shared_ptr<Int32Value>;
        using Uint64Value = PrimitiveValue<std::uint64_t, Type::KindUint64>;
        using Uint64ValuePtr = std::shared_ptr<Uint64Value>;
        using Int64Value = PrimitiveValue<std::int64_t, Type::KindInt64>;
        using Int64ValuePtr = std::shared_ptr<Int64Value>;
        using DoubleValue = PrimitiveValue<double, Type::KindDouble>;
        using DoubleValuePtr = std::shared_ptr<DoubleValue>;
        using StringValue = PrimitiveValue<string, Type::KindString>;
        using StringValuePtr = std::shared_ptr<StringValue>;
        using ObjectPathValue = PrimitiveValue<string, Type::KindObjectPath>;
        using ObjectPathValuePtr = std::shared_ptr<ObjectPathValue>;
        using SignatureValue = PrimitiveValue<string, Type::KindSignature>;
        using SignatureValuePtr = std::shared_ptr<SignatureValue>;
        using UnixFDValue = PrimitiveValue<unsigned int, Type::KindUnixFD>;
        using UnixFDValuePtr = std::shared_ptr<UnixFDValue>;

        class VariantValue;
        using VariantValuePtr = std::shared_ptr<VariantValue>;

        class VariantValue : public Value, public std::enable_shared_from_this<VariantValue>
        {
        public:
            VariantValue() : _type(make_shared<VariantType>()) {}

            VariantValue(const ValuePtr& val) : v(val), _type(make_shared<VariantType>()) {}

            virtual TypePtr getType() const { return _type; }

            virtual ValuePtr clone() const { return const_cast<VariantValue*>(this)->shared_from_this(); }

            virtual std::string toString() const { return v ? v->toString() : "nil"; }

            ValuePtr v;

        protected:
            virtual void print(std::ostream& ostr) { ostr << v; }

        private:
            TypePtr _type;
        };

        class DictEntryValue;
        using DictEntryValuePtr = std::shared_ptr<DictEntryValue>;

        class DictEntryValue : public Value
        {
        public:
            DictEntryValue(const DictEntryTypePtr& t) : _type(t) {}

            DictEntryValue(const DictEntryTypePtr& t, const ValuePtr& k, const ValuePtr& v) : key(k), value(v), _type(t)
            {
            }

            virtual TypePtr getType() const { return _type; }

            virtual ValuePtr clone() const
            {
                DictEntryValuePtr r = make_shared<DictEntryValue>(_type);
                r->key = key->clone();
                r->value = value->clone();
                return r;
            }

            virtual std::string toString() const
            {
                std::ostringstream out;
                out << key->toString() << "=" << value->toString();
                return out.str();
            }

            ValuePtr key;
            ValuePtr value;

        protected:
            virtual void print(std::ostream& ostr) { ostr << '{' << key << ": " << value << '}' << endl; }

        private:
            DictEntryTypePtr _type;
        };

        class ArrayValue;
        using ArrayValuePtr = std::shared_ptr<ArrayValue>;

        class ArrayValue : public Value
        {
        public:
            ArrayValue(const TypePtr& t) : _type(t) {}

            virtual TypePtr getType() const { return _type; }

            virtual ValuePtr clone() const
            {
                auto r = make_shared<ArrayValue>(_type);
                for (std::vector<ValuePtr>::const_iterator p = elements.begin(); p != elements.end(); ++p)
                {
                    r->elements.push_back((*p)->clone());
                }
                return r;
            }

            virtual std::string toString() const
            {
                std::ostringstream out;
                for (std::vector<ValuePtr>::const_iterator p = elements.begin(); p != elements.end(); ++p)
                {
                    if (p != elements.begin())
                    {
                        out << ',';
                    }
                    out << (*p)->toString();
                }
                return out.str();
            }

            void toStringMap(std::map<std::string, ValuePtr>& m)
            {
                for (std::vector<ValuePtr>::const_iterator p = elements.begin(); p != elements.end(); ++p)
                {
                    auto de = dynamic_pointer_cast<DictEntryValue>(*p);
                    assert(de);
                    auto s = dynamic_pointer_cast<StringValue>(de->key);
                    assert(s);
                    m[s->v] = de->value;
                }
            }

            std::vector<ValuePtr> elements;

        protected:
            virtual void print(std::ostream& ostr)
            {
                for (std::vector<ValuePtr>::const_iterator p = elements.begin(); p != elements.end(); ++p)
                {
                    ostr << *p << endl;
                }
            }

        private:
            TypePtr _type;
        };

        class StructValue;
        using StructValuePtr = std::shared_ptr<StructValue>;

        class StructValue final : public Value
        {
        public:
            StructValue(const StructTypePtr& t) : _type(t) {}

            TypePtr getType() const final { return _type; }

            ValuePtr clone() const final
            {
                auto r = make_shared<StructValue>(_type);
                for (std::vector<ValuePtr>::const_iterator p = members.begin(); p != members.end(); ++p)
                {
                    r->members.push_back((*p)->clone());
                }
                return r;
            }

            std::string toString() const final
            {
                std::ostringstream out;
                for (std::vector<ValuePtr>::const_iterator p = members.begin(); p != members.end(); ++p)
                {
                    if (p != members.begin())
                    {
                        out << ',';
                    }
                    out << (*p)->toString();
                }
                return out.str();
            }

            std::vector<ValuePtr> members;

        private:
            void print(std::ostream& ostr) final
            {
                for (std::vector<ValuePtr>::const_iterator p = members.begin(); p != members.end(); ++p)
                {
                    ostr << *p << endl;
                }
            }

            StructTypePtr _type;
        };

        //
        // Message encapsulates a DBus message. It only provides the functionality required by the IceBT transport.
        //
        class Message;
        using MessagePtr = std::shared_ptr<Message>;

        class Message
        {
        public:
            virtual bool isError() const = 0;
            virtual std::string getErrorName() const = 0;
            virtual void throwException() = 0;

            virtual bool isSignal() const = 0;
            virtual bool isMethodCall() const = 0;
            virtual bool isMethodReturn() const = 0;

            virtual std::string getPath() const = 0;
            virtual std::string getInterface() const = 0;
            virtual std::string getMember() const = 0;
            virtual std::string getDestination() const = 0;

            //
            // Writing arguments.
            //
            virtual void write(const ValuePtr&) = 0;
            virtual void write(const std::vector<ValuePtr>&) = 0;

            //
            // Reading arguments.
            //
            virtual bool checkTypes(const std::vector<TypePtr>&) const = 0;
            virtual ValuePtr read() = 0;
            virtual std::vector<ValuePtr> readAll() = 0;

            static MessagePtr
            createCall(const string& dest, const string& path, const string& iface, const string& method);
            static MessagePtr createReturn(const MessagePtr& call);
        };

        class AsyncResult;
        using AsyncResultPtr = std::shared_ptr<AsyncResult>;

        class AsyncCallback
        {
        public:
            virtual void completed(const AsyncResultPtr&) = 0;
        };
        using AsyncCallbackPtr = std::shared_ptr<AsyncCallback>;

        //
        // The result of an asynchronous DBus operation.
        //
        class AsyncResult
        {
        public:
            virtual bool isPending() const = 0;
            virtual bool isComplete() const = 0;

            virtual MessagePtr waitUntilFinished() const = 0;

            virtual MessagePtr getReply() const = 0;

            virtual void setCallback(const AsyncCallbackPtr&) = 0;
        };

        class Connection;
        using ConnectionPtr = std::shared_ptr<Connection>;

        //
        // Allows a subclass to intercept DBus messages.
        //
        class Filter
        {
        public:
            //
            // Return true if message is handled or false otherwise.
            //
            virtual bool handleMessage(const ConnectionPtr&, const MessagePtr&) = 0;
        };
        using FilterPtr = std::shared_ptr<Filter>;

        //
        // Allows a subclass to receive DBus method invocations.
        //
        class Service
        {
        public:
            virtual void handleMethodCall(const ConnectionPtr&, const MessagePtr&) = 0;
        };
        using ServicePtr = std::shared_ptr<Service>;

        //
        // Encapsulates a DBus connection.
        //
        class Connection
        {
        public:
            static ConnectionPtr getSystemBus();
            static ConnectionPtr getSessionBus();

            virtual void addFilter(const FilterPtr&) = 0;
            virtual void removeFilter(const FilterPtr&) = 0;

            virtual void addService(const std::string&, const ServicePtr&) = 0;
            virtual void removeService(const std::string&) = 0;

            //
            // Asynchronously invokes a method call. The returned AsyncResult can be used
            // to determine completion status and obtain the reply, or supply a callback
            // to be notified when the call completes.
            //
            virtual AsyncResultPtr callAsync(const MessagePtr&, const AsyncCallbackPtr& = 0) = 0;

            //
            // Sends a message without blocking. Use this to send signals and replies.
            //
            virtual void sendAsync(const MessagePtr&) = 0;

            virtual void close() = 0;
        };

        void initThreads();

    }

}

#endif
