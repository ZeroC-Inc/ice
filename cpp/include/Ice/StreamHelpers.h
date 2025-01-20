// Copyright (c) ZeroC, Inc.

#ifndef ICE_STREAM_HELPERS_H
#define ICE_STREAM_HELPERS_H

#include "InputStream.h"
#include "OutputStream.h"

#include <iterator>

#if __has_include(<span>)
#    include <span>
#endif

namespace Ice
{
    /// \cond STREAM

    // StreamHelper templates used by streams to read and write data.

    /**
     * Helper for smaller built-in type that are typically passed by value.
     * \headerfile Ice/Ice.h
     */
    template<typename T> struct StreamHelper<T, StreamHelperCategoryBuiltinValue>
    {
        static void write(OutputStream* stream, T v) { stream->write(v); }

        static void read(InputStream* stream, T& v) { stream->read(v); }
    };

    /**
     * Helper to marshal a std::string_view as a Slice string.
     * \headerfile Ice/Ice.h
     */
    template<> struct StreamHelper<std::string_view, StreamHelperCategoryBuiltinValue>
    {
        static void write(OutputStream* stream, std::string_view v) { stream->write(v); }

        // No read: we marshal string views but unmarshal strings.
    };

    /**
     * Helper to marshal a std::wstring_view as a Slice string.
     * \headerfile Ice/Ice.h
     */
    template<> struct StreamHelper<std::wstring_view, StreamHelperCategoryBuiltinValue>
    {
        static void write(OutputStream* stream, std::wstring_view v) { stream->write(v); }

        // No read: we marshal wstring views but unmarshal wstrings.
    };

    /**
     * Helper for larger built-in types that are typically not passed by value.
     * \headerfile Ice/Ice.h
     */
    template<typename T> struct StreamHelper<T, StreamHelperCategoryBuiltin>
    {
        static void write(OutputStream* stream, const T& v) { stream->write(v); }

        static void read(InputStream* stream, T& v) { stream->read(v); }
    };

    //
    // "helpers" for the StreamHelper<T, StreamHelperCategoryStruct> below
    // slice2cpp generates specializations as needed
    //

    /**
     * Reader used/generated for structs. Always specialized.
     * \headerfile Ice/Ice.h
     */
    template<typename T> struct StreamReader
    {
    };

    /**
     * Helper for structs.
     * \headerfile Ice/Ice.h
     */
    template<typename T> struct StreamHelper<T, StreamHelperCategoryStruct>
    {
        static void write(OutputStream* stream, const T& v) { stream->writeAll(v.ice_tuple()); }

        static void read(InputStream* stream, T& v) { StreamReader<T>::read(stream, v); }
    };

    /**
     * Helper for enums.
     * \headerfile Ice/Ice.h
     */
    template<typename T> struct StreamHelper<T, StreamHelperCategoryEnum>
    {
        static void write(OutputStream* stream, const T& v)
        {
            if (static_cast<std::int32_t>(v) < StreamableTraits<T>::minValue ||
                static_cast<std::int32_t>(v) > StreamableTraits<T>::maxValue)
            {
                IceInternal::Ex::throwMarshalException(__FILE__, __LINE__, "enumerator out of range");
            }
            stream->writeEnum(static_cast<std::int32_t>(v), StreamableTraits<T>::maxValue);
        }

        static void read(InputStream* stream, T& v)
        {
            std::int32_t value = stream->readEnum(StreamableTraits<T>::maxValue);
            if (value < StreamableTraits<T>::minValue || value > StreamableTraits<T>::maxValue)
            {
                IceInternal::Ex::throwMarshalException(__FILE__, __LINE__, "enumerator out of range");
            }
            v = static_cast<T>(value); // NOLINT
        }
    };

    /**
     * Helper for sequences.
     * \headerfile Ice/Ice.h
     */
    template<typename T> struct StreamHelper<T, StreamHelperCategorySequence>
    {
        static void write(OutputStream* stream, const T& v)
        {
            stream->writeSize(static_cast<std::int32_t>(v.size()));
            for (const auto& element : v)
            {
                stream->write(element);
            }
        }

        static void read(InputStream* stream, T& v)
        {
            std::int32_t sz = stream->readAndCheckSeqSize(StreamableTraits<typename T::value_type>::minWireSize);
            T(static_cast<size_t>(sz)).swap(v);
            for (auto& element : v)
            {
                stream->read(element);
            }
        }
    };

