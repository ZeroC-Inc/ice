//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifdef _WIN32
     // Prevents windows.h from defining min/max macros.
#    define NOMINMAX
#endif

#include <Types.h>
#include <Current.h>
#include <Proxy.h>
#include <Thread.h>
#include <Util.h>
#include <IceUtil/InputUtil.h>
#include <Ice/InputStream.h>
#include <Ice/LocalException.h>
#include <Ice/OutputStream.h>
#include <Ice/SlicedData.h>

#include <IceUtil/DisableWarnings.h>

#include <list>
#include <limits>

using namespace std;
using namespace IcePy;
using namespace IceUtil;
using namespace IceUtilInternal;

typedef map<string, ClassInfoPtr> ClassInfoMap;
static ClassInfoMap _classInfoMap;

typedef map<string, ValueInfoPtr> ValueInfoMap;
static ValueInfoMap _valueInfoMap;

typedef map<int32_t, ValueInfoPtr> CompactIdMap;
static CompactIdMap _compactIdMap;

typedef map<string, ProxyInfoPtr> ProxyInfoMap;
static ProxyInfoMap _proxyInfoMap;

typedef map<string, ExceptionInfoPtr> ExceptionInfoMap;
static ExceptionInfoMap _exceptionInfoMap;

namespace
{

const char* emptySeq = "";

//
// This exception is raised if the factory specified in a sequence metadata
// cannot be load or is not valid
//
class InvalidSequenceFactoryException
{
};

}

namespace IcePy
{

struct TypeInfoObject
{
    PyObject_HEAD
    IcePy::TypeInfoPtr* info;
};

struct ExceptionInfoObject
{
    PyObject_HEAD
    IcePy::ExceptionInfoPtr* info;
};

struct BufferObject
{
    PyObject_HEAD
    IcePy::BufferPtr* buffer;
};

extern PyTypeObject TypeInfoType;
extern PyTypeObject ExceptionInfoType;

bool
writeString(PyObject* p, Ice::OutputStream* os)
{
    if(p == Py_None)
    {
        os->write(string(), false); // Bypass string conversion.
    }
    else if(checkString(p))
    {
        os->write(getString(p), false); // Bypass string conversion.
    }
    else
    {
        assert(false);
    }

    return true;
}

}

#ifdef WIN32
extern "C"
#endif
static TypeInfoObject*
typeInfoNew(PyTypeObject* type, PyObject* /*args*/, PyObject* /*kwds*/)
{
    TypeInfoObject* self = reinterpret_cast<TypeInfoObject*>(type->tp_alloc(type, 0));
    if(!self)
    {
        return 0;
    }
    self->info = 0;
    return self;
}

