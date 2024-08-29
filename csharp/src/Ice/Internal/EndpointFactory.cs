// Copyright (c) ZeroC, Inc.

namespace Ice.Internal;

using System.Collections.Generic;

public interface EndpointFactory
{
    void initialize();

    short type();

    string protocol();

    EndpointI create(List<string> args, bool oaEndpoint);

    EndpointI read(Ice.InputStream s);

    void destroy();

    EndpointFactory clone(ProtocolInstance instance);
}

public abstract class EndpointFactoryWithUnderlying : EndpointFactory
{
    protected EndpointFactoryWithUnderlying(ProtocolInstance instance, short type)
    {
        instance_ = instance;
        _type = type;
    }

    public void initialize()
    {
        //
        // Get the endpoint factory for the underlying type and clone it with
        // our protocol instance.
        //
        EndpointFactory factory = instance_.getEndpointFactory(_type);
        if (factory != null)
        {
            _underlying = factory.clone(instance_);
            _underlying.initialize();
        }
    }

    public short type()
    {
        return instance_.type();
    }

    public string protocol()
    {
        return instance_.protocol();
    }

    public EndpointI create(List<string> args, bool oaEndpoint)
    {
        if (_underlying == null)
        {
            return null; // Can't create an endpoint without underlying factory.
        }
        return createWithUnderlying(_underlying.create(args, oaEndpoint), args, oaEndpoint);
    }

    public EndpointI read(Ice.InputStream s)
    {
        if (_underlying == null)
        {
            return null; // Can't create an endpoint without underlying factory.
        }
        return readWithUnderlying(_underlying.read(s), s);
    }

    public void destroy()
    {
        if (_underlying != null)
        {
            _underlying.destroy();
        }
        instance_ = null;
    }

    public EndpointFactory clone(ProtocolInstance instance)
    {
        return cloneWithUnderlying(instance, _type);
    }

    public abstract EndpointFactory cloneWithUnderlying(ProtocolInstance instance, short underlying);

    protected abstract EndpointI createWithUnderlying(EndpointI underlying, List<string> args, bool oaEndpoint);

    protected abstract EndpointI readWithUnderlying(EndpointI underlying, Ice.InputStream s);

    protected ProtocolInstance instance_;

    private readonly short _type;
    private EndpointFactory _underlying;
}
