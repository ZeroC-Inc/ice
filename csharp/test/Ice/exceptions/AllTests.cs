//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using Ice.exceptions.Test;

namespace Ice.exceptions
{
    public class AllTests : global::Test.AllTests
    {
        public static IThrowerPrx allTests(global::Test.TestHelper helper)
        {
            Communicator communicator = helper.communicator();
            var output = helper.getWriter();
            {
                output.Write("testing object adapter registration exceptions... ");
                ObjectAdapter first;
                try
                {
                    first = communicator.CreateObjectAdapter("TestAdapter0");
                }
                catch (Ice.InvalidConfigurationException)
                {
                    // Expected
                }

                communicator.SetProperty("TestAdapter0.Endpoints", "tcp -h *");
                first = communicator.CreateObjectAdapter("TestAdapter0");
                try
                {
                    communicator.CreateObjectAdapter("TestAdapter0");
                    test(false);
                }
                catch (ArgumentException)
                {
                    // Expected.
                }

                try
                {
                    ObjectAdapter second =
                        communicator.CreateObjectAdapterWithEndpoints("TestAdapter0", "ssl -h foo -p 12011");
                    test(false);

                    //
                    // Quell mono error that variable second isn't used.
                    //
                    second.Deactivate();
                }
                catch (ArgumentException)
                {
                    // Expected
                }
                first.Deactivate();
                output.WriteLine("ok");
            }

            {
                output.Write("testing servant registration exceptions... ");
                communicator.SetProperty("TestAdapter1.Endpoints", "tcp -h *");
                ObjectAdapter adapter = communicator.CreateObjectAdapter("TestAdapter1");
                var obj = new Empty();
                adapter.Add("x", obj);
                try
                {
                    adapter.Add("x", obj);
                    test(false);
                }
                catch (ArgumentException)
                {
                }

                try
                {
                    adapter.Add("", obj);
                    test(false);
                }
                catch (FormatException)
                {
                }

                adapter.Remove("x");
                adapter.Remove("x"); // as of Ice 4.0, can remove multiple times
                adapter.Deactivate();
                output.WriteLine("ok");
            }

            output.Write("testing stringToProxy... ");
            output.Flush();
            string @ref = "thrower:" + helper.getTestEndpoint(0);
            var @base = IObjectPrx.Parse(@ref, communicator);
            test(@base != null);
            output.WriteLine("ok");

            output.Write("testing checked cast... ");
            output.Flush();
            var thrower = IThrowerPrx.CheckedCast(@base);

            test(thrower != null);
            test(thrower.Equals(@base));
            output.WriteLine("ok");

            output.Write("catching exact types... ");
            output.Flush();

            try
            {
                thrower.throwAasA(1);
                test(false);
            }
            catch (A ex)
            {
                test(ex.aMem == 1);
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex);
                test(false);
            }

            try
            {
                thrower.throwAorDasAorD(1);
                test(false);
            }
            catch (A ex)
            {
                test(ex.aMem == 1);
            }
            catch (System.Exception)
            {
                test(false);
            }

            try
            {
                thrower.throwAorDasAorD(-1);
                test(false);
            }
            catch (D ex)
            {
                test(ex.dMem == -1);
            }
            catch (System.Exception)
            {
                test(false);
            }

            try
            {
                thrower.throwBasB(1, 2);
                test(false);
            }
            catch (Test.B ex)
            {
                test(ex.aMem == 1);
                test(ex.bMem == 2);
            }
            catch (System.Exception)
            {
                test(false);
            }

            try
            {
                thrower.throwCasC(1, 2, 3);
                test(false);
            }
            catch (C ex)
            {
                test(ex.aMem == 1);
                test(ex.bMem == 2);
                test(ex.cMem == 3);
            }
            catch (System.Exception)
            {
                test(false);
            }

            output.WriteLine("ok");

            output.Write("catching base types... ");
            output.Flush();

            try
            {
                thrower.throwBasB(1, 2);
                test(false);
            }
            catch (A ex)
            {
                test(ex.aMem == 1);
            }
            catch (System.Exception)
            {
                test(false);
            }

            try
            {
                thrower.throwCasC(1, 2, 3);
                test(false);
            }
            catch (B ex)
            {
                test(ex.aMem == 1);
                test(ex.bMem == 2);
            }
            catch (System.Exception)
            {
                test(false);
            }

            output.WriteLine("ok");

