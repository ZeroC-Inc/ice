//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Collections.Generic;
using System.Threading;

namespace Ice.operations
{
    class Twoways
    {
        private static void test(bool b)
        {
            if (!b)
            {
                throw new System.Exception();
            }
        }

        class PerThreadContextInvokeThread
        {
            public PerThreadContextInvokeThread(Test.IMyClassPrx proxy)
            {
                _proxy = proxy;
            }

            public void Join()
            {
                _thread.Join();
            }

            public void Start()
            {
                _thread = new Thread(new ThreadStart(Run));
                _thread.Start();
            }

            public void Run()
            {
                Dictionary<string, string> ctx = _proxy.Communicator.getImplicitContext().getContext();
                test(ctx.Count == 0);
                ctx["one"] = "ONE";
                _proxy.Communicator.getImplicitContext().setContext(ctx);
                test(Ice.Collections.Equals(_proxy.opContext(), ctx));
            }

            private Test.IMyClassPrx _proxy;
            private Thread _thread;
        }

        internal static void twoways(global::Test.TestHelper helper, Test.IMyClassPrx p)
        {
            Ice.Communicator communicator = helper.communicator();
            string[] literals = p.opStringLiterals();

            test(Test.s0.value.Equals("\\") &&
                    Test.s0.value.Equals(Test.sw0.value) &&
                    Test.s0.value.Equals(literals[0]) &&
                    Test.s0.value.Equals(literals[11]));

            test(Test.s1.value.Equals("A") &&
                    Test.s1.value.Equals(Test.sw1.value) &&
                    Test.s1.value.Equals(literals[1]) &&
                    Test.s1.value.Equals(literals[12]));

            test(Test.s2.value.Equals("Ice") &&
                    Test.s2.value.Equals(Test.sw2.value) &&
                    Test.s2.value.Equals(literals[2]) &&
                    Test.s2.value.Equals(literals[13]));

            test(Test.s3.value.Equals("A21") &&
                    Test.s3.value.Equals(Test.sw3.value) &&
                    Test.s3.value.Equals(literals[3]) &&
                    Test.s3.value.Equals(literals[14]));

            test(Test.s4.value.Equals("\\u0041 \\U00000041") &&
                    Test.s4.value.Equals(Test.sw4.value) &&
                    Test.s4.value.Equals(literals[4]) &&
                    Test.s4.value.Equals(literals[15]));

            test(Test.s5.value.Equals("\u00FF") &&
                    Test.s5.value.Equals(Test.sw5.value) &&
                    Test.s5.value.Equals(literals[5]) &&
                    Test.s5.value.Equals(literals[16]));

            test(Test.s6.value.Equals("\u03FF") &&
                    Test.s6.value.Equals(Test.sw6.value) &&
                    Test.s6.value.Equals(literals[6]) &&
                    Test.s6.value.Equals(literals[17]));

            test(Test.s7.value.Equals("\u05F0") &&
                    Test.s7.value.Equals(Test.sw7.value) &&
                    Test.s7.value.Equals(literals[7]) &&
                    Test.s7.value.Equals(literals[18]));

            test(Test.s8.value.Equals("\U00010000") &&
                    Test.s8.value.Equals(Test.sw8.value) &&
                    Test.s8.value.Equals(literals[8]) &&
                    Test.s8.value.Equals(literals[19]));

            test(Test.s9.value.Equals("\U0001F34C") &&
                    Test.s9.value.Equals(Test.sw9.value) &&
                    Test.s9.value.Equals(literals[9]) &&
                    Test.s9.value.Equals(literals[20]));

            test(Test.s10.value.Equals("\u0DA7") &&
                    Test.s10.value.Equals(Test.sw10.value) &&
                    Test.s10.value.Equals(literals[10]) &&
                    Test.s10.value.Equals(literals[21]));

            test(Test.ss0.value.Equals("\'\"\u003f\\\a\b\f\n\r\t\v\u0006") &&
                    Test.ss0.value.Equals(Test.ss1.value) &&
                    Test.ss0.value.Equals(Test.ss2.value) &&
                    Test.ss0.value.Equals(literals[22]) &&
                    Test.ss0.value.Equals(literals[23]) &&
                    Test.ss0.value.Equals(literals[24]));

            test(Test.ss3.value.Equals("\\\\U\\u\\") &&
                    Test.ss3.value.Equals(literals[25]));

            test(Test.ss4.value.Equals("\\A\\") &&
                Test.ss4.value.Equals(literals[26]));

            test(Test.ss5.value.Equals("\\u0041\\") &&
                    Test.ss5.value.Equals(literals[27]));

            test(Test.su0.value.Equals(Test.su1.value) &&
                    Test.su0.value.Equals(Test.su2.value) &&
                    Test.su0.value.Equals(literals[28]) &&
                    Test.su0.value.Equals(literals[29]) &&
                    Test.su0.value.Equals(literals[30]));

            p.IcePing();

            test(p.IceIsA("::Test::MyClass"));
            test(p.IceId().Equals("::Test::MyDerivedClass"));

            {
                string[] ids = p.IceIds();
                test(ids.Length == 3);
                test(ids[0].Equals("::Ice::Object"));
                test(ids[1].Equals("::Test::MyClass"));
                test(ids[2].Equals("::Test::MyDerivedClass"));
            }

            {
                p.opVoid();
            }

            {
                byte b;
                byte r;

                (r, b) = p.opByte(0xff, 0x0f);
                test(b == 0xf0);
                test(r == 0xff);
            }

            {
                bool b;
                bool r;

                (r, b) = p.opBool(true, false);
                test(b);
                test(!r);
            }

            {
                short s;
                int i;
                long l;
                long r;

                (r, s, i, l) = p.opShortIntLong(10, 11, 12L);
                test(s == 10);
                test(i == 11);
                test(l == 12);
                test(r == 12L);

                (r, s, i, l) = p.opShortIntLong(short.MinValue, int.MinValue, long.MinValue);
                test(s == short.MinValue);
                test(i == int.MinValue);
                test(l == long.MinValue);
                test(r == long.MinValue);

                (r, s, i, l) = p.opShortIntLong(short.MaxValue, int.MaxValue, long.MaxValue);
                test(s == short.MaxValue);
                test(i == int.MaxValue);
                test(l == long.MaxValue);
                test(r == long.MaxValue);
            }

            {
                float f;
                double d;
                double r;

                (r, f, d) = p.opFloatDouble(3.14f, 1.1e10);
                test(f == 3.14f);
                test(d == 1.1e10);
                test(r == 1.1e10);

                (r, f, d) = p.opFloatDouble(float.Epsilon, double.MinValue);
                test(f == float.Epsilon);
                test(d == double.MinValue);
                test(r == double.MinValue);

                (r, f, d) = p.opFloatDouble(float.MaxValue, double.MaxValue);
                test(f == float.MaxValue);
                test(d == double.MaxValue);
                test(r == double.MaxValue);
            }

            {
                string s;
                string r;

                (r, s) = p.opString("hello", "world");
                test(s.Equals("world hello"));
                test(r.Equals("hello world"));
            }

            {
                Test.MyEnum e;
                Test.MyEnum r;

                (r, e) = p.opMyEnum(Test.MyEnum.enum2);
                test(e == Test.MyEnum.enum2);
                test(r == Test.MyEnum.enum3);
            }

            {
                Test.IMyClassPrx c1;
                Test.IMyClassPrx c2;
                Test.IMyClassPrx r;

                (r, c1, c2) = p.opMyClass(p);
                ProxyIdentityFacetComparer comparer;
                test(comparer.Compare(c1, p) == 0);
                test(comparer.Compare(c2, p) != 0);
                test(comparer.Compare(r, p) == 0);
                test(c1.Identity.Equals(Identity.Parse("test")));
                test(c2.Identity.Equals(Identity.Parse("noSuchIdentity")));
                test(r.Identity.Equals(Identity.Parse("test")));
                r.opVoid();
                c1.opVoid();
                try
                {
                    c2.opVoid();
                    test(false);
                }
                catch (ObjectNotExistException)
                {
                }

                (r, c1, c2) = p.opMyClass(null);
                test(c1 == null);
                test(c2 != null);
                test(comparer.Compare(r, p) == 0);
                r.opVoid();
            }

            {
                Test.Structure si1 = new Test.Structure();
                si1.p = p;
                si1.e = Test.MyEnum.enum3;
                si1.s = new Test.AnotherStruct();
                si1.s.s = "abc";
                Test.Structure si2 = new Test.Structure();
                si2.p = null;
                si2.e = Test.MyEnum.enum2;
                si2.s = new Test.AnotherStruct();
                si2.s.s = "def";

                var (rso, so) = p.opStruct(si1, si2);
                test(rso.p == null);
                test(rso.e == Test.MyEnum.enum2);
                test(rso.s.s.Equals("def"));
                test(so.p.Equals(p));
                test(so.e == Test.MyEnum.enum3);
                test(so.s.s.Equals("a new string"));
                so.p.opVoid();

                //
                // Test marshalling of null structs and structs with null members.
                //
                si1 = new Test.Structure();
                si2 = new Test.Structure();

                (rso, so) = p.opStruct(si1, si2);
                test(rso.p == null);
                test(rso.e == Test.MyEnum.enum1);
                test(rso.s.s.Equals(""));
                test(so.p == null);
                test(so.e == Test.MyEnum.enum1);
                test(so.s.s.Equals("a new string"));
            }

            {
                byte[] bsi1 = new byte[] { 0x01, 0x11, 0x12, 0x22 };
                byte[] bsi2 = new byte[] { 0xf1, 0xf2, 0xf3, 0xf4 };

                byte[] bso;
                byte[] rso;

                (rso, bso) = p.opByteS(bsi1, bsi2);
                test(bso.Length == 4);
                test(bso[0] == 0x22);
                test(bso[1] == 0x12);
                test(bso[2] == 0x11);
                test(bso[3] == 0x01);
                test(rso.Length == 8);
                test(rso[0] == 0x01);
                test(rso[1] == 0x11);
                test(rso[2] == 0x12);
                test(rso[3] == 0x22);
                test(rso[4] == 0xf1);
                test(rso[5] == 0xf2);
                test(rso[6] == 0xf3);
                test(rso[7] == 0xf4);
            }

            {
                bool[] bsi1 = new bool[] { true, true, false };
                bool[] bsi2 = new bool[] { false };

                bool[] bso;
                bool[] rso;

                (rso, bso) = p.opBoolS(bsi1, bsi2);
                test(bso.Length == 4);
                test(bso[0]);
                test(bso[1]);
                test(!bso[2]);
                test(!bso[3]);
                test(rso.Length == 3);
                test(!rso[0]);
                test(rso[1]);
                test(rso[2]);
            }

            {
                short[] ssi = new short[] { 1, 2, 3 };
                int[] isi = new int[] { 5, 6, 7, 8 };
                long[] lsi = new long[] { 10, 30, 20 };

                short[] sso;
                int[] iso;
                long[] lso;
                long[] rso;

                (rso, sso, iso, lso) = p.opShortIntLongS(ssi, isi, lsi);
                test(sso.Length == 3);
                test(sso[0] == 1);
                test(sso[1] == 2);
                test(sso[2] == 3);
                test(iso.Length == 4);
                test(iso[0] == 8);
                test(iso[1] == 7);
                test(iso[2] == 6);
                test(iso[3] == 5);
                test(lso.Length == 6);
                test(lso[0] == 10);
                test(lso[1] == 30);
                test(lso[2] == 20);
                test(lso[3] == 10);
                test(lso[4] == 30);
                test(lso[5] == 20);
                test(rso.Length == 3);
                test(rso[0] == 10);
                test(rso[1] == 30);
                test(rso[2] == 20);
            }

            {
                float[] fsi = new float[] { 3.14f, 1.11f };
                double[] dsi = new double[] { 1.1e10, 1.2e10, 1.3e10 };

                float[] fso;
                double[] dso;
                double[] rso;

                (rso, fso, dso) = p.opFloatDoubleS(fsi, dsi);
                test(fso.Length == 2);
                test(fso[0] == 3.14f);
                test(fso[1] == 1.11f);
                test(dso.Length == 3);
                test(dso[0] == 1.3e10);
                test(dso[1] == 1.2e10);
                test(dso[2] == 1.1e10);
                test(rso.Length == 5);
                test(rso[0] == 1.1e10);
                test(rso[1] == 1.2e10);
                test(rso[2] == 1.3e10);
                test((float)rso[3] == 3.14f);
                test((float)rso[4] == 1.11f);
            }

            {
                string[] ssi1 = new string[] { "abc", "de", "fghi" };
                string[] ssi2 = new string[] { "xyz" };

                string[] sso;
                string[] rso;

                (rso, sso) = p.opStringS(ssi1, ssi2);
                test(sso.Length == 4);
                test(sso[0].Equals("abc"));
                test(sso[1].Equals("de"));
                test(sso[2].Equals("fghi"));
                test(sso[3].Equals("xyz"));
                test(rso.Length == 3);
                test(rso[0].Equals("fghi"));
                test(rso[1].Equals("de"));
                test(rso[2].Equals("abc"));
            }

            {
                byte[] s11 = new byte[] { 0x01, 0x11, 0x12 };
                byte[] s12 = new byte[] { 0xff };
                byte[][] bsi1 = new byte[][] { s11, s12 };

                byte[] s21 = new byte[] { 0x0e };
                byte[] s22 = new byte[] { 0xf2, 0xf1 };
                byte[][] bsi2 = new byte[][] { s21, s22 };

                byte[][] bso;
                byte[][] rso;

                (rso, bso) = p.opByteSS(bsi1, bsi2);
                test(bso.Length == 2);
                test(bso[0].Length == 1);
                test(bso[0][0] == 0xff);
                test(bso[1].Length == 3);
                test(bso[1][0] == 0x01);
                test(bso[1][1] == 0x11);
                test(bso[1][2] == 0x12);
                test(rso.Length == 4);
                test(rso[0].Length == 3);
                test(rso[0][0] == 0x01);
                test(rso[0][1] == 0x11);
                test(rso[0][2] == 0x12);
                test(rso[1].Length == 1);
                test(rso[1][0] == 0xff);
                test(rso[2].Length == 1);
                test(rso[2][0] == 0x0e);
                test(rso[3].Length == 2);
                test(rso[3][0] == 0xf2);
                test(rso[3][1] == 0xf1);
            }

            {
                bool[] s11 = new bool[] { true };
                bool[] s12 = new bool[] { false };
                bool[] s13 = new bool[] { true, true };
                bool[][] bsi1 = new bool[][] { s11, s12, s13 };

                bool[] s21 = new bool[] { false, false, true };
                bool[][] bsi2 = new bool[][] { s21 };

                bool[][] rso;
                bool[][] bso;

                (rso, bso) = p.opBoolSS(bsi1, bsi2);
                test(bso.Length == 4);
                test(bso[0].Length == 1);
                test(bso[0][0]);
                test(bso[1].Length == 1);
                test(!bso[1][0]);
                test(bso[2].Length == 2);
                test(bso[2][0]);
                test(bso[2][1]);
                test(bso[3].Length == 3);
                test(!bso[3][0]);
                test(!bso[3][1]);
                test(bso[3][2]);
                test(rso.Length == 3);
                test(rso[0].Length == 2);
                test(rso[0][0]);
                test(rso[0][1]);
                test(rso[1].Length == 1);
                test(!rso[1][0]);
                test(rso[2].Length == 1);
                test(rso[2][0]);
            }

            {
                short[] s11 = new short[] { 1, 2, 5 };
                short[] s12 = new short[] { 13 };
                short[] s13 = new short[] { };
                short[][] ssi = new short[][] { s11, s12, s13 };

                int[] i11 = new int[] { 24, 98 };
                int[] i12 = new int[] { 42 };
                int[][] isi = new int[][] { i11, i12 };

                long[] l11 = new long[] { 496, 1729 };
                long[][] lsi = new long[][] { l11 };

                short[][] sso;
                int[][] iso;
                long[][] lso;
                long[][] rso;

                (rso, sso, iso, lso) = p.opShortIntLongSS(ssi, isi, lsi);
                test(rso.Length == 1);
                test(rso[0].Length == 2);
                test(rso[0][0] == 496);
                test(rso[0][1] == 1729);
                test(sso.Length == 3);
                test(sso[0].Length == 3);
                test(sso[0][0] == 1);
                test(sso[0][1] == 2);
                test(sso[0][2] == 5);
                test(sso[1].Length == 1);
                test(sso[1][0] == 13);
                test(sso[2].Length == 0);
                test(iso.Length == 2);
                test(iso[0].Length == 1);
                test(iso[0][0] == 42);
                test(iso[1].Length == 2);
                test(iso[1][0] == 24);
                test(iso[1][1] == 98);
                test(lso.Length == 2);
                test(lso[0].Length == 2);
                test(lso[0][0] == 496);
                test(lso[0][1] == 1729);
                test(lso[1].Length == 2);
                test(lso[1][0] == 496);
                test(lso[1][1] == 1729);
            }

            {
                float[] f11 = new float[] { 3.14f };
                float[] f12 = new float[] { 1.11f };
                float[] f13 = new float[] { };
                float[][] fsi = new float[][] { f11, f12, f13 };

                double[] d11 = new double[] { 1.1e10, 1.2e10, 1.3e10 };
                double[][] dsi = new double[][] { d11 };

                float[][] fso;
                double[][] dso;
                double[][] rso;

                (rso, fso, dso) = p.opFloatDoubleSS(fsi, dsi);
                test(fso.Length == 3);
                test(fso[0].Length == 1);
                test(fso[0][0] == 3.14f);
                test(fso[1].Length == 1);
                test(fso[1][0] == 1.11f);
                test(fso[2].Length == 0);
                test(dso.Length == 1);
                test(dso[0].Length == 3);
                test(dso[0][0] == 1.1e10);
                test(dso[0][1] == 1.2e10);
                test(dso[0][2] == 1.3e10);
                test(rso.Length == 2);
                test(rso[0].Length == 3);
                test(rso[0][0] == 1.1e10);
                test(rso[0][1] == 1.2e10);
                test(rso[0][2] == 1.3e10);
                test(rso[1].Length == 3);
                test(rso[1][0] == 1.1e10);
                test(rso[1][1] == 1.2e10);
                test(rso[1][2] == 1.3e10);
            }

            {
                string[] s11 = new string[] { "abc" };
                string[] s12 = new string[] { "de", "fghi" };
                string[][] ssi1 = new string[][] { s11, s12 };

                string[] s21 = new string[] { };
                string[] s22 = new string[] { };
                string[] s23 = new string[] { "xyz" };
                string[][] ssi2 = new string[][] { s21, s22, s23 };

                string[][] sso;
                string[][] rso;

                (rso, sso) = p.opStringSS(ssi1, ssi2);
                test(sso.Length == 5);
                test(sso[0].Length == 1);
                test(sso[0][0].Equals("abc"));
                test(sso[1].Length == 2);
                test(sso[1][0].Equals("de"));
                test(sso[1][1].Equals("fghi"));
                test(sso[2].Length == 0);
                test(sso[3].Length == 0);
                test(sso[4].Length == 1);
                test(sso[4][0].Equals("xyz"));
                test(rso.Length == 3);
                test(rso[0].Length == 1);
                test(rso[0][0].Equals("xyz"));
                test(rso[1].Length == 0);
                test(rso[2].Length == 0);
            }

            {
                string[] s111 = new string[] { "abc", "de" };
                string[] s112 = new string[] { "xyz" };
                string[][] ss11 = new string[][] { s111, s112 };
                string[] s121 = new string[] { "hello" };
                string[][] ss12 = new string[][] { s121 };
                string[][][] sssi1 = new string[][][] { ss11, ss12 };

                string[] s211 = new string[] { "", "" };
                string[] s212 = new string[] { "abcd" };
                string[][] ss21 = new string[][] { s211, s212 };
                string[] s221 = new string[] { "" };
                string[][] ss22 = new string[][] { s221 };
                string[][] ss23 = new string[][] { };
                string[][][] sssi2 = new string[][][] { ss21, ss22, ss23 };

                string[][][] ssso;
                string[][][] rsso;

                (rsso, ssso) = p.opStringSSS(sssi1, sssi2);
                test(ssso.Length == 5);
                test(ssso[0].Length == 2);
                test(ssso[0][0].Length == 2);
                test(ssso[0][1].Length == 1);
                test(ssso[1].Length == 1);
                test(ssso[1][0].Length == 1);
                test(ssso[2].Length == 2);
                test(ssso[2][0].Length == 2);
                test(ssso[2][1].Length == 1);
                test(ssso[3].Length == 1);
                test(ssso[3][0].Length == 1);
                test(ssso[4].Length == 0);
                test(ssso[0][0][0].Equals("abc"));
                test(ssso[0][0][1].Equals("de"));
                test(ssso[0][1][0].Equals("xyz"));
                test(ssso[1][0][0].Equals("hello"));
                test(ssso[2][0][0].Equals(""));
                test(ssso[2][0][1].Equals(""));
                test(ssso[2][1][0].Equals("abcd"));
                test(ssso[3][0][0].Equals(""));

                test(rsso.Length == 3);
                test(rsso[0].Length == 0);
                test(rsso[1].Length == 1);
                test(rsso[1][0].Length == 1);
                test(rsso[2].Length == 2);
                test(rsso[2][0].Length == 2);
                test(rsso[2][1].Length == 1);
                test(rsso[1][0][0].Equals(""));
                test(rsso[2][0][0].Equals(""));
                test(rsso[2][0][1].Equals(""));
                test(rsso[2][1][0].Equals("abcd"));
            }

            {
                Dictionary<byte, bool> di1 = new Dictionary<byte, bool>();
                di1[10] = true;
                di1[100] = false;
                Dictionary<byte, bool> di2 = new Dictionary<byte, bool>();
                di2[10] = true;
                di2[11] = false;
                di2[101] = true;

                var (ro, _do) = p.opByteBoolD(di1, di2);

                test(Ice.Collections.Equals(_do, di1));
                test(ro.Count == 4);
                test(ro[10] == true);
                test(ro[11] == false);
                test(ro[100] == false);
                test(ro[101] == true);
            }

            {
                Dictionary<short, int> di1 = new Dictionary<short, int>();
                di1[110] = -1;
                di1[1100] = 123123;
                Dictionary<short, int> di2 = new Dictionary<short, int>();
                di2[110] = -1;
                di2[111] = -100;
                di2[1101] = 0;

                var (ro, _do) = p.opShortIntD(di1, di2);

                test(Ice.Collections.Equals(_do, di1));
                test(ro.Count == 4);
                test(ro[110] == -1);
                test(ro[111] == -100);
                test(ro[1100] == 123123);
                test(ro[1101] == 0);
            }

            {
                Dictionary<long, float> di1 = new Dictionary<long, float>();
                di1[999999110L] = -1.1f;
                di1[999999111L] = 123123.2f;
                Dictionary<long, float> di2 = new Dictionary<long, float>();
                di2[999999110L] = -1.1f;
                di2[999999120L] = -100.4f;
                di2[999999130L] = 0.5f;

                var (ro, _do) = p.opLongFloatD(di1, di2);

                test(Ice.Collections.Equals(_do, di1));
                test(ro.Count == 4);
                test(ro[999999110L] == -1.1f);
                test(ro[999999120L] == -100.4f);
                test(ro[999999111L] == 123123.2f);
                test(ro[999999130L] == 0.5f);
            }

            {
                Dictionary<string, string> di1 = new Dictionary<string, string>();
                di1["foo"] = "abc -1.1";
                di1["bar"] = "abc 123123.2";
                Dictionary<string, string> di2 = new Dictionary<string, string>();
                di2["foo"] = "abc -1.1";
                di2["FOO"] = "abc -100.4";
                di2["BAR"] = "abc 0.5";

                var (ro, _do) = p.opStringStringD(di1, di2);

                test(Ice.Collections.Equals(_do, di1));
                test(ro.Count == 4);
                test(ro["foo"].Equals("abc -1.1"));
                test(ro["FOO"].Equals("abc -100.4"));
                test(ro["bar"].Equals("abc 123123.2"));
                test(ro["BAR"].Equals("abc 0.5"));
            }

            {
                var di1 = new Dictionary<string, Test.MyEnum>();
                di1["abc"] = Test.MyEnum.enum1;
                di1[""] = Test.MyEnum.enum2;
                var di2 = new Dictionary<string, Test.MyEnum>();
                di2["abc"] = Test.MyEnum.enum1;
                di2["qwerty"] = Test.MyEnum.enum3;
                di2["Hello!!"] = Test.MyEnum.enum2;

                var (ro, _do) = p.opStringMyEnumD(di1, di2);

                test(Ice.Collections.Equals(_do, di1));
                test(ro.Count == 4);
                test(ro["abc"] == Test.MyEnum.enum1);
                test(ro["qwerty"] == Test.MyEnum.enum3);
                test(ro[""] == Test.MyEnum.enum2);
                test(ro["Hello!!"] == Test.MyEnum.enum2);
            }

            {
                var di1 = new Dictionary<Test.MyEnum, string>();
                di1[Test.MyEnum.enum1] = "abc";
                var di2 = new Dictionary<Test.MyEnum, string>();
                di2[Test.MyEnum.enum2] = "Hello!!";
                di2[Test.MyEnum.enum3] = "qwerty";

                var (ro, _do) = p.opMyEnumStringD(di1, di2);

                test(Collections.Equals(_do, di1));
                test(ro.Count == 3);
                test(ro[Test.MyEnum.enum1].Equals("abc"));
                test(ro[Test.MyEnum.enum2].Equals("Hello!!"));
                test(ro[Test.MyEnum.enum3].Equals("qwerty"));
            }

            {
                var s11 = new Test.MyStruct(1, 1);
                var s12 = new Test.MyStruct(1, 2);
                var di1 = new Dictionary<Test.MyStruct, Test.MyEnum>();
                di1[s11] = Test.MyEnum.enum1;
                di1[s12] = Test.MyEnum.enum2;

                var s22 = new Test.MyStruct(2, 2);
                var s23 = new Test.MyStruct(2, 3);
                var di2 = new Dictionary<Test.MyStruct, Test.MyEnum>();
                di2[s11] = Test.MyEnum.enum1;
                di2[s22] = Test.MyEnum.enum3;
                di2[s23] = Test.MyEnum.enum2;

                var (ro, _do) = p.opMyStructMyEnumD(di1, di2);

                test(Ice.Collections.Equals(_do, di1));
                test(ro.Count == 4);
                test(ro[s11] == Test.MyEnum.enum1);
                test(ro[s12] == Test.MyEnum.enum2);
                test(ro[s22] == Test.MyEnum.enum3);
                test(ro[s23] == Test.MyEnum.enum2);
            }

            {
                Dictionary<byte, bool>[] dsi1 = new Dictionary<byte, bool>[2];
                Dictionary<byte, bool>[] dsi2 = new Dictionary<byte, bool>[1];

                Dictionary<byte, bool> di1 = new Dictionary<byte, bool>();
                di1[10] = true;
                di1[100] = false;
                Dictionary<byte, bool> di2 = new Dictionary<byte, bool>();
                di2[10] = true;
                di2[11] = false;
                di2[101] = true;
                Dictionary<byte, bool> di3 = new Dictionary<byte, bool>();
                di3[100] = false;
                di3[101] = false;

                dsi1[0] = di1;
                dsi1[1] = di2;
                dsi2[0] = di3;

                var (ro, _do) = p.opByteBoolDS(dsi1, dsi2);

                test(ro.Length == 2);
                test(ro[0].Count == 3);
                test(ro[0][10]);
                test(!ro[0][11]);
                test(ro[0][101]);
                test(ro[1].Count == 2);
                test(ro[1][10]);
                test(!ro[1][100]);

                test(_do.Length == 3);
                test(_do[0].Count == 2);
                test(!_do[0][100]);
                test(!_do[0][101]);
                test(_do[1].Count == 2);
                test(_do[1][10]);
                test(!_do[1][100]);
                test(_do[2].Count == 3);
                test(_do[2][10]);
                test(!_do[2][11]);
                test(_do[2][101]);
            }

            {
                Dictionary<short, int>[] dsi1 = new Dictionary<short, int>[2];
                Dictionary<short, int>[] dsi2 = new Dictionary<short, int>[1];

                Dictionary<short, int> di1 = new Dictionary<short, int>();
                di1[110] = -1;
                di1[1100] = 123123;
                Dictionary<short, int> di2 = new Dictionary<short, int>();
                di2[110] = -1;
                di2[111] = -100;
                di2[1101] = 0;
                Dictionary<short, int> di3 = new Dictionary<short, int>();
                di3[100] = -1001;

                dsi1[0] = di1;
                dsi1[1] = di2;
                dsi2[0] = di3;

                var (ro, _do) = p.opShortIntDS(dsi1, dsi2);

                test(ro.Length == 2);
                test(ro[0].Count == 3);
                test(ro[0][110] == -1);
                test(ro[0][111] == -100);
                test(ro[0][1101] == 0);
                test(ro[1].Count == 2);
                test(ro[1][110] == -1);
                test(ro[1][1100] == 123123);

                test(_do.Length == 3);
                test(_do[0].Count == 1);
                test(_do[0][100] == -1001);
                test(_do[1].Count == 2);
                test(_do[1][110] == -1);
                test(_do[1][1100] == 123123);
                test(_do[2].Count == 3);
                test(_do[2][110] == -1);
                test(_do[2][111] == -100);
                test(_do[2][1101] == 0);
            }

            {
                Dictionary<long, float>[] dsi1 = new Dictionary<long, float>[2];
                Dictionary<long, float>[] dsi2 = new Dictionary<long, float>[1];

                Dictionary<long, float> di1 = new Dictionary<long, float>();
                di1[999999110L] = -1.1f;
                di1[999999111L] = 123123.2f;
                Dictionary<long, float> di2 = new Dictionary<long, float>();
                di2[999999110L] = -1.1f;
                di2[999999120L] = -100.4f;
                di2[999999130L] = 0.5f;
                Dictionary<long, float> di3 = new Dictionary<long, float>();
                di3[999999140L] = 3.14f;

                dsi1[0] = di1;
                dsi1[1] = di2;
                dsi2[0] = di3;

                var (ro, _do) = p.opLongFloatDS(dsi1, dsi2);

                test(ro.Length == 2);
                test(ro[0].Count == 3);
                test(ro[0][999999110L] == -1.1f);
                test(ro[0][999999120L] == -100.4f);
                test(ro[0][999999130L] == 0.5f);
                test(ro[1].Count == 2);
                test(ro[1][999999110L] == -1.1f);
                test(ro[1][999999111L] == 123123.2f);

                test(_do.Length == 3);
                test(_do[0].Count == 1);
                test(_do[0][999999140L] == 3.14f);
                test(_do[1].Count == 2);
                test(_do[1][999999110L] == -1.1f);
                test(_do[1][999999111L] == 123123.2f);
                test(_do[2].Count == 3);
                test(_do[2][999999110L] == -1.1f);
                test(_do[2][999999120L] == -100.4f);
                test(_do[2][999999130L] == 0.5f);

            }

            {
                Dictionary<string, string>[] dsi1 = new Dictionary<string, string>[2];
                Dictionary<string, string>[] dsi2 = new Dictionary<string, string>[1];

                Dictionary<string, string> di1 = new Dictionary<string, string>();
                di1["foo"] = "abc -1.1";
                di1["bar"] = "abc 123123.2";
                Dictionary<string, string> di2 = new Dictionary<string, string>();
                di2["foo"] = "abc -1.1";
                di2["FOO"] = "abc -100.4";
                di2["BAR"] = "abc 0.5";
                Dictionary<string, string> di3 = new Dictionary<string, string>();
                di3["f00"] = "ABC -3.14";

                dsi1[0] = di1;
                dsi1[1] = di2;
                dsi2[0] = di3;

                var (ro, _do) = p.opStringStringDS(dsi1, dsi2);

                test(ro.Length == 2);
                test(ro[0].Count == 3);
                test(ro[0]["foo"].Equals("abc -1.1"));
                test(ro[0]["FOO"].Equals("abc -100.4"));
                test(ro[0]["BAR"].Equals("abc 0.5"));
                test(ro[1].Count == 2);
                test(ro[1]["foo"] == "abc -1.1");
                test(ro[1]["bar"] == "abc 123123.2");

                test(_do.Length == 3);
                test(_do[0].Count == 1);
                test(_do[0]["f00"].Equals("ABC -3.14"));
                test(_do[1].Count == 2);
                test(_do[1]["foo"].Equals("abc -1.1"));
                test(_do[1]["bar"].Equals("abc 123123.2"));
                test(_do[2].Count == 3);
                test(_do[2]["foo"].Equals("abc -1.1"));
                test(_do[2]["FOO"].Equals("abc -100.4"));
                test(_do[2]["BAR"].Equals("abc 0.5"));
            }

            {
                var dsi1 = new Dictionary<string, Test.MyEnum>[2];
                var dsi2 = new Dictionary<string, Test.MyEnum>[1];

                var di1 = new Dictionary<string, Test.MyEnum>();
                di1["abc"] = Test.MyEnum.enum1;
                di1[""] = Test.MyEnum.enum2;
                var di2 = new Dictionary<string, Test.MyEnum>();
                di2["abc"] = Test.MyEnum.enum1;
                di2["qwerty"] = Test.MyEnum.enum3;
                di2["Hello!!"] = Test.MyEnum.enum2;
                var di3 = new Dictionary<string, Test.MyEnum>();
                di3["Goodbye"] = Test.MyEnum.enum1;

                dsi1[0] = di1;
                dsi1[1] = di2;
                dsi2[0] = di3;

                var (ro, _do) = p.opStringMyEnumDS(dsi1, dsi2);

                test(ro.Length == 2);
                test(ro[0].Count == 3);
                test(ro[0]["abc"] == Test.MyEnum.enum1);
                test(ro[0]["qwerty"] == Test.MyEnum.enum3);
                test(ro[0]["Hello!!"] == Test.MyEnum.enum2);
                test(ro[1].Count == 2);
                test(ro[1]["abc"] == Test.MyEnum.enum1);
                test(ro[1][""] == Test.MyEnum.enum2);

                test(_do.Length == 3);
                test(_do[0].Count == 1);
                test(_do[0]["Goodbye"] == Test.MyEnum.enum1);
                test(_do[1].Count == 2);
                test(_do[1]["abc"] == Test.MyEnum.enum1);
                test(_do[1][""] == Test.MyEnum.enum2);
                test(_do[2].Count == 3);
                test(_do[2]["abc"] == Test.MyEnum.enum1);
                test(_do[2]["qwerty"] == Test.MyEnum.enum3);
                test(_do[2]["Hello!!"] == Test.MyEnum.enum2);
            }

            {
                var dsi1 = new Dictionary<Test.MyEnum, string>[2];
                var dsi2 = new Dictionary<Test.MyEnum, string>[1];

                var di1 = new Dictionary<Test.MyEnum, string>();
                di1[Test.MyEnum.enum1] = "abc";
                var di2 = new Dictionary<Test.MyEnum, string>();
                di2[Test.MyEnum.enum2] = "Hello!!";
                di2[Test.MyEnum.enum3] = "qwerty";
                var di3 = new Dictionary<Test.MyEnum, string>();
                di3[Test.MyEnum.enum1] = "Goodbye";

                dsi1[0] = di1;
                dsi1[1] = di2;
                dsi2[0] = di3;

                var (ro, _do) = p.opMyEnumStringDS(dsi1, dsi2);

                test(ro.Length == 2);
                test(ro[0].Count == 2);
                test(ro[0][Test.MyEnum.enum2].Equals("Hello!!"));
                test(ro[0][Test.MyEnum.enum3].Equals("qwerty"));
                test(ro[1].Count == 1);
                test(ro[1][Test.MyEnum.enum1].Equals("abc"));

                test(_do.Length == 3);
                test(_do[0].Count == 1);
                test(_do[0][Test.MyEnum.enum1].Equals("Goodbye"));
                test(_do[1].Count == 1);
                test(_do[1][Test.MyEnum.enum1].Equals("abc"));
                test(_do[2].Count == 2);
                test(_do[2][Test.MyEnum.enum2].Equals("Hello!!"));
                test(_do[2][Test.MyEnum.enum3].Equals("qwerty"));
            }

            {
                var dsi1 = new Dictionary<Test.MyStruct, Test.MyEnum>[2];
                var dsi2 = new Dictionary<Test.MyStruct, Test.MyEnum>[1];

                var s11 = new Test.MyStruct(1, 1);
                var s12 = new Test.MyStruct(1, 2);
                var di1 = new Dictionary<Test.MyStruct, Test.MyEnum>();
                di1[s11] = Test.MyEnum.enum1;
                di1[s12] = Test.MyEnum.enum2;

                var s22 = new Test.MyStruct(2, 2);
                var s23 = new Test.MyStruct(2, 3);
                var di2 = new Dictionary<Test.MyStruct, Test.MyEnum>();
                di2[s11] = Test.MyEnum.enum1;
                di2[s22] = Test.MyEnum.enum3;
                di2[s23] = Test.MyEnum.enum2;

                var di3 = new Dictionary<Test.MyStruct, Test.MyEnum>();
                di3[s23] = Test.MyEnum.enum3;

                dsi1[0] = di1;
                dsi1[1] = di2;
                dsi2[0] = di3;

                var (ro, _do) = p.opMyStructMyEnumDS(dsi1, dsi2);

                test(ro.Length == 2);
                test(ro[0].Count == 3);
                test(ro[0][s11] == Test.MyEnum.enum1);
                test(ro[0][s22] == Test.MyEnum.enum3);
                test(ro[0][s23] == Test.MyEnum.enum2);
                test(ro[1].Count == 2);
                test(ro[1][s11] == Test.MyEnum.enum1);
                test(ro[1][s12] == Test.MyEnum.enum2);

                test(_do.Length == 3);
                test(_do[0].Count == 1);
                test(_do[0][s23] == Test.MyEnum.enum3);
                test(_do[1].Count == 2);
                test(_do[1][s11] == Test.MyEnum.enum1);
                test(_do[1][s12] == Test.MyEnum.enum2);
                test(_do[2].Count == 3);
                test(_do[2][s11] == Test.MyEnum.enum1);
                test(_do[2][s22] == Test.MyEnum.enum3);
                test(_do[2][s23] == Test.MyEnum.enum2);
            }

            {
                var sdi1 = new Dictionary<byte, byte[]>();
                var sdi2 = new Dictionary<byte, byte[]>();

                byte[] si1 = new byte[] { 0x01, 0x11 };
                byte[] si2 = new byte[] { 0x12 };
                byte[] si3 = new byte[] { 0xf2, 0xf3 };

                sdi1[0x01] = si1;
                sdi1[0x22] = si2;
                sdi2[0xf1] = si3;

                var (ro, _do) = p.opByteByteSD(sdi1, sdi2);

                test(_do.Count == 1);
                test(_do[0xf1].Length == 2);
                test(_do[0xf1][0] == 0xf2);
                test(_do[0xf1][1] == 0xf3);

                test(ro.Count == 3);
                test(ro[0x01].Length == 2);
                test(ro[0x01][0] == 0x01);
                test(ro[0x01][1] == 0x11);
                test(ro[0x22].Length == 1);
                test(ro[0x22][0] == 0x12);
                test(ro[0xf1].Length == 2);
                test(ro[0xf1][0] == 0xf2);
                test(ro[0xf1][1] == 0xf3);
            }

            {
                var sdi1 = new Dictionary<bool, bool[]>();
                var sdi2 = new Dictionary<bool, bool[]>();

                bool[] si1 = new bool[] { true, false };
                bool[] si2 = new bool[] { false, true, true };

                sdi1[false] = si1;
                sdi1[true] = si2;
                sdi2[false] = si1;

                var (ro, _do) = p.opBoolBoolSD(sdi1, sdi2);

                test(_do.Count == 1);
                test(_do[false].Length == 2);
                test(_do[false][0]);
                test(!_do[false][1]);
                test(ro.Count == 2);
                test(ro[false].Length == 2);
                test(ro[false][0]);
                test(!ro[false][1]);
                test(ro[true].Length == 3);
                test(!ro[true][0]);
                test(ro[true][1]);
                test(ro[true][2]);
            }

            {
                var sdi1 = new Dictionary<short, short[]>();
                var sdi2 = new Dictionary<short, short[]>();

                short[] si1 = new short[] { 1, 2, 3 };
                short[] si2 = new short[] { 4, 5 };
                short[] si3 = new short[] { 6, 7 };

                sdi1[1] = si1;
                sdi1[2] = si2;
                sdi2[4] = si3;

                var (ro, _do) = p.opShortShortSD(sdi1, sdi2);

                test(_do.Count == 1);
                test(_do[4].Length == 2);
                test(_do[4][0] == 6);
                test(_do[4][1] == 7);

                test(ro.Count == 3);
                test(ro[1].Length == 3);
                test(ro[1][0] == 1);
                test(ro[1][1] == 2);
                test(ro[1][2] == 3);
                test(ro[2].Length == 2);
                test(ro[2][0] == 4);
                test(ro[2][1] == 5);
                test(ro[4].Length == 2);
                test(ro[4][0] == 6);
                test(ro[4][1] == 7);
            }

            {
                var sdi1 = new Dictionary<int, int[]>();
                var sdi2 = new Dictionary<int, int[]>();

                int[] si1 = new int[] { 100, 200, 300 };
                int[] si2 = new int[] { 400, 500 };
                int[] si3 = new int[] { 600, 700 };

                sdi1[100] = si1;
                sdi1[200] = si2;
                sdi2[400] = si3;

                var (ro, _do) = p.opIntIntSD(sdi1, sdi2);

                test(_do.Count == 1);
                test(_do[400].Length == 2);
                test(_do[400][0] == 600);
                test(_do[400][1] == 700);

                test(ro.Count == 3);
                test(ro[100].Length == 3);
                test(ro[100][0] == 100);
                test(ro[100][1] == 200);
                test(ro[100][2] == 300);
                test(ro[200].Length == 2);
                test(ro[200][0] == 400);
                test(ro[200][1] == 500);
                test(ro[400].Length == 2);
                test(ro[400][0] == 600);
                test(ro[400][1] == 700);
            }

            {
                var sdi1 = new Dictionary<long, long[]>();
                var sdi2 = new Dictionary<long, long[]>();

                var si1 = new long[] { 999999110L, 999999111L, 999999110L };
                var si2 = new long[] { 999999120L, 999999130L };
                long[] si3 = new long[] { 999999110L, 999999120L };

                sdi1[999999990L] = si1;
                sdi1[999999991L] = si2;
                sdi2[999999992L] = si3;

                var (ro, _do) = p.opLongLongSD(sdi1, sdi2);

                test(_do.Count == 1);
                test(_do[999999992L].Length == 2);
                test(_do[999999992L][0] == 999999110L);
                test(_do[999999992L][1] == 999999120L);
                test(ro.Count == 3);
                test(ro[999999990L].Length == 3);
                test(ro[999999990L][0] == 999999110L);
                test(ro[999999990L][1] == 999999111L);
                test(ro[999999990L][2] == 999999110L);
                test(ro[999999991L].Length == 2);
                test(ro[999999991L][0] == 999999120L);
                test(ro[999999991L][1] == 999999130L);
                test(ro[999999992L].Length == 2);
                test(ro[999999992L][0] == 999999110L);
                test(ro[999999992L][1] == 999999120L);
            }

            {
                var sdi1 = new Dictionary<string, float[]>();
                var sdi2 = new Dictionary<string, float[]>();

                var si1 = new float[] { -1.1f, 123123.2f, 100.0f };
                var si2 = new float[] { 42.24f, -1.61f };
                var si3 = new float[] { -3.14f, 3.14f };

                sdi1["abc"] = si1;
                sdi1["ABC"] = si2;
                sdi2["aBc"] = si3;

                var (ro, _do) = p.opStringFloatSD(sdi1, sdi2);

                test(_do.Count == 1);
                test(_do["aBc"].Length == 2);
                test(_do["aBc"][0] == -3.14f);
                test(_do["aBc"][1] == 3.14f);

                test(ro.Count == 3);
                test(ro["abc"].Length == 3);
                test(ro["abc"][0] == -1.1f);
                test(ro["abc"][1] == 123123.2f);
                test(ro["abc"][2] == 100.0f);
                test(ro["ABC"].Length == 2);
                test(ro["ABC"][0] == 42.24f);
                test(ro["ABC"][1] == -1.61f);
                test(ro["aBc"].Length == 2);
                test(ro["aBc"][0] == -3.14f);
                test(ro["aBc"][1] == 3.14f);
            }

            {
                var sdi1 = new Dictionary<string, double[]>();
                var sdi2 = new Dictionary<string, double[]>();

                var si1 = new double[] { 1.1E10, 1.2E10, 1.3E10 };
                var si2 = new double[] { 1.4E10, 1.5E10 };
                var si3 = new double[] { 1.6E10, 1.7E10 };

                sdi1["Hello!!"] = si1;
                sdi1["Goodbye"] = si2;
                sdi2[""] = si3;

                var (ro, _do) = p.opStringDoubleSD(sdi1, sdi2);

                test(_do.Count == 1);
                test(_do[""].Length == 2);
                test(_do[""][0] == 1.6E10);
                test(_do[""][1] == 1.7E10);
                test(ro.Count == 3);
                test(ro["Hello!!"].Length == 3);
                test(ro["Hello!!"][0] == 1.1E10);
                test(ro["Hello!!"][1] == 1.2E10);
                test(ro["Hello!!"][2] == 1.3E10);
                test(ro["Goodbye"].Length == 2);
                test(ro["Goodbye"][0] == 1.4E10);
                test(ro["Goodbye"][1] == 1.5E10);
                test(ro[""].Length == 2);
                test(ro[""][0] == 1.6E10);
                test(ro[""][1] == 1.7E10);
            }

            {
                var sdi1 = new Dictionary<string, string[]>();
                var sdi2 = new Dictionary<string, string[]>();

                var si1 = new string[] { "abc", "de", "fghi" };
                var si2 = new string[] { "xyz", "or" };
                var si3 = new string[] { "and", "xor" };

                sdi1["abc"] = si1;
                sdi1["def"] = si2;
                sdi2["ghi"] = si3;

                var (ro, _do) = p.opStringStringSD(sdi1, sdi2);

                test(_do.Count == 1);
                test(_do["ghi"].Length == 2);
                test(_do["ghi"][0].Equals("and"));
                test(_do["ghi"][1].Equals("xor"));

                test(ro.Count == 3);
                test(ro["abc"].Length == 3);
                test(ro["abc"][0].Equals("abc"));
                test(ro["abc"][1].Equals("de"));
                test(ro["abc"][2].Equals("fghi"));
                test(ro["def"].Length == 2);
                test(ro["def"][0].Equals("xyz"));
                test(ro["def"][1].Equals("or"));
                test(ro["ghi"].Length == 2);
                test(ro["ghi"][0].Equals("and"));
                test(ro["ghi"][1].Equals("xor"));
            }

            {
                var sdi1 = new Dictionary<Test.MyEnum, Test.MyEnum[]>();
                var sdi2 = new Dictionary<Test.MyEnum, Test.MyEnum[]>();

                var si1 = new Test.MyEnum[] { Test.MyEnum.enum1, Test.MyEnum.enum1, Test.MyEnum.enum2 };
                var si2 = new Test.MyEnum[] { Test.MyEnum.enum1, Test.MyEnum.enum2 };
                var si3 = new Test.MyEnum[] { Test.MyEnum.enum3, Test.MyEnum.enum3 };

                sdi1[Test.MyEnum.enum3] = si1;
                sdi1[Test.MyEnum.enum2] = si2;
                sdi2[Test.MyEnum.enum1] = si3;

                var (ro, _do) = p.opMyEnumMyEnumSD(sdi1, sdi2);

                test(_do.Count == 1);
                test(_do[Test.MyEnum.enum1].Length == 2);
                test(_do[Test.MyEnum.enum1][0] == Test.MyEnum.enum3);
                test(_do[Test.MyEnum.enum1][1] == Test.MyEnum.enum3);
                test(ro.Count == 3);
                test(ro[Test.MyEnum.enum3].Length == 3);
                test(ro[Test.MyEnum.enum3][0] == Test.MyEnum.enum1);
                test(ro[Test.MyEnum.enum3][1] == Test.MyEnum.enum1);
                test(ro[Test.MyEnum.enum3][2] == Test.MyEnum.enum2);
                test(ro[Test.MyEnum.enum2].Length == 2);
                test(ro[Test.MyEnum.enum2][0] == Test.MyEnum.enum1);
                test(ro[Test.MyEnum.enum2][1] == Test.MyEnum.enum2);
                test(ro[Test.MyEnum.enum1].Length == 2);
                test(ro[Test.MyEnum.enum1][0] == Test.MyEnum.enum3);
                test(ro[Test.MyEnum.enum1][1] == Test.MyEnum.enum3);
            }

            {
                int[] lengths = new int[] { 0, 1, 2, 126, 127, 128, 129, 253, 254, 255, 256, 257, 1000 };

                for (int l = 0; l < lengths.Length; ++l)
                {
                    int[] s = new int[lengths[l]];
                    for (int i = 0; i < lengths[l]; ++i)
                    {
                        s[i] = i;
                    }

                    int[] r = p.opIntS(s);
                    test(r.Length == lengths[l]);
                    for (int j = 0; j < r.Length; ++j)
                    {
                        test(r[j] == -j);
                    }
                }
            }

            {
                Dictionary<string, string> ctx = new Dictionary<string, string>();
                ctx["one"] = "ONE";
                ctx["two"] = "TWO";
                ctx["three"] = "THREE";
                {
                    test(p.Context.Count == 0);
                    Dictionary<string, string> r = p.opContext();
                    test(!r.Equals(ctx));
                }
                {
                    Dictionary<string, string> r = p.opContext(ctx);
                    test(p.Context.Count == 0);
                    test(Collections.Equals(r, ctx));
                }
                {
                    var p2 = p.Clone(context: ctx);
                    test(Collections.Equals(p2.Context, ctx));
                    Dictionary<string, string> r = p2.opContext();
                    test(Collections.Equals(r, ctx));
                    r = p2.opContext(ctx);
                    test(Collections.Equals(r, ctx));
                }
            }

            if (p.GetConnection() != null)
            {
                //
                // Test implicit context propagation
                //

                string[] impls = { "Shared", "PerThread" };
                for (int i = 0; i < 2; i++)
                {
                    var properties = communicator.GetProperties();
                    properties["Ice.ImplicitContext"] = impls[i];

                    Communicator ic = helper.initialize(properties);

                    Dictionary<string, string> ctx = new Dictionary<string, string>();
                    ctx["one"] = "ONE";
                    ctx["two"] = "TWO";
                    ctx["three"] = "THREE";

                    var p3 = Test.IMyClassPrx.Parse($"test:{helper.getTestEndpoint(0)}", ic);

                    ic.getImplicitContext().setContext(ctx);
                    test(Collections.Equals(ic.getImplicitContext().getContext(), ctx));
                    test(Collections.Equals(p3.opContext(), ctx));

                    test(ic.getImplicitContext().containsKey("zero") == false);
                    string r = ic.getImplicitContext().put("zero", "ZERO");
                    test(r.Equals(""));
                    test(ic.getImplicitContext().get("zero").Equals("ZERO"));

                    ctx = ic.getImplicitContext().getContext();
                    test(Collections.Equals(p3.opContext(), ctx));

                    Dictionary<string, string> prxContext = new Dictionary<string, string>();
                    prxContext["one"] = "UN";
                    prxContext["four"] = "QUATRE";

                    Dictionary<string, string> combined = new Dictionary<string, string>(prxContext);
                    foreach (KeyValuePair<string, string> e in ctx)
                    {
                        try
                        {
                            combined.Add(e.Key, e.Value);
                        }
                        catch (ArgumentException)
                        {
                            // Ignore.
                        }
                    }
                    test(combined["one"].Equals("UN"));

                    p3 = p3.Clone(context: prxContext);

                    ic.getImplicitContext().setContext(null);
                    test(Collections.Equals(p3.opContext(), prxContext));

                    ic.getImplicitContext().setContext(ctx);
                    test(Collections.Equals(p3.opContext(), combined));

                    test(ic.getImplicitContext().remove("one").Equals("ONE"));

                    if (impls[i].Equals("PerThread"))
                    {
                        var thread = new PerThreadContextInvokeThread(p3.Clone(context: new Dictionary<string, string>()));
                        thread.Start();
                        thread.Join();
                    }
                    ic.destroy();
                }
            }

            {
                p.opIdempotent();
            }

            {
                p.opNonmutating();
            }

            {
                test(p.opByte1(0xFF) == 0xFF);
                test(p.opShort1(0x7FFF) == 0x7FFF);
                test(p.opInt1(0x7FFFFFFF) == 0x7FFFFFFF);
                test(p.opLong1(0x7FFFFFFFFFFFFFFF) == 0x7FFFFFFFFFFFFFFF);
                test(p.opFloat1(1.0f) == 1.0f);
                test(p.opDouble1(1.0d) == 1.0d);
                test(p.opString1("opString1").Equals("opString1"));
                test(p.opStringS1(null).Length == 0);
                test(p.opByteBoolD1(null).Count == 0);
                test(p.opStringS2(null).Length == 0);
                test(p.opByteBoolD2(null).Count == 0);

                var d = Test.IMyDerivedClassPrx.UncheckedCast(p);
                var s = new Test.MyStruct1();
                s.tesT = "MyStruct1.s";
                s.myClass = null;
                s.myStruct1 = "MyStruct1.myStruct1";
                s = d.opMyStruct1(s);
                test(s.tesT.Equals("MyStruct1.s"));
                test(s.myClass == null);
                test(s.myStruct1.Equals("MyStruct1.myStruct1"));
                var c = new Test.MyClass1();
                c.tesT = "MyClass1.testT";
                c.myClass = null;
                c.myClass1 = "MyClass1.myClass1";
                c = d.opMyClass1(c);
                test(c.tesT.Equals("MyClass1.testT"));
                test(c.myClass == null);
                test(c.myClass1.Equals("MyClass1.myClass1"));
            }

            {
                var p1 = p.opMStruct1();
                p1.e = Test.MyEnum.enum3;
                Test.Structure p2, p3;
                (p3, p2) = p.opMStruct2(p1);
                test(p2.Equals(p1) && p3.Equals(p1));
            }

            {
                p.opMSeq1();

                string[] p1 = new string[1];
                p1[0] = "test";
                string[] p2, p3;
                (p3, p2) = p.opMSeq2(p1);
                test(Collections.Equals(p2, p1) && Collections.Equals(p3, p1));
            }

            {
                p.opMDict1();

                Dictionary<string, string> p1 = new Dictionary<string, string>();
                p1["test"] = "test";
                Dictionary<string, string> p2, p3;
                (p3, p2) = p.opMDict2(p1);
                test(Collections.Equals(p2, p1) && Collections.Equals(p3, p1));
            }
        }
    }
}