    /**
     * Helper for array custom sequence parameters.
     * \headerfile Ice/Ice.h
     */
    template<typename T> struct StreamHelper<std::pair<const T*, const T*>, StreamHelperCategorySequence>
    {
        static void write(OutputStream* stream, std::pair<const T*, const T*> v) { stream->write(v.first, v.second); }

        static void read(InputStream* stream, std::pair<const T*, const T*>& v) { stream->read(v); }
    };

#ifdef __cpp_lib_span
    /**
     * Helper for span (C++20 or later).
     * \headerfile Ice/Ice.h
     */
    template<typename T> struct StreamHelper<std::span<T>, StreamHelperCategorySequence>
    {
        static void write(OutputStream* stream, const std::span<T>& v) { stream->write(v.data(), v.data() + v.size()); }

        // No read. span are only for view types.
    };
#endif

    /**
     * Helper for dictionaries.
     * \headerfile Ice/Ice.h
     */
    template<typename T> struct StreamHelper<T, StreamHelperCategoryDictionary>
    {
        static void write(OutputStream* stream, const T& v)
        {
            stream->writeSize(static_cast<std::int32_t>(v.size()));
            for (const auto& entry : v)
            {
                stream->write(entry.first);
                stream->write(entry.second);
            }
        }

        static void read(InputStream* stream, T& v)
        {
            std::int32_t sz = stream->readSize();
            v.clear();
            while (sz--)
            {
                typename T::value_type p;
                stream->read(const_cast<typename T::key_type&>(p.first));
                auto i = v.insert(v.end(), p);
                stream->read(i->second);
            }
        }
    };

    /**
     * Helper for user exceptions.
     * \headerfile Ice/Ice.h
     */
    template<typename T> struct StreamHelper<T, StreamHelperCategoryUserException>
    {
        static void write(OutputStream* stream, const T& v) { stream->writeException(v); }

        // no read: only used for marshaling
    };

    /**
     * Helper for proxies.
     * \headerfile Ice/Ice.h
     */
    template<typename T> struct StreamHelper<T, StreamHelperCategoryProxy>
    {
        static void write(OutputStream* stream, const T& v) { stream->write(v); }

        static void read(InputStream* stream, T& v) { stream->read(v); }
    };

    /**
     * Helper for classes.
     * \headerfile Ice/Ice.h
     */
    template<typename T> struct StreamHelper<T, StreamHelperCategoryClass>
    {
        static void write(OutputStream* stream, const T& v) { stream->write(v); }

        static void read(InputStream* stream, T& v) { stream->read(v); }
    };

    //
    // Helpers to read/write optional attributes or members.
    //

    //
    // Extract / compute the optionalFormat
    // This is used _only_ for the base StreamOptionalHelper below
    // /!\ Do not use in StreamOptionalHelper specializations, and do
    // not provide specialization not handled by the base StreamOptionalHelper
    //
    /**
     * Extract / compute the optionalFormat.
     * \headerfile Ice/Ice.h
     */
    template<StreamHelperCategory st, int minWireSize, bool fixedLength> struct GetOptionalFormat;

    /**
     * Specialization for 1-byte built-in fixed-length types.
     * \headerfile Ice/Ice.h
     */
    template<> struct GetOptionalFormat<StreamHelperCategoryBuiltinValue, 1, true>
    {
        static const OptionalFormat value = OptionalFormat::F1;
    };

    /**
     * Specialization for 2-byte built-in fixed-length types.
     * \headerfile Ice/Ice.h
     */
    template<> struct GetOptionalFormat<StreamHelperCategoryBuiltinValue, 2, true>
    {
        static const OptionalFormat value = OptionalFormat::F2;
    };

    /**
     * Specialization for 4-byte built-in fixed-length types.
     * \headerfile Ice/Ice.h
     */
    template<> struct GetOptionalFormat<StreamHelperCategoryBuiltinValue, 4, true>
    {
        static const OptionalFormat value = OptionalFormat::F4;
    };

    /**
     * Specialization for 8-byte built-in fixed-length types.
     * \headerfile Ice/Ice.h
     */
    template<> struct GetOptionalFormat<StreamHelperCategoryBuiltinValue, 8, true>
    {
        static const OptionalFormat value = OptionalFormat::F8;
    };

    /**
     * Specialization for built-in variable-length types.
     * \headerfile Ice/Ice.h
     */
    template<> struct GetOptionalFormat<StreamHelperCategoryBuiltinValue, 1, false>
    {
        static const OptionalFormat value = OptionalFormat::VSize;
    };