            output.Write("catching derived types... ");
            output.Flush();

            try
            {
                thrower.throwBasA(1, 2);
                test(false);
            }
            catch (B ex)
            {
                test(ex.aMem == 1);
                test(ex.bMem == 2);
            }
            catch (System.Exception)
            {
                test(false);
            }

            try
            {
                thrower.throwCasA(1, 2, 3);
                test(false);
            }
            catch (C ex)
            {
                test(ex.aMem == 1);
                test(ex.bMem == 2);
                test(ex.cMem == 3);
            }
            catch (System.Exception)
            {
                test(false);
            }

            try
            {
                thrower.throwCasB(1, 2, 3);
                test(false);
            }
            catch (C ex)
            {
                test(ex.aMem == 1);
                test(ex.bMem == 2);
                test(ex.cMem == 3);
            }
            catch (System.Exception)
            {
                test(false);
            }

            output.WriteLine("ok");

            output.Write("catching remote exception... ");
            output.Flush();

            try
            {
                thrower.throwUndeclaredA(1);
                test(false);
            }
            catch (A)
            {
            }
            catch (System.Exception)
            {
                test(false);
            }

            try
            {
                thrower.throwUndeclaredB(1, 2);
                test(false);
            }
            catch (B)
            {
            }
            catch (System.Exception)
            {
                test(false);
            }

            try
            {
                thrower.throwUndeclaredC(1, 2, 3);
                test(false);
            }
            catch (C)
            {
            }
            catch (System.Exception)
            {
                test(false);
            }

            output.WriteLine("ok");

            if (thrower.GetConnection() != null)
            {
                output.Write("testing memory limit marshal exception...");
                output.Flush();
                try
                {
                    thrower.throwMemoryLimitException(Array.Empty<byte>());
                    test(false);
                }
                catch (MemoryLimitException)
                {
                }
                catch (System.Exception)
                {
                    test(false);
                }

                try
                {
                    thrower.throwMemoryLimitException(new byte[20 * 1024]); // 20KB
                    test(false);
                }
                catch (ConnectionLostException)
                {
                }
                catch (UnhandledException)
                {
                    // Expected with JS bidir server
                }
                catch (System.Exception)
                {
                    test(false);
                }

                try
                {
                    var thrower2 = IThrowerPrx.Parse("thrower:" + helper.getTestEndpoint(1), communicator);
                    try
                    {
                        thrower2.throwMemoryLimitException(new byte[2 * 1024 * 1024]); // 2MB(no limits)
                    }
                    catch (MemoryLimitException)
                    {
                    }
                    var thrower3 = IThrowerPrx.Parse("thrower:" + helper.getTestEndpoint(2), communicator);
                    try
                    {
                        thrower3.throwMemoryLimitException(new byte[1024]); // 1KB limit
                        test(false);
                    }
                    catch (ConnectionLostException)
                    {
                    }
                }
                catch (ConnectionRefusedException)
                {
                    // Expected with JS bidir server
                }

                output.WriteLine("ok");
            }

            output.Write("catching object not exist exception... ");
            output.Flush();

            {
                Identity id = Identity.Parse("does not exist");
                try
                {
                    var thrower2 = IThrowerPrx.UncheckedCast(thrower.Clone(id));
                    thrower2.IcePing();
                    test(false);
                }
                catch (ObjectNotExistException ex)
                {
                    test(ex.Id.Equals(id));
                    test(ex.Message.Contains("servant")); // verify we don't get system message
                }
                catch (System.Exception)
                {
                    test(false);
                }
            }

            output.WriteLine("ok");

            output.Write("catching object not exist exception... ");
            output.Flush();

            try
            {
                var thrower2 = IThrowerPrx.UncheckedCast(thrower.Clone(facet: "no such facet"));
                try
                {
                    thrower2.IcePing();
                    test(false);
                }
                catch (ObjectNotExistException ex)
                {
                    test(ex.Facet.Equals("no such facet"));
                    test(ex.Message.Contains("with facet")); // verify we don't get system message
                }
            }
            catch (System.Exception)
            {
                test(false);
            }

            output.WriteLine("ok");

            output.Write("catching operation not exist exception... ");
            output.Flush();

