// Copyright (c) ZeroC, Inc. All rights reserved.

using System;
using System.Diagnostics;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Test;

namespace ZeroC.Ice.Test.Interceptor
{
    public class RetryException : Exception
    {
    }

    public static class DispatchInterceptors
    {
        public static AsyncLocal<int> LocalContext { get; } = new AsyncLocal<int>();
        public static async ValueTask ActivateAsync(ObjectAdapter adapter)
        {
            DispatchInterceptor raiseInterceptor = async (request, current, next, cancel) =>
            {
                // Ensure the invocation plug-in interceptor added this entry to the context, with ice1 the interceptors
                // cannot modify the conext because it is marshalled before the interceptors run.
                Debug.Assert(request.Protocol == Protocol.Ice1 ||
                             current.Context["InvocationPlugin"] == "1");

                // The dispatch plug-in interceptor runs before the interceptors registered with
                // the adapter.
                Debug.Assert(current.Context.ContainsKey("DispatchPlugin"));
                if (current.Context.TryGetValue("raiseBeforeDispatch", out string? context))
                {
                    if (context == "invalidInput")
                    {
                        throw new InvalidInputException("intercept");
                    }
                    else if (context == "notExist")
                    {
                        throw new ObjectNotExistException();
                    }
                }

                OutgoingResponseFrame response = await next(request, current, cancel);

                if (current.Context.TryGetValue("raiseAfterDispatch", out context))
                {
                    if (context == "invalidInput")
                    {
                        throw new InvalidInputException("raiseAfterDispatch");
                    }
                    else if (context == "notExist")
                    {
                        throw new ObjectNotExistException();
                    }
                }

                return response;
            };

            DispatchInterceptor addWithRetry = async (request, current, next, cancel) =>
                {
                    if (current.Operation == "addWithRetry")
                    {
                        for (int i = 0; i < 10; ++i)
                        {
                            try
                            {
                                await next(request, current, cancel).ConfigureAwait(false);
                                TestHelper.Assert(false);
                            }
                            catch (RetryException)
                            {
                                // Expected, retry
                            }
                        }
                        current.Context["retry"] = "no";
                    }
                    return await next(request, current, cancel);
                };

            DispatchInterceptor retry = async (request, current, next, cancel) =>
                {
                    if (current.Context.TryGetValue("retry", out string? context) && context.Equals("yes"))
                    {
                        // Retry the dispatch to ensure that abandoning the result of the dispatch works fine and is
                        // thread-safe
                        ValueTask<OutgoingResponseFrame> vt1 = next(request, current, cancel);
                        ValueTask<OutgoingResponseFrame> vt2 = next(request, current, cancel);
                        await vt1.ConfigureAwait(false);
                        return await vt2.ConfigureAwait(false);
                    }
                    return await next(request, current, cancel);
                };

            DispatchInterceptor opWithBianryContext = async (request, current, next, cancel) =>
                {
                    if (current.Operation == "opWithBinaryContext" && request.Protocol == Protocol.Ice2)
                    {
                        Debug.Assert(request.BinaryContext.ContainsKey(3));
                        short size = request.BinaryContext[3].Read(istr => istr.ReadShort());
                        var t2 = new Token(1, "mytoken", Enumerable.Range(0, size).Select(i => (byte)2).ToArray());
                        Debug.Assert(request.BinaryContext.ContainsKey(1));
                        Token t1 = request.BinaryContext[1].Read(Token.IceReader);
                        TestHelper.Assert(t1.Hash == t2.Hash);
                        TestHelper.Assert(t1.Expiration == t2.Expiration);
                        TestHelper.Assert(t1.Payload.SequenceEqual(t2.Payload));
                        Debug.Assert(request.BinaryContext.ContainsKey(2));
                        string[] s2 = request.BinaryContext[2].Read(istr =>
                            istr.ReadArray(1, InputStream.IceReaderIntoString));
                        Enumerable.Range(0, 10).Select(i => $"string-{i}").SequenceEqual(s2);

                        if (request.HasCompressedPayload)
                        {
                            request.DecompressPayload();

                            Debug.Assert(request.BinaryContext.ContainsKey(3));
                            size = request.BinaryContext[3].Read(istr => istr.ReadShort());

                            Debug.Assert(request.BinaryContext.ContainsKey(1));
                            t1 = request.BinaryContext[1].Read(Token.IceReader);
                            t2 = request.ReadArgs(current.Communicator, Token.IceReader);
                            TestHelper.Assert(t1.Hash == t2.Hash);
                            TestHelper.Assert(t1.Expiration == t2.Expiration);
                            TestHelper.Assert(t1.Payload.SequenceEqual(t2.Payload));
                            Debug.Assert(request.BinaryContext.ContainsKey(2));
                            s2 = request.BinaryContext[2].Read(istr =>
                                istr.ReadArray(1, InputStream.IceReaderIntoString));
                            Enumerable.Range(0, 10).Select(i => $"string-{i}").SequenceEqual(s2);
                        }
                    }
                    return await next(request, current, cancel);
                };

            DispatchInterceptor op1 = async (request, current, next, cancel) =>
                {
                    if (current.Operation == "op1")
                    {
                        LocalContext.Value = int.Parse(current.Context["local-user"]);
                        if (request.Protocol == Protocol.Ice2)
                        {
                            OutgoingResponseFrame response = await next(request, current, cancel);
                            response.AddBinaryContextEntry(110, 110, (ostr, value) => ostr.WriteInt(value));
                            response.AddBinaryContextEntry(120, 120, (ostr, value) => ostr.WriteInt(value));
                            return response;
                        }
                    }
                    return await next(request, current, cancel);
                };
            await adapter.ActivateAsync(raiseInterceptor, addWithRetry, retry, opWithBianryContext, op1);
        }
    }
}