    /**
     * Specialization for built-in variable-length types.
     * \headerfile Ice/Ice.h
     */
    template<> struct GetOptionalFormat<StreamHelperCategoryBuiltin, 1, false>
    {
        static const OptionalFormat value = OptionalFormat::VSize;
    };

    /**
     * Specialization for enum types.
     * \headerfile Ice/Ice.h
     */
    template<int minWireSize> struct GetOptionalFormat<StreamHelperCategoryEnum, minWireSize, false>
    {
        static const OptionalFormat value = OptionalFormat::Size;
    };

    /**
     * Base helper for optional values: simply read/write the data.
     * \headerfile Ice/Ice.h
     */
    template<typename T, StreamHelperCategory st, bool fixedLength> struct StreamOptionalHelper
    {
        using Traits = StreamableTraits<T>;

        // If this optionalFormat fails to compile, you must either define your specialization
        // for GetOptionalFormat (in which case the optional data will be marshaled/unmarshaled
        // with straight calls to write/read on the stream), or define your own
        // StreamOptionalHelper specialization (which gives you more control over marshaling)
        //
        static const OptionalFormat optionalFormat = GetOptionalFormat<st, Traits::minWireSize, fixedLength>::value;

        static void write(OutputStream* stream, const T& v) { stream->write(v); }

        static void read(InputStream* stream, T& v) { stream->read(v); }
    };

    /**
     * Helper to write fixed-size structs.
     * \headerfile Ice/Ice.h
     */
    template<typename T> struct StreamOptionalHelper<T, StreamHelperCategoryStruct, true>
    {
        static const OptionalFormat optionalFormat = OptionalFormat::VSize;

        static void write(OutputStream* stream, const T& v)
        {
            stream->writeSize(StreamableTraits<T>::minWireSize);
            stream->write(v);
        }

        static void read(InputStream* stream, T& v)
        {
            stream->skipSize();
            stream->read(v);
        }
    };

    /**
     * Helper to write variable-size structs.
     * \headerfile Ice/Ice.h
     */
    template<typename T> struct StreamOptionalHelper<T, StreamHelperCategoryStruct, false>
    {
        static const OptionalFormat optionalFormat = OptionalFormat::FSize;

        static void write(OutputStream* stream, const T& v)
        {
            OutputStream::size_type pos = stream->startSize();
            stream->write(v);
            stream->endSize(pos);
        }

        static void read(InputStream* stream, T& v)
        {
            stream->skip(4);
            stream->read(v);
        }
    };

    // InputStream and OutputStream have special logic for optional proxies that does not rely on the
    // StreamOptional helpers.

    /**
     * Helper to read/write optional sequences or dictionaries.
     * \headerfile Ice/Ice.h
     */
    template<typename T, bool fixedLength, int sz> struct StreamOptionalContainerHelper;

    /**
     * Encode containers of variable-size elements with the FSize optional
     * type, since we can't easily figure out the size of the container
     * before encoding. This is the same encoding as variable size structs
     * so we just re-use its implementation.
     * \headerfile Ice/Ice.h
     */
    template<typename T, int sz> struct StreamOptionalContainerHelper<T, false, sz>
    {
        static const OptionalFormat optionalFormat = OptionalFormat::FSize;

        static void write(OutputStream* stream, const T& v, std::int32_t)
        {
            StreamOptionalHelper<T, StreamHelperCategoryStruct, false>::write(stream, v);
        }

        static void read(InputStream* stream, T& v)
        {
            StreamOptionalHelper<T, StreamHelperCategoryStruct, false>::read(stream, v);
        }
    };

    /**
     * Encode containers of fixed-size elements with the VSize optional
     * type since we can figure out the size of the container before
     * encoding.
     * \headerfile Ice/Ice.h
     */
    template<typename T, int sz> struct StreamOptionalContainerHelper<T, true, sz>
    {
        static const OptionalFormat optionalFormat = OptionalFormat::VSize;

        static void write(OutputStream* stream, const T& v, std::int32_t n)
        {
            //
            // The container size is the number of elements * the size of
            // an element and the size-encoded number of elements (1 or
            // 5 depending on the number of elements).
            //
            stream->writeSize(sz * n + (n < 255 ? 1 : 5));
            stream->write(v);
        }

        static void read(InputStream* stream, T& v)
        {
            stream->skipSize();
            stream->read(v);
        }
    };