            try
            {
                var thrower2 = Test.IWrongOperationPrx.UncheckedCast(thrower);
                thrower2.noSuchOperation();
                test(false);
            }
            catch (OperationNotExistException ex)
            {
                test(ex.Operation.Equals("noSuchOperation"));
                test(ex.Message.Contains("could not find operation")); // verify we don't get system message
            }
            catch (System.Exception)
            {
                test(false);
            }

            output.WriteLine("ok");

            output.Write("catching unhandled local exception... ");
            output.Flush();

            try
            {
                thrower.throwLocalException();
                test(false);
            }
            catch (UnhandledException ex)
            {
                 test(ex.Message.Contains("unhandled exception")); // verify we get custom message
            }
            catch (System.Exception)
            {
                test(false);
            }
            try
            {
                thrower.throwLocalExceptionIdempotent();
                test(false);
            }
            catch (UnhandledException)
            {
            }
            catch (System.Exception)
            {
                test(false);
            }

            output.WriteLine("ok");

            output.Write("catching unhandled non-Ice exception... ");
            output.Flush();

            try
            {
                thrower.throwNonIceException();
                test(false);
            }
            catch (UnhandledException ex)
            {
            }
            catch (System.Exception)
            {
                test(false);
            }

            output.WriteLine("ok");

            output.Write("catching unhandled remote exception... ");
            output.Flush();
            try
            {
                thrower.throwAConvertedToUnhandled();
                test(false);
            }
            catch (UnhandledException)
            {
            }
            catch (System.Exception)
            {
                test(false);
            }
            output.WriteLine("ok");

            output.Write("testing asynchronous exceptions... ");
            output.Flush();

            try
            {
                thrower.throwAfterResponse();
            }
            catch (System.Exception)
            {
                test(false);
            }

            try
            {
                thrower.throwAfterException();
                test(false);
            }
            catch (A)
            {
            }
            catch (System.Exception)
            {
                test(false);
            }

            output.WriteLine("ok");

            output.Write("catching exact types with AMI... ");
            output.Flush();

            {
                try
                {
                    thrower.throwAasAAsync(1).Wait();
                }
                catch (AggregateException exc)
                {
                    test(exc.InnerException is A);
                    var ex = exc.InnerException as Test.A;
                    test(ex.aMem == 1);
                }
            }

