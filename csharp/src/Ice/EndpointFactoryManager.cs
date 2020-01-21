//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

namespace IceInternal
{
    using System.Collections.Generic;
    using System.Diagnostics;

    public sealed class EndpointFactoryManager
    {
        internal EndpointFactoryManager(Ice.Communicator communicator)
        {
            _communicator = communicator;
            _factories = new List<IEndpointFactory>();
        }

        public void initialize()
        {
            foreach (IEndpointFactory f in _factories)
            {
                f.initialize();
            }
        }

        public void add(IEndpointFactory factory)
        {
            lock (this)
            {
                foreach (IEndpointFactory f in _factories)
                {
                    if (f.type() == factory.type())
                    {
                        Debug.Assert(false);
                    }
                }
                _factories.Add(factory);
            }
        }

        public IEndpointFactory? get(short type)
        {
            lock (this)
            {
                foreach (IEndpointFactory f in _factories)
                {
                    if (f.type() == type)
                    {
                        return f;
                    }
                }
                return null;
            }
        }

        public Endpoint? create(string str, bool oaEndpoint)
        {
            string[]? arr = IceUtilInternal.StringUtil.splitString(str, " \t\r\n");
            if (arr == null)
            {
                throw new System.FormatException("mismatched quote");
            }

            if (arr.Length == 0)
            {
                throw new System.FormatException("value has no non-whitespace characters");
            }

            List<string> v = new List<string>(arr);
            string protocol = v[0];
            v.RemoveAt(0);

            if (protocol.Equals("default"))
            {
                protocol = _communicator.DefaultsAndOverrides.defaultProtocol;
            }

            IEndpointFactory? factory = null;

            lock (this)
            {
                for (int i = 0; i < _factories.Count; i++)
                {
                    IEndpointFactory f = _factories[i];
                    if (f.protocol().Equals(protocol))
                    {
                        factory = f;
                    }
                }
            }

            if (factory != null)
            {
                Endpoint e = factory.create(v, oaEndpoint);
                if (v.Count > 0)
                {
                    throw new System.FormatException($"unrecognized argument `{v[0]}' in endpoint `{str}'");
                }
                return e;

                // Code below left in place for debugging.

                /*
                EndpointI e = f.create(s.Substring(m.Index + m.Length), oaEndpoint);
                BasicStream bs = new BasicStream(_instance, true);
                e.streamWrite(bs);
                Buffer buf = bs.getBuffer();
                buf.b.position(0);
                short type = bs.readShort();
                EndpointI ue = new IceInternal.OpaqueEndpointI(type, bs);
                System.Console.Error.WriteLine("Normal: " + e);
                System.Console.Error.WriteLine("Opaque: " + ue);
                return e;
                */
            }

            //
            // If the stringified endpoint is opaque, create an unknown endpoint,
            // then see whether the type matches one of the known endpoints.
            //
            if (protocol.Equals("opaque"))
            {
                Endpoint ue = new OpaqueEndpointI(v);
                if (v.Count > 0)
                {
                    throw new System.FormatException($"unrecognized argument `{v[0]}' in endpoint `{str}'");
                }
                factory = get(ue.type());
                if (factory != null)
                {
                    //
                    // Make a temporary stream, write the opaque endpoint data into the stream,
                    // and ask the factory to read the endpoint data from that stream to create
                    // the actual endpoint.
                    //
                    Ice.OutputStream os = new Ice.OutputStream(_communicator, Ice.Util.currentProtocolEncoding);
                    os.WriteShort(ue.type());
                    ue.streamWrite(os);
                    Ice.InputStream iss =
                        new Ice.InputStream(_communicator, Ice.Util.currentProtocolEncoding, os.GetBuffer(), true);
                    iss.Pos = 0;
                    iss.ReadShort(); // type
                    iss.StartEndpointEncapsulation();
                    Endpoint? e = factory.read(iss);
                    iss.EndEndpointEncapsulation();
                    return e;
                }
                return ue; // Endpoint is opaque, but we don't have a factory for its type.
            }

            return null;
        }

        public Endpoint read(Ice.InputStream s)
        {
            lock (this)
            {
                short type = s.ReadShort();

                IEndpointFactory? factory = get(type);
                Endpoint? e = null;

                s.StartEndpointEncapsulation();

                if (factory != null)
                {
                    e = factory.read(s);
                }
                //
                // If the factory failed to read the endpoint, return an opaque endpoint. This can
                // occur if for example the factory delegates to another factory and this factory
                // isn't available. In this case, the factory needs to make sure the stream position
                // is preserved for reading the opaque endpoint.
                //
                if (e == null)
                {
                    e = new OpaqueEndpointI(type, s);
                }

                s.EndEndpointEncapsulation();

                return e;
            }
        }

        internal void destroy()
        {
            foreach (IEndpointFactory f in _factories)
            {
                f.destroy();
            }
            _factories.Clear();
        }

        private readonly Ice.Communicator _communicator;
        private readonly List<IEndpointFactory> _factories;
    }

}
