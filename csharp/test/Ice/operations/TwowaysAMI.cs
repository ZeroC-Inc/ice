//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace Ice
{
    namespace operations
    {
        public class TwowaysAMI
        {
            private static void test(bool b)
            {
                if (!b)
                {
                    throw new System.Exception();
                }
            }

            internal static async Task twowaysAMI(global::Test.TestHelper helper, Test.MyClassPrx p)
            {
                Communicator communicator = helper.communicator();

                {
                    await p.ice_pingAsync();
                }

                {
                    var result = await p.ice_isAAsync(Test.MyClassDisp_.ice_staticId());
                    test(result);
                }

                {
                    var result = await p.ice_idsAsync();
                    test(result.Length == 3);
                }

                {
                    var result = await p.ice_idAsync();
                    test(result.Equals(Test.MyDerivedClassDisp_.ice_staticId()));
                }

                {
                    await p.opVoidAsync();
                }

                {
                    var result = await p.opByteAsync(0xff, 0x0f);
                    test(result.p3 == 0xf0);
                    test(result.returnValue == 0xff);
                }

                {
                    var result = await p.opBoolAsync(true, false);
                    test(result.p3);
                    test(!result.returnValue);
                }

                {
                    var result = await p.opShortIntLongAsync(10, 11, 12);
                    test(result.p4 == 10);
                    test(result.p5 == 11);
                    test(result.p6 == 12);
                    test(result.returnValue == 12);
                }

                {
                    var result = await p.opFloatDoubleAsync(3.14f, 1.1E10);
                    test(result.p3 == 3.14f);
                    test(result.p4 == 1.1e10);
                    test(result.returnValue == 1.1e10);
                }

                {
                    var result = await p.opStringAsync("hello", "world");
                    test(result.p3.Equals("world hello"));
                    test(result.returnValue.Equals("hello world"));
                }

                {
                    var result = p.opMyEnumAsync(Test.MyEnum.enum2).Result;
                    test(result.p2 == Test.MyEnum.enum2);
                    test(result.returnValue == Test.MyEnum.enum3);
                }

                {
                    var ret = p.opMyClassAsync(p).Result;
                    test(ret.p2.ice_getIdentity().Equals(Util.stringToIdentity("test")));
                    test(ret.p3.ice_getIdentity().Equals(Util.stringToIdentity("noSuchIdentity")));
                    test(ret.returnValue.ice_getIdentity().Equals(Util.stringToIdentity("test")));

                    //
                    // We can't do the callbacks below in connection serialization mode.
                    //
                    if (communicator.getProperties().getPropertyAsInt("Ice.ThreadPool.Client.Serialize") == 0)
                    {
                        ret.returnValue.opVoid();
                        ret.p2.opVoid();
                        try
                        {
                            ret.p3.opVoid();
                            test(false);
                        }
                        catch (ObjectNotExistException)
                        {
                        }
                    }
                }

                {
                    var si1 = new Test.Structure();
                    si1.p = p;
                    si1.e = Test.MyEnum.enum3;
                    si1.s = new Test.AnotherStruct();
                    si1.s.s = "abc";
                    var si2 = new Test.Structure();
                    si2.p = null;
                    si2.e = Test.MyEnum.enum2;
                    si2.s = new Test.AnotherStruct();
                    si2.s.s = "def";

                    var ret = p.opStructAsync(si1, si2).Result;
                    test(ret.returnValue.p == null);
                    test(ret.returnValue.e == Test.MyEnum.enum2);
                    test(ret.returnValue.s.s.Equals("def"));
                    test(ret.p3.e == Test.MyEnum.enum3);
                    test(ret.p3.s.s.Equals("a new string"));

                    //
                    // We can't do the callbacks below in connection serialization mode.
                    //
                    if (communicator.getProperties().getPropertyAsInt("Ice.ThreadPool.Client.Serialize") == 0)
                    {
                        ret.p3.p.opVoid();
                    }
                }

                {
                    byte[] bsi1 = new byte[] { 0x01, 0x11, 0x12, 0x22 };
                    byte[] bsi2 = new byte[] { 0xf1, 0xf2, 0xf3, 0xf4 };

                    var ret = p.opByteSAsync(bsi1, bsi2).Result;
                    test(ret.p3.Length == 4);
                    test(ret.p3[0] == 0x22);
                    test(ret.p3[1] == 0x12);
                    test(ret.p3[2] == 0x11);
                    test(ret.p3[3] == 0x01);
                    test(ret.returnValue.Length == 8);
                    test(ret.returnValue[0] == 0x01);
                    test(ret.returnValue[1] == 0x11);
                    test(ret.returnValue[2] == 0x12);
                    test(ret.returnValue[3] == 0x22);
                    test(ret.returnValue[4] == 0xf1);
                    test(ret.returnValue[5] == 0xf2);
                    test(ret.returnValue[6] == 0xf3);
                    test(ret.returnValue[7] == 0xf4);
                }

                {
                    bool[] bsi1 = new bool[] { true, true, false };
                    bool[] bsi2 = new bool[] { false };

                    var result = p.opBoolSAsync(bsi1, bsi2).Result;
                    test(result.p3.Length == 4);
                    test(result.p3[0]);
                    test(result.p3[1]);
                    test(!result.p3[2]);
                    test(!result.p3[3]);
                    test(result.returnValue.Length == 3);
                    test(!result.returnValue[0]);
                    test(result.returnValue[1]);
                    test(result.returnValue[2]);
                }

                {
                    short[] ssi = new short[] { 1, 2, 3 };
                    int[] isi = new int[] { 5, 6, 7, 8 };
                    long[] lsi = new long[] { 10, 30, 20 };

                    var ret = p.opShortIntLongSAsync(ssi, isi, lsi).Result;
                    test(ret.p4.Length == 3);
                    test(ret.p4[0] == 1);
                    test(ret.p4[1] == 2);
                    test(ret.p4[2] == 3);
                    test(ret.p5.Length == 4);
                    test(ret.p5[0] == 8);
                    test(ret.p5[1] == 7);
                    test(ret.p5[2] == 6);
                    test(ret.p5[3] == 5);
                    test(ret.p6.Length == 6);
                    test(ret.p6[0] == 10);
                    test(ret.p6[1] == 30);
                    test(ret.p6[2] == 20);
                    test(ret.p6[3] == 10);
                    test(ret.p6[4] == 30);
                    test(ret.p6[5] == 20);
                    test(ret.returnValue.Length == 3);
                    test(ret.returnValue[0] == 10);
                    test(ret.returnValue[1] == 30);
                    test(ret.returnValue[2] == 20);
                }

                {
                    float[] fsi = new float[] { 3.14f, 1.11f };
                    double[] dsi = new double[] { 1.1e10, 1.2e10, 1.3e10 };

                    var result = p.opFloatDoubleSAsync(fsi, dsi).Result;
                    test(result.p3.Length == 2);
                    test(result.p3[0] == 3.14f);
                    test(result.p3[1] == 1.11f);
                    test(result.p4.Length == 3);
                    test(result.p4[0] == 1.3e10);
                    test(result.p4[1] == 1.2e10);
                    test(result.p4[2] == 1.1e10);
                    test(result.returnValue.Length == 5);
                    test(result.returnValue[0] == 1.1e10);
                    test(result.returnValue[1] == 1.2e10);
                    test(result.returnValue[2] == 1.3e10);
                    test((float)result.returnValue[3] == 3.14f);
                    test((float)result.returnValue[4] == 1.11f);
                }

                {
                    string[] ssi1 = new string[] { "abc", "de", "fghi" };
                    string[] ssi2 = new string[] { "xyz" };

                    var result = await p.opStringSAsync(ssi1, ssi2);
                    test(result.p3.Length == 4);
                    test(result.p3[0].Equals("abc"));
                    test(result.p3[1].Equals("de"));
                    test(result.p3[2].Equals("fghi"));
                    test(result.p3[3].Equals("xyz"));
                    test(result.returnValue.Length == 3);
                    test(result.returnValue[0].Equals("fghi"));
                    test(result.returnValue[1].Equals("de"));
                    test(result.returnValue[2].Equals("abc"));
                }

                {
                    byte[] s11 = new byte[] { 0x01, 0x11, 0x12 };
                    byte[] s12 = new byte[] { 0xff };
                    byte[][] bsi1 = new byte[][] { s11, s12 };

                    byte[] s21 = new byte[] { 0x0e };
                    byte[] s22 = new byte[] { 0xf2, 0xf1 };
                    byte[][] bsi2 = new byte[][] { s21, s22 };

                    var ret = p.opByteSSAsync(bsi1, bsi2).Result;
                    test(ret.p3.Length == 2);
                    test(ret.p3[0].Length == 1);
                    test(ret.p3[0][0] == 0xff);
                    test(ret.p3[1].Length == 3);
                    test(ret.p3[1][0] == 0x01);
                    test(ret.p3[1][1] == 0x11);
                    test(ret.p3[1][2] == 0x12);
                    test(ret.returnValue.Length == 4);
                    test(ret.returnValue[0].Length == 3);
                    test(ret.returnValue[0][0] == 0x01);
                    test(ret.returnValue[0][1] == 0x11);
                    test(ret.returnValue[0][2] == 0x12);
                    test(ret.returnValue[1].Length == 1);
                    test(ret.returnValue[1][0] == 0xff);
                    test(ret.returnValue[2].Length == 1);
                    test(ret.returnValue[2][0] == 0x0e);
                    test(ret.returnValue[3].Length == 2);
                    test(ret.returnValue[3][0] == 0xf2);
                    test(ret.returnValue[3][1] == 0xf1);
                }

                {
                    bool[] s11 = new bool[] { true };
                    bool[] s12 = new bool[] { false };
                    bool[] s13 = new bool[] { true, true };
                    bool[][] bsi1 = new bool[][] { s11, s12, s13 };

                    bool[] s21 = new bool[] { false, false, true };
                    bool[][] bsi2 = new bool[][] { s21 };

                    var ret = p.opBoolSSAsync(bsi1, bsi2).Result;
                    test(ret.p3.Length == 4);
                    test(ret.p3[0].Length == 1);
                    test(ret.p3[0][0]);
                    test(ret.p3[1].Length == 1);
                    test(!ret.p3[1][0]);
                    test(ret.p3[2].Length == 2);
                    test(ret.p3[2][0]);
                    test(ret.p3[2][1]);
                    test(ret.p3[3].Length == 3);
                    test(!ret.p3[3][0]);
                    test(!ret.p3[3][1]);
                    test(ret.p3[3][2]);
                    test(ret.returnValue.Length == 3);
                    test(ret.returnValue[0].Length == 2);
                    test(ret.returnValue[0][0]);
                    test(ret.returnValue[0][1]);
                    test(ret.returnValue[1].Length == 1);
                    test(!ret.returnValue[1][0]);
                    test(ret.returnValue[2].Length == 1);
                    test(ret.returnValue[2][0]);
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

                    var result = await p.opShortIntLongSSAsync(ssi, isi, lsi);
                    test(result.returnValue.Length == 1);
                    test(result.returnValue[0].Length == 2);
                    test(result.returnValue[0][0] == 496);
                    test(result.returnValue[0][1] == 1729);
                    test(result.p4.Length == 3);
                    test(result.p4[0].Length == 3);
                    test(result.p4[0][0] == 1);
                    test(result.p4[0][1] == 2);
                    test(result.p4[0][2] == 5);
                    test(result.p4[1].Length == 1);
                    test(result.p4[1][0] == 13);
                    test(result.p4[2].Length == 0);
                    test(result.p5.Length == 2);
                    test(result.p5[0].Length == 1);
                    test(result.p5[0][0] == 42);
                    test(result.p5[1].Length == 2);
                    test(result.p5[1][0] == 24);
                    test(result.p5[1][1] == 98);
                    test(result.p6.Length == 2);
                    test(result.p6[0].Length == 2);
                    test(result.p6[0][0] == 496);
                    test(result.p6[0][1] == 1729);
                    test(result.p6[1].Length == 2);
                    test(result.p6[1][0] == 496);
                    test(result.p6[1][1] == 1729);
                }

                {
                    float[] f11 = new float[] { 3.14f };
                    float[] f12 = new float[] { 1.11f };
                    float[] f13 = new float[] { };
                    float[][] fsi = new float[][] { f11, f12, f13 };

                    double[] d11 = new double[] { 1.1e10, 1.2e10, 1.3e10 };
                    double[][] dsi = new double[][] { d11 };

                    var result = await p.opFloatDoubleSSAsync(fsi, dsi);
                    test(result.p3.Length == 3);
                    test(result.p3[0].Length == 1);
                    test(result.p3[0][0] == 3.14f);
                    test(result.p3[1].Length == 1);
                    test(result.p3[1][0] == 1.11f);
                    test(result.p3[2].Length == 0);
                    test(result.p4.Length == 1);
                    test(result.p4[0].Length == 3);
                    test(result.p4[0][0] == 1.1e10);
                    test(result.p4[0][1] == 1.2e10);
                    test(result.p4[0][2] == 1.3e10);
                    test(result.returnValue.Length == 2);
                    test(result.returnValue[0].Length == 3);
                    test(result.returnValue[0][0] == 1.1e10);
                    test(result.returnValue[0][1] == 1.2e10);
                    test(result.returnValue[0][2] == 1.3e10);
                    test(result.returnValue[1].Length == 3);
                    test(result.returnValue[1][0] == 1.1e10);
                    test(result.returnValue[1][1] == 1.2e10);
                    test(result.returnValue[1][2] == 1.3e10);
                }

                {
                    string[] s11 = new string[] { "abc" };
                    string[] s12 = new string[] { "de", "fghi" };
                    string[][] ssi1 = new string[][] { s11, s12 };

                    string[] s21 = new string[] { };
                    string[] s22 = new string[] { };
                    string[] s23 = new string[] { "xyz" };
                    string[][] ssi2 = new string[][] { s21, s22, s23 };

                    var result = await p.opStringSSAsync(ssi1, ssi2);
                    test(result.p3.Length == 5);
                    test(result.p3[0].Length == 1);
                    test(result.p3[0][0].Equals("abc"));
                    test(result.p3[1].Length == 2);
                    test(result.p3[1][0].Equals("de"));
                    test(result.p3[1][1].Equals("fghi"));
                    test(result.p3[2].Length == 0);
                    test(result.p3[3].Length == 0);
                    test(result.p3[4].Length == 1);
                    test(result.p3[4][0].Equals("xyz"));
                    test(result.returnValue.Length == 3);
                    test(result.returnValue[0].Length == 1);
                    test(result.returnValue[0][0].Equals("xyz"));
                    test(result.returnValue[1].Length == 0);
                    test(result.returnValue[2].Length == 0);
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

                    var result = p.opStringSSSAsync(sssi1, sssi2).Result;
                    test(result.p3.Length == 5);
                    test(result.p3[0].Length == 2);
                    test(result.p3[0][0].Length == 2);
                    test(result.p3[0][1].Length == 1);
                    test(result.p3[1].Length == 1);
                    test(result.p3[1][0].Length == 1);
                    test(result.p3[2].Length == 2);
                    test(result.p3[2][0].Length == 2);
                    test(result.p3[2][1].Length == 1);
                    test(result.p3[3].Length == 1);
                    test(result.p3[3][0].Length == 1);
                    test(result.p3[4].Length == 0);
                    test(result.p3[0][0][0].Equals("abc"));
                    test(result.p3[0][0][1].Equals("de"));
                    test(result.p3[0][1][0].Equals("xyz"));
                    test(result.p3[1][0][0].Equals("hello"));
                    test(result.p3[2][0][0].Equals(""));
                    test(result.p3[2][0][1].Equals(""));
                    test(result.p3[2][1][0].Equals("abcd"));
                    test(result.p3[3][0][0].Equals(""));

                    test(result.returnValue.Length == 3);
                    test(result.returnValue[0].Length == 0);
                    test(result.returnValue[1].Length == 1);
                    test(result.returnValue[1][0].Length == 1);
                    test(result.returnValue[2].Length == 2);
                    test(result.returnValue[2][0].Length == 2);
                    test(result.returnValue[2][1].Length == 1);
                    test(result.returnValue[1][0][0].Equals(""));
                    test(result.returnValue[2][0][0].Equals(""));
                    test(result.returnValue[2][0][1].Equals(""));
                    test(result.returnValue[2][1][0].Equals("abcd"));
                }

                {
                    var di1 = new Dictionary<byte, bool>
                    {
                        [10] = true,
                        [100] = false
                    };
                    var di2 = new Dictionary<byte, bool>
                    {
                        [10] = true,
                        [11] = false,
                        [100] = false,
                        [101] = true
                    };

                    var result = p.opByteBoolDAsync(di1, di2).Result;

                    test(CollectionComparer.Equals(result.p3, di1));
                    test(CollectionComparer.Equals(result.returnValue, di2));
                }

                {
                    var di1 = new Dictionary<short, int>
                    {
                        [110] = -1,
                        [1100] = 123123
                    };
                    var di2 = new Dictionary<short, int>
                    {
                        [110] = -1,
                        [111] = -100,
                        [1100] = 123123,
                        [1101] = 0
                    };

                    var result = await p.opShortIntDAsync(di1, di2);

                    test(CollectionComparer.Equals(result.p3, di1));
                    test(CollectionComparer.Equals(result.returnValue, di2));
                }

                {
                    var di1 = new Dictionary<long, float>
                    {
                        [999999110L] = -1.1f,
                        [999999111L] = 123123.2f
                    };
                    var di2 = new Dictionary<long, float>
                    {
                        [999999110L] = -1.1f,
                        [999999120L] = -100.4f,
                        [999999111L] = 123123.2f,
                        [999999130L] = 0.5f
                    };

                    var result = p.opLongFloatDAsync(di1, di2).Result;

                    test(CollectionComparer.Equals(result.p3, di1));
                    test(CollectionComparer.Equals(result.returnValue, di2));
                }

                {
                    var di1 = new Dictionary<string, string>
                    {
                        ["foo"] = "abc -1.1",
                        ["bar"] = "abc 123123.2"
                    };
                    var di2 = new Dictionary<string, string>
                    {
                        ["foo"] = "abc -1.1",
                        ["FOO"] = "abc -100.4",
                        ["bar"] = "abc 123123.2",
                        ["BAR"] = "abc 0.5"
                    };

                    var result = p.opStringStringDAsync(di1, di2).Result;

                    test(CollectionComparer.Equals(result.p3, di1));
                    test(CollectionComparer.Equals(result.returnValue, di2));
                }

                {
                    var di1 = new Dictionary<string, Test.MyEnum>();
                    di1["abc"] = Test.MyEnum.enum1;
                    di1[""] = Test.MyEnum.enum2;
                    var di2 = new Dictionary<string, Test.MyEnum>();
                    di2["abc"] = Test.MyEnum.enum1;
                    di2["qwerty"] = Test.MyEnum.enum3;
                    di2[""] = Test.MyEnum.enum2;
                    di2["Hello!!"] = Test.MyEnum.enum2;

                    var result = p.opStringMyEnumDAsync(di1, di2).Result;

                    test(CollectionComparer.Equals(result.p3, di1));
                    test(CollectionComparer.Equals(result.returnValue, di2));
                }

                {
                    var di1 = new Dictionary<Test.MyEnum, string>();
                    di1[Test.MyEnum.enum1] = "abc";
                    var di2 = new Dictionary<Test.MyEnum, string>();
                    di2[Test.MyEnum.enum1] = "abc";
                    di2[Test.MyEnum.enum2] = "Hello!!";
                    di2[Test.MyEnum.enum3] = "qwerty";

                    var result = await p.opMyEnumStringDAsync(di1, di2);

                    test(CollectionComparer.Equals(result.p3, di1));
                    test(CollectionComparer.Equals(result.returnValue, di2));
                }

                {
                    var s11 = new Test.MyStruct(1, 1);
                    var s12 = new Test.MyStruct(1, 2);
                    var di1 = new Dictionary<Test.MyStruct, Test.MyEnum>
                    {
                        [s11] = Test.MyEnum.enum1,
                        [s12] = Test.MyEnum.enum2
                    };

                    var s22 = new Test.MyStruct(2, 2);
                    var s23 = new Test.MyStruct(2, 3);
                    var di2 = new Dictionary<Test.MyStruct, Test.MyEnum>
                    {
                        [s11] = Test.MyEnum.enum1,
                        [s22] = Test.MyEnum.enum3,
                        [s23] = Test.MyEnum.enum2
                    };

                    var result = await p.opMyStructMyEnumDAsync(di1, di2);
                    di2[s12] = Test.MyEnum.enum2;

                    test(CollectionComparer.Equals(result.p3, di1));
                    test(CollectionComparer.Equals(result.returnValue, di2));
                }

                {
                    Dictionary<byte, bool>[] dsi1 = new Dictionary<byte, bool>[2];
                    Dictionary<byte, bool>[] dsi2 = new Dictionary<byte, bool>[1];

                    var di1 = new Dictionary<byte, bool>
                    {
                        [10] = true,
                        [100] = false
                    };

                    var di2 = new Dictionary<byte, bool>
                    {
                        [10] = true,
                        [11] = false,
                        [101] = true
                    };

                    var di3 = new Dictionary<byte, bool>
                    {
                        [100] = false,
                        [101] = false
                    };

                    dsi1[0] = di1;
                    dsi1[1] = di2;
                    dsi2[0] = di3;

                    var result = await p.opByteBoolDSAsync(dsi1, dsi2);

                    test(result.returnValue.Length == 2);
                    test(result.returnValue[0].Count == 3);
                    test(result.returnValue[0][10]);
                    test(!result.returnValue[0][11]);
                    test(result.returnValue[0][101]);
                    test(result.returnValue[1].Count == 2);
                    test(result.returnValue[1][10]);
                    test(!result.returnValue[1][100]);

                    test(result.p3.Length == 3);
                    test(result.p3[0].Count == 2);
                    test(!result.p3[0][100]);
                    test(!result.p3[0][101]);
                    test(result.p3[1].Count == 2);
                    test(result.p3[1][10]);
                    test(!result.p3[1][100]);
                    test(result.p3[2].Count == 3);
                    test(result.p3[2][10]);
                    test(!result.p3[2][11]);
                    test(result.p3[2][101]);
                }

                {
                    Dictionary<short, int>[] dsi1 = new Dictionary<short, int>[2];
                    Dictionary<short, int>[] dsi2 = new Dictionary<short, int>[1];

                    var di1 = new Dictionary<short, int>
                    {
                        [110] = -1,
                        [1100] = 123123
                    };
                    var di2 = new Dictionary<short, int>
                    {
                        [110] = -1,
                        [111] = -100,
                        [1101] = 0
                    };
                    var di3 = new Dictionary<short, int>
                    {
                        [100] = -1001
                    };

                    dsi1[0] = di1;
                    dsi1[1] = di2;
                    dsi2[0] = di3;

                    var result = p.opShortIntDSAsync(dsi1, dsi2).Result;

                    test(result.returnValue.Length == 2);
                    test(result.returnValue[0].Count == 3);
                    test(result.returnValue[0][110] == -1);
                    test(result.returnValue[0][111] == -100);
                    test(result.returnValue[0][1101] == 0);
                    test(result.returnValue[1].Count == 2);
                    test(result.returnValue[1][110] == -1);
                    test(result.returnValue[1][1100] == 123123);

                    test(result.p3.Length == 3);
                    test(result.p3[0].Count == 1);
                    test(result.p3[0][100] == -1001);
                    test(result.p3[1].Count == 2);
                    test(result.p3[1][110] == -1);
                    test(result.p3[1][1100] == 123123);
                    test(result.p3[2].Count == 3);
                    test(result.p3[2][110] == -1);
                    test(result.p3[2][111] == -100);
                    test(result.p3[2][1101] == 0);
                }

                {
                    Dictionary<long, float>[] dsi1 = new Dictionary<long, float>[2];
                    Dictionary<long, float>[] dsi2 = new Dictionary<long, float>[1];

                    var di1 = new Dictionary<long, float>
                    {
                        [999999110L] = -1.1f,
                        [999999111L] = 123123.2f
                    };
                    var di2 = new Dictionary<long, float>
                    {
                        [999999110L] = -1.1f,
                        [999999120L] = -100.4f,
                        [999999130L] = 0.5f
                    };
                    var di3 = new Dictionary<long, float>
                    {
                        [999999140L] = 3.14f
                    };

                    dsi1[0] = di1;
                    dsi1[1] = di2;
                    dsi2[0] = di3;

                    var result = await p.opLongFloatDSAsync(dsi1, dsi2);

                    test(result.returnValue.Length == 2);
                    test(result.returnValue[0].Count == 3);
                    test(result.returnValue[0][999999110L] == -1.1f);
                    test(result.returnValue[0][999999120L] == -100.4f);
                    test(result.returnValue[0][999999130L] == 0.5f);
                    test(result.returnValue[1].Count == 2);
                    test(result.returnValue[1][999999110L] == -1.1f);
                    test(result.returnValue[1][999999111L] == 123123.2f);

                    test(result.p3.Length == 3);
                    test(result.p3[0].Count == 1);
                    test(result.p3[0][999999140L] == 3.14f);
                    test(result.p3[1].Count == 2);
                    test(result.p3[1][999999110L] == -1.1f);
                    test(result.p3[1][999999111L] == 123123.2f);
                    test(result.p3[2].Count == 3);
                    test(result.p3[2][999999110L] == -1.1f);
                    test(result.p3[2][999999120L] == -100.4f);
                    test(result.p3[2][999999130L] == 0.5f);
                }

                {
                    Dictionary<string, string>[] dsi1 = new Dictionary<string, string>[2];
                    Dictionary<string, string>[] dsi2 = new Dictionary<string, string>[1];

                    var di1 = new Dictionary<string, string>
                    {
                        ["foo"] = "abc -1.1",
                        ["bar"] = "abc 123123.2"
                    };
                    var di2 = new Dictionary<string, string>
                    {
                        ["foo"] = "abc -1.1",
                        ["FOO"] = "abc -100.4",
                        ["BAR"] = "abc 0.5"
                    };
                    var di3 = new Dictionary<string, string>
                    {
                        ["f00"] = "ABC -3.14"
                    };

                    dsi1[0] = di1;
                    dsi1[1] = di2;
                    dsi2[0] = di3;

                    var result = p.opStringStringDSAsync(dsi1, dsi2).Result;

                    test(result.returnValue.Length == 2);
                    test(result.returnValue[0].Count == 3);
                    test(result.returnValue[0]["foo"].Equals("abc -1.1"));
                    test(result.returnValue[0]["FOO"].Equals("abc -100.4"));
                    test(result.returnValue[0]["BAR"].Equals("abc 0.5"));
                    test(result.returnValue[1].Count == 2);
                    test(result.returnValue[1]["foo"] == "abc -1.1");
                    test(result.returnValue[1]["bar"] == "abc 123123.2");

                    test(result.p3.Length == 3);
                    test(result.p3[0].Count == 1);
                    test(result.p3[0]["f00"].Equals("ABC -3.14"));
                    test(result.p3[1].Count == 2);
                    test(result.p3[1]["foo"].Equals("abc -1.1"));
                    test(result.p3[1]["bar"].Equals("abc 123123.2"));
                    test(result.p3[2].Count == 3);
                    test(result.p3[2]["foo"].Equals("abc -1.1"));
                    test(result.p3[2]["FOO"].Equals("abc -100.4"));
                    test(result.p3[2]["BAR"].Equals("abc 0.5"));
                }

                {
                    var dsi1 = new Dictionary<string, Test.MyEnum>[2];
                    var dsi2 = new Dictionary<string, Test.MyEnum>[1];

                    var di1 = new Dictionary<string, Test.MyEnum>
                    {
                        ["abc"] = Test.MyEnum.enum1,
                        [""] = Test.MyEnum.enum2
                    };
                    var di2 = new Dictionary<string, Test.MyEnum>
                    {
                        ["abc"] = Test.MyEnum.enum1,
                        ["qwerty"] = Test.MyEnum.enum3,
                        ["Hello!!"] = Test.MyEnum.enum2
                    };
                    var di3 = new Dictionary<string, Test.MyEnum>
                    {
                        ["Goodbye"] = Test.MyEnum.enum1
                    };

                    dsi1[0] = di1;
                    dsi1[1] = di2;
                    dsi2[0] = di3;

                    var result = await p.opStringMyEnumDSAsync(dsi1, dsi2);

                    test(result.returnValue.Length == 2);
                    test(result.returnValue[0].Count == 3);
                    test(result.returnValue[0]["abc"] == Test.MyEnum.enum1);
                    test(result.returnValue[0]["qwerty"] == Test.MyEnum.enum3);
                    test(result.returnValue[0]["Hello!!"] == Test.MyEnum.enum2);
                    test(result.returnValue[1].Count == 2);
                    test(result.returnValue[1]["abc"] == Test.MyEnum.enum1);
                    test(result.returnValue[1][""] == Test.MyEnum.enum2);

                    test(result.p3.Length == 3);
                    test(result.p3[0].Count == 1);
                    test(result.p3[0]["Goodbye"] == Test.MyEnum.enum1);
                    test(result.p3[1].Count == 2);
                    test(result.p3[1]["abc"] == Test.MyEnum.enum1);
                    test(result.p3[1][""] == Test.MyEnum.enum2);
                    test(result.p3[2].Count == 3);
                    test(result.p3[2]["abc"] == Test.MyEnum.enum1);
                    test(result.p3[2]["qwerty"] == Test.MyEnum.enum3);
                    test(result.p3[2]["Hello!!"] == Test.MyEnum.enum2);
                }

                {
                    var dsi1 = new Dictionary<Test.MyEnum, string>[2];
                    var dsi2 = new Dictionary<Test.MyEnum, string>[1];

                    var di1 = new Dictionary<Test.MyEnum, string>
                    {
                        [Test.MyEnum.enum1] = "abc"
                    };
                    var di2 = new Dictionary<Test.MyEnum, string>
                    {
                        [Test.MyEnum.enum2] = "Hello!!",
                        [Test.MyEnum.enum3] = "qwerty"
                    };
                    var di3 = new Dictionary<Test.MyEnum, string>
                    {
                        [Test.MyEnum.enum1] = "Goodbye"
                    };

                    dsi1[0] = di1;
                    dsi1[1] = di2;
                    dsi2[0] = di3;

                    var result = await p.opMyEnumStringDSAsync(dsi1, dsi2);

                    test(result.returnValue.Length == 2);
                    test(result.returnValue[0].Count == 2);
                    test(result.returnValue[0][Test.MyEnum.enum2].Equals("Hello!!"));
                    test(result.returnValue[0][Test.MyEnum.enum3].Equals("qwerty"));
                    test(result.returnValue[1].Count == 1);
                    test(result.returnValue[1][Test.MyEnum.enum1].Equals("abc"));

                    test(result.p3.Length == 3);
                    test(result.p3[0].Count == 1);
                    test(result.p3[0][Test.MyEnum.enum1].Equals("Goodbye"));
                    test(result.p3[1].Count == 1);
                    test(result.p3[1][Test.MyEnum.enum1].Equals("abc"));
                    test(result.p3[2].Count == 2);
                    test(result.p3[2][Test.MyEnum.enum2].Equals("Hello!!"));
                    test(result.p3[2][Test.MyEnum.enum3].Equals("qwerty"));
                }

                {
                    var dsi1 = new Dictionary<Test.MyStruct, Test.MyEnum>[2];
                    var dsi2 = new Dictionary<Test.MyStruct, Test.MyEnum>[1];

                    var s11 = new Test.MyStruct(1, 1);
                    var s12 = new Test.MyStruct(1, 2);
                    var di1 = new Dictionary<Test.MyStruct, Test.MyEnum>
                    {
                        [s11] = Test.MyEnum.enum1,
                        [s12] = Test.MyEnum.enum2
                    };

                    var s22 = new Test.MyStruct(2, 2);
                    var s23 = new Test.MyStruct(2, 3);
                    var di2 = new Dictionary<Test.MyStruct, Test.MyEnum>
                    {
                        [s11] = Test.MyEnum.enum1,
                        [s22] = Test.MyEnum.enum3,
                        [s23] = Test.MyEnum.enum2
                    };

                    var di3 = new Dictionary<Test.MyStruct, Test.MyEnum>
                    {
                        [s23] = Test.MyEnum.enum3
                    };

                    dsi1[0] = di1;
                    dsi1[1] = di2;
                    dsi2[0] = di3;

                    var result = await p.opMyStructMyEnumDSAsync(dsi1, dsi2);

                    test(result.returnValue.Length == 2);
                    test(result.returnValue[0].Count == 3);
                    test(result.returnValue[0][s11] == Test.MyEnum.enum1);
                    test(result.returnValue[0][s22] == Test.MyEnum.enum3);
                    test(result.returnValue[0][s23] == Test.MyEnum.enum2);
                    test(result.returnValue[1].Count == 2);
                    test(result.returnValue[1][s11] == Test.MyEnum.enum1);
                    test(result.returnValue[1][s12] == Test.MyEnum.enum2);

                    test(result.p3.Length == 3);
                    test(result.p3[0].Count == 1);
                    test(result.p3[0][s23] == Test.MyEnum.enum3);
                    test(result.p3[1].Count == 2);
                    test(result.p3[1][s11] == Test.MyEnum.enum1);
                    test(result.p3[1][s12] == Test.MyEnum.enum2);
                    test(result.p3[2].Count == 3);
                    test(result.p3[2][s11] == Test.MyEnum.enum1);
                    test(result.p3[2][s22] == Test.MyEnum.enum3);
                    test(result.p3[2][s23] == Test.MyEnum.enum2);
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

                    var result = await p.opByteByteSDAsync(sdi1, sdi2);

                    test(result.p3.Count == 1);
                    test(result.p3[0xf1].Length == 2);
                    test(result.p3[0xf1][0] == 0xf2);
                    test(result.p3[0xf1][1] == 0xf3);

                    test(result.returnValue.Count == 3);
                    test(result.returnValue[0x01].Length == 2);
                    test(result.returnValue[0x01][0] == 0x01);
                    test(result.returnValue[0x01][1] == 0x11);
                    test(result.returnValue[0x22].Length == 1);
                    test(result.returnValue[0x22][0] == 0x12);
                    test(result.returnValue[0xf1].Length == 2);
                    test(result.returnValue[0xf1][0] == 0xf2);
                    test(result.returnValue[0xf1][1] == 0xf3);
                }

                {
                    var sdi1 = new Dictionary<bool, bool[]>();
                    var sdi2 = new Dictionary<bool, bool[]>();

                    bool[] si1 = new bool[] { true, false };
                    bool[] si2 = new bool[] { false, true, true };

                    sdi1[false] = si1;
                    sdi1[true] = si2;
                    sdi2[false] = si1;

                    var result = await p.opBoolBoolSDAsync(sdi1, sdi2);

                    test(result.p3.Count == 1);
                    test(result.p3[false].Length == 2);
                    test(result.p3[false][0]);
                    test(!result.p3[false][1]);
                    test(result.returnValue.Count == 2);
                    test(result.returnValue[false].Length == 2);
                    test(result.returnValue[false][0]);
                    test(!result.returnValue[false][1]);
                    test(result.returnValue[true].Length == 3);
                    test(!result.returnValue[true][0]);
                    test(result.returnValue[true][1]);
                    test(result.returnValue[true][2]);
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

                    var result = await p.opShortShortSDAsync(sdi1, sdi2);

                    test(result.p3.Count == 1);
                    test(result.p3[4].Length == 2);
                    test(result.p3[4][0] == 6);
                    test(result.p3[4][1] == 7);

                    test(result.returnValue.Count == 3);
                    test(result.returnValue[1].Length == 3);
                    test(result.returnValue[1][0] == 1);
                    test(result.returnValue[1][1] == 2);
                    test(result.returnValue[1][2] == 3);
                    test(result.returnValue[2].Length == 2);
                    test(result.returnValue[2][0] == 4);
                    test(result.returnValue[2][1] == 5);
                    test(result.returnValue[4].Length == 2);
                    test(result.returnValue[4][0] == 6);
                    test(result.returnValue[4][1] == 7);
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

                    var result = await p.opIntIntSDAsync(sdi1, sdi2);

                    test(result.p3.Count == 1);
                    test(result.p3[400].Length == 2);
                    test(result.p3[400][0] == 600);
                    test(result.p3[400][1] == 700);

                    test(result.returnValue.Count == 3);
                    test(result.returnValue[100].Length == 3);
                    test(result.returnValue[100][0] == 100);
                    test(result.returnValue[100][1] == 200);
                    test(result.returnValue[100][2] == 300);
                    test(result.returnValue[200].Length == 2);
                    test(result.returnValue[200][0] == 400);
                    test(result.returnValue[200][1] == 500);
                    test(result.returnValue[400].Length == 2);
                    test(result.returnValue[400][0] == 600);
                    test(result.returnValue[400][1] == 700);
                }

                {
                    var sdi1 = new Dictionary<long, long[]>();
                    var sdi2 = new Dictionary<long, long[]>();

                    long[] si1 = new long[] { 999999110L, 999999111L, 999999110L };
                    long[] si2 = new long[] { 999999120L, 999999130L };
                    long[] si3 = new long[] { 999999110L, 999999120L };

                    sdi1[999999990L] = si1;
                    sdi1[999999991L] = si2;
                    sdi2[999999992L] = si3;

                    var result = await p.opLongLongSDAsync(sdi1, sdi2);

                    test(result.p3.Count == 1);
                    test(result.p3[999999992L].Length == 2);
                    test(result.p3[999999992L][0] == 999999110L);
                    test(result.p3[999999992L][1] == 999999120L);
                    test(result.returnValue.Count == 3);
                    test(result.returnValue[999999990L].Length == 3);
                    test(result.returnValue[999999990L][0] == 999999110L);
                    test(result.returnValue[999999990L][1] == 999999111L);
                    test(result.returnValue[999999990L][2] == 999999110L);
                    test(result.returnValue[999999991L].Length == 2);
                    test(result.returnValue[999999991L][0] == 999999120L);
                    test(result.returnValue[999999991L][1] == 999999130L);
                    test(result.returnValue[999999992L].Length == 2);
                    test(result.returnValue[999999992L][0] == 999999110L);
                    test(result.returnValue[999999992L][1] == 999999120L);
                }

                {
                    var sdi1 = new Dictionary<string, float[]>();
                    var sdi2 = new Dictionary<string, float[]>();

                    float[] si1 = new float[] { -1.1f, 123123.2f, 100.0f };
                    float[] si2 = new float[] { 42.24f, -1.61f };
                    float[] si3 = new float[] { -3.14f, 3.14f };

                    sdi1["abc"] = si1;
                    sdi1["ABC"] = si2;
                    sdi2["aBc"] = si3;

                    var result = await p.opStringFloatSDAsync(sdi1, sdi2);

                    test(result.p3.Count == 1);
                    test(result.p3["aBc"].Length == 2);
                    test(result.p3["aBc"][0] == -3.14f);
                    test(result.p3["aBc"][1] == 3.14f);

                    test(result.returnValue.Count == 3);
                    test(result.returnValue["abc"].Length == 3);
                    test(result.returnValue["abc"][0] == -1.1f);
                    test(result.returnValue["abc"][1] == 123123.2f);
                    test(result.returnValue["abc"][2] == 100.0f);
                    test(result.returnValue["ABC"].Length == 2);
                    test(result.returnValue["ABC"][0] == 42.24f);
                    test(result.returnValue["ABC"][1] == -1.61f);
                    test(result.returnValue["aBc"].Length == 2);
                    test(result.returnValue["aBc"][0] == -3.14f);
                    test(result.returnValue["aBc"][1] == 3.14f);
                }

                {
                    var sdi1 = new Dictionary<string, double[]>();
                    var sdi2 = new Dictionary<string, double[]>();

                    double[] si1 = new double[] { 1.1E10, 1.2E10, 1.3E10 };
                    double[] si2 = new double[] { 1.4E10, 1.5E10 };
                    double[] si3 = new double[] { 1.6E10, 1.7E10 };

                    sdi1["Hello!!"] = si1;
                    sdi1["Goodbye"] = si2;
                    sdi2[""] = si3;

                    var result = await p.opStringDoubleSDAsync(sdi1, sdi2);

                    test(result.p3.Count == 1);
                    test(result.p3[""].Length == 2);
                    test(result.p3[""][0] == 1.6E10);
                    test(result.p3[""][1] == 1.7E10);
                    test(result.returnValue.Count == 3);
                    test(result.returnValue["Hello!!"].Length == 3);
                    test(result.returnValue["Hello!!"][0] == 1.1E10);
                    test(result.returnValue["Hello!!"][1] == 1.2E10);
                    test(result.returnValue["Hello!!"][2] == 1.3E10);
                    test(result.returnValue["Goodbye"].Length == 2);
                    test(result.returnValue["Goodbye"][0] == 1.4E10);
                    test(result.returnValue["Goodbye"][1] == 1.5E10);
                    test(result.returnValue[""].Length == 2);
                    test(result.returnValue[""][0] == 1.6E10);
                    test(result.returnValue[""][1] == 1.7E10);
                }

                {
                    var sdi1 = new Dictionary<string, string[]>();
                    var sdi2 = new Dictionary<string, string[]>();

                    string[] si1 = new string[] { "abc", "de", "fghi" };
                    string[] si2 = new string[] { "xyz", "or" };
                    string[] si3 = new string[] { "and", "xor" };

                    sdi1["abc"] = si1;
                    sdi1["def"] = si2;
                    sdi2["ghi"] = si3;

                    var result = await p.opStringStringSDAsync(sdi1, sdi2);

                    test(result.p3.Count == 1);
                    test(result.p3["ghi"].Length == 2);
                    test(result.p3["ghi"][0].Equals("and"));
                    test(result.p3["ghi"][1].Equals("xor"));

                    test(result.returnValue.Count == 3);
                    test(result.returnValue["abc"].Length == 3);
                    test(result.returnValue["abc"][0].Equals("abc"));
                    test(result.returnValue["abc"][1].Equals("de"));
                    test(result.returnValue["abc"][2].Equals("fghi"));
                    test(result.returnValue["def"].Length == 2);
                    test(result.returnValue["def"][0].Equals("xyz"));
                    test(result.returnValue["def"][1].Equals("or"));
                    test(result.returnValue["ghi"].Length == 2);
                    test(result.returnValue["ghi"][0].Equals("and"));
                    test(result.returnValue["ghi"][1].Equals("xor"));
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

                    var result = await p.opMyEnumMyEnumSDAsync(sdi1, sdi2);

                    test(result.p3.Count == 1);
                    test(result.p3[Test.MyEnum.enum1].Length == 2);
                    test(result.p3[Test.MyEnum.enum1][0] == Test.MyEnum.enum3);
                    test(result.p3[Test.MyEnum.enum1][1] == Test.MyEnum.enum3);
                    test(result.returnValue.Count == 3);
                    test(result.returnValue[Test.MyEnum.enum3].Length == 3);
                    test(result.returnValue[Test.MyEnum.enum3][0] == Test.MyEnum.enum1);
                    test(result.returnValue[Test.MyEnum.enum3][1] == Test.MyEnum.enum1);
                    test(result.returnValue[Test.MyEnum.enum3][2] == Test.MyEnum.enum2);
                    test(result.returnValue[Test.MyEnum.enum2].Length == 2);
                    test(result.returnValue[Test.MyEnum.enum2][0] == Test.MyEnum.enum1);
                    test(result.returnValue[Test.MyEnum.enum2][1] == Test.MyEnum.enum2);
                    test(result.returnValue[Test.MyEnum.enum1].Length == 2);
                    test(result.returnValue[Test.MyEnum.enum1][0] == Test.MyEnum.enum3);
                    test(result.returnValue[Test.MyEnum.enum1][1] == Test.MyEnum.enum3);
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

                        var result = await p.opIntSAsync(s);

                        for (int j = 0; j < result.Length; ++j)
                        {
                            test(result[j] == -j);
                        }
                    }
                }

                {
                    Dictionary<string, string> ctx = new Dictionary<string, string>();
                    ctx["one"] = "ONE";
                    ctx["two"] = "TWO";
                    ctx["three"] = "THREE";
                    {
                        test(p.ice_getContext().Count == 0);
                        var result = await p.opContextAsync();
                        test(!CollectionComparer.Equals(result, ctx));
                    }
                    {
                        test(p.ice_getContext().Count == 0);
                        var result = await p.opContextAsync(ctx);
                        test(CollectionComparer.Equals(result, ctx));
                    }
                    {
                        var p2 = Test.MyClassPrxHelper.checkedCast(p.ice_context(ctx));
                        test(CollectionComparer.Equals(p2.ice_getContext(), ctx));
                        var result = await p2.opContextAsync();
                        test(CollectionComparer.Equals(result, ctx));
                    }
                    {
                        var p2 = Test.MyClassPrxHelper.checkedCast(p.ice_context(ctx));
                        var result = await p2.opContextAsync(ctx);
                        test(CollectionComparer.Equals(result, ctx));
                    }
                }

                //
                // Test implicit context propagation with async task
                //
                if (p.ice_getConnection() != null)
                {
                    string[] impls = { "Shared", "PerThread" };
                    for (int i = 0; i < 2; i++)
                    {
                        var initData = new InitializationData();
                        initData.properties = communicator.getProperties().ice_clone_();
                        initData.properties.setProperty("Ice.ImplicitContext", impls[i]);

                        Communicator ic = helper.initialize(initData);

                        var ctx = new Dictionary<string, string>
                        {
                            ["one"] = "ONE",
                            ["two"] = "TWO",
                            ["three"] = "THREE"
                        };

                        var p3 =
                            Test.MyClassPrxHelper.uncheckedCast(ic.stringToProxy("test:" + helper.getTestEndpoint(0)));

                        ic.getImplicitContext().setContext(ctx);
                        test(CollectionComparer.Equals(ic.getImplicitContext().getContext(), ctx));
                        {
                            test(CollectionComparer.Equals(p3.opContextAsync().Result, ctx));
                        }

                        ic.getImplicitContext().put("zero", "ZERO");

                        ctx = ic.getImplicitContext().getContext();
                        {
                            test(CollectionComparer.Equals(p3.opContextAsync().Result, ctx));
                        }

                        var prxContext = new Dictionary<string, string>
                        {
                            ["one"] = "UN",
                            ["four"] = "QUATRE"
                        };

                        Dictionary<string, string> combined = prxContext;
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

                        p3 = Test.MyClassPrxHelper.uncheckedCast(p.ice_context(prxContext));

                        ic.getImplicitContext().setContext(null);
                        {
                            test(CollectionComparer.Equals(p3.opContextAsync().Result, prxContext));
                        }

                        ic.getImplicitContext().setContext(ctx);
                        {
                            test(CollectionComparer.Equals(p3.opContextAsync().Result, combined));
                        }

                        //ic.getImplicitContext().setContext(null);
                        ic.destroy();
                    }
                }

                {
                    p.opIdempotentAsync().Wait();
                }

                {
                    p.opNonmutatingAsync().Wait();
                }

                {
                    var derived = Test.MyDerivedClassPrxHelper.checkedCast(p);
                    test(derived != null);
                    derived.opDerivedAsync().Wait();
                }

                {
                    test(p.opByte1Async(0xFF).Result == 0xFF);
                }

                {
                    test(p.opShort1Async(0x7FFF).Result == 0x7FFF);
                }

                {
                    test(p.opInt1Async(0x7FFFFFFF).Result == 0x7FFFFFFF);
                }

                {
                    test(p.opLong1Async(0x7FFFFFFFFFFFFFFF).Result == 0x7FFFFFFFFFFFFFFF);
                }

                {
                    test(p.opFloat1Async(1.0f).Result == 1.0f);
                }

                {
                    test(p.opDouble1Async(1.0d).Result == 1.0d);
                }

                {
                    test(p.opString1Async("opString1").Result.Equals("opString1"));
                }

                {
                    test(p.opStringS1Async(null).Result.Length == 0);
                }

                {
                    test(p.opByteBoolD1Async(null).Result.Count == 0);
                }

                {
                    test(p.opStringS2Async(null).Result.Length == 0);
                }

                {
                    test(p.opByteBoolD2Async(null).Result.Count == 0);
                }

                Func<Task> task = async () =>
                {
                    {
                        var p1 = await p.opMStruct1Async();

                        p1.e = Test.MyEnum.enum3;
                        var r = await p.opMStruct2Async(p1);
                        test(r.p2.Equals(p1) && r.returnValue.Equals(p1));
                    }

                    {
                        await p.opMSeq1Async();

                        var p1 = new string[1];
                        p1[0] = "test";
                        var r = await p.opMSeq2Async(p1);
                        test(CollectionComparer.Equals(r.p2, p1) &&
                             CollectionComparer.Equals(r.returnValue, p1));
                    }

                    {
                        await p.opMDict1Async();

                        var p1 = new Dictionary<string, string>();
                        p1["test"] = "test";
                        var r = await p.opMDict2Async(p1);
                        test(CollectionComparer.Equals(r.p2, p1) &&
                             CollectionComparer.Equals(r.returnValue, p1));
                    }
                };
            }
        }
    }
}