            {
                try
                {
                    thrower.throwAorDasAorDAsync(1).Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (A ex)
                    {
                        test(ex.aMem == 1);
                    }
                    catch (D ex)
                    {
                        test(ex.dMem == -1);
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }

            {
                try
                {
                    thrower.throwAorDasAorDAsync(-1).Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (A ex)
                    {
                        test(ex.aMem == 1);
                    }
                    catch (D ex)
                    {
                        test(ex.dMem == -1);
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }

            {
                try
                {
                    thrower.throwBasBAsync(1, 2).Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (B ex)
                    {
                        test(ex.aMem == 1);
                        test(ex.bMem == 2);
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }

            {
                try
                {
                    thrower.throwCasCAsync(1, 2, 3).Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (C ex)
                    {
                        test(ex.aMem == 1);
                        test(ex.bMem == 2);
                        test(ex.cMem == 3);
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }

            output.WriteLine("ok");

            output.Write("catching derived types with AMI... ");
            output.Flush();

            {
                try
                {
                    thrower.throwBasAAsync(1, 2).Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (B ex)
                    {
                        test(ex.aMem == 1);
                        test(ex.bMem == 2);
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }

            {
                try
                {
                    thrower.throwCasAAsync(1, 2, 3).Wait();
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (C ex)
                    {
                        test(ex.aMem == 1);
                        test(ex.bMem == 2);
                        test(ex.cMem == 3);
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }

            {
                try
                {
                    thrower.throwCasBAsync(1, 2, 3).Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (C ex)
                    {
                        test(ex.aMem == 1);
                        test(ex.bMem == 2);
                        test(ex.cMem == 3);
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }

            output.WriteLine("ok");

            output.Write("catching remote exception with AMI... ");
            output.Flush();

            {
                try
                {
                    thrower.throwUndeclaredAAsync(1).Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (A)
                    {
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }

            {
                try
                {
                    thrower.throwUndeclaredBAsync(1, 2).Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (B)
                    {
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }

            {
                try
                {
                    thrower.throwUndeclaredCAsync(1, 2, 3).Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (C)
                    {
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }

            output.WriteLine("ok");

            output.Write("catching object not exist exception with AMI... ");
            output.Flush();

            {
                Identity id = Identity.Parse("does not exist");
                var thrower2 = IThrowerPrx.UncheckedCast(thrower.Clone(id));
                try
                {
                    thrower2.throwAasAAsync(1).Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (ObjectNotExistException ex)
                    {
                        test(ex.Id.Equals(id));
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }

            output.WriteLine("ok");

            output.Write("catching object not exist exception with AMI... ");
            output.Flush();

            {
                var thrower2 = IThrowerPrx.UncheckedCast(thrower.Clone(facet: "no such facet"));
                try
                {
                    thrower2.throwAasAAsync(1).Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (ObjectNotExistException ex)
                    {
                        test(ex.Facet.Equals("no such facet"));
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }

            output.WriteLine("ok");

            output.Write("catching operation not exist exception with AMI... ");
            output.Flush();

            {
                try
                {
                    var thrower4 = IWrongOperationPrx.UncheckedCast(thrower);
                    thrower4.noSuchOperationAsync().Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (OperationNotExistException ex)
                    {
                        test(ex.Operation.Equals("noSuchOperation"));
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }

            output.WriteLine("ok");

            output.Write("catching unhandled local exception with AMI... ");
            output.Flush();

            {
                try
                {
                    thrower.throwLocalExceptionAsync().Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (UnhandledException)
                    {
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }

            {
                try
                {
                    thrower.throwLocalExceptionIdempotentAsync().Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (UnhandledException)
                    {
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }

            output.WriteLine("ok");

            output.Write("catching unhandled non-Ice exception with AMI... ");
            output.Flush();
            {
                try
                {
                    thrower.throwNonIceExceptionAsync().Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (UnhandledException)
                    {
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }
            output.WriteLine("ok");

            output.Write("catching remote exception with AMI... ");
            output.Flush();

            {
                try
                {
                    thrower.throwUndeclaredAAsync(1).Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (A)
                    {
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }

            {
                try
                {
                    thrower.throwUndeclaredBAsync(1, 2).Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (B)
                    {
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }

            {
                try
                {
                    thrower.throwUndeclaredCAsync(1, 2, 3).Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (C)
                    {
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }

            output.WriteLine("ok");

            output.Write("catching object not exist exception with AMI... ");
            output.Flush();

            {
                Identity id = Identity.Parse("does not exist");
                var thrower2 = IThrowerPrx.UncheckedCast(thrower.Clone(id));
                try
                {
                    thrower2.throwAasAAsync(1).Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (ObjectNotExistException ex)
                    {
                        test(ex.Id.Equals(id));
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }

            output.WriteLine("ok");

            output.Write("catching object not exist exception with AMI... ");
            output.Flush();

            {
                var thrower2 = IThrowerPrx.UncheckedCast(thrower.Clone(facet: "no such facet"));
                try
                {
                    thrower2.throwAasAAsync(1).Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (ObjectNotExistException ex)
                    {
                        test(ex.Facet.Equals("no such facet"));
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }

            output.WriteLine("ok");

            output.Write("catching operation not exist exception with AMI... ");
            output.Flush();

            {
                var thrower4 = IWrongOperationPrx.UncheckedCast(thrower);
                try
                {
                    thrower4.noSuchOperationAsync().Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (OperationNotExistException ex)
                    {
                        test(ex.Operation.Equals("noSuchOperation"));
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }

            output.WriteLine("ok");

            output.Write("catching unhandled local exception with AMI... ");
            output.Flush();

            {
                try
                {
                    thrower.throwLocalExceptionAsync().Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (UnhandledException)
                    {
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }

            {
                try
                {
                    thrower.throwLocalExceptionIdempotentAsync().Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (UnhandledException)
                    {
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }

            output.WriteLine("ok");

            output.Write("catching unhandled non-Ice exception with AMI... ");
            output.Flush();
            {
                try
                {
                    thrower.throwNonIceExceptionAsync().Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (UnhandledException)
                    {
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }
            output.WriteLine("ok");

            output.Write("catching unhandled remote exception with AMI... ");
            output.Flush();
            {
                try
                {
                    thrower.throwAConvertedToUnhandledAsync().Wait();
                    test(false);
                }
                catch (AggregateException exc)
                {
                    try
                    {
                        throw exc.InnerException;
                    }
                    catch (UnhandledException)
                    {
                    }
                    catch (System.Exception)
                    {
                        test(false);
                    }
                }
            }
            output.WriteLine("ok");

            return thrower;
        }
    }
}