    /**
     * Optimization: containers of 1 byte elements are encoded with the
     * VSize optional type. There's no need to encode an additional size
     * for those, the number of elements of the container can be used to
     * skip the optional.
     * \headerfile Ice/Ice.h
     */
    template<typename T> struct StreamOptionalContainerHelper<T, true, 1>
    {
        static const OptionalFormat optionalFormat = OptionalFormat::VSize;

        static void write(OutputStream* stream, const T& v, std::int32_t) { stream->write(v); }

        static void read(InputStream* stream, T& v) { stream->read(v); }
    };

    /**
     * Helper to write sequences, delegates to the optional container
     * helper template partial specializations.
     * \headerfile Ice/Ice.h
     */
    template<typename T> struct StreamOptionalHelper<T, StreamHelperCategorySequence, false>
    {
        using E = typename T::value_type;
        static const int size = StreamableTraits<E>::minWireSize;
        static const bool fixedLength = StreamableTraits<E>::fixedLength;

        // The optional type of a sequence depends on whether or not elements are fixed
        // or variable size elements and their size.
        static const OptionalFormat optionalFormat =
            StreamOptionalContainerHelper<T, fixedLength, size>::optionalFormat;

        static void write(OutputStream* stream, const T& v)
        {
            StreamOptionalContainerHelper<T, fixedLength, size>::write(stream, v, static_cast<std::int32_t>(v.size()));
        }

        static void read(InputStream* stream, T& v)
        {
            StreamOptionalContainerHelper<T, fixedLength, size>::read(stream, v);
        }
    };

    /**
     * Helper to write sequences, delegates to the optional container
     * helper template partial specializations.
     * \headerfile Ice/Ice.h
     */
    template<typename T> struct StreamOptionalHelper<std::pair<const T*, const T*>, StreamHelperCategorySequence, false>
    {
        using P = std::pair<const T*, const T*>;
        static const int size = StreamableTraits<T>::minWireSize;
        static const bool fixedLength = StreamableTraits<T>::fixedLength;

        // The optional type of a sequence depends on whether or not elements are fixed
        // or variable size elements and their size.
        static const OptionalFormat optionalFormat =
            StreamOptionalContainerHelper<P, fixedLength, size>::optionalFormat;

        static void write(OutputStream* stream, const P& v)
        {
            auto n = static_cast<std::int32_t>(v.second - v.first);
            StreamOptionalContainerHelper<P, fixedLength, size>::write(stream, v, n);
        }

        static void read(InputStream* stream, P& v)
        {
            StreamOptionalContainerHelper<P, fixedLength, size>::read(stream, v);
        }
    };

    /**
     * Helper to write dictionaries, delegates to the optional container
     * helper template partial specializations.
     * \headerfile Ice/Ice.h
     */
    template<typename T> struct StreamOptionalHelper<T, StreamHelperCategoryDictionary, false>
    {
        using K = typename T::key_type;
        using V = typename T::mapped_type;

        static const int size = StreamableTraits<K>::minWireSize + StreamableTraits<V>::minWireSize;
        static const bool fixedLength = StreamableTraits<K>::fixedLength && StreamableTraits<V>::fixedLength;

        // The optional type of a dictionary depends on whether or not elements are fixed
        // or variable size elements.
        static const OptionalFormat optionalFormat =
            StreamOptionalContainerHelper<T, fixedLength, size>::optionalFormat;

        static void write(OutputStream* stream, const T& v)
        {
            StreamOptionalContainerHelper<T, fixedLength, size>::write(stream, v, static_cast<std::int32_t>(v.size()));
        }

        static void read(InputStream* stream, T& v)
        {
            StreamOptionalContainerHelper<T, fixedLength, size>::read(stream, v);
        }
    };

    // Specializations for ProtocolVersion and EncodingVersion

    template<> struct StreamableTraits<ProtocolVersion>
    {
        static const StreamHelperCategory helper = StreamHelperCategoryStruct;
        static const int minWireSize = 2;
        static const bool fixedLength = true;
    };

    template<> struct StreamReader<ProtocolVersion>
    {
        static void read(InputStream* istr, ProtocolVersion& v) { istr->readAll(v.major, v.minor); }
    };

    template<> struct StreamableTraits<EncodingVersion>
    {
        static const StreamHelperCategory helper = StreamHelperCategoryStruct;
        static const int minWireSize = 2;
        static const bool fixedLength = true;
    };

    template<> struct StreamReader<EncodingVersion>
    {
        static void read(InputStream* istr, EncodingVersion& v) { istr->readAll(v.major, v.minor); }
    };

    /// \endcond
}

#endif
