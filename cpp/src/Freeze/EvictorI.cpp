// **********************************************************************
//
// Copyright (c) 2001
// MutableRealms, Inc.
// Huntsville, AL, USA
//
// All Rights Reserved
//
// **********************************************************************

#include <Freeze/EvictorI.h>
#include <sstream>

using namespace std;
using namespace Ice;
using namespace Freeze;

Freeze::EvictorI::EvictorI(const DBPtr& db, const CommunicatorPtr& communicator) :
    _db(db),
    _evictorSize(static_cast<map<string, EvictorElement>::size_type>(10)),
    _logger(communicator->getLogger()),
    _trace(0)
{
    PropertiesPtr properties = communicator->getProperties();
    string value;

    value = properties->getProperty("Freeze.Trace.Evictor");
    if(!value.empty())
    {
	_trace = atoi(value.c_str());
    }
}

void
Freeze::EvictorI::setSize(Int evictorSize)
{
    JTCSyncT<JTCMutex> sync(*this);

    //
    // Ignore requests to set the evictor size to values smaller or
    // equal to zero.
    //
    if (evictorSize <= 0)
    {
	return;
    }

    //
    // Update the evictor size.
    //
    _evictorSize = static_cast<map<string, EvictorElement>::size_type>(evictorSize);

    //
    // Evict as many elements as necessary.
    //
    evict();
}

Int
Freeze::EvictorI::getSize()
{
    JTCSyncT<JTCMutex> sync(*this);

    return static_cast<Int>(_evictorSize);
}

void
Freeze::EvictorI::createObject(const string& identity, const ObjectPtr& servant)
{
    JTCSyncT<JTCMutex> sync(*this);

    //
    // Save the new Ice Object to the database.
    //
    _db->put(identity, servant);
    add(identity, servant);

    if (_trace >= 1)
    {
	ostringstream s;
	s << "created \"" << identity << "\"";
	_logger->trace("Evictor", s.str());
    }
}

void
Freeze::EvictorI::destroyObject(const string& identity)
{
    JTCSyncT<JTCMutex> sync(*this);

    //
    // Delete the Ice Object from the database.
    //
    _db->del(identity);
    remove(identity);

    if (_trace >= 1)
    {
	ostringstream s;
	s << "destroyed \"" << identity << "\"";
	_logger->trace("Evictor", s.str());
    }
}

void
Freeze::EvictorI::installServantInitializer(const ServantInitializerPtr& initializer)
{
    JTCSyncT<JTCMutex> sync(*this);

    _initializer = initializer;
}

ObjectPtr
Freeze::EvictorI::locate(const ObjectAdapterPtr& adapter, const string& identity, ObjectPtr&)
{
    JTCSyncT<JTCMutex> sync(*this);
    
    map<string, EvictorElement>::iterator p = _evictorMap.find(identity);
    if (p != _evictorMap.end())
    {
	if (_trace >= 2)
	{
	    ostringstream s;
	    s << "found \"" << identity << "\" in queue";
	    _logger->trace("Evictor", s.str());
	}

	//
	// Ice Object found in evictor map. Push it to the front of
	// the evictor list, so that it will be evicted last.
	//
	_evictorList.erase(p->second.position);
	_evictorList.push_front(identity);
	p->second.position = _evictorList.begin();

	//
	// Return the servant for the Ice Object.
	//
	return p->second.servant;
    }
    else
    {
	if (_trace >= 2)
	{
	    ostringstream s;
	    s << "couldn't find \"" << identity << "\" in queue\n"
	      << "loading \"" << identity << "\" from database";
	    _logger->trace("Evictor", s.str());
	}

	//
	// Load the Ice Object from database and create and add a
	// Servant for it.
	//
	ObjectPtr servant = _db->get(identity);
	if (!servant)
	{
	    //
            // Ice Object with the given identity does not exist,
            // client will get an ObjectNotExistException.
	    //
	    return 0;
	}
	add(identity, servant);

	//
	// If an initializer is installed, call it now.
	//
	if (_initializer)
	{
	    _initializer->initialize(adapter, identity, servant);
	}

	//
	// Return the new servant for the Ice Object from the database.
	//
	return servant;
    }
}

void
Freeze::EvictorI::finished(const ObjectAdapterPtr&, const string&, const ObjectPtr&, const ObjectPtr&)
{
    //JTCSyncT<JTCMutex> sync(*this);
}

void
Freeze::EvictorI::deactivate()
{
    JTCSyncT<JTCMutex> sync(*this);

    if (_trace >= 1)
    {
	ostringstream s;
	s << "deactivating, saving all Ice Objects in queue in database";
	_logger->trace("Evictor", s.str());
    }

    //
    // Save all Ice Objects in the database upon deactivation, and
    // clear the evictor map and list.
    //
    for (map<string, EvictorElement>::iterator p = _evictorMap.begin(); p != _evictorMap.end(); ++p)
    {
	_db->put(*(p->second.position), p->second.servant);
    }
    _evictorMap.clear();
    _evictorList.clear();
}

void
Freeze::EvictorI::evict()
{
    //
    // With most STL implementations, _evictorMap.size() is faster
    // than _evictorList.size().
    //
    while (_evictorMap.size() > _evictorSize)
    {
	//
	// Get the last element of the Evictor queue.
	//
	string identity = _evictorList.back();
	map<string, EvictorElement>::iterator p = _evictorMap.find(identity);
	assert(p != _evictorMap.end());
	ObjectPtr servant = p->second.servant;
	assert(servant);

	//
	// Remove last element from the evictor queue.
	//
	_evictorMap.erase(identity);
	_evictorList.pop_back();
	assert(_evictorMap.size() == _evictorSize);

	//
	// Save the evicted Ice Object to the database.
	//
	_db->put(identity, servant);

	if (_trace >= 2)
	{
	    ostringstream s;
	    s << "evicted \"" << identity << "\" from queue\n"
	      << "number of elements in queue: " << _evictorMap.size();
	    _logger->trace("Evictor", s.str());
	}
    }
}

void
Freeze::EvictorI::add(const string& identity, const ObjectPtr& servant)
{
    //
    // Ignore the request if the Ice Object is already in the queue.
    //
    if(_evictorMap.find(identity) != _evictorMap.end())
    {
	return;
    }    

    //
    // Add an Ice Object with its Servant to the evictor list and
    // evictor map.
    //
    _evictorList.push_front(identity);
    EvictorElement evictorElement;
    evictorElement.servant = servant;
    evictorElement.position = _evictorList.begin();
    _evictorMap[identity] = evictorElement;

    //
    // Evict as many elements as necessary.
    //
    evict();
}

void
Freeze::EvictorI::remove(const string& identity)
{
    //
    // If the Ice Object is currently in the evictor, remove it.
    //
    map<string, EvictorElement>::iterator p = _evictorMap.find(identity);
    if (p != _evictorMap.end())
    {
	_evictorList.erase(p->second.position);
	_evictorMap.erase(p);
    }
}