#ifdef WIN32
extern "C"
#endif
static void
typeInfoDealloc(TypeInfoObject* self)
{
    //
    // Every TypeInfo object is assigned to a "_t_XXX" variable in its enclosing module. Python releases these
    // objects during shutdown, which gives us a chance to release resources and break cyclic references.
    //
    assert(self->info);
    (*self->info)->destroy();
    delete self->info;
    Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

#ifdef WIN32
extern "C"
#endif
static ExceptionInfoObject*
exceptionInfoNew(PyTypeObject* type, PyObject* /*args*/, PyObject* /*kwds*/)
{
    ExceptionInfoObject* self = reinterpret_cast<ExceptionInfoObject*>(type->tp_alloc(type, 0));
    if(!self)
    {
        return 0;
    }
    self->info = 0;
    return self;
}

#ifdef WIN32
extern "C"
#endif
static void
exceptionInfoDealloc(ExceptionInfoObject* self)
{
    delete self->info;
    Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

#ifdef WIN32
extern "C"
#endif
static void
unsetDealloc(PyTypeObject* /*self*/)
{
    Py_FatalError("deallocating Unset");
}

#ifdef WIN32
extern "C"
#endif
static int
unsetNonzero(PyObject* /*v*/)
{
    //
    // We define tp_as_number->nb_nonzero so that the Unset marker value evaluates as "zero" or "false".
    //
    return 0;
}

#ifdef WIN32
extern "C"
#endif
static PyObject*
unsetRepr(PyObject* /*v*/)
{
    return PyBytes_FromString("Unset");
}

//
// addClassInfo()
//
static void
addClassInfo(const string& id, const ClassInfoPtr& info)
{
    //
    // Do not assert. An application may load statically-
    // translated definitions and then dynamically load
    // duplicate definitions.
    //
//    assert(_classInfoMap.find(id) == _classInfoMap.end());
    ClassInfoMap::iterator p = _classInfoMap.find(id);
    if(p != _classInfoMap.end())
    {
        _classInfoMap.erase(p);
    }
    _classInfoMap.insert(ClassInfoMap::value_type(id, info));
}

//
// addValueInfo()
//
static void
addValueInfo(const string& id, const ValueInfoPtr& info)
{
    //
    // Do not assert. An application may load statically-
    // translated definitions and then dynamically load
    // duplicate definitions.
    //
//    assert(_valueInfoMap.find(id) == _valueInfoMap.end());
    ValueInfoMap::iterator p = _valueInfoMap.find(id);
    if(p != _valueInfoMap.end())
    {
        _valueInfoMap.erase(p);
    }
    _valueInfoMap.insert(ValueInfoMap::value_type(id, info));
}

//
// addProxyInfo()
//
static void
addProxyInfo(const string& id, const ProxyInfoPtr& info)
{
    //
    // Do not assert. An application may load statically-
    // translated definitions and then dynamically load
    // duplicate definitions.
    //
//    assert(_proxyInfoMap.find(id) == _proxyInfoMap.end());
    ProxyInfoMap::iterator p = _proxyInfoMap.find(id);
    if(p != _proxyInfoMap.end())
    {
        _proxyInfoMap.erase(p);
    }
    _proxyInfoMap.insert(ProxyInfoMap::value_type(id, info));
}

//
// lookupProxyInfo()
//
static IcePy::ProxyInfoPtr
lookupProxyInfo(const string& id)
{
    ProxyInfoMap::iterator p = _proxyInfoMap.find(id);
    if(p != _proxyInfoMap.end())
    {
        return p->second;
    }
    return 0;
}

//
// addExceptionInfo()
//
static void
addExceptionInfo(const string& id, const ExceptionInfoPtr& info)
{
    //
    // Do not assert. An application may load statically-
    // translated definitions and then dynamically load
    // duplicate definitions.
    //
//    assert(_exceptionInfoMap.find(id) == _exceptionInfoMap.end());
    _exceptionInfoMap.insert(ExceptionInfoMap::value_type(id, info));
}

//
// StreamUtil implementation
//
PyObject* IcePy::StreamUtil::_slicedDataType = 0;
PyObject* IcePy::StreamUtil::_sliceInfoType = 0;

IcePy::StreamUtil::StreamUtil()
{
}

IcePy::StreamUtil::~StreamUtil()
{
    //
    // Make sure we break any cycles among the ValueReaders in preserved slices.
    //
    for(set<ValueReaderPtr>::iterator p = _readers.begin(); p != _readers.end(); ++p)
    {
        Ice::SlicedDataPtr slicedData = (*p)->getSlicedData();
        for(Ice::SliceInfoSeq::const_iterator q = slicedData->slices.begin(); q != slicedData->slices.end(); ++q)
        {
            //
            // Don't just call (*q)->instances.clear(), as releasing references
            // to the instances could have unexpected side effects. We exchange
            // the vector into a temporary and then let the temporary fall out
            // of scope.
            //
            vector<std::shared_ptr<Ice::Value>> tmp;
            tmp.swap((*q)->instances);
        }
    }
}

void
IcePy::StreamUtil::add(const ReadValueCallbackPtr& callback)
{
    _callbacks.push_back(callback);
}

void
IcePy::StreamUtil::add(const ValueReaderPtr& reader)
{
    assert(reader->getSlicedData());
    _readers.insert(reader);
}

void
IcePy::StreamUtil::updateSlicedData()
{
    for(set<ValueReaderPtr>::iterator p = _readers.begin(); p != _readers.end(); ++p)
    {
        setSlicedDataMember((*p)->getObject(), (*p)->getSlicedData());
    }
}

void
IcePy::StreamUtil::setSlicedDataMember(PyObject* obj, const Ice::SlicedDataPtr& slicedData)
{
    //
    // Create a Python equivalent of the SlicedData object.
    //

    assert(slicedData);

    if(!_slicedDataType)
    {
        _slicedDataType = lookupType("Ice.SlicedData");
        assert(_slicedDataType);
    }
    if(!_sliceInfoType)
    {
        _sliceInfoType = lookupType("Ice.SliceInfo");
        assert(_sliceInfoType);
    }

    IcePy::PyObjectHandle args = PyTuple_New(0);
    if(!args.get())
    {
        assert(PyErr_Occurred());
        throw AbortMarshaling();
    }

    PyObjectHandle sd = PyEval_CallObject(_slicedDataType, args.get());
    if(!sd.get())
    {
        assert(PyErr_Occurred());
        throw AbortMarshaling();
    }

    PyObjectHandle slices = PyTuple_New(static_cast<Py_ssize_t>(slicedData->slices.size()));
    if(!slices.get())
    {
        assert(PyErr_Occurred());
        throw AbortMarshaling();
    }

    if(PyObject_SetAttrString(sd.get(), STRCAST("slices"), slices.get()) < 0)
    {
        assert(PyErr_Occurred());
        throw AbortMarshaling();
    }

    //
    // Translate each SliceInfo object into its Python equivalent.
    //
    int i = 0;
    for(vector<Ice::SliceInfoPtr>::const_iterator p = slicedData->slices.begin(); p != slicedData->slices.end(); ++p)
    {
        PyObjectHandle slice = PyEval_CallObject(_sliceInfoType, args.get());
        if(!slice.get())
        {
            assert(PyErr_Occurred());
            throw AbortMarshaling();
        }

        PyTuple_SET_ITEM(slices.get(), i++, slice.get());
        Py_INCREF(slice.get()); // PyTuple_SET_ITEM steals a reference.

        //
        // typeId
        //
        PyObjectHandle typeId = createString((*p)->typeId);
        if(!typeId.get() || PyObject_SetAttrString(slice.get(), STRCAST("typeId"), typeId.get()) < 0)
        {
            assert(PyErr_Occurred());
            throw AbortMarshaling();
        }

        //
        // compactId
        //
        PyObjectHandle compactId = PyLong_FromLong((*p)->compactId);
        if(!compactId.get() || PyObject_SetAttrString(slice.get(), STRCAST("compactId"), compactId.get()) < 0)
        {
            assert(PyErr_Occurred());
            throw AbortMarshaling();
        }

        //
        // bytes
        //
        PyObjectHandle bytes;
        if((*p)->bytes.size() > 0)
        {
            bytes = PyBytes_FromStringAndSize((*p)->bytes.empty() ? 0 : reinterpret_cast<const char*>(&(*p)->bytes[0]),
                                              static_cast<Py_ssize_t>((*p)->bytes.size()));
        }
        else
        {
            bytes = PyBytes_FromStringAndSize(0, 0);
        }
        if(!bytes.get() || PyObject_SetAttrString(slice.get(), STRCAST("bytes"), bytes.get()) < 0)
        {
            assert(PyErr_Occurred());
            throw AbortMarshaling();
        }

        //
        // instances
        //
        PyObjectHandle instances = PyTuple_New(static_cast<Py_ssize_t>((*p)->instances.size()));
        if(!instances.get() || PyObject_SetAttrString(slice.get(), STRCAST("instances"), instances.get()) < 0)
        {
            assert(PyErr_Occurred());
            throw AbortMarshaling();
        }

        int j = 0;
        for(vector<std::shared_ptr<Ice::Value>>::iterator q = (*p)->instances.begin(); q != (*p)->instances.end(); ++q)
        {
            //
            // Each element in the instances list is an instance of ValueReader that wraps a Python object.
            //
            assert(*q);
            auto r = dynamic_pointer_cast<ValueReader>(*q);
            assert(r);
            PyObject* pobj = r->getObject();
            assert(pobj != Py_None); // Should be non-nil.
            PyTuple_SET_ITEM(instances.get(), j++, pobj);
            Py_INCREF(pobj); // PyTuple_SET_ITEM steals a reference.
        }

        //
        // hasOptionalMembers
        //
        PyObject* hasOptionalMembers = (*p)->hasOptionalMembers ? getTrue() : getFalse();
        if(PyObject_SetAttrString(slice.get(), STRCAST("hasOptionalMembers"), hasOptionalMembers) < 0)
        {
            assert(PyErr_Occurred());
            throw AbortMarshaling();
        }

        //
        // isLastSlice
        //
        PyObject* isLastSlice = (*p)->isLastSlice ? getTrue() : getFalse();
        if(PyObject_SetAttrString(slice.get(), STRCAST("isLastSlice"), isLastSlice) < 0)
        {
            assert(PyErr_Occurred());
            throw AbortMarshaling();
        }
    }

    if(PyObject_SetAttrString(obj, STRCAST("_ice_slicedData"), sd.get()) < 0)
    {
        assert(PyErr_Occurred());
        throw AbortMarshaling();
    }
}

//
// Instances of preserved class and exception types may have a data member
// named _ice_slicedData which is an instance of the Python class Ice.SlicedData.
//
Ice::SlicedDataPtr
IcePy::StreamUtil::getSlicedDataMember(PyObject* obj, ObjectMap* objectMap)
{
    Ice::SlicedDataPtr slicedData;

    if(PyObject_HasAttrString(obj, STRCAST("_ice_slicedData")))
    {
        PyObjectHandle sd = getAttr(obj, "_ice_slicedData", false);
        assert(sd.get());

        if(sd.get() != Py_None)
        {
            //
            // The "slices" member is a tuple of Ice.SliceInfo objects.
            //
            PyObjectHandle sl = getAttr(sd.get(), "slices", false);
            assert(sl.get());
            assert(PyTuple_Check(sl.get()));

            Ice::SliceInfoSeq slices;

            Py_ssize_t sz = PyTuple_GET_SIZE(sl.get());
            for(Py_ssize_t i = 0; i < sz; ++i)
            {
                PyObjectHandle s = PyTuple_GET_ITEM(sl.get(), i);
                Py_INCREF(s.get());

                Ice::SliceInfoPtr info = std::make_shared<Ice::SliceInfo>();

                PyObjectHandle typeId = getAttr(s.get(), "typeId", false);
                assert(typeId.get());
                info->typeId = getString(typeId.get());

                PyObjectHandle compactId = getAttr(s.get(), "compactId", false);
                assert(compactId.get());
                info->compactId = static_cast<int>(PyLong_AsLong(compactId.get()));

                PyObjectHandle bytes = getAttr(s.get(), "bytes", false);
                assert(bytes.get());
                char* str;
                Py_ssize_t strsz;
                assert(PyBytes_Check(bytes.get()));
                PyBytes_AsStringAndSize(bytes.get(), &str, &strsz);
                vector<Ice::Byte> vtmp(reinterpret_cast<Ice::Byte*>(str), reinterpret_cast<Ice::Byte*>(str + strsz));
                info->bytes.swap(vtmp);

                PyObjectHandle instances = getAttr(s.get(), "instances", false);
                assert(instances.get());
                assert(PyTuple_Check(instances.get()));
                Py_ssize_t osz = PyTuple_GET_SIZE(instances.get());
                for(Py_ssize_t j = 0; j < osz; ++j)
                {
                    PyObject* o = PyTuple_GET_ITEM(instances.get(), j);

                    std::shared_ptr<Ice::Value> writer;

                    ObjectMap::iterator k = objectMap->find(o);
                    if(k == objectMap->end())
                    {
                        writer = make_shared<ValueWriter>(o, objectMap, nullptr);
                        objectMap->insert(ObjectMap::value_type(o, writer));
                    }
                    else
                    {
                        writer = k->second;
                    }

                    info->instances.push_back(writer);
                }

                PyObjectHandle hasOptionalMembers = getAttr(s.get(), "hasOptionalMembers", false);
                assert(hasOptionalMembers.get());
                info->hasOptionalMembers = PyObject_IsTrue(hasOptionalMembers.get()) ? true : false;

                PyObjectHandle isLastSlice = getAttr(s.get(), "isLastSlice", false);
                assert(isLastSlice.get());
                info->isLastSlice = PyObject_IsTrue(isLastSlice.get()) ? true : false;

                slices.push_back(info);
            }

            slicedData = std::make_shared<Ice::SlicedData>(slices);
        }
    }

    return slicedData;
}

//
// UnmarshalCallback implementation.
//
IcePy::UnmarshalCallback::~UnmarshalCallback()
{
}

//
// TypeInfo implementation.
//
IcePy::TypeInfo::TypeInfo()
{
}

bool
IcePy::TypeInfo::usesClasses() const
{
    return false;
}

void
IcePy::TypeInfo::unmarshaled(PyObject*, PyObject*, void*)
{
    assert(false);
}

void
IcePy::TypeInfo::destroy()
{
}

//
// PrimitiveInfo implementation.
//
IcePy::PrimitiveInfo::PrimitiveInfo(Kind k) :
    kind(k)
{
}

string
IcePy::PrimitiveInfo::getId() const
{
    switch(kind)
    {
    case KindBool:
        return "bool";
    case KindByte:
        return "byte";
    case KindShort:
        return "short";
    case KindInt:
        return "int";
    case KindLong:
        return "long";
    case KindFloat:
        return "float";
    case KindDouble:
        return "double";
    case KindString:
        return "string";
    }
    assert(false);
    return string();
}

bool
IcePy::PrimitiveInfo::validate(PyObject* p)
{
    switch(kind)
    {
    case PrimitiveInfo::KindBool:
    {
        int isTrue = PyObject_IsTrue(p);
        if(isTrue < 0)
        {
            return false;
        }
        break;
    }
    case PrimitiveInfo::KindByte:
    {
        long val = PyLong_AsLong(p);

        if(PyErr_Occurred() || val < 0 || val > 255)
        {
            return false;
        }
        break;
    }
    case PrimitiveInfo::KindShort:
    {
        long val = PyLong_AsLong(p);

        if(PyErr_Occurred() || val < SHRT_MIN || val > SHRT_MAX)
        {
            return false;
        }
        break;
    }
    case PrimitiveInfo::KindInt:
    {
        long val = PyLong_AsLong(p);

        if(PyErr_Occurred() || val < INT_MIN || val > INT_MAX)
        {
            return false;
        }
        break;
    }
    case PrimitiveInfo::KindLong:
    {
        PyLong_AsLongLong(p); // Just to see if it raises an error.

        if(PyErr_Occurred())
        {
            return false;
        }

        break;
    }
    case PrimitiveInfo::KindFloat:
    {
        if(!PyFloat_Check(p))
        {
            if(PyLong_Check(p))
            {
                PyLong_AsDouble(p); // Just to see if it raises an error.
                if(PyErr_Occurred())
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
        else
        {
            // Ensure double does not exceed maximum float value before casting
            double val = PyFloat_AsDouble(p);
            return (val <= numeric_limits<float>::max() && val >= -numeric_limits<float>::max()) || !isfinite(val);
        }

        break;
    }
    case PrimitiveInfo::KindDouble:
    {
        if(!PyFloat_Check(p))
        {
            if(PyLong_Check(p))
            {
                PyLong_AsDouble(p); // Just to see if it raises an error.
                if(PyErr_Occurred())
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }

        break;
    }
    case PrimitiveInfo::KindString:
    {
        if(p != Py_None && !checkString(p))
        {
            return false;
        }
        break;
    }
    }

    return true;
}

bool
IcePy::PrimitiveInfo::variableLength() const
{
    return kind == KindString;
}

int
IcePy::PrimitiveInfo::wireSize() const
{
    switch(kind)
    {
    case KindBool:
    case KindByte:
        return 1;
    case KindShort:
        return 2;
    case KindInt:
        return 4;
    case KindLong:
        return 8;
    case KindFloat:
        return 4;
    case KindDouble:
        return 8;
    case KindString:
        return 1;
    }
    assert(false);
    return 0;
}

Ice::OptionalFormat
IcePy::PrimitiveInfo::optionalFormat() const
{
    switch(kind)
    {
    case KindBool:
    case KindByte:
        return Ice::OptionalFormat::F1;
    case KindShort:
        return Ice::OptionalFormat::F2;
    case KindInt:
        return Ice::OptionalFormat::F4;
    case KindLong:
        return Ice::OptionalFormat::F8;
    case KindFloat:
        return Ice::OptionalFormat::F4;
    case KindDouble:
        return Ice::OptionalFormat::F8;
    case KindString:
        return Ice::OptionalFormat::VSize;
    }

    assert(false);
    return Ice::OptionalFormat::F1;
}

void
IcePy::PrimitiveInfo::marshal(PyObject* p, Ice::OutputStream* os, ObjectMap*, bool, const Ice::StringSeq*)
{
    switch(kind)
    {
    case PrimitiveInfo::KindBool:
    {
        int isTrue = PyObject_IsTrue(p);
        if(isTrue < 0)
        {
            assert(false); // validate() should have caught this.
        }
        os->write(isTrue ? true : false);
        break;
    }
    case PrimitiveInfo::KindByte:
    {
        long val = PyLong_AsLong(p);
        assert(!PyErr_Occurred()); // validate() should have caught this.
        assert(val >= 0 && val <= 255); // validate() should have caught this.
        os->write(static_cast<Ice::Byte>(val));
        break;
    }
    case PrimitiveInfo::KindShort:
    {
        long val = PyLong_AsLong(p);
        assert(!PyErr_Occurred()); // validate() should have caught this.
        assert(val >= SHRT_MIN && val <= SHRT_MAX); // validate() should have caught this.
        os->write(static_cast<Ice::Short>(val));
        break;
    }
    case PrimitiveInfo::KindInt:
    {
        long val = PyLong_AsLong(p);
        assert(!PyErr_Occurred()); // validate() should have caught this.
        assert(val >= INT_MIN && val <= INT_MAX); // validate() should have caught this.
        os->write(static_cast<int32_t>(val));
        break;
    }
    case PrimitiveInfo::KindLong:
    {
        int64_t val = PyLong_AsLongLong(p);
        assert(!PyErr_Occurred()); // validate() should have caught this.
        os->write(val);
        break;
    }
    case PrimitiveInfo::KindFloat:
    {
        float val = static_cast<float>(PyFloat_AsDouble(p)); // Attempts to perform conversion.
        if(PyErr_Occurred())
        {
            throw AbortMarshaling();
        }

        os->write(val);
        break;
    }
    case PrimitiveInfo::KindDouble:
    {
        double val = PyFloat_AsDouble(p); // Attempts to perform conversion.
        if(PyErr_Occurred())
        {
            throw AbortMarshaling();
        }

        os->write(val);
        break;
    }
    case PrimitiveInfo::KindString:
    {
        if(!writeString(p, os))
        {
            assert(PyErr_Occurred());
            throw AbortMarshaling();
        }
        break;
    }
    }
}

void
IcePy::PrimitiveInfo::unmarshal(Ice::InputStream* is, const UnmarshalCallbackPtr& cb, PyObject* target,
                                void* closure, bool, const Ice::StringSeq*)
{
    switch(kind)
    {
    case PrimitiveInfo::KindBool:
    {
        bool b;
        is->read(b);
        if(b)
        {
            cb->unmarshaled(getTrue(), target, closure);
        }
        else
        {
            cb->unmarshaled(getFalse(), target, closure);
        }
        break;
    }
    case PrimitiveInfo::KindByte:
    {
        Ice::Byte val;
        is->read(val);
        PyObjectHandle p = PyLong_FromLong(val);
        cb->unmarshaled(p.get(), target, closure);
        break;
    }
    case PrimitiveInfo::KindShort:
    {
        Ice::Short val;
        is->read(val);
        PyObjectHandle p = PyLong_FromLong(val);
        cb->unmarshaled(p.get(), target, closure);
        break;
    }
    case PrimitiveInfo::KindInt:
    {
        int32_t val;
        is->read(val);
        PyObjectHandle p = PyLong_FromLong(val);
        cb->unmarshaled(p.get(), target, closure);
        break;
    }
    case PrimitiveInfo::KindLong:
    {
        int64_t val;
        is->read(val);
        PyObjectHandle p = PyLong_FromLongLong(val);
        cb->unmarshaled(p.get(), target, closure);
        break;
    }
    case PrimitiveInfo::KindFloat:
    {
        Ice::Float val;
        is->read(val);
        PyObjectHandle p = PyFloat_FromDouble(val);
        cb->unmarshaled(p.get(), target, closure);
        break;
    }
    case PrimitiveInfo::KindDouble:
    {
        Ice::Double val;
        is->read(val);
        PyObjectHandle p = PyFloat_FromDouble(val);
        cb->unmarshaled(p.get(), target, closure);
        break;
    }
    case PrimitiveInfo::KindString:
    {
        string val;
        is->read(val, false); // Bypass string conversion.
        PyObjectHandle p = createString(val);
        cb->unmarshaled(p.get(), target, closure);
        break;
    }
    }
}

void
IcePy::PrimitiveInfo::print(PyObject* value, IceUtilInternal::Output& out, PrintObjectHistory*)
{
    if(!validate(value))
    {
        out << "<invalid value - expected " << getId() << ">";
        return;
    }
    PyObjectHandle p = PyObject_Str(value);
    if(!p.get())
    {
        return;
    }
    assert(checkString(p.get()));
    out << getString(p.get());
}

//
// EnumInfo implementation.
//
IcePy::EnumInfo::EnumInfo(const string& ident, PyObject* t, PyObject* e) :
    id(ident), pythonType(t), maxValue(0)
{
    assert(PyType_Check(t));
    assert(PyDict_Check(e));

    Py_ssize_t pos = 0;
    PyObject* key;
    PyObject* value;
    while(PyDict_Next(e, &pos, &key, &value))
    {
        assert(PyLong_Check(key));
        const int32_t val = static_cast<int32_t>(PyLong_AsLong(key));
        assert(enumerators.find(val) == enumerators.end());

        Py_INCREF(value);
        assert(PyObject_IsInstance(value, t));
        const_cast<EnumeratorMap&>(enumerators)[val] = value;

        if(val > maxValue)
        {
            const_cast<int32_t&>(maxValue) = val;
        }
    }
}

string
IcePy::EnumInfo::getId() const
{
    return id;
}

bool
IcePy::EnumInfo::validate(PyObject* val)
{
    return PyObject_IsInstance(val, pythonType) == 1;
}

bool
IcePy::EnumInfo::variableLength() const
{
    return true;
}

int
IcePy::EnumInfo::wireSize() const
{
    return 1;
}

Ice::OptionalFormat
IcePy::EnumInfo::optionalFormat() const
{
    return Ice::OptionalFormat::Size;
}

void
IcePy::EnumInfo::marshal(PyObject* p, Ice::OutputStream* os, ObjectMap*, bool /*optional*/, const Ice::StringSeq*)
{
    //
    // Validate value.
    //
    const int32_t val = valueForEnumerator(p);
    if(val < 0)
    {
        assert(PyErr_Occurred());
        throw AbortMarshaling();
    }

    os->writeEnum(val, maxValue);
}

void
IcePy::EnumInfo::unmarshal(Ice::InputStream* is, const UnmarshalCallbackPtr& cb, PyObject* target,
                           void* closure, bool, const Ice::StringSeq*)
{
    int32_t val = is->readEnum(maxValue);

    PyObjectHandle p = enumeratorForValue(val);
    if(!p.get())
    {
        ostringstream ostr;
        ostr << "enumerator " << val << " is out of range for enum " << id;
        setPythonException(Ice::MarshalException(__FILE__, __LINE__, ostr.str()));
        throw AbortMarshaling();
    }

    cb->unmarshaled(p.get(), target, closure);
}

void
IcePy::EnumInfo::print(PyObject* value, IceUtilInternal::Output& out, PrintObjectHistory*)
{
    if(!validate(value))
    {
        out << "<invalid value - expected " << id << ">";
        return;
    }
    PyObjectHandle p = PyObject_Str(value);
    if(!p.get())
    {
        return;
    }
    assert(checkString(p.get()));
    out << getString(p.get());
}

void
IcePy::EnumInfo::destroy()
{
    const_cast<EnumeratorMap&>(enumerators).clear();
}

int32_t
IcePy::EnumInfo::valueForEnumerator(PyObject* p) const
{
    assert(PyObject_IsInstance(p, pythonType) == 1);

    PyObjectHandle v = PyObject_GetAttrString(p, STRCAST("_value"));
    if(!v.get())
    {
        assert(PyErr_Occurred());
        return -1;
    }
    if(!PyLong_Check(v.get()))
    {
        PyErr_Format(PyExc_ValueError, STRCAST("value for enum %s is not an int"), id.c_str());
        return -1;
    }
    const int32_t val = static_cast<int32_t>(PyLong_AsLong(v.get()));
    if(enumerators.find(val) == enumerators.end())
    {
        PyErr_Format(PyExc_ValueError, STRCAST("illegal value %d for enum %s"), val, id.c_str());
        return -1;
    }

    return val;
}

PyObject*
IcePy::EnumInfo::enumeratorForValue(int32_t v) const
{
    EnumeratorMap::const_iterator p = enumerators.find(v);
    if(p == enumerators.end())
    {
        return 0;
    }
    PyObject* r = p->second.get();
    Py_INCREF(r);
    return r;
}

//
// DataMember implementation.
//
void
IcePy::DataMember::unmarshaled(PyObject* val, PyObject* target, void*)
{
    if(PyObject_SetAttrString(target, const_cast<char*>(name.c_str()), val) < 0)
    {
        assert(PyErr_Occurred());
        throw AbortMarshaling();
    }
}

static void
convertDataMembers(PyObject* members, DataMemberList& reqMembers, DataMemberList& optMembers, bool allowOptional)
{
    list<DataMemberPtr> optList;

    Py_ssize_t sz = PyTuple_GET_SIZE(members);
    for(Py_ssize_t i = 0; i < sz; ++i)
    {
        PyObject* m = PyTuple_GET_ITEM(members, i);
        assert(PyTuple_Check(m));
        assert(PyTuple_GET_SIZE(m) == (allowOptional ? 5 : 3));

        PyObject* name = PyTuple_GET_ITEM(m, 0); // Member name.
        assert(checkString(name));
        PyObject* meta = PyTuple_GET_ITEM(m, 1); // Member metadata.
        assert(PyTuple_Check(meta));
        PyObject* t = PyTuple_GET_ITEM(m, 2); // Member type.

        PyObject* opt = 0;
        PyObject* tag = 0;
        if(allowOptional)
        {
            opt = PyTuple_GET_ITEM(m, 3); // Optional?
            tag = PyTuple_GET_ITEM(m, 4);
            assert(PyLong_Check(tag));
        }

        DataMemberPtr member = make_shared<DataMember>();
        member->name = getString(name);
#ifndef NDEBUG
        bool b =
#endif
        tupleToStringSeq(meta, member->metaData);
        assert(b);
        member->type = getType(t);
        if(allowOptional)
        {
            member->optional = PyObject_IsTrue(opt) == 1;
            member->tag = static_cast<int>(PyLong_AsLong(tag));
        }
        else
        {
            member->optional = false;
            member->tag = 0;
        }

        if(member->optional)
        {
            optList.push_back(member);
        }
        else
        {
            reqMembers.push_back(member);
        }
    }

    if(allowOptional)
    {
        class SortFn
        {
        public:
            static bool compare(const DataMemberPtr& lhs, const DataMemberPtr& rhs)
            {
                return lhs->tag < rhs->tag;
            }
        };

        optList.sort(SortFn::compare);
        copy(optList.begin(), optList.end(), back_inserter(optMembers));
    }
}

//
// StructInfo implementation.
//
IcePy::StructInfo::StructInfo(const string& ident, PyObject* t, PyObject* m) :
    id(ident), pythonType(t)
{
    assert(PyType_Check(t));
    assert(PyTuple_Check(m));

    DataMemberList opt;
    convertDataMembers(m, const_cast<DataMemberList&>(members), opt, false);
    assert(opt.empty());

    _variableLength = false;
    _wireSize = 0;
    for(DataMemberList::const_iterator p = members.begin(); p != members.end(); ++p)
    {
        if(!_variableLength && (*p)->type->variableLength())
        {
            _variableLength = true;
        }
        _wireSize += (*p)->type->wireSize();
    }
}

string
IcePy::StructInfo::getId() const
{
    return id;
}

bool
IcePy::StructInfo::validate(PyObject* val)
{
    return val == Py_None || PyObject_IsInstance(val, pythonType) == 1;
}

bool
IcePy::StructInfo::variableLength() const
{
    return _variableLength;
}

int
IcePy::StructInfo::wireSize() const
{
    return _wireSize;
}

Ice::OptionalFormat
IcePy::StructInfo::optionalFormat() const
{
    return _variableLength ? Ice::OptionalFormat::FSize : Ice::OptionalFormat::VSize;
}

bool
IcePy::StructInfo::usesClasses() const
{
    for(DataMemberList::const_iterator p = members.begin(); p != members.end(); ++p)
    {
        if((*p)->type->usesClasses())
        {
            return true;
        }
    }

    return false;
}

void
IcePy::StructInfo::marshal(PyObject* p, Ice::OutputStream* os, ObjectMap* objectMap, bool optional,
                           const Ice::StringSeq*)
{
    assert(p == Py_None || PyObject_IsInstance(p, pythonType) == 1); // validate() should have caught this.

    if(p == Py_None)
    {
        if(!_nullMarshalValue.get())
        {
            PyObjectHandle args = PyTuple_New(0);
            PyTypeObject* type = reinterpret_cast<PyTypeObject*>(pythonType);
            _nullMarshalValue = type->tp_new(type, args.get(), 0);
            type->tp_init(_nullMarshalValue.get(), args.get(), 0); // Initialize the struct members
        }
        p = _nullMarshalValue.get();
    }

    Ice::OutputStream::size_type sizePos = 0;
    if(optional)
    {
        if(_variableLength)
        {
            sizePos = os->startSize();
        }
        else
        {
            os->writeSize(_wireSize);
        }
    }

    for(DataMemberList::const_iterator q = members.begin(); q != members.end(); ++q)
    {
        DataMemberPtr member = *q;
        char* memberName = const_cast<char*>(member->name.c_str());
        PyObjectHandle attr = getAttr(p, member->name, true);
        if(!attr.get())
        {
            PyErr_Format(PyExc_AttributeError, STRCAST("no member `%s' found in %s value"), memberName,
                         const_cast<char*>(id.c_str()));
            throw AbortMarshaling();
        }
        if(!member->type->validate(attr.get()))
        {
            PyErr_Format(PyExc_ValueError, STRCAST("invalid value for %s member `%s'"), const_cast<char*>(id.c_str()),
                         memberName);
            throw AbortMarshaling();
        }
        member->type->marshal(attr.get(), os, objectMap, false, &member->metaData);
    }

    if(optional && _variableLength)
    {
        os->endSize(sizePos);
    }
}

void
IcePy::StructInfo::unmarshal(Ice::InputStream* is, const UnmarshalCallbackPtr& cb, PyObject* target,
                             void* closure, bool optional, const Ice::StringSeq*)
{
    PyObjectHandle p = instantiate(pythonType);
    if(!p.get())
    {
        assert(PyErr_Occurred());
        throw AbortMarshaling();
    }

    if(optional)
    {
        if(_variableLength)
        {
            is->skip(4);
        }
        else
        {
            is->skipSize();
        }
    }

    for(DataMemberList::const_iterator q = members.begin(); q != members.end(); ++q)
    {
        DataMemberPtr member = *q;
        member->type->unmarshal(is, member, p.get(), 0, false, &member->metaData);
    }

    cb->unmarshaled(p.get(), target, closure);
}

void
IcePy::StructInfo::print(PyObject* value, IceUtilInternal::Output& out, PrintObjectHistory* history)
{
    if(!validate(value))
    {
        out << "<invalid value - expected " << id << ">";
        return;
    }

    if(value == Py_None)
    {
        out << "<nil>";
    }
    else
    {
        out.sb();
        for(DataMemberList::const_iterator q = members.begin(); q != members.end(); ++q)
        {
            DataMemberPtr member = *q;
            PyObjectHandle attr = getAttr(value, member->name, true);
            out << nl << member->name << " = ";
            if(!attr.get())
            {
                out << "<not defined>";
            }
            else
            {
                member->type->print(attr.get(), out, history);
            }
        }
        out.eb();
    }
}

void
IcePy::StructInfo::destroy()
{
    const_cast<DataMemberList&>(members).clear();
    _nullMarshalValue = 0;
}

PyObject*
IcePy::StructInfo::instantiate(PyObject* pythonType)
{
    PyObjectHandle args = PyTuple_New(0);
    PyTypeObject* type = reinterpret_cast<PyTypeObject*>(pythonType);
    return type->tp_new(type, args.get(), 0);
}

//
// SequenceInfo implementation.
//
IcePy::SequenceInfo::SequenceInfo(const string& ident, PyObject* m, PyObject* t) :
    id(ident)
{
    assert(PyTuple_Check(m));

    vector<string> metaData;
#ifndef NDEBUG
    bool b =
#endif
    tupleToStringSeq(m, metaData);
    assert(b);

    const_cast<SequenceMappingPtr&>(mapping) = make_shared<SequenceMapping>(metaData);
    mapping->init(metaData);
    const_cast<TypeInfoPtr&>(elementType) = getType(t);
}

string
IcePy::SequenceInfo::getId() const
{
    return id;
}

bool
IcePy::SequenceInfo::validate(PyObject* val)
{
    return val == Py_None || PySequence_Check(val) == 1;
}

bool
IcePy::SequenceInfo::variableLength() const
{
    return true;
}

int
IcePy::SequenceInfo::wireSize() const
{
    return 1;
}

Ice::OptionalFormat
IcePy::SequenceInfo::optionalFormat() const
{
    return elementType->variableLength() ? Ice::OptionalFormat::FSize : Ice::OptionalFormat::VSize;
}

bool
IcePy::SequenceInfo::usesClasses() const
{
    return elementType->usesClasses();
}

void
IcePy::SequenceInfo::marshal(PyObject* p, Ice::OutputStream* os, ObjectMap* objectMap, bool optional,
                             const Ice::StringSeq* /*metaData*/)
{
    auto pi = dynamic_pointer_cast<PrimitiveInfo>(elementType);

    Ice::OutputStream::size_type sizePos = 0;
    if(optional)
    {
        if(elementType->variableLength())
        {
            sizePos = os->startSize();
        }
        else if(elementType->wireSize() > 1)
        {
            //
            // Determine the sequence size.
            //
            Py_ssize_t sz = 0;
            if(p != Py_None)
            {
                const void* buf = 0;
                if(PyObject_AsReadBuffer(p, &buf, &sz) == 0)
                {
                    if(pi->kind == PrimitiveInfo::KindString)
                    {
                        PyErr_Format(PyExc_ValueError, STRCAST("expected sequence value"));
                        throw AbortMarshaling();
                    }
                }
                else
                {
                    PyErr_Clear(); // PyObject_AsReadBuffer sets an exception on failure.

                    PyObjectHandle fs;
                    if(pi)
                    {
                        fs = getSequence(pi, p);
                    }
                    else
                    {
                        fs = PySequence_Fast(p, STRCAST("expected a sequence value"));
                    }
                    if(!fs.get())
                    {
                        assert(PyErr_Occurred());
                        return;
                    }
                    sz = PySequence_Fast_GET_SIZE(fs.get());
                }
            }

            const int32_t isz = static_cast<int32_t>(sz);
            os->writeSize(isz == 0 ? 1 : isz * elementType->wireSize() + (isz > 254 ? 5 : 1));
        }
    }

    if(p == Py_None)
    {
        os->writeSize(0);
    }
    else if(pi)
    {
        marshalPrimitiveSequence(pi, p, os);
    }
    else
    {
        PyObjectHandle fastSeq = PySequence_Fast(p, STRCAST("expected a sequence value"));
        if(!fastSeq.get())
        {
            return;
        }

        Py_ssize_t sz = PySequence_Fast_GET_SIZE(fastSeq.get());
        os->writeSize(static_cast<int>(sz));
        for(Py_ssize_t i = 0; i < sz; ++i)
        {
            PyObject* item = PySequence_Fast_GET_ITEM(fastSeq.get(), i);
            if(!item)
            {
                assert(PyErr_Occurred());
                throw AbortMarshaling();
            }
            if(!elementType->validate(item))
            {
                PyErr_Format(PyExc_ValueError, STRCAST("invalid value for element %d of `%s'"), static_cast<int>(i),
                             const_cast<char*>(id.c_str()));
                throw AbortMarshaling();
            }
            elementType->marshal(item, os, objectMap, false);
        }
    }

    if(optional && elementType->variableLength())
    {
        os->endSize(sizePos);
    }
}

void
IcePy::SequenceInfo::unmarshal(Ice::InputStream* is, const UnmarshalCallbackPtr& cb, PyObject* target,
                               void* closure, bool optional, const Ice::StringSeq* metaData)
{
    if(optional)
    {
        if(elementType->variableLength())
        {
            is->skip(4);
        }
        else if(elementType->wireSize() > 1)
        {
            is->skipSize();
        }
    }

    //
    // Determine the mapping to use for this sequence. Highest priority is given
    // to the metaData argument, otherwise we use the mapping of the sequence
    // definition.
    //
    SequenceMappingPtr sm;
    if(metaData)
    {
        SequenceMapping::Type type;
        if(!SequenceMapping::getType(*metaData, type) || type == mapping->type)
        {
            sm = mapping;
        }
        else
        {
            sm = make_shared<SequenceMapping>(type);
            try
            {
                sm->init(*metaData);
            }
            catch(const InvalidSequenceFactoryException&)
            {
                throw AbortMarshaling();
            }
        }
    }
    else
    {
        sm = mapping;
    }

    auto pi = dynamic_pointer_cast<PrimitiveInfo>(elementType);
    if(pi)
    {
        unmarshalPrimitiveSequence(pi, is, cb, target, closure, sm);
        return;
    }

    int32_t sz = is->readSize();
    PyObjectHandle result = sm->createContainer(sz);

    if(!result.get())
    {
        assert(PyErr_Occurred());
        throw AbortMarshaling();
    }

    for(int32_t i = 0; i < sz; ++i)
    {
        void* cl = reinterpret_cast<void*>(static_cast<Py_ssize_t>(i));
        elementType->unmarshal(is, sm, result.get(), cl, false);
    }

    cb->unmarshaled(result.get(), target, closure);
}

void
IcePy::SequenceInfo::print(PyObject* value, IceUtilInternal::Output& out, PrintObjectHistory* history)
{
    if(!validate(value))
    {
        out << "<invalid value - expected " << id << ">";
        return;
    }

    if(value == Py_None)
    {
        out << "{}";
    }
    else
    {
        PyObjectHandle fastSeq = PySequence_Fast(value, STRCAST("expected a sequence value"));
        if(!fastSeq.get())
        {
            return;
        }

        Py_ssize_t sz = PySequence_Fast_GET_SIZE(fastSeq.get());

        out.sb();
        for(Py_ssize_t i = 0; i < sz; ++i)
        {
            PyObject* item = PySequence_Fast_GET_ITEM(fastSeq.get(), i);
            if(!item)
            {
                break;
            }
            out << nl << '[' << static_cast<int>(i) << "] = ";
            elementType->print(item, out, history);
        }
        out.eb();
    }
}

void
IcePy::SequenceInfo::destroy()
{
    const_cast<TypeInfoPtr&>(elementType) = 0;
}

PyObject*
IcePy::SequenceInfo::getSequence(const PrimitiveInfoPtr& pi, PyObject* p)
{
    PyObjectHandle fs;

    if(pi->kind == PrimitiveInfo::KindByte)
    {
        //
        // For sequence<byte>, accept a bytes object or a sequence.
        //
        if(!PyBytes_Check(p))
        {
            fs = PySequence_Fast(p, STRCAST("expected a bytes, sequence, or buffer value"));
        }
    }
    else
    {
        fs = PySequence_Fast(p, STRCAST("expected a sequence or buffer value"));
    }

    return fs.release();
}

void
IcePy::SequenceInfo::marshalPrimitiveSequence(const PrimitiveInfoPtr& pi, PyObject* p, Ice::OutputStream* os)
{
    //
    // For most types, we accept an object that implements the buffer protocol
    // (this includes the array.array type).
    //
    if(pi->kind != PrimitiveInfo::KindString)
    {
        //
        // With Python 3 and greater we marshal sequences of pritive types using the new
        // buffer protocol when possible, for older versions we use the old buffer protocol.
        //
        Py_buffer pybuf;
        if(PyObject_GetBuffer(p, &pybuf, PyBUF_SIMPLE | PyBUF_FORMAT) == 0)
        {
            static const int itemsize[] =
            {
                1, // KindBool,
                1, // KindByte,
                2, // KindShort,
                4, // KindInt,
                8, // KindLong,
                4, // KindFloat,
                8, // KindDouble
            };

            static const char* itemtype[] =
            {
                "bool",   // KindBool,
                "byte",   // KindByte,
                "short",  // KindShort,
                "int",    // KindInt,
                "long",   // KindLong,
                "float",  // KindFloat,
                "double", // KindDouble
            };

            if(pi->kind != PrimitiveInfo::KindByte)
            {
#   ifdef ICE_BIG_ENDIAN
                if(pybuf.format != 0 && pybuf.format[0] == '<')
                {
                    PyErr_Format(PyExc_ValueError,
                                 "sequence buffer byte order doesn't match the platform native byte-order "
                                 "`big-endian'");
                    PyBuffer_Release(&pybuf);
                    throw AbortMarshaling();
                }
#   else
                if(pybuf.format != 0 && (pybuf.format[0] == '>' || pybuf.format[0] == '!'))
                {
                    PyErr_Format(PyExc_ValueError,
                                 "sequence buffer byte order doesn't match the platform native byte-order "
                                 "`little-endian'");
                    PyBuffer_Release(&pybuf);
                    throw AbortMarshaling();
                }
#   endif
                if(pybuf.itemsize != itemsize[pi->kind])
                {
                    PyErr_Format(PyExc_ValueError,
                                 "sequence item size doesn't match the size of the sequence type `%s'",
                                 itemtype[pi->kind]);
                    PyBuffer_Release(&pybuf);
                    throw AbortMarshaling();
                }
            }
            const Ice::Byte* b = reinterpret_cast<const Ice::Byte*>(pybuf.buf);
            Py_ssize_t sz = pybuf.len;

            switch(pi->kind)
            {
                case PrimitiveInfo::KindBool:
                {
                    os->write(reinterpret_cast<const bool*>(b),
                              reinterpret_cast<const bool*>(b + sz));
                    break;
                }
                case PrimitiveInfo::KindByte:
                {
                    os->write(reinterpret_cast<const Ice::Byte*>(b),
                              reinterpret_cast<const Ice::Byte*>(b + sz));
                    break;
                }
                case PrimitiveInfo::KindShort:
                {
                    os->write(reinterpret_cast<const Ice::Short*>(b),
                              reinterpret_cast<const Ice::Short*>(b + sz));
                    break;
                }
                case PrimitiveInfo::KindInt:
                {
                    os->write(reinterpret_cast<const int32_t*>(b),
                              reinterpret_cast<const int32_t*>(b + sz));
                    break;
                }
                case PrimitiveInfo::KindLong:
                {
                    os->write(reinterpret_cast<const int64_t*>(b),
                              reinterpret_cast<const int64_t*>(b + sz));
                    break;
                }
                case PrimitiveInfo::KindFloat:
                {
                    os->write(reinterpret_cast<const Ice::Float*>(b),
                              reinterpret_cast<const Ice::Float*>(b + sz));
                    break;
                }
                case PrimitiveInfo::KindDouble:
                {
                    os->write(reinterpret_cast<const Ice::Double*>(b),
                              reinterpret_cast<const Ice::Double*>(b + sz));
                    break;
                }
                default:
                {
                    assert(false);
                }
            }
            PyBuffer_Release(&pybuf);
            return;
        }
        else
        {
            PyErr_Clear(); // PyObject_GetBuffer/PyObject_AsReadBuffer sets an exception on failure.
        }
    }

    PyObjectHandle fs = getSequence(pi, p);
    if(!fs.get())
    {
        assert(PyErr_Occurred());
        return;
    }

    Py_ssize_t sz = 0;
    switch(pi->kind)
    {
    case PrimitiveInfo::KindBool:
    {
        sz = PySequence_Fast_GET_SIZE(fs.get());
        Ice::BoolSeq seq(static_cast<size_t>(sz));
        for(Py_ssize_t i = 0; i < sz; ++i)
        {
            PyObject* item = PySequence_Fast_GET_ITEM(fs.get(), i);
            if(!item)
            {
                assert(PyErr_Occurred());
                throw AbortMarshaling();
            }
            int isTrue = PyObject_IsTrue(item);
            if(isTrue < 0)
            {
                PyErr_Format(PyExc_ValueError, STRCAST("invalid value for element %d of sequence<bool>"),
                             static_cast<int>(i));
                throw AbortMarshaling();
            }
            seq[static_cast<size_t>(i)] = isTrue ? true : false;
        }
        os->write(seq);
        break;
    }
    case PrimitiveInfo::KindByte:
    {
        if(!fs.get())
        {
            assert(PyBytes_Check(p));
            char* str;
            PyBytes_AsStringAndSize(p, &str, &sz);
            os->write(reinterpret_cast<const Ice::Byte*>(str), reinterpret_cast<const Ice::Byte*>(str + sz));
        }
        else
        {
            sz = PySequence_Fast_GET_SIZE(fs.get());
            Ice::ByteSeq seq(static_cast<size_t>(sz));
            for(Py_ssize_t i = 0; i < sz; ++i)
            {
                PyObject* item = PySequence_Fast_GET_ITEM(fs.get(), i);
                if(!item)
                {
                    assert(PyErr_Occurred());
                    throw AbortMarshaling();
                }

                long val = PyLong_AsLong(item);

                if(PyErr_Occurred() || val < 0 || val > 255)
                {
                    PyErr_Format(PyExc_ValueError, STRCAST("invalid value for element %d of sequence<byte>"),
                                 static_cast<int>(i));
                    throw AbortMarshaling();
                }
                seq[static_cast<size_t>(i)] = static_cast<Ice::Byte>(val);
            }
            os->write(seq);
        }
        break;
    }
    case PrimitiveInfo::KindShort:
    {
        sz = PySequence_Fast_GET_SIZE(fs.get());
        Ice::ShortSeq seq(static_cast<size_t>(sz));
        for(Py_ssize_t i = 0; i < sz; ++i)
        {
            PyObject* item = PySequence_Fast_GET_ITEM(fs.get(), i);
            if(!item)
            {
                assert(PyErr_Occurred());
                throw AbortMarshaling();
            }

            long val = PyLong_AsLong(item);

            if(PyErr_Occurred() || val < SHRT_MIN || val > SHRT_MAX)
            {
                PyErr_Format(PyExc_ValueError, STRCAST("invalid value for element %d of sequence<short>"),
                             static_cast<int>(i));
                throw AbortMarshaling();
            }
            seq[static_cast<size_t>(i)] = static_cast<Ice::Short>(val);
        }
        os->write(seq);
        break;
    }
    case PrimitiveInfo::KindInt:
    {
        sz = PySequence_Fast_GET_SIZE(fs.get());
        IntSeq seq(static_cast<size_t>(sz));
        for(Py_ssize_t i = 0; i < sz; ++i)
        {
            PyObject* item = PySequence_Fast_GET_ITEM(fs.get(), i);
            if(!item)
            {
                assert(PyErr_Occurred());
                throw AbortMarshaling();
            }

            long val = PyLong_AsLong(item);

            if(PyErr_Occurred() || val < INT_MIN || val > INT_MAX)
            {
                PyErr_Format(PyExc_ValueError, STRCAST("invalid value for element %d of sequence<int>"),
                             static_cast<int>(i));
                throw AbortMarshaling();
            }
            seq[static_cast<size_t>(i)] = static_cast<int32_t>(val);
        }
        os->write(seq);
        break;
    }
    case PrimitiveInfo::KindLong:
    {
        sz = PySequence_Fast_GET_SIZE(fs.get());
        Ice::LongSeq seq(static_cast<size_t>(sz));
        for(Py_ssize_t i = 0; i < sz; ++i)
        {
            PyObject* item = PySequence_Fast_GET_ITEM(fs.get(), i);
            if(!item)
            {
                assert(PyErr_Occurred());
                throw AbortMarshaling();
            }

            int64_t val = PyLong_AsLongLong(item);

            if(PyErr_Occurred())
            {
                PyErr_Format(PyExc_ValueError, STRCAST("invalid value for element %d of sequence<long>"),
                             static_cast<int>(i));
                throw AbortMarshaling();
            }
            seq[static_cast<size_t>(i)] = val;
        }
        os->write(seq);
        break;
    }
    case PrimitiveInfo::KindFloat:
    {
        sz = PySequence_Fast_GET_SIZE(fs.get());
        Ice::FloatSeq seq(static_cast<size_t>(sz));
        for(Py_ssize_t i = 0; i < sz; ++i)
        {
            PyObject* item = PySequence_Fast_GET_ITEM(fs.get(), i);
            if(!item)
            {
                assert(PyErr_Occurred());
                throw AbortMarshaling();
            }

            float val = static_cast<float>(PyFloat_AsDouble(item));
            if(PyErr_Occurred())
            {
                PyErr_Format(PyExc_ValueError, STRCAST("invalid value for element %d of sequence<float>"),
                             static_cast<int>(i));
                throw AbortMarshaling();
            }

            seq[static_cast<size_t>(i)] = val;
        }
        os->write(seq);
        break;
    }
    case PrimitiveInfo::KindDouble:
    {
        sz = PySequence_Fast_GET_SIZE(fs.get());
        Ice::DoubleSeq seq(static_cast<size_t>(sz));
        for(Py_ssize_t i = 0; i < sz; ++i)
        {
            PyObject* item = PySequence_Fast_GET_ITEM(fs.get(), i);
            if(!item)
            {
                assert(PyErr_Occurred());
                throw AbortMarshaling();
            }

            double val = PyFloat_AsDouble(item);
            if(PyErr_Occurred())
            {
                PyErr_Format(PyExc_ValueError, STRCAST("invalid value for element %d of sequence<double>"),
                             static_cast<int>(i));
                throw AbortMarshaling();
            }

            seq[static_cast<size_t>(i)] = val;
        }
        os->write(seq);
        break;
    }
    case PrimitiveInfo::KindString:
    {
        sz = PySequence_Fast_GET_SIZE(fs.get());
        os->writeSize(static_cast<int>(sz));
        for(Py_ssize_t i = 0; i < sz; ++i)
        {
            PyObject* item = PySequence_Fast_GET_ITEM(fs.get(), i);
            if(!item)
            {
                assert(PyErr_Occurred());
                throw AbortMarshaling();
            }

            if(item != Py_None && !checkString(item))
            {
                PyErr_Format(PyExc_ValueError, STRCAST("invalid value for element %d of sequence<string>"),
                             static_cast<int>(i));
                throw AbortMarshaling();
            }

            if(!writeString(item, os))
            {
                assert(PyErr_Occurred());
                throw AbortMarshaling();
            }
        }
        break;
    }
    }
}

PyObject*
IcePy::SequenceInfo::createSequenceFromMemory(
    const SequenceMappingPtr& sm,
    const char* buffer,
    Py_ssize_t size,
    BuiltinType type)
{
    PyObjectHandle memoryview;
    char* buf = const_cast<char*>(size == 0 ? emptySeq : buffer);
    memoryview = PyMemoryView_FromMemory(buf, size, PyBUF_READ);

    if(!memoryview.get())
    {
        assert(PyErr_Occurred());
        throw AbortMarshaling();
    }

    PyObjectHandle builtinType = PyLong_FromLong(static_cast<int>(type));
    if(!builtinType.get())
    {
        assert(PyErr_Occurred());
        throw AbortMarshaling();
    }

    AdoptThread adoptThread; // Ensure the current thread is able to call into Python.

    PyObjectHandle args = PyTuple_New(3);
    PyTuple_SET_ITEM(args.get(), 0, incRef(memoryview.get()));
    PyTuple_SET_ITEM(args.get(), 1, incRef(builtinType.get()));
    PyTuple_SET_ITEM(args.get(), 2, incTrue());
    PyObjectHandle result = PyObject_Call(sm->factory, args.get(), 0);

    if(!result.get())
    {
        assert(PyErr_Occurred());
        throw AbortMarshaling();
    }
    else if(result.get() == Py_None)
    {
        PyErr_Format(PyExc_ValueError, STRCAST("invalid container return from factory"));
        throw AbortMarshaling();
    }
    return result.release();
}

void
IcePy::SequenceInfo::unmarshalPrimitiveSequence(const PrimitiveInfoPtr& pi, Ice::InputStream* is,
                                                const UnmarshalCallbackPtr& cb, PyObject* target, void* closure,
                                                const SequenceMappingPtr& sm)
{
    PyObjectHandle result;

    switch(pi->kind)
    {
    case PrimitiveInfo::KindBool:
    {
        pair<const bool*, const bool*> p;
        is->read(p);
        int sz = static_cast<int>(p.second - p.first);
        if(sm->factory)
        {
            const char* data = reinterpret_cast<const char*>(p.first);
            result = createSequenceFromMemory(sm, data, sz, BuiltinTypeBool);
        }
        else
        {
            result = sm->createContainer(sz);
            if(!result.get())
            {
                assert(PyErr_Occurred());
                throw AbortMarshaling();
            }

            for(int i = 0; i < sz; ++i)
            {
                sm->setItem(result.get(), i, p.first[i] ? getTrue() : getFalse());
            }
        }
        break;
    }
    case PrimitiveInfo::KindByte:
    {
        pair<const Ice::Byte*, const Ice::Byte*> p;
        is->read(p);
        int sz = static_cast<int>(p.second - p.first);
        if(sm->factory)
        {
            result = createSequenceFromMemory(sm, reinterpret_cast<const char*>(p.first), sz, BuiltinTypeByte);
        }
        else if(sm->type == SequenceMapping::SEQ_DEFAULT)
        {
            result = PyBytes_FromStringAndSize(reinterpret_cast<const char*>(p.first), sz);
            if(!result.get())
            {
                assert(PyErr_Occurred());
                throw AbortMarshaling();
            }
        }
        else
        {
            result = sm->createContainer(sz);
            if(!result.get())
            {
                assert(PyErr_Occurred());
                throw AbortMarshaling();
            }

            for(int i = 0; i < sz; ++i)
            {
                PyObjectHandle item = PyLong_FromLong(p.first[i]);
                if(!item.get())
                {
                    assert(PyErr_Occurred());
                    throw AbortMarshaling();
                }
                sm->setItem(result.get(), i, item.get());
            }
        }
        break;
    }
    case PrimitiveInfo::KindShort:
    {
        pair<const Ice::Short*, const Ice::Short*> p;
        is->read(p);
        int sz = static_cast<int>(p.second - p.first);
        if(sm->factory)
        {
            const char* data = reinterpret_cast<const char*>(p.first);
            result = createSequenceFromMemory(sm, data, sz * 2, BuiltinTypeShort);
        }
        else
        {
            result = sm->createContainer(sz);
            if(!result.get())
            {
                assert(PyErr_Occurred());
                throw AbortMarshaling();
            }

            for(int i = 0; i < sz; ++i)
            {
                PyObjectHandle item = PyLong_FromLong(p.first[i]);
                if(!item.get())
                {
                    assert(PyErr_Occurred());
                    throw AbortMarshaling();
                }
                sm->setItem(result.get(), i, item.get());
            }
        }
        break;
    }
    case PrimitiveInfo::KindInt:
    {
        pair<const int32_t*, const int32_t*> p;
        is->read(p);
        int sz = static_cast<int>(p.second - p.first);
        if(sm->factory)
        {
            const char* data = reinterpret_cast<const char*>(p.first);
            result = createSequenceFromMemory(sm, data, sz * 4, BuiltinTypeInt);
        }
        else
        {
            result = sm->createContainer(sz);
            if(!result.get())
            {
                assert(PyErr_Occurred());
                throw AbortMarshaling();
            }
            for(int i = 0; i < sz; ++i)
            {
                PyObjectHandle item = PyLong_FromLong(p.first[i]);
                if(!item.get())
                {
                    assert(PyErr_Occurred());
                    throw AbortMarshaling();
                }
                sm->setItem(result.get(), i, item.get());
            }
        }
        break;
    }
    case PrimitiveInfo::KindLong:
    {
        pair<const int64_t*, const int64_t*> p;
        is->read(p);
        int sz = static_cast<int>(p.second - p.first);
        if(sm->factory)
        {
            const char* data = reinterpret_cast<const char*>(p.first);
            result = createSequenceFromMemory(sm, data, sz * 8, BuiltinTypeLong);
        }
        else
        {
            result = sm->createContainer(sz);
            if(!result.get())
            {
                assert(PyErr_Occurred());
                throw AbortMarshaling();
            }

            for(int i = 0; i < sz; ++i)
            {
                PyObjectHandle item = PyLong_FromLongLong(p.first[i]);
                if(!item.get())
                {
                    assert(PyErr_Occurred());
                    throw AbortMarshaling();
                }
                sm->setItem(result.get(), i, item.get());
            }
        }
        break;
    }
    case PrimitiveInfo::KindFloat:
    {
        pair<const Ice::Float*, const Ice::Float*> p;
        is->read(p);
        int sz = static_cast<int>(p.second - p.first);
        if(sm->factory)
        {
            const char* data = reinterpret_cast<const char*>(p.first);
            result = createSequenceFromMemory(sm, data, sz * 4, BuiltinTypeFloat);
        }
        else
        {
            result = sm->createContainer(sz);
            if(!result.get())
            {
                assert(PyErr_Occurred());
                throw AbortMarshaling();
            }

            for(int i = 0; i < sz; ++i)
            {
                PyObjectHandle item = PyFloat_FromDouble(p.first[i]);
                if(!item.get())
                {
                    assert(PyErr_Occurred());
                    throw AbortMarshaling();
                }
                sm->setItem(result.get(), i, item.get());
            }
        }
        break;
    }
    case PrimitiveInfo::KindDouble:
    {
        pair<const Ice::Double*, const Ice::Double*> p;
        is->read(p);
        int sz = static_cast<int>(p.second - p.first);
        if(sm->factory)
        {
            const char* data = reinterpret_cast<const char*>(p.first);
            result = createSequenceFromMemory(sm, data, sz * 8, BuiltinTypeDouble);
        }
        else
        {
            result = sm->createContainer(sz);
            if(!result.get())
            {
                assert(PyErr_Occurred());
                throw AbortMarshaling();
            }

            for(int i = 0; i < sz; ++i)
            {
                PyObjectHandle item = PyFloat_FromDouble(p.first[i]);
                if(!item.get())
                {
                    assert(PyErr_Occurred());
                    throw AbortMarshaling();
                }
                sm->setItem(result.get(), i, item.get());
            }
        }
        break;
    }
    case PrimitiveInfo::KindString:
    {
        Ice::StringSeq seq;
        is->read(seq, false); // Bypass string conversion.
        int sz = static_cast<int>(seq.size());
        result = sm->createContainer(sz);
        if(!result.get())
        {
            assert(PyErr_Occurred());
            throw AbortMarshaling();
        }

        for(int i = 0; i < sz; ++i)
        {
            PyObjectHandle item = createString(seq[static_cast<size_t>(i)]);
            if(!item.get())
            {
                assert(PyErr_Occurred());
                throw AbortMarshaling();
            }
            sm->setItem(result.get(), i, item.get());
        }
        break;
    }
    }
    cb->unmarshaled(result.get(), target, closure);
}

bool
IcePy::SequenceInfo::SequenceMapping::getType(const Ice::StringSeq& metaData, Type& t)
{
    for(Ice::StringSeq::const_iterator p = metaData.begin(); p != metaData.end(); ++p)
    {
        if((*p) == "python:seq:default" || (*p) == "python:default")
        {
            t = SEQ_DEFAULT;
            return true;
        }
        else if((*p) == "python:seq:tuple" || (*p) == "python:tuple")
        {
            t = SEQ_TUPLE;
            return true;
        }
        else if((*p) == "python:seq:list" || (*p) == "python:list")
        {
            t = SEQ_LIST;
            return true;
        }
        else if((*p) == "python:array.array")
        {
            t = SEQ_ARRAY;
            return true;
        }
        else if((*p) == "python:numpy.ndarray")
        {
            t = SEQ_NUMPYARRAY;
            return true;
        }
        else if(p->find("python:memoryview:") == 0)
        {
            t = SEQ_MEMORYVIEW;
            return true;
        }
    }
    return false;
}

IcePy::SequenceInfo::SequenceMapping::SequenceMapping(Type t) :
    type(t),
    factory(0)
{
}

IcePy::SequenceInfo::SequenceMapping::SequenceMapping(const Ice::StringSeq& meta) :
    factory(0)
{
    if(!getType(meta, type))
    {
        type = SEQ_DEFAULT;
    }
}

void
IcePy::SequenceInfo::SequenceMapping::init(const Ice::StringSeq& meta)
{
    if(type == SEQ_ARRAY)
    {
        factory = lookupType("Ice.createArray");
        if(!factory)
        {
            PyErr_Format(PyExc_ImportError, STRCAST("factory type not found `Ice.createArray'"));
            throw InvalidSequenceFactoryException();
        }
    }
    else if(type == SEQ_NUMPYARRAY)
    {
        factory = lookupType("Ice.createNumPyArray");
        if(!factory)
        {
            PyErr_Format(PyExc_ImportError, STRCAST("factory type not found `Ice.createNumPyArray'"));
            throw InvalidSequenceFactoryException();
        }
    }
    else if(type == SEQ_MEMORYVIEW)
    {
        const string prefix = "python:memoryview:";
        for(Ice::StringSeq::const_iterator i = meta.begin(); i != meta.end(); ++i)
        {
            if(i->find(prefix) == 0)
            {
                const string typestr = i->substr(prefix.size());
                factory = lookupType(typestr);
                if(!factory)
                {
                    PyErr_Format(PyExc_ImportError, STRCAST("factory type not found `%s'"), typestr.c_str());
                    throw InvalidSequenceFactoryException();
                }
                if(!PyCallable_Check(factory))
                {
                    PyErr_Format(PyExc_RuntimeError, STRCAST("factory type `%s' is not callable"), typestr.c_str());
                    throw InvalidSequenceFactoryException();
                }
                break;
            }
        }
    }
}

void
IcePy::SequenceInfo::SequenceMapping::unmarshaled(PyObject* val, PyObject* target, void* closure)
{
    Py_ssize_t i = reinterpret_cast<Py_ssize_t>(closure);
    if(type == SEQ_DEFAULT || type == SEQ_LIST)
    {
        PyList_SET_ITEM(target, i, val);
        Py_INCREF(val); // PyList_SET_ITEM steals a reference.
    }
    else
    {
        assert(type == SEQ_TUPLE);
        PyTuple_SET_ITEM(target, i, val);
        Py_INCREF(val); // PyTuple_SET_ITEM steals a reference.
    }
}

PyObject*
IcePy::SequenceInfo::SequenceMapping::createContainer(int sz) const
{
    if(type == SEQ_DEFAULT || type == SEQ_LIST)
    {
        return PyList_New(sz);
    }
    else if(type == SEQ_TUPLE)
    {
        return PyTuple_New(sz);
    }
    else
    {
        //
        // Must never be called for SEQ_MEMORYVIEW, as the container
        // is created by the user factory function.
        //
        assert(false);
        return 0;
    }
}

void
IcePy::SequenceInfo::SequenceMapping::setItem(PyObject* cont, int i, PyObject* val) const
{
    if(type == SEQ_DEFAULT || type == SEQ_LIST)
    {
        Py_INCREF(val);
        PyList_SET_ITEM(cont, i, val); // PyList_SET_ITEM steals a reference.
    }
    else
    {
        assert(type == SEQ_TUPLE);
        Py_INCREF(val);
        PyTuple_SET_ITEM(cont, i, val); // PyTuple_SET_ITEM steals a reference.
    }
}

//
// CustomInfo implementation.
//
IcePy::CustomInfo::CustomInfo(const string& ident, PyObject* t) :
    id(ident), pythonType(t)
{
    assert(PyType_Check(t));
}

string
IcePy::CustomInfo::getId() const
{
    return id;
}

bool
IcePy::CustomInfo::validate(PyObject* val)
{
    return PyObject_IsInstance(val, pythonType) == 1;
}

bool
IcePy::CustomInfo::variableLength() const
{
    return true;
}

int
IcePy::CustomInfo::wireSize() const
{
    return 1;
}

Ice::OptionalFormat
IcePy::CustomInfo::optionalFormat() const
{
    return Ice::OptionalFormat::VSize;
}

bool
IcePy::CustomInfo::usesClasses() const
{
    return false;
}

void
IcePy::CustomInfo::marshal(PyObject* p, Ice::OutputStream* os, ObjectMap* /*objectMap*/, bool,
                           const Ice::StringSeq* /*metaData*/)
{
    assert(PyObject_IsInstance(p, pythonType) == 1); // validate() should have caught this.

    PyObjectHandle obj = PyObject_CallMethod(p, STRCAST("IsInitialized"), 0);
    if(!obj.get())
    {
        throwPythonException();
    }
    if(!PyObject_IsTrue(obj.get()))
    {
        setPythonException(Ice::MarshalException(__FILE__, __LINE__, "type not fully initialized"));
        throw AbortMarshaling();
    }

    obj = PyObject_CallMethod(p, STRCAST("SerializeToString"), 0);
    if(!obj.get())
    {
        assert(PyErr_Occurred());
        throw AbortMarshaling();
    }

    assert(checkString(obj.get()));
    char* str;
    Py_ssize_t sz;
    PyBytes_AsStringAndSize(obj.get(), &str, &sz);
    os->write(reinterpret_cast<const Ice::Byte*>(str), reinterpret_cast<const Ice::Byte*>(str + sz));
}

void
IcePy::CustomInfo::unmarshal(Ice::InputStream* is, const UnmarshalCallbackPtr& cb, PyObject* target,
                             void* closure, bool, const Ice::StringSeq* /*metaData*/)
{
    //
    // Unmarshal the raw byte sequence.
    //
    pair<const Ice::Byte*, const Ice::Byte*> seq;
    is->read(seq);
    int sz = static_cast<int>(seq.second - seq.first);

    //
    // Create a new instance of the protobuf type.
    //
    PyObjectHandle args = PyTuple_New(0);
    if(!args.get())
    {
        assert(PyErr_Occurred());
        throw AbortMarshaling();
    }
    PyTypeObject* type = reinterpret_cast<PyTypeObject*>(pythonType);
    PyObjectHandle p = type->tp_new(type, args.get(), 0);
    if(!p.get())
    {
        assert(PyErr_Occurred());
        throw AbortMarshaling();
    }

    //
    // Initialize the object.
    //
    PyObjectHandle obj = PyObject_CallMethod(p.get(), STRCAST("__init__"), 0, 0);
    if(!obj.get())
    {
        assert(PyErr_Occurred());
        throw AbortMarshaling();
    }

    //
    // Convert the seq to a string.
    //
    obj = PyBytes_FromStringAndSize(reinterpret_cast<const char*>(seq.first), sz);
    if(!obj.get())
    {
        assert(PyErr_Occurred());
        throw AbortMarshaling();
    }

    //
    // Parse the string.
    //
    obj = PyObject_CallMethod(p.get(), STRCAST("ParseFromString"), STRCAST("O"), obj.get(), 0);
    if(!obj.get())
    {
        assert(PyErr_Occurred());
        throw AbortMarshaling();
    }

    cb->unmarshaled(p.get(), target, closure);
}

void
IcePy::CustomInfo::print(PyObject* value, IceUtilInternal::Output& out, PrintObjectHistory*)
{
    if(!validate(value))
    {
        out << "<invalid value - expected " << id << ">";
        return;
    }

    if(value == Py_None)
    {
        out << "{}";
    }
    else
    {
    }
}

//
// DictionaryInfo implementation.
//
IcePy::DictionaryInfo::DictionaryInfo(const string& ident, PyObject* kt, PyObject* vt) :
    id(ident)
{
    const_cast<TypeInfoPtr&>(keyType) = getType(kt);
    const_cast<TypeInfoPtr&>(valueType) = getType(vt);

    _variableLength = keyType->variableLength() || valueType->variableLength();
    _wireSize = keyType->wireSize() + valueType->wireSize();
}

string
IcePy::DictionaryInfo::getId() const
{
    return id;
}

bool
IcePy::DictionaryInfo::validate(PyObject* val)
{
    return val == Py_None || PyDict_Check(val) == 1;
}

bool
IcePy::DictionaryInfo::variableLength() const
{
    return true;
}

int
IcePy::DictionaryInfo::wireSize() const
{
    return 1;
}

Ice::OptionalFormat
IcePy::DictionaryInfo::optionalFormat() const
{
    return _variableLength ? Ice::OptionalFormat::FSize : Ice::OptionalFormat::VSize;
}

bool
IcePy::DictionaryInfo::usesClasses() const
{
    return valueType->usesClasses();
}

void
IcePy::DictionaryInfo::marshal(PyObject* p, Ice::OutputStream* os, ObjectMap* objectMap, bool optional,
                               const Ice::StringSeq*)
{
    if(p != Py_None && !PyDict_Check(p))
    {
        PyErr_Format(PyExc_ValueError, STRCAST("expected dictionary value"));
        throw AbortMarshaling();
    }

    const int32_t sz = p == Py_None ? 0 : static_cast<int32_t>(PyDict_Size(p));

    Ice::OutputStream::size_type sizePos = 0;
    if(optional)
    {
        if(_variableLength)
        {
            sizePos = os->startSize();
        }
        else
        {
            os->writeSize(sz == 0 ? 1 : sz * _wireSize + (sz > 254 ? 5 : 1));
        }
    }

    if(p == Py_None)
    {
        os->writeSize(0);
    }
    else
    {
        os->writeSize(sz);

        Py_ssize_t pos = 0;
        PyObject* key;
        PyObject* value;
        while(PyDict_Next(p, &pos, &key, &value))
        {
            if(!keyType->validate(key))
            {
                PyErr_Format(PyExc_ValueError, STRCAST("invalid key in `%s' element"), const_cast<char*>(id.c_str()));
                throw AbortMarshaling();
            }
            keyType->marshal(key, os, objectMap, false);

            if(!valueType->validate(value))
            {
                PyErr_Format(PyExc_ValueError, STRCAST("invalid value in `%s' element"), const_cast<char*>(id.c_str()));
                throw AbortMarshaling();
            }
            valueType->marshal(value, os, objectMap, false);
        }
    }

    if(optional && _variableLength)
    {
        os->endSize(sizePos);
    }
}

void
IcePy::DictionaryInfo::unmarshal(Ice::InputStream* is, const UnmarshalCallbackPtr& cb, PyObject* target,
                                 void* closure, bool optional, const Ice::StringSeq*)
{
    if(optional)
    {
        if(_variableLength)
        {
            is->skip(4);
        }
        else
        {
            is->skipSize();
        }
    }

    PyObjectHandle p = PyDict_New();
    if(!p.get())
    {
        assert(PyErr_Occurred());
        throw AbortMarshaling();
    }

    KeyCallbackPtr keyCB = make_shared<KeyCallback>();
    keyCB->key = 0;

    int32_t sz = is->readSize();
    for(int32_t i = 0; i < sz; ++i)
    {
        //
        // A dictionary key cannot be a class (or contain one), so the key must be
        // available immediately.
        //
        keyType->unmarshal(is, keyCB, 0, 0, false);
        assert(keyCB->key.get());

        //
        // Insert the key into the dictionary with a dummy value in order to hold
        // a reference to the key. In case of an exception, we don't want to leak
        // the key.
        //
        if(PyDict_SetItem(p.get(), keyCB->key.get(), Py_None) < 0)
        {
            assert(PyErr_Occurred());
            throw AbortMarshaling();
        }

        //
        // The callback will reset the dictionary entry with the unmarshaled value,
        // so we pass it the key.
        //
        void* cl = reinterpret_cast<void*>(keyCB->key.get());
        valueType->unmarshal(is, shared_from_this(), p.get(), cl, false);
    }

    cb->unmarshaled(p.get(), target, closure);
}

void
IcePy::DictionaryInfo::unmarshaled(PyObject* val, PyObject* target, void* closure)
{
    PyObject* key = reinterpret_cast<PyObject*>(closure);
    if(PyDict_SetItem(target, key, val) < 0)
    {
        assert(PyErr_Occurred());
        throw AbortMarshaling();
    }
}

void
IcePy::DictionaryInfo::print(PyObject* value, IceUtilInternal::Output& out, PrintObjectHistory* history)
{
    if(!validate(value))
    {
        out << "<invalid value - expected " << id << ">";
        return;
    }

    if(value == Py_None)
    {
        out << "{}";
    }
    else
    {
        Py_ssize_t pos = 0;
        PyObject* elemKey;
        PyObject* elemValue;
        out.sb();
        bool first = true;
        while(PyDict_Next(value, &pos, &elemKey, &elemValue))
        {
            if(first)
            {
                first = false;
            }
            else
            {
                out << nl;
            }
            out << nl << "key = ";
            keyType->print(elemKey, out, history);
            out << nl << "value = ";
            valueType->print(elemValue, out, history);
        }
        out.eb();
    }
}

void
IcePy::DictionaryInfo::KeyCallback::unmarshaled(PyObject* val, PyObject*, void*)
{
    key = val;
    Py_INCREF(val);
}

void
IcePy::DictionaryInfo::destroy()
{
    keyType = 0;
    valueType = 0;
}

//
// ClassInfo implementation.
//
IcePy::ClassInfo::ClassInfo(const string& ident) :
    id(ident), defined(false)
{
}

void
IcePy::ClassInfo::init()
{
    typeObj = createType(shared_from_this());
}

void
IcePy::ClassInfo::define(PyObject* t, PyObject* b, PyObject* i)
{
    assert(PyType_Check(t));
    assert(PyTuple_Check(i));

    if(b != Py_None)
    {
        const_cast<ClassInfoPtr&>(base) = dynamic_pointer_cast<ClassInfo>(getType(b));
        assert(base);
    }

    Py_ssize_t n, sz;
    sz = PyTuple_GET_SIZE(i);
    for(n = 0; n < sz; ++n)
    {
        PyObject* o = PyTuple_GET_ITEM(i, n);
        auto iface = dynamic_pointer_cast<ClassInfo>(getType(o));
        assert(iface);
        const_cast<ClassInfoList&>(interfaces).push_back(iface);
    }

    pythonType = t;

    const_cast<bool&>(defined) = true;
}

string
IcePy::ClassInfo::getId() const
{
    return id;
}

bool
IcePy::ClassInfo::validate(PyObject*)
{
    assert(false);
    return true;
}

bool
IcePy::ClassInfo::variableLength() const
{
    assert(false);
    return true;
}

int
IcePy::ClassInfo::wireSize() const
{
    return 1;
}

Ice::OptionalFormat
IcePy::ClassInfo::optionalFormat() const
{
    return Ice::OptionalFormat::Class;
}

bool
IcePy::ClassInfo::usesClasses() const
{
    return true;
}

void
IcePy::ClassInfo::marshal(PyObject*, Ice::OutputStream*, ObjectMap*, bool,
                          const Ice::StringSeq*)
{
    assert(false);
    throw AbortMarshaling();
}

void
IcePy::ClassInfo::unmarshal(Ice::InputStream*, const UnmarshalCallbackPtr&, PyObject*,
                            void*, bool, const Ice::StringSeq*)
{
    assert(false);
    throw AbortMarshaling();
}

void
IcePy::ClassInfo::print(PyObject* value, IceUtilInternal::Output& out, PrintObjectHistory* history)
{
    if(!validate(value))
    {
        out << "<invalid value - expected " << id << ">";
        return;
    }

    if(value == Py_None)
    {
        out << "<nil>";
    }
    else
    {
        map<PyObject*, int>::iterator q = history->objects.find(value);
        if(q != history->objects.end())
        {
            out << "<object #" << q->second << ">";
        }
        else
        {
            PyObjectHandle iceType = getAttr(value, "_ice_type", false);
            ClassInfoPtr info;
            if(!iceType.get())
            {
                //
                // The _ice_type attribute will be missing in an instance of LocalObject
                // that does not derive from a user-defined type.
                //
                assert(id == "::Ice::LocalObject");
                info = shared_from_this();
            }
            else
            {
                info = dynamic_pointer_cast<ClassInfo>(getType(iceType.get()));
                assert(info);
            }
            out << "object #" << history->index << " (" << info->id << ')';
            history->objects.insert(map<PyObject*, int>::value_type(value, history->index));
            ++history->index;
        }
    }
}

void
IcePy::ClassInfo::destroy()
{
    const_cast<ClassInfoPtr&>(base) = 0;
    const_cast<ClassInfoList&>(interfaces).clear();
}

//
// ValueInfo implementation.
//
IcePy::ValueInfo::ValueInfo(const string& ident) :
    id(ident), compactId(-1), preserve(false), interface(false), defined(false)
{
}

void
IcePy::ValueInfo::init()
{
    typeObj = createType(shared_from_this());
}

void
IcePy::ValueInfo::define(PyObject* t, int compact, bool pres, bool intf, PyObject* b, PyObject* m)
{
    assert(PyType_Check(t));
    assert(PyTuple_Check(m));

    const_cast<int&>(compactId) = compact;
    const_cast<bool&>(preserve) = pres;
    const_cast<bool&>(interface) = intf;

    if(b != Py_None)
    {
        const_cast<ValueInfoPtr&>(base) = dynamic_pointer_cast<ValueInfo>(getType(b));
        assert(base);
    }

    convertDataMembers(m, const_cast<DataMemberList&>(members), const_cast<DataMemberList&>(optionalMembers), true);

    pythonType = t;

    const_cast<bool&>(defined) = true;
}

string
IcePy::ValueInfo::getId() const
{
    return id;
}

bool
IcePy::ValueInfo::validate(PyObject* val)
{
    return val == Py_None || PyObject_IsInstance(val, pythonType) == 1;
}

bool
IcePy::ValueInfo::variableLength() const
{
    return true;
}

int
IcePy::ValueInfo::wireSize() const
{
    return 1;
}

Ice::OptionalFormat
IcePy::ValueInfo::optionalFormat() const
{
    return Ice::OptionalFormat::Class;
}

bool
IcePy::ValueInfo::usesClasses() const
{
    return true;
}

void
IcePy::ValueInfo::marshal(PyObject* p, Ice::OutputStream* os, ObjectMap* objectMap, bool,
                          const Ice::StringSeq*)
{
    if(!pythonType)
    {
        PyErr_Format(PyExc_RuntimeError, STRCAST("class %s is declared but not defined"), id.c_str());
        throw AbortMarshaling();
    }

    if(p == Py_None)
    {
        std::shared_ptr<Ice::Value> nil;
        os->write(nil);
        return;
    }

    if(!PyObject_IsInstance(p, pythonType))
    {
        PyErr_Format(PyExc_ValueError, STRCAST("expected value of type %s"), id.c_str());
        throw AbortMarshaling();
    }

    //
    // Ice::ValueWriter is a subclass of Ice::Value that wraps a Python object for marshaling.
    // It is possible that this Python object has already been marshaled, therefore we first must
    // check the object map to see if this object is present. If so, we use the existing ValueWriter,
    // otherwise we create a new one.
    //
    std::shared_ptr<Ice::Value> writer;
    assert(objectMap);
    ObjectMap::iterator q = objectMap->find(p);
    if(q == objectMap->end())
    {
        writer = make_shared<ValueWriter>(p, objectMap, shared_from_this());
        objectMap->insert(ObjectMap::value_type(p, writer));
    }
    else
    {
        writer = q->second;
    }

    //
    // Give the writer to the stream. The stream will eventually call write() on it.
    //
    os->write(writer);
}

namespace
{

void
patchObject(void* addr, const std::shared_ptr<Ice::Value>& v)
{
    ReadValueCallback* cb = static_cast<ReadValueCallback*>(addr);
    assert(cb);
    cb->invoke(v);
}

}

void
IcePy::ValueInfo::unmarshal(Ice::InputStream* is, const UnmarshalCallbackPtr& cb, PyObject* target,
                            void* closure, bool, const Ice::StringSeq*)
{
    if(!pythonType)
    {
        PyErr_Format(PyExc_RuntimeError, STRCAST("class %s is declared but not defined"), id.c_str());
        throw AbortMarshaling();
    }

    //
    // This callback is notified when the Slice value is actually read. The StreamUtil object
    // attached to the stream keeps a reference to the callback object to ensure it lives
    // long enough.
    //
    ReadValueCallbackPtr rocb = make_shared<ReadValueCallback>(shared_from_this(), cb, target, closure);
    StreamUtil* util = reinterpret_cast<StreamUtil*>(is->getClosure());
    assert(util);
    util->add(rocb);
    is->read(patchObject, rocb.get());
}

void
IcePy::ValueInfo::print(PyObject* value, IceUtilInternal::Output& out, PrintObjectHistory* history)
{
    if(!validate(value))
    {
        out << "<invalid value - expected " << id << ">";
        return;
    }

    if(value == Py_None)
    {
        out << "<nil>";
    }
    else
    {
        map<PyObject*, int>::iterator q = history->objects.find(value);
        if(q != history->objects.end())
        {
            out << "<object #" << q->second << ">";
        }
        else
        {
            PyObjectHandle iceType = getAttr(value, "_ice_type", false);
            ValueInfoPtr info;
            if(!iceType.get())
            {
                //
                // The _ice_type attribute will be missing in an instance of LocalObject
                // that does not derive from a user-defined type.
                //
                assert(id == "::Ice::LocalObject");
                info = shared_from_this();
            }
            else
            {
                info = dynamic_pointer_cast<ValueInfo>(getType(iceType.get()));
                assert(info);
            }
            out << "object #" << history->index << " (" << info->id << ')';
            history->objects.insert(map<PyObject*, int>::value_type(value, history->index));
            ++history->index;
            out.sb();
            info->printMembers(value, out, history);
            out.eb();
        }
    }
}

void
IcePy::ValueInfo::destroy()
{
    const_cast<ValueInfoPtr&>(base) = 0;
    const_cast<DataMemberList&>(members).clear();
}

void
IcePy::ValueInfo::printMembers(PyObject* value, IceUtilInternal::Output& out, PrintObjectHistory* history)
{
    if(base)
    {
        base->printMembers(value, out, history);
    }

    DataMemberList::const_iterator q;

    for(q = members.begin(); q != members.end(); ++q)
    {
        DataMemberPtr member = *q;
        PyObjectHandle attr = getAttr(value, member->name, true);
        out << nl << member->name << " = ";
        if(!attr.get())
        {
            out << "<not defined>";
        }
        else
        {
            member->type->print(attr.get(), out, history);
        }
    }

    for(q = optionalMembers.begin(); q != optionalMembers.end(); ++q)
    {
        DataMemberPtr member = *q;
        PyObjectHandle attr = getAttr(value, member->name, true);
        out << nl << member->name << " = ";
        if(!attr.get())
        {
            out << "<not defined>";
        }
        else if(attr.get() == Unset)
        {
            out << "<unset>";
        }
        else
        {
            member->type->print(attr.get(), out, history);
        }
    }
}

//
// ProxyInfo implementation.
//
IcePy::ProxyInfo::ProxyInfo(const string& ident) :
    id(ident)
{
}

void
IcePy::ProxyInfo::init()
{
    typeObj = createType(shared_from_this());
}

void
IcePy::ProxyInfo::define(PyObject* t)
{
    pythonType = t; // Borrowed reference.
}

string
IcePy::ProxyInfo::getId() const
{
    return id;
}

bool
IcePy::ProxyInfo::validate(PyObject* val)
{
    return val == Py_None || PyObject_IsInstance(val, pythonType) == 1;
}

bool
IcePy::ProxyInfo::variableLength() const
{
    return true;
}

int
IcePy::ProxyInfo::wireSize() const
{
    return 1;
}

Ice::OptionalFormat
IcePy::ProxyInfo::optionalFormat() const
{
    return Ice::OptionalFormat::FSize;
}

void
IcePy::ProxyInfo::marshal(PyObject* p, Ice::OutputStream* os, ObjectMap*, bool optional, const Ice::StringSeq*)
{
    Ice::OutputStream::size_type sizePos = 0;
    if(optional)
    {
        sizePos = os->startSize();
    }

    std::optional<Ice::ObjectPrx> proxy;

    if(p != Py_None)
    {
        if(checkProxy(p))
        {
            proxy = getProxy(p);
        }
        else
        {
            assert(false); // validate() should have caught this.
        }
    }
    os->write(proxy);

    if(optional)
    {
        os->endSize(sizePos);
    }
}

void
IcePy::ProxyInfo::unmarshal(Ice::InputStream* is, const UnmarshalCallbackPtr& cb, PyObject* target,
                            void* closure, bool optional, const Ice::StringSeq*)
{
    if(optional)
    {
        is->skip(4);
    }

    std::optional<Ice::ObjectPrx> proxy;
    is->read(proxy);

    if(!proxy)
    {
        cb->unmarshaled(Py_None, target, closure);
        return;
    }

    if(!pythonType)
    {
        PyErr_Format(PyExc_RuntimeError, STRCAST("class %s is declared but not defined"), id.c_str());
        throw AbortMarshaling();
    }

    PyObjectHandle p = createProxy(proxy.value(), proxy->ice_getCommunicator(), pythonType);
    cb->unmarshaled(p.get(), target, closure);
}

void
IcePy::ProxyInfo::print(PyObject* value, IceUtilInternal::Output& out, PrintObjectHistory*)
{
    if(!validate(value))
    {
        out << "<invalid value - expected " << getId() << ">";
        return;
    }

    if(value == Py_None)
    {
        out << "<nil>";
    }
    else
    {
        PyObjectHandle p = PyObject_Str(value);
        if(!p.get())
        {
            return;
        }
        assert(checkString(p.get()));
        out << getString(p.get());
    }
}

//
// ValueWriter implementation.
//
IcePy::ValueWriter::ValueWriter(PyObject* object, ObjectMap* objectMap, const ValueInfoPtr& formal) :
    _object(object), _map(objectMap), _formal(formal)
{
    Py_INCREF(_object);
    if(!_formal || !_formal->interface)
    {
        PyObjectHandle iceType = getAttr(object, "_ice_type", false);
        if(!iceType.get())
        {
            assert(PyErr_Occurred());
            throw AbortMarshaling();
        }
        _info = dynamic_pointer_cast<ValueInfo>(getType(iceType.get()));
        assert(_info);
    }
}

IcePy::ValueWriter::~ValueWriter()
{
    Py_DECREF(_object);
}

void
IcePy::ValueWriter::ice_preMarshal()
{
    if(PyObject_HasAttrString(_object, STRCAST("ice_preMarshal")) == 1)
    {
        PyObjectHandle tmp = PyObject_CallMethod(_object, STRCAST("ice_preMarshal"), 0);
        if(!tmp.get())
        {
            assert(PyErr_Occurred());
            throw AbortMarshaling();
        }
    }
}

void
IcePy::ValueWriter::_iceWrite(Ice::OutputStream* os) const
{
    Ice::SlicedDataPtr slicedData;

    if(_info && _info->preserve)
    {
        //
        // Retrieve the SlicedData object that we stored as a hidden member of the Python object.
        //
        slicedData = StreamUtil::getSlicedDataMember(_object, const_cast<ObjectMap*>(_map));
    }

    os->startValue(slicedData);

    if(_formal && _formal->interface)
    {
        PyObjectHandle ret = PyObject_CallMethod(_object, STRCAST("ice_id"), 0);
        if(!ret.get())
        {
            assert(PyErr_Occurred());
            throw AbortMarshaling();
        }
        string id = getString(ret.get());
        os->startSlice(id, -1, true);
        os->endSlice();
    }
    else
    {
        if(_info->id != "::Ice::UnknownSlicedValue")
        {
            ValueInfoPtr info = _info;
            while(info)
            {
                os->startSlice(info->id, info->compactId, !info->base);

                writeMembers(os, info->members);
                writeMembers(os, info->optionalMembers); // The optional members have already been sorted by tag.

                os->endSlice();

                info = info->base;
            }
        }
    }

    os->endValue();
}

void
IcePy::ValueWriter::_iceRead(Ice::InputStream*)
{
    assert(false);
}

void
IcePy::ValueWriter::writeMembers(Ice::OutputStream* os, const DataMemberList& members) const
{
    for(DataMemberList::const_iterator q = members.begin(); q != members.end(); ++q)
    {
        DataMemberPtr member = *q;

        char* memberName = const_cast<char*>(member->name.c_str());

        PyObjectHandle val = getAttr(_object, member->name, true);
        if(!val.get())
        {
            if(member->optional)
            {
                PyErr_Clear();
                continue;
            }
            else
            {
                PyErr_Format(PyExc_AttributeError, STRCAST("no member `%s' found in %s value"), memberName,
                             const_cast<char*>(_info->id.c_str()));
                throw AbortMarshaling();
            }
        }
        else if(member->optional &&
                (val.get() == Unset || !os->writeOptional(member->tag, member->type->optionalFormat())))
        {
            continue;
        }

        if(!member->type->validate(val.get()))
        {
            PyErr_Format(PyExc_ValueError, STRCAST("invalid value for %s member `%s'"),
                         const_cast<char*>(_info->id.c_str()), memberName);
            throw AbortMarshaling();
        }

        member->type->marshal(val.get(), os, _map, member->optional, &member->metaData);
    }
}

//
// ValueReader implementation.
//
IcePy::ValueReader::ValueReader(PyObject* object, const ValueInfoPtr& info) :
    _object(object), _info(info)
{
    Py_INCREF(_object);
}

IcePy::ValueReader::~ValueReader()
{
    Py_DECREF(_object);
}

void
IcePy::ValueReader::ice_postUnmarshal()
{
    if(PyObject_HasAttrString(_object, STRCAST("ice_postUnmarshal")) == 1)
    {
        PyObjectHandle tmp = PyObject_CallMethod(_object, STRCAST("ice_postUnmarshal"), 0);
        if(!tmp.get())
        {
            assert(PyErr_Occurred());
            throw AbortMarshaling();
        }
    }
}

void
IcePy::ValueReader::_iceWrite(Ice::OutputStream*) const
{
    assert(false);
}

void
IcePy::ValueReader::_iceRead(Ice::InputStream* is)
{
    is->startValue();

    const bool unknown = _info->id == "::Ice::UnknownSlicedValue";

    //
    // Unmarshal the slices of a user-defined class.
    //
    if(!unknown && _info->id != Ice::Value::ice_staticId())
    {
        ValueInfoPtr info = _info;
        while(info)
        {
            is->startSlice();

            DataMemberList::const_iterator p;

            for(p = info->members.begin(); p != info->members.end(); ++p)
            {
                DataMemberPtr member = *p;
                member->type->unmarshal(is, member, _object, 0, false, &member->metaData);
            }

            //
            // The optional members have already been sorted by tag.
            //
            for(p = info->optionalMembers.begin(); p != info->optionalMembers.end(); ++p)
            {
                DataMemberPtr member = *p;
                if(is->readOptional(member->tag, member->type->optionalFormat()))
                {
                    member->type->unmarshal(is, member, _object, 0, true, &member->metaData);
                }
                else if(PyObject_SetAttrString(_object, const_cast<char*>(member->name.c_str()), Unset) < 0)
                {
                    assert(PyErr_Occurred());
                    throw AbortMarshaling();
                }
            }

            is->endSlice();

            info = info->base;
        }
    }

    _slicedData = is->endValue(_info->preserve);

    if(_slicedData)
    {
        StreamUtil* util = reinterpret_cast<StreamUtil*>(is->getClosure());
        assert(util);
        util->add(shared_from_this());

        //
        // Define the "unknownTypeId" member for an instance of UnknownSlicedObject.
        //
        if(unknown)
        {
            assert(!_slicedData->slices.empty());

            PyObjectHandle typeId = createString(_slicedData->slices[0]->typeId);
            if(!typeId.get() || PyObject_SetAttrString(_object, STRCAST("unknownTypeId"), typeId.get()) < 0)
            {
                assert(PyErr_Occurred());
                throw AbortMarshaling();
            }
        }
    }
}

ValueInfoPtr
IcePy::ValueReader::getInfo() const
{
    return _info;
}

PyObject*
IcePy::ValueReader::getObject() const
{
    return _object;
}

Ice::SlicedDataPtr
IcePy::ValueReader::getSlicedData() const
{
    return _slicedData;
}

//
// ReadValueCallback implementation.
//
IcePy::ReadValueCallback::ReadValueCallback(const ValueInfoPtr& info, const UnmarshalCallbackPtr& cb,
                                              PyObject* target, void* closure) :
    _info(info), _cb(cb), _target(target), _closure(closure)
{
    Py_XINCREF(_target);
}

IcePy::ReadValueCallback::~ReadValueCallback()
{
    Py_XDECREF(_target);
}

void
IcePy::ReadValueCallback::invoke(const std::shared_ptr<Ice::Value>& p)
{
    if(p)
    {
        auto reader = dynamic_pointer_cast<ValueReader>(p);
        assert(reader);

        //
        // Verify that the value's type is compatible with the formal type.
        //
        PyObject* obj = reader->getObject(); // Borrowed reference.
        if(!PyObject_IsInstance(obj, _info->pythonType))
        {
            Ice::UnexpectedObjectException ex(__FILE__, __LINE__);
            ex.reason = "unmarshaled value is not an instance of " + _info->id;
            ex.type = reader->getInfo()->getId();
            ex.expectedType = _info->id;
            throw ex;
        }

        _cb->unmarshaled(obj, _target, _closure);
    }
    else
    {
        _cb->unmarshaled(Py_None, _target, _closure);
    }
}

//
// ExceptionInfo implementation.
//
void
IcePy::ExceptionInfo::marshal(PyObject* p, Ice::OutputStream* os, ObjectMap* objectMap)
{
    if(!PyObject_IsInstance(p, pythonType))
    {
        PyErr_Format(PyExc_ValueError, STRCAST("expected exception %s"), id.c_str());
        throw AbortMarshaling();
    }

    Ice::SlicedDataPtr slicedData;

    if(preserve)
    {
        //
        // Retrieve the SlicedData object that we stored as a hidden member of the Python object.
        //
        slicedData = StreamUtil::getSlicedDataMember(p, objectMap);
    }

    os->startException(slicedData);

    ExceptionInfoPtr info = shared_from_this();
    while(info)
    {
        os->startSlice(info->id, -1, !info->base);

        writeMembers(p, os, info->members, objectMap);
        writeMembers(p, os, info->optionalMembers, objectMap); // The optional members have already been sorted by tag.

        os->endSlice();

        info = info->base;
    }

    os->endException();
}

void
IcePy::ExceptionInfo::writeMembers(PyObject* p, Ice::OutputStream* os, const DataMemberList& membersP,
                                   ObjectMap* objectMap) const
{
    for(DataMemberList::const_iterator q = membersP.begin(); q != membersP.end(); ++q)
    {
        DataMemberPtr member = *q;

        char* memberName = const_cast<char*>(member->name.c_str());

        PyObjectHandle val = getAttr(p, member->name, true);
        if(!val.get())
        {
            if(member->optional)
            {
                PyErr_Clear();
                continue;
            }
            else
            {
                PyErr_Format(PyExc_AttributeError, STRCAST("no member `%s' found in %s value"), memberName,
                             const_cast<char*>(id.c_str()));
                throw AbortMarshaling();
            }
        }
        else if(member->optional &&
                (val.get() == Unset || !os->writeOptional(member->tag, member->type->optionalFormat())))
        {
            continue;
        }

        if(!member->type->validate(val.get()))
        {
            PyErr_Format(PyExc_ValueError, STRCAST("invalid value for %s member `%s'"),
                         const_cast<char*>(id.c_str()), memberName);
            throw AbortMarshaling();
        }

        member->type->marshal(val.get(), os, objectMap, member->optional, &member->metaData);
    }
}

PyObject*
IcePy::ExceptionInfo::unmarshal(Ice::InputStream* is)
{
    PyObjectHandle p = createExceptionInstance(pythonType);
    if(!p.get())
    {
        assert(PyErr_Occurred());
        throw AbortMarshaling();
    }

    ExceptionInfoPtr info = shared_from_this();
    while(info)
    {
        is->startSlice();

        DataMemberList::iterator q;

        for(q = info->members.begin(); q != info->members.end(); ++q)
        {
            DataMemberPtr member = *q;
            member->type->unmarshal(is, member, p.get(), 0, false, &member->metaData);
        }

        //
        // The optional members have already been sorted by tag.
        //
        for(q = info->optionalMembers.begin(); q != info->optionalMembers.end(); ++q)
        {
            DataMemberPtr member = *q;
            if(is->readOptional(member->tag, member->type->optionalFormat()))
            {
                member->type->unmarshal(is, member, p.get(), 0, true, &member->metaData);
            }
            else if(PyObject_SetAttrString(p.get(), const_cast<char*>(member->name.c_str()), Unset) < 0)
            {
                assert(PyErr_Occurred());
                throw AbortMarshaling();
            }
        }

        is->endSlice();

        info = info->base;
    }

    return p.release();
}

void
IcePy::ExceptionInfo::print(PyObject* value, IceUtilInternal::Output& out)
{
    if(!PyObject_IsInstance(value, pythonType))
    {
        out << "<invalid value - expected " << id << ">";
        return;
    }

    PrintObjectHistory history;
    history.index = 0;

    out << "exception " << id;
    out.sb();
    printMembers(value, out, &history);
    out.eb();
}

void
IcePy::ExceptionInfo::printMembers(PyObject* value, IceUtilInternal::Output& out, PrintObjectHistory* history)
{
    if(base)
    {
        base->printMembers(value, out, history);
    }

    DataMemberList::iterator q;

    for(q = members.begin(); q != members.end(); ++q)
    {
        DataMemberPtr member = *q;
        PyObjectHandle attr = getAttr(value, member->name, true);
        out << nl << member->name << " = ";
        if(!attr.get() || attr.get() == Unset)
        {
            out << "<not defined>";
        }
        else
        {
            member->type->print(attr.get(), out, history);
        }
    }

    for(q = optionalMembers.begin(); q != optionalMembers.end(); ++q)
    {
        DataMemberPtr member = *q;
        PyObjectHandle attr = getAttr(value, member->name, true);
        out << nl << member->name << " = ";
        if(!attr.get())
        {
            out << "<not defined>";
        }
        else if(attr.get() == Unset)
        {
            out << "<unset>";
        }
        else
        {
            member->type->print(attr.get(), out, history);
        }
    }
}

//
// ExceptionWriter implementation.
//
IcePy::ExceptionWriter::ExceptionWriter(const PyObjectHandle& ex, const ExceptionInfoPtr& info) :
    _ex(ex), _info(info)
{
    if(!info)
    {
        PyObjectHandle iceType = getAttr(ex.get(), "_ice_type", false);
        assert(iceType.get());
        _info = dynamic_pointer_cast<ExceptionInfo>(getException(iceType.get()));
        assert(_info);
    }
}

IcePy::ExceptionWriter::~ExceptionWriter()
{
    AdoptThread adoptThread; // Ensure the current thread is able to call into Python.

    _ex = 0;
}

string
IcePy::ExceptionWriter::ice_id() const
{
    return _info->id;
}

Ice::UserException*
IcePy::ExceptionWriter::ice_cloneImpl() const
{
    return new ExceptionWriter(*this);
}

void
IcePy::ExceptionWriter::ice_throw() const
{
    throw *this;
}

void
IcePy::ExceptionWriter::_write(Ice::OutputStream* os) const
{
    AdoptThread adoptThread; // Ensure the current thread is able to call into Python.

    _info->marshal(_ex.get(), os, const_cast<ObjectMap*>(&_objects));
}

void
IcePy::ExceptionWriter::_read(Ice::InputStream*)
{
}

bool
IcePy::ExceptionWriter::_usesClasses() const
{
    return _info->usesClasses;
}

//
// ExceptionReader implementation.
//
IcePy::ExceptionReader::ExceptionReader(const ExceptionInfoPtr& info) :
    _info(info)
{
}

IcePy::ExceptionReader::~ExceptionReader()
{
    AdoptThread adoptThread; // Ensure the current thread is able to call into Python.

    _ex = 0;
}

string
IcePy::ExceptionReader::ice_id() const
{
    return _info->id;
}

Ice::UserException*
IcePy::ExceptionReader::ice_cloneImpl() const
{
    assert(false);
    return 0;
}

void
IcePy::ExceptionReader::ice_throw() const
{
    throw *this;
}

void
IcePy::ExceptionReader::_write(Ice::OutputStream*) const
{
    assert(false);
}

void
IcePy::ExceptionReader::_read(Ice::InputStream* is)
{
    AdoptThread adoptThread; // Ensure the current thread is able to call into Python.

    is->startException();

    const_cast<PyObjectHandle&>(_ex) = _info->unmarshal(is);

    const_cast<Ice::SlicedDataPtr&>(_slicedData) = is->endException(_info->preserve);
}

bool
IcePy::ExceptionReader::_usesClasses() const
{
    return _info->usesClasses;
}

PyObject*
IcePy::ExceptionReader::getException() const
{
    return _ex.get();
}

Ice::SlicedDataPtr
IcePy::ExceptionReader::getSlicedData() const
{
    return _slicedData;
}

//
// IdResolver
//
string
IcePy::resolveCompactId(int32_t id)
{
    CompactIdMap::iterator p = _compactIdMap.find(id);
    if(p != _compactIdMap.end())
    {
        return p->second->id;
    }
    return string();
}

//
// lookupClassInfo()
//
IcePy::ClassInfoPtr
IcePy::lookupClassInfo(const string& id)
{
    ClassInfoMap::iterator p = _classInfoMap.find(id);
    if(p != _classInfoMap.end())
    {
        return p->second;
    }
    return nullptr;
}

//
// lookupClassInfo()
//
IcePy::ValueInfoPtr
IcePy::lookupValueInfo(const string& id)
{
    ValueInfoMap::iterator p = _valueInfoMap.find(id);
    if(p != _valueInfoMap.end())
    {
        return p->second;
    }
    return 0;
}

//
// lookupExceptionInfo()
//
IcePy::ExceptionInfoPtr
IcePy::lookupExceptionInfo(const string& id)
{
    ExceptionInfoMap::iterator p = _exceptionInfoMap.find(id);
    if(p != _exceptionInfoMap.end())
    {
        return p->second;
    }
    return 0;
}

namespace IcePy
{

PyTypeObject TypeInfoType =
{
    /* The ob_type field must be initialized in the module init function
     * to be portable to Windows without using C++. */
    PyVarObject_HEAD_INIT(0, 0)
    STRCAST("IcePy.TypeInfo"),       /* tp_name */
    sizeof(TypeInfoObject),          /* tp_basicsize */
    0,                               /* tp_itemsize */
    /* methods */
    reinterpret_cast<destructor>(typeInfoDealloc), /* tp_dealloc */
    0,                               /* tp_print */
    0,                               /* tp_getattr */
    0,                               /* tp_setattr */
    0,                               /* tp_reserved */
    0,                               /* tp_repr */
    0,                               /* tp_as_number */
    0,                               /* tp_as_sequence */
    0,                               /* tp_as_mapping */
    0,                               /* tp_hash */
    0,                               /* tp_call */
    0,                               /* tp_str */
    0,                               /* tp_getattro */
    0,                               /* tp_setattro */
    0,                               /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,              /* tp_flags */
    0,                               /* tp_doc */
    0,                               /* tp_traverse */
    0,                               /* tp_clear */
    0,                               /* tp_richcompare */
    0,                               /* tp_weaklistoffset */
    0,                               /* tp_iter */
    0,                               /* tp_iternext */
    0,                               /* tp_methods */
    0,                               /* tp_members */
    0,                               /* tp_getset */
    0,                               /* tp_base */
    0,                               /* tp_dict */
    0,                               /* tp_descr_get */
    0,                               /* tp_descr_set */
    0,                               /* tp_dictoffset */
    0,                               /* tp_init */
    0,                               /* tp_alloc */
    reinterpret_cast<newfunc>(typeInfoNew), /* tp_new */
    0,                               /* tp_free */
    0,                               /* tp_is_gc */
};

PyTypeObject ExceptionInfoType =
{
    /* The ob_type field must be initialized in the module init function
     * to be portable to Windows without using C++. */
    PyVarObject_HEAD_INIT(0, 0)
    STRCAST("IcePy.ExceptionInfo"),  /* tp_name */
    sizeof(ExceptionInfoObject),     /* tp_basicsize */
    0,                               /* tp_itemsize */
    /* methods */
    reinterpret_cast<destructor>(exceptionInfoDealloc), /* tp_dealloc */
    0,                               /* tp_print */
    0,                               /* tp_getattr */
    0,                               /* tp_setattr */
    0,                               /* tp_reserved */
    0,                               /* tp_repr */
    0,                               /* tp_as_number */
    0,                               /* tp_as_sequence */
    0,                               /* tp_as_mapping */
    0,                               /* tp_hash */
    0,                               /* tp_call */
    0,                               /* tp_str */
    0,                               /* tp_getattro */
    0,                               /* tp_setattro */
    0,                               /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,              /* tp_flags */
    0,                               /* tp_doc */
    0,                               /* tp_traverse */
    0,                               /* tp_clear */
    0,                               /* tp_richcompare */
    0,                               /* tp_weaklistoffset */
    0,                               /* tp_iter */
    0,                               /* tp_iternext */
    0,                               /* tp_methods */
    0,                               /* tp_members */
    0,                               /* tp_getset */
    0,                               /* tp_base */
    0,                               /* tp_dict */
    0,                               /* tp_descr_get */
    0,                               /* tp_descr_set */
    0,                               /* tp_dictoffset */
    0,                               /* tp_init */
    0,                               /* tp_alloc */
    reinterpret_cast<newfunc>(exceptionInfoNew), /* tp_new */
    0,                               /* tp_free */
    0,                               /* tp_is_gc */
};

static PyNumberMethods UnsetAsNumber =
{
    0,                          /* nb_add */
    0,                          /* nb_subtract */
    0,                          /* nb_multiply */
    0,                          /* nb_remainder */
    0,                          /* nb_divmod */
    0,                          /* nb_power */
    0,                          /* nb_negative */
    0,                          /* nb_positive */
    0,                          /* nb_absolute */
    reinterpret_cast<inquiry>(unsetNonzero), /* nb_nonzero/nb_bool */
};

PyTypeObject UnsetType =
{
    /* The ob_type field must be initialized in the module init function
     * to be portable to Windows without using C++. */
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    STRCAST("IcePy.UnsetType"),      /* tp_name */
    0,                               /* tp_basicsize */
    0,                               /* tp_itemsize */
    /* methods */
    reinterpret_cast<destructor>(unsetDealloc), /* tp_dealloc */
    0,                               /* tp_print */
    0,                               /* tp_getattr */
    0,                               /* tp_setattr */
    0,                               /* tp_reserved */
    reinterpret_cast<reprfunc>(unsetRepr), /* tp_repr */
    &UnsetAsNumber,                  /* tp_as_number */
    0,                               /* tp_as_sequence */
    0,                               /* tp_as_mapping */
    0,                               /* tp_hash */
    0,                               /* tp_call */
    0,                               /* tp_str */
    0,                               /* tp_getattro */
    0,                               /* tp_setattro */
    0,                               /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,              /* tp_flags */
    0,                               /* tp_doc */
    0,                               /* tp_traverse */
    0,                               /* tp_clear */
    0,                               /* tp_richcompare */
    0,                               /* tp_weaklistoffset */
    0,                               /* tp_iter */
    0,                               /* tp_iternext */
    0,                               /* tp_methods */
    0,                               /* tp_members */
    0,                               /* tp_getset */
    0,                               /* tp_base */
    0,                               /* tp_dict */
    0,                               /* tp_descr_get */
    0,                               /* tp_descr_set */
    0,                               /* tp_dictoffset */
    0,                               /* tp_init */
    0,                               /* tp_alloc */
    0,                               /* tp_new */
    0,                               /* tp_free */
    0,                               /* tp_is_gc */
};

//
// Unset is a singleton, similar to None.
//
PyObject UnsetValue =
{
    _PyObject_EXTRA_INIT
#if PY_VERSION_HEX >= 0x030c0000
    {1},
#else
    1,
#endif
    &UnsetType
};

PyObject* Unset = &UnsetValue;

}

bool
IcePy::initTypes(PyObject* module)
{
    if(PyType_Ready(&TypeInfoType) < 0)
    {
        return false;
    }
    PyTypeObject* typeInfoType = &TypeInfoType; // Necessary to prevent GCC's strict-alias warnings.
    if(PyModule_AddObject(module, STRCAST("TypeInfo"), reinterpret_cast<PyObject*>(typeInfoType)) < 0)
    {
        return false;
    }

    if(PyType_Ready(&ExceptionInfoType) < 0)
    {
        return false;
    }
    PyTypeObject* exceptionInfoType = &ExceptionInfoType; // Necessary to prevent GCC's strict-alias warnings.
    if(PyModule_AddObject(module, STRCAST("ExceptionInfo"), reinterpret_cast<PyObject*>(exceptionInfoType)) < 0)
    {
        return false;
    }

    PrimitiveInfoPtr boolType = make_shared<PrimitiveInfo>(PrimitiveInfo::KindBool);
    PyObjectHandle boolTypeObj = createType(boolType);
    if(PyModule_AddObject(module, STRCAST("_t_bool"), boolTypeObj.get()) < 0)
    {
        return false;
    }
    boolTypeObj.release(); // PyModule_AddObject steals a reference.

    PrimitiveInfoPtr byteType = make_shared<PrimitiveInfo>(PrimitiveInfo::KindByte);
    PyObjectHandle byteTypeObj = createType(byteType);
    if(PyModule_AddObject(module, STRCAST("_t_byte"), byteTypeObj.get()) < 0)
    {
        return false;
    }
    byteTypeObj.release(); // PyModule_AddObject steals a reference.

    PrimitiveInfoPtr shortType = make_shared<PrimitiveInfo>(PrimitiveInfo::KindShort);
    PyObjectHandle shortTypeObj = createType(shortType);
    if(PyModule_AddObject(module, STRCAST("_t_short"), shortTypeObj.get()) < 0)
    {
        return false;
    }
    shortTypeObj.release(); // PyModule_AddObject steals a reference.

    PrimitiveInfoPtr intType = make_shared<PrimitiveInfo>(PrimitiveInfo::KindInt);
    PyObjectHandle intTypeObj = createType(intType);
    if(PyModule_AddObject(module, STRCAST("_t_int"), intTypeObj.get()) < 0)
    {
        return false;
    }
    intTypeObj.release(); // PyModule_AddObject steals a reference.

    PrimitiveInfoPtr longType = make_shared<PrimitiveInfo>(PrimitiveInfo::KindLong);
    PyObjectHandle longTypeObj = createType(longType);
    if(PyModule_AddObject(module, STRCAST("_t_long"), longTypeObj.get()) < 0)
    {
        return false;
    }
    longTypeObj.release(); // PyModule_AddObject steals a reference.

    PrimitiveInfoPtr floatType = make_shared<PrimitiveInfo>(PrimitiveInfo::KindFloat);
    PyObjectHandle floatTypeObj = createType(floatType);
    if(PyModule_AddObject(module, STRCAST("_t_float"), floatTypeObj.get()) < 0)
    {
        return false;
    }
    floatTypeObj.release(); // PyModule_AddObject steals a reference.

    PrimitiveInfoPtr doubleType = make_shared<PrimitiveInfo>(PrimitiveInfo::KindDouble);
    PyObjectHandle doubleTypeObj = createType(doubleType);
    if(PyModule_AddObject(module, STRCAST("_t_double"), doubleTypeObj.get()) < 0)
    {
        return false;
    }
    doubleTypeObj.release(); // PyModule_AddObject steals a reference.

    PrimitiveInfoPtr stringType = make_shared<PrimitiveInfo>(PrimitiveInfo::KindString);
    PyObjectHandle stringTypeObj = createType(stringType);
    if(PyModule_AddObject(module, STRCAST("_t_string"), stringTypeObj.get()) < 0)
    {
        return false;
    }
    stringTypeObj.release(); // PyModule_AddObject steals a reference.

    if(PyModule_AddObject(module, STRCAST("Unset"), Unset) < 0)
    {
        return false;
    }
    Py_IncRef(Unset); // PyModule_AddObject steals a reference.

    return true;
}

IcePy::TypeInfoPtr
IcePy::getType(PyObject* obj)
{
    assert(PyObject_IsInstance(obj, reinterpret_cast<PyObject*>(&TypeInfoType)));
    TypeInfoObject* p = reinterpret_cast<TypeInfoObject*>(obj);
    return *p->info;
}

PyObject*
IcePy::createType(const TypeInfoPtr& info)
{
    TypeInfoObject* obj = typeInfoNew(&TypeInfoType, 0, 0);
    if(obj)
    {
        obj->info = new IcePy::TypeInfoPtr(info);
    }
    return reinterpret_cast<PyObject*>(obj);
}

IcePy::ExceptionInfoPtr
IcePy::getException(PyObject* obj)
{
    assert(PyObject_IsInstance(obj, reinterpret_cast<PyObject*>(&ExceptionInfoType)));
    ExceptionInfoObject* p = reinterpret_cast<ExceptionInfoObject*>(obj);
    return *p->info;
}

PyObject*
IcePy::createException(const ExceptionInfoPtr& info)
{
    ExceptionInfoObject* obj = exceptionInfoNew(&ExceptionInfoType, 0, 0);
    if(obj)
    {
        obj->info = new IcePy::ExceptionInfoPtr(info);
    }
    return reinterpret_cast<PyObject*>(obj);
}

extern "C"
PyObject*
IcePy_defineEnum(PyObject*, PyObject* args)
{
    char* id;
    PyObject* type;
    PyObject* meta; // Not currently used.
    PyObject* enumerators;
    if(!PyArg_ParseTuple(args, STRCAST("sOOO"), &id, &type, &meta, &enumerators))
    {
        return 0;
    }

    assert(PyTuple_Check(meta));

    EnumInfoPtr info = make_shared<EnumInfo>(id, type, enumerators);

    return createType(info);
}

extern "C"
PyObject*
IcePy_defineStruct(PyObject*, PyObject* args)
{
    char* id;
    PyObject* type;
    PyObject* meta; // Not currently used.
    PyObject* members;
    if(!PyArg_ParseTuple(args, STRCAST("sOOO"), &id, &type, &meta, &members))
    {
        return 0;
    }

    assert(PyType_Check(type));
    assert(PyTuple_Check(meta));
    assert(PyTuple_Check(members));

    StructInfoPtr info = make_shared<StructInfo>(id, type, members);

    return createType(info);
}

extern "C"
PyObject*
IcePy_defineSequence(PyObject*, PyObject* args)
{
    char* id;
    PyObject* meta;
    PyObject* elementType;
    if(!PyArg_ParseTuple(args, STRCAST("sOO"), &id, &meta, &elementType))
    {
        return 0;
    }

    try
    {
        SequenceInfoPtr info = make_shared<SequenceInfo>(id, meta, elementType);
        return createType(info);
    }
    catch(const InvalidSequenceFactoryException&)
    {
        assert(PyErr_Occurred());
        return 0;
    }
}

extern "C"
PyObject*
IcePy_defineCustom(PyObject*, PyObject* args)
{
    char* id;
    PyObject* type;
    if(!PyArg_ParseTuple(args, STRCAST("sO"), &id, &type))
    {
        return 0;
    }

    CustomInfoPtr info = make_shared<CustomInfo>(id, type);

    return createType(info);
}

extern "C"
PyObject*
IcePy_defineDictionary(PyObject*, PyObject* args)
{
    char* id;
    PyObject* meta; // Not currently used.
    PyObject* keyType;
    PyObject* valueType;
    if(!PyArg_ParseTuple(args, STRCAST("sOOO"), &id, &meta, &keyType, &valueType))
    {
        return 0;
    }

    assert(PyTuple_Check(meta));

    DictionaryInfoPtr info = make_shared<DictionaryInfo>(id, keyType, valueType);

    return createType(info);
}

extern "C"
PyObject*
IcePy_declareProxy(PyObject*, PyObject* args)
{
    char* id;
    if(!PyArg_ParseTuple(args, STRCAST("s"), &id))
    {
        return 0;
    }

    string proxyId = id;
    proxyId += "Prx";

    ProxyInfoPtr info = lookupProxyInfo(proxyId);
    if(!info)
    {
        info = make_shared<ProxyInfo>(proxyId);
        info->init();
        addProxyInfo(proxyId, info);
        return info->typeObj; // Delegate ownership to the global "_t_XXX" variable.
    }
    else
    {
        Py_INCREF(info->typeObj);
        return info->typeObj;
    }
}

extern "C"
PyObject*
IcePy_defineProxy(PyObject*, PyObject* args)
{
    char* id;
    PyObject* type;
    if(!PyArg_ParseTuple(args, STRCAST("sO"), &id, &type))
    {
        return 0;
    }

    assert(PyType_Check(type));

    string proxyId = id;
    proxyId += "Prx";

    ProxyInfoPtr info = lookupProxyInfo(proxyId);
    if(!info)
    {
        info = make_shared<ProxyInfo>(proxyId);
        info->init();
        addProxyInfo(proxyId, info);
        info->define(type);
        return info->typeObj; // Delegate ownership to the global "_t_XXX" variable.
    }
    else
    {
        info->define(type);
        Py_INCREF(info->typeObj);
        return info->typeObj;
    }
}

extern "C"
PyObject*
IcePy_declareClass(PyObject*, PyObject* args)
{
    char* id;
    if(!PyArg_ParseTuple(args, STRCAST("s"), &id))
    {
        return 0;
    }

    ClassInfoPtr info = lookupClassInfo(id);
    if (!info)
    {
        info = make_shared<ClassInfo>(id);
        info->init();
        addClassInfo(id, info);
        return info->typeObj; // Delegate ownership to the global "_t_XXX" variable.
    }
    else
    {
        Py_INCREF(info->typeObj);
        return info->typeObj;
    }
}

extern "C"
PyObject*
IcePy_defineClass(PyObject*, PyObject* args)
{
    char* id;
    PyObject* type;
    PyObject* meta; // Not currently used.
    PyObject* base;
    PyObject* interfaces;
    if(!PyArg_ParseTuple(args, STRCAST("sOOOO"), &id, &type, &meta, &base, &interfaces))
    {
        return 0;
    }

    assert(PyTuple_Check(meta));

    //
    // A ClassInfo object will already exist for this id if a forward declaration
    // was encountered, or if the Slice definition is being reloaded. In the latter
    // case, we act as if it hasn't been defined yet.
    //
    ClassInfoPtr info = lookupClassInfo(id);
    if(!info || info->defined)
    {
        info = make_shared<ClassInfo>(id);
        info->init();
        addClassInfo(id, info);
        info->define(type, base, interfaces);
        return info->typeObj; // Delegate ownership to the global "_t_XXX" variable.
    }
    else
    {
        info->define(type, base, interfaces);
        Py_INCREF(info->typeObj);
        return info->typeObj;
    }
}

extern "C"
PyObject*
IcePy_declareValue(PyObject*, PyObject* args)
{
    char* id;
    if(!PyArg_ParseTuple(args, STRCAST("s"), &id))
    {
        return 0;
    }

    ValueInfoPtr info = lookupValueInfo(id);
    if(!info)
    {
        info = make_shared<ValueInfo>(id);
        info->init();
        addValueInfo(id, info);
        return info->typeObj; // Delegate ownership to the global "_t_XXX" variable.
    }
    else
    {
        Py_INCREF(info->typeObj);
        return info->typeObj;
    }
}

extern "C"
PyObject*
IcePy_defineValue(PyObject*, PyObject* args)
{
    char* id;
    PyObject* type;
    int compactId;
    PyObject* meta; // Not currently used.
    int preserve;
    int interface;
    PyObject* base;
    PyObject* members;
    if(!PyArg_ParseTuple(args, STRCAST("sOiOiiOO"), &id, &type, &compactId, &meta, &preserve, &interface, &base,
                         &members))
    {
        return 0;
    }

    assert(PyTuple_Check(meta));

    PyObject* r;

    //
    // A ClassInfo object will already exist for this id if a forward declaration
    // was encountered, or if the Slice definition is being reloaded. In the latter
    // case, we act as if it hasn't been defined yet.
    //
    ValueInfoPtr info = lookupValueInfo(id);
    if(!info || info->defined)
    {
        info = make_shared<ValueInfo>(id);
        info->init();
        addValueInfo(id, info);
        r = info->typeObj; // Delegate ownership to the global "_t_XXX" variable.
    }
    else
    {
        Py_INCREF(info->typeObj);
        r = info->typeObj;
    }

    info->define(type, compactId, preserve ? true : false, interface ? true : false, base, members);

    if(info->compactId != -1)
    {
        CompactIdMap::iterator q = _compactIdMap.find(info->compactId);
        if(q != _compactIdMap.end())
        {
            _compactIdMap.erase(q);
        }
        _compactIdMap.insert(CompactIdMap::value_type(info->compactId, info));
    }

    return r;
}

extern "C"
PyObject*
IcePy_defineException(PyObject*, PyObject* args)
{
    char* id;
    PyObject* type;
    PyObject* meta;
    int preserve;
    PyObject* base;
    PyObject* members;
    if(!PyArg_ParseTuple(args, STRCAST("sOOiOO"), &id, &type, &meta, &preserve, &base, &members))
    {
        return 0;
    }

    assert(PyExceptionClass_Check(type));
    assert(PyTuple_Check(meta));
    assert(PyTuple_Check(members));

    ExceptionInfoPtr info = make_shared<ExceptionInfo>();
    info->id = id;

    info->preserve = preserve ? true : false;

    if(base != Py_None)
    {
        info->base = dynamic_pointer_cast<ExceptionInfo>(getException(base));
        assert(info->base);
    }

    convertDataMembers(members, info->members, info->optionalMembers, true);

    info->usesClasses = false;

    //
    // Only examine the required members to see if any use classes.
    //
    for(DataMemberList::iterator p = info->members.begin(); p != info->members.end(); ++p)
    {
        if(!info->usesClasses)
        {
            info->usesClasses = (*p)->type->usesClasses();
        }
    }

    info->pythonType = type;

    addExceptionInfo(id, info);

    return createException(info);
}

extern "C"
PyObject*
IcePy_stringify(PyObject*, PyObject* args)
{
    PyObject* value;
    PyObject* type;
    if(!PyArg_ParseTuple(args, STRCAST("OO"), &value, &type))
    {
        return 0;
    }

    TypeInfoPtr info = getType(type);
    assert(info);

    ostringstream ostr;
    IceUtilInternal::Output out(ostr);
    PrintObjectHistory history;
    history.index = 0;
    info->print(value, out, &history);

    string str = ostr.str();
    return createString(str);
}

extern "C"
PyObject*
IcePy_stringifyException(PyObject*, PyObject* args)
{
    PyObject* value;
    if(!PyArg_ParseTuple(args, STRCAST("O"), &value))
    {
        return 0;
    }

    PyObjectHandle iceType = getAttr(value, "_ice_type", false);
    assert(iceType.get());
    ExceptionInfoPtr info = getException(iceType.get());
    assert(info);

    ostringstream ostr;
    IceUtilInternal::Output out(ostr);
    info->print(value, out);

    string str = ostr.str();
    return createString(str);
}
