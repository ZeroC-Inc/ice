//
// Copyright(c) ZeroC, Inc. All rights reserved.
//

using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;

namespace Ice.stream
{
    public class AllTests : global::Test.AllTests
    {
        //
        // There does not appear to be any way to compare collections
        // in either C# or with the .NET framework. Something like
        // C++ STL EqualRange would be n..
        //
        private static bool Compare(ICollection c1, ICollection c2)
        {
            if (c1 == null)
            {
                return c2 == null;
            }
            if (c2 == null)
            {
                return false;
            }
            if (!c1.GetType().Equals(c2.GetType()))
            {
                return false;
            }

            if (c1.Count != c2.Count)
            {
                return false;
            }

            IEnumerator i1 = c1.GetEnumerator();
            IEnumerator i2 = c2.GetEnumerator();
            while (i1.MoveNext())
            {
                i2.MoveNext();
                if (i1.Current is ICollection)
                {
                    Debug.Assert(i2.Current is ICollection);
                    if (!Compare((ICollection)i1.Current, (ICollection)i2.Current))
                    {
                        return false;
                    }
                }
                else if (!i1.Current.Equals(i2.Current))
                {
                    return false;
                }
            }
            return true;
        }

        static public int allTests(global::Test.TestHelper helper)
        {
            var communicator = helper.communicator();
            InputStream inS;
            OutputStream outS;

            var output = helper.getWriter();
            output.Write("testing primitive types... ");
            output.Flush();

            {
                byte[] data = new byte[0];
                inS = new InputStream(communicator, data);
            }

            {
                outS = new OutputStream(communicator);
                outS.StartEncapsulation();
                outS.WriteBool(true);
                outS.EndEncapsulation();
                var data = outS.Finished();

                inS = new InputStream(communicator, data);
                inS.StartEncapsulation();
                test(inS.ReadBool());
                inS.EndEncapsulation();

                inS = new InputStream(communicator, data);
                inS.StartEncapsulation();
                test(inS.ReadBool());
                inS.EndEncapsulation();
            }

            {
                var data = new byte[0];
                inS = new InputStream(communicator, data);
                try
                {
                    inS.ReadBool();
                    test(false);
                }
                catch (UnmarshalOutOfBoundsException)
                {
                }
            }

            {
                outS = new OutputStream(communicator);
                outS.WriteBool(true);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                test(inS.ReadBool());
            }

            {
                outS = new OutputStream(communicator);
                outS.WriteByte(1);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                test(inS.ReadByte() == 1);
            }

            {
                outS = new OutputStream(communicator);
                outS.WriteShort(2);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                test(inS.ReadShort() == 2);
            }

            {
                outS = new OutputStream(communicator);
                outS.WriteInt(3);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                test(inS.ReadInt() == 3);
            }

            {
                outS = new OutputStream(communicator);
                outS.WriteLong(4);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                test(inS.ReadLong() == 4);
            }

            {
                outS = new OutputStream(communicator);
                outS.WriteFloat((float)5.0);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                test(inS.ReadFloat() == (float)5.0);
            }

            {
                outS = new OutputStream(communicator);
                outS.WriteDouble(6.0);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                test(inS.ReadDouble() == 6.0);
            }

            {
                outS = new OutputStream(communicator);
                outS.WriteString("hello world");
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                test(inS.ReadString().Equals("hello world"));
            }

            output.WriteLine("ok");

            output.Write("testing constructed types... ");
            output.Flush();

            {
                outS = new OutputStream(communicator);
                Test.MyEnumHelper.OutputStreamWriter(outS, Test.MyEnum.enum3);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var e = Test.MyEnumHelper.InputStreamReader(inS);
                test(e == Test.MyEnum.enum3);
            }

            {
                outS = new OutputStream(communicator);
                var s = new Test.SmallStruct();
                s.bo = true;
                s.by = 1;
                s.sh = 2;
                s.i = 3;
                s.l = 4;
                s.f = 5.0f;
                s.d = 6.0;
                s.str = "7";
                s.e = Test.MyEnum.enum2;
                s.p = Test.IMyInterfacePrx.Parse("test:default", communicator);
                s.IceWrite(outS);
                var data = outS.Finished();
                var s2 = new Test.SmallStruct(new InputStream(communicator, data));
                test(s2.Equals(s));
            }

            {
                outS = new OutputStream(communicator);
                var o = new Test.OptionalClass();
                o.bo = true;
                o.by = 5;
                o.sh = 4;
                o.i = 3;
                // Can only read/write classes within encaps
                outS.StartEncapsulation();
                outS.WriteClass(o);
                outS.EndEncapsulation();
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                inS.StartEncapsulation();
                var o2 = inS.ReadClass<Test.OptionalClass>();
                inS.EndEncapsulation();
                test(o2.bo == o.bo);
                test(o2.by == o.by);
                test(o2.sh == o.sh);
                test(o2.i == o.i);
            }

            {
                bool[] arr = { true, false, true, false };
                outS = new OutputStream(communicator);
                outS.WriteBoolSeq(arr);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var arr2 = inS.ReadBoolArray();
                test(Compare(arr2, arr));

                bool[][] arrS = { arr, new bool[0], arr };
                outS = new OutputStream(communicator);
                Test.BoolSSHelper.OutputStreamWriter(outS, arrS);
                data = outS.Finished();
                inS = new InputStream(communicator, data);
                var arr2S = Test.BoolSSHelper.InputStreamReader(inS);
                test(Compare(arr2S, arrS));
            }

            {
                byte[] arr = { 0x01, 0x11, 0x12, 0x22 };
                outS = new OutputStream(communicator);
                outS.WriteByteSeq(arr);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var arr2 = inS.ReadByteArray();
                test(Compare(arr2, arr));

                byte[][] arrS = { arr, new byte[0], arr };
                outS = new OutputStream(communicator);
                Test.ByteSSHelper.OutputStreamWriter(outS, arrS);
                data = outS.Finished();
                inS = new InputStream(communicator, data);
                var arr2S = Test.ByteSSHelper.InputStreamReader(inS);
                test(Compare(arr2S, arrS));
            }

            {
                Serialize.Small small = new Serialize.Small();
                small.i = 99;
                outS = new OutputStream(communicator);
                outS.WriteSerializable(small);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var small2 = (Serialize.Small)inS.ReadSerializable();
                test(small2.i == 99);
            }

            {
                short[] arr = { 0x01, 0x11, 0x12, 0x22 };
                outS = new OutputStream(communicator);
                outS.WriteShortSeq(arr);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var arr2 = inS.ReadShortArray();
                test(Compare(arr2, arr));

                short[][] arrS = { arr, new short[0], arr };
                outS = new OutputStream(communicator);
                Test.ShortSSHelper.OutputStreamWriter(outS, arrS);
                data = outS.Finished();
                inS = new InputStream(communicator, data);
                var arr2S = Test.ShortSSHelper.InputStreamReader(inS);
                test(Compare(arr2S, arrS));
            }

            {
                int[] arr = { 0x01, 0x11, 0x12, 0x22 };
                outS = new OutputStream(communicator);
                outS.WriteIntSeq(arr);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var arr2 = inS.ReadIntArray();
                test(Compare(arr2, arr));

                int[][] arrS = { arr, new int[0], arr };
                outS = new OutputStream(communicator);
                Test.IntSSHelper.OutputStreamWriter(outS, arrS);
                data = outS.Finished();
                inS = new InputStream(communicator, data);
                var arr2S = Test.IntSSHelper.InputStreamReader(inS);
                test(Compare(arr2S, arrS));
            }

            {
                long[] arr = { 0x01, 0x11, 0x12, 0x22 };
                outS = new OutputStream(communicator);
                outS.WriteLongSeq(arr);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var arr2 = inS.ReadLongArray();
                test(Compare(arr2, arr));

                long[][] arrS = { arr, new long[0], arr };
                outS = new OutputStream(communicator);
                Test.LongSSHelper.OutputStreamWriter(outS, arrS);
                data = outS.Finished();
                inS = new InputStream(communicator, data);
                var arr2S = Test.LongSSHelper.InputStreamReader(inS);
                test(Compare(arr2S, arrS));
            }

            {
                float[] arr = { 1, 2, 3, 4 };
                outS = new OutputStream(communicator);
                outS.WriteFloatSeq(arr);
                byte[] data = outS.Finished();
                inS = new InputStream(communicator, data);
                float[] arr2 = inS.ReadFloatArray();
                test(Compare(arr2, arr));

                float[][] arrS = { arr, new float[0], arr };
                outS = new OutputStream(communicator);
                Test.FloatSSHelper.OutputStreamWriter(outS, arrS);
                data = outS.Finished();
                inS = new InputStream(communicator, data);
                var arr2S = Test.FloatSSHelper.InputStreamReader(inS);
                test(Compare(arr2S, arrS));
            }

            {
                double[] arr =
                    {
                        1,
                        2,
                        3,
                        4
                    };
                outS = new OutputStream(communicator);
                outS.WriteDoubleSeq(arr);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var arr2 = inS.ReadDoubleArray();
                test(Compare(arr2, arr));

                double[][] arrS = { arr, new double[0], arr };
                outS = new OutputStream(communicator);
                Test.DoubleSSHelper.OutputStreamWriter(outS, arrS);
                data = outS.Finished();
                inS = new InputStream(communicator, data);
                var arr2S = Test.DoubleSSHelper.InputStreamReader(inS);
                test(Compare(arr2S, arrS));
            }

            {
                string[] arr = { "string1", "string2", "string3", "string4" };
                outS = new OutputStream(communicator);
                outS.WriteStringSeq(arr);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var arr2 = inS.ReadStringArray();
                test(Compare(arr2, arr));

                string[][] arrS = { arr, new string[0], arr };
                outS = new OutputStream(communicator);
                Test.StringSSHelper.OutputStreamWriter(outS, arrS);
                data = outS.Finished();
                inS = new InputStream(communicator, data);
                var arr2S = Test.StringSSHelper.InputStreamReader(inS);
                test(Compare(arr2S, arrS));
            }

            {
                Test.MyEnum[] arr = {
                        Test.MyEnum.enum3,
                        Test.MyEnum.enum2,
                        Test.MyEnum.enum1,
                        Test.MyEnum.enum2
                    };
                outS = new OutputStream(communicator);
                outS.WriteEnumSeq(arr, Test.MyEnumHelper.OutputStreamWriter);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var arr2 = inS.ReaEnumArray(Test.MyEnumHelper.InputStreamReader);
                test(Compare(arr2, arr));

                Test.MyEnum[][] arrS = { arr, new Test.MyEnum[0], arr };
                outS = new OutputStream(communicator);
                Test.MyEnumSSHelper.OutputStreamWriter(outS, arrS);
                data = outS.Finished();
                inS = new InputStream(communicator, data);
                var arr2S = Test.MyEnumSSHelper.InputStreamReader(inS);
                test(Compare(arr2S, arrS));
            }

            var smallStructArray = new Test.SmallStruct[3];
            for (int i = 0; i < smallStructArray.Length; ++i)
            {
                smallStructArray[i] = new Test.SmallStruct();
                smallStructArray[i].bo = true;
                smallStructArray[i].by = 1;
                smallStructArray[i].sh = 2;
                smallStructArray[i].i = 3;
                smallStructArray[i].l = 4;
                smallStructArray[i].f = 5.0f;
                smallStructArray[i].d = 6.0;
                smallStructArray[i].str = "7";
                smallStructArray[i].e = Test.MyEnum.enum2;
                smallStructArray[i].p = Test.IMyInterfacePrx.Parse("test:default", communicator);
            }

            var myClassArray = new Test.MyClass[4];
            for (int i = 0; i < myClassArray.Length; ++i)
            {
                myClassArray[i] = new Test.MyClass();
                myClassArray[i].c = myClassArray[i];
                myClassArray[i].o = myClassArray[i];
                myClassArray[i].s = new Test.SmallStruct();
                myClassArray[i].s.e = Test.MyEnum.enum2;
                myClassArray[i].seq1 = new bool[] { true, false, true, false };
                myClassArray[i].seq2 = new byte[] { 1, 2, 3, 4 };
                myClassArray[i].seq3 = new short[] { 1, 2, 3, 4 };
                myClassArray[i].seq4 = new int[] { 1, 2, 3, 4 };
                myClassArray[i].seq5 = new long[] { 1, 2, 3, 4 };
                myClassArray[i].seq6 = new float[] { 1, 2, 3, 4 };
                myClassArray[i].seq7 = new double[] { 1, 2, 3, 4 };
                myClassArray[i].seq8 = new string[] { "string1", "string2", "string3", "string4" };
                myClassArray[i].seq9 = new Test.MyEnum[] { Test.MyEnum.enum3, Test.MyEnum.enum2, Test.MyEnum.enum1 };
                myClassArray[i].seq10 = new Test.MyClass[4]; // null elements.
                myClassArray[i].d = new Dictionary<string, Test.MyClass>();
                myClassArray[i].d["hi"] = myClassArray[i];
            }

            {
                outS = new OutputStream(communicator);
                outS.StartEncapsulation();
                outS.WriteClassSeq(myClassArray);
                outS.EndEncapsulation();
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                inS.StartEncapsulation();
                var arr2 = inS.ReadClassArray<Test.MyClass>();
                inS.EndEncapsulation();
                test(arr2.Length == myClassArray.Length);
                for (int i = 0; i < arr2.Length; ++i)
                {
                    test(arr2[i] != null);
                    test(arr2[i].c == arr2[i]);
                    test(arr2[i].o == arr2[i]);
                    test(arr2[i].s.e == Test.MyEnum.enum2);
                    test(Compare(arr2[i].seq1, myClassArray[i].seq1));
                    test(Compare(arr2[i].seq2, myClassArray[i].seq2));
                    test(Compare(arr2[i].seq3, myClassArray[i].seq3));
                    test(Compare(arr2[i].seq4, myClassArray[i].seq4));
                    test(Compare(arr2[i].seq5, myClassArray[i].seq5));
                    test(Compare(arr2[i].seq6, myClassArray[i].seq6));
                    test(Compare(arr2[i].seq7, myClassArray[i].seq7));
                    test(Compare(arr2[i].seq8, myClassArray[i].seq8));
                    test(Compare(arr2[i].seq9, myClassArray[i].seq9));
                    test(arr2[i].d["hi"].Equals(arr2[i]));
                }

                Test.MyClass[][] arrS = { myClassArray, new Test.MyClass[0], myClassArray };
                outS = new OutputStream(communicator);
                outS.StartEncapsulation();
                Test.MyClassSSHelper.OutputStreamWriter(outS, arrS);
                outS.EndEncapsulation();
                data = outS.Finished();
                inS = new InputStream(communicator, data);
                inS.StartEncapsulation();
                var arr2S = Test.MyClassSSHelper.InputStreamReader(inS);
                inS.EndEncapsulation();
                test(arr2S.Length == arrS.Length);
                test(arr2S[0].Length == arrS[0].Length);
                test(arr2S[1].Length == arrS[1].Length);
                test(arr2S[2].Length == arrS[2].Length);

                for (int j = 0; j < arr2S.Length; ++j)
                {
                    for (int k = 0; k < arr2S[j].Length; ++k)
                    {
                        test(arr2S[j][k].c == arr2S[j][k]);
                        test(arr2S[j][k].o == arr2S[j][k]);
                        test(arr2S[j][k].s.e == Test.MyEnum.enum2);
                        test(Compare(arr2S[j][k].seq1, myClassArray[k].seq1));
                        test(Compare(arr2S[j][k].seq2, myClassArray[k].seq2));
                        test(Compare(arr2S[j][k].seq3, myClassArray[k].seq3));
                        test(Compare(arr2S[j][k].seq4, myClassArray[k].seq4));
                        test(Compare(arr2S[j][k].seq5, myClassArray[k].seq5));
                        test(Compare(arr2S[j][k].seq6, myClassArray[k].seq6));
                        test(Compare(arr2S[j][k].seq7, myClassArray[k].seq7));
                        test(Compare(arr2S[j][k].seq8, myClassArray[k].seq8));
                        test(Compare(arr2S[j][k].seq9, myClassArray[k].seq9));
                        test(arr2S[j][k].d["hi"].Equals(arr2S[j][k]));
                    }
                }
            }

            {
                outS = new OutputStream(communicator);
                var obj = new Test.MyClass();
                obj.s = new Test.SmallStruct();
                obj.s.e = Test.MyEnum.enum2;
                outS.StartEncapsulation();
                outS.WriteClass(obj);
                outS.EndEncapsulation();
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                inS.StartEncapsulation();
                var robj = inS.ReadClass<Test.MyClass>();
                inS.EndEncapsulation();
                test(robj != null);
                test(robj.s.e == Test.MyEnum.enum2);
            }

            {
                outS = new OutputStream(communicator);
                outS.StartEncapsulation();
                var ex = new Test.MyException();

                var c = new Test.MyClass();
                c.c = c;
                c.o = c;
                c.s = new Test.SmallStruct();
                c.s.e = Test.MyEnum.enum2;
                c.seq1 = new bool[] { true, false, true, false };
                c.seq2 = new byte[] { 1, 2, 3, 4 };
                c.seq3 = new short[] { 1, 2, 3, 4 };
                c.seq4 = new int[] { 1, 2, 3, 4 };
                c.seq5 = new long[] { 1, 2, 3, 4 };
                c.seq6 = new float[] { 1, 2, 3, 4 };
                c.seq7 = new double[] { 1, 2, 3, 4 };
                c.seq8 = new string[] { "string1", "string2", "string3", "string4" };
                c.seq9 = new Test.MyEnum[] { Test.MyEnum.enum3, Test.MyEnum.enum2, Test.MyEnum.enum1 };
                c.seq10 = new Test.MyClass[4]; // null elements.
                c.d = new Dictionary<string, Test.MyClass>();
                c.d.Add("hi", c);

                ex.c = c;

                outS.WriteException(ex);
                outS.EndEncapsulation();
                var data = outS.Finished();

                inS = new InputStream(communicator, data);
                inS.StartEncapsulation();
                try
                {
                    inS.ThrowException();
                    test(false);
                }
                catch (Test.MyException ex1)
                {
                    test(ex1.c.s.e == c.s.e);
                    test(Compare(ex1.c.seq1, c.seq1));
                    test(Compare(ex1.c.seq2, c.seq2));
                    test(Compare(ex1.c.seq3, c.seq3));
                    test(Compare(ex1.c.seq4, c.seq4));
                    test(Compare(ex1.c.seq5, c.seq5));
                    test(Compare(ex1.c.seq6, c.seq6));
                    test(Compare(ex1.c.seq7, c.seq7));
                    test(Compare(ex1.c.seq8, c.seq8));
                    test(Compare(ex1.c.seq9, c.seq9));
                }
                catch (UserException)
                {
                    test(false);
                }
                inS.EndEncapsulation();
            }

            {
                var dict = new Dictionary<byte, bool>();
                dict.Add(4, true);
                dict.Add(1, false);
                outS = new OutputStream(communicator);
                Test.ByteBoolDHelper.OutputStreamWriter(outS, dict);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var dict2 = Test.ByteBoolDHelper.InputStreamReader(inS);
                test(Collections.Equals(dict2, dict));
            }

            {
                var dict = new Dictionary<short, int>();
                dict.Add(1, 9);
                dict.Add(4, 8);
                outS = new OutputStream(communicator);
                Test.ShortIntDHelper.OutputStreamWriter(outS, dict);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var dict2 = Test.ShortIntDHelper.InputStreamReader(inS);
                test(Collections.Equals(dict2, dict));
            }

            {
                var dict = new Dictionary<long, float>();
                dict.Add(123809828, 0.51f);
                dict.Add(123809829, 0.56f);
                outS = new OutputStream(communicator);
                Test.LongFloatDHelper.OutputStreamWriter(outS, dict);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var dict2 = Test.LongFloatDHelper.InputStreamReader(inS);
                test(Collections.Equals(dict2, dict));
            }

            {
                var dict = new Dictionary<string, string>();
                dict.Add("key1", "value1");
                dict.Add("key2", "value2");
                outS = new OutputStream(communicator);
                Test.StringStringDHelper.OutputStreamWriter(outS, dict);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var dict2 = Test.StringStringDHelper.InputStreamReader(inS);
                test(Collections.Equals(dict2, dict));
            }

            {
                var dict = new Dictionary<string, Test.MyClass>();
                var c = new Test.MyClass();
                c.s = new Test.SmallStruct();
                c.s.e = Test.MyEnum.enum2;
                dict.Add("key1", c);
                c = new Test.MyClass();
                c.s = new Test.SmallStruct();
                c.s.e = Test.MyEnum.enum3;
                dict.Add("key2", c);
                outS = new OutputStream(communicator);
                outS.StartEncapsulation();
                Test.StringMyClassDHelper.OutputStreamWriter(outS, dict);
                outS.EndEncapsulation();
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                inS.StartEncapsulation();
                var dict2 = Test.StringMyClassDHelper.InputStreamReader(inS);
                inS.EndEncapsulation();
                test(dict2.Count == dict.Count);
                test(dict2["key1"].s.e == Test.MyEnum.enum2);
                test(dict2["key2"].s.e == Test.MyEnum.enum3);
            }

            {
                bool[] arr = { true, false, true, false };
                outS = new OutputStream(communicator);
                var l = new List<bool>(arr);
                outS.StartEncapsulation();
                outS.WriteBoolSeq(l);
                outS.EndEncapsulation();
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                inS.StartEncapsulation();
                var l2 = new List<bool>(inS.ReadBoolArray());
                inS.EndEncapsulation();
                test(Compare(l, l2));
            }

            {
                byte[] arr = { 0x01, 0x11, 0x12, 0x22 };
                outS = new OutputStream(communicator);
                var l = new List<byte>(arr);
                outS.WriteByteSeq(l);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var l2 = new List<byte>(inS.ReadByteArray());
                test(Compare(l2, l));
            }

            {
                Test.MyEnum[] arr = { Test.MyEnum.enum3, Test.MyEnum.enum2, Test.MyEnum.enum1, Test.MyEnum.enum2 };
                outS = new OutputStream(communicator);
                var l = new List<Test.MyEnum>(arr);
                outS.WriteEnumSeq(l, Test.MyEnumHelper.OutputStreamWriter);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var l2 = inS.ReadEnumList(Test.MyEnumHelper.InputStreamReader);
                test(Compare(l2, l));
            }

            {
                outS = new OutputStream(communicator);
                var l = new List<Test.SmallStruct>(smallStructArray);
                outS.WriteStructSeq(l);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var l2 = inS.ReadList(Test.SmallStruct.IceInputStreamReader, Test.SmallStruct.IceMinWireSize);
                test(l2.Count == l.Count);
                for (int i = 0; i < l2.Count; ++i)
                {
                    test(l2[i].Equals(smallStructArray[i]));
                }
            }

            {
                outS = new OutputStream(communicator);
                outS.StartEncapsulation();
                var l = new List<Test.MyClass>(myClassArray);
                outS.WriteClassSeq(l);
                outS.EndEncapsulation();
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                inS.StartEncapsulation();
                var l2 = inS.ReadClassList<Test.MyClass>();
                inS.EndEncapsulation();
                test(l2.Count == l.Count);
                for (int i = 0; i < l2.Count; ++i)
                {
                    test(l2[i] != null);
                    test(l2[i].c == l2[i]);
                    test(l2[i].o == l2[i]);
                    test(l2[i].s.e == Test.MyEnum.enum2);
                    test(Compare(l2[i].seq1, l[i].seq1));
                    test(Compare(l2[i].seq2, l[i].seq2));
                    test(Compare(l2[i].seq3, l[i].seq3));
                    test(Compare(l2[i].seq4, l[i].seq4));
                    test(Compare(l2[i].seq5, l[i].seq5));
                    test(Compare(l2[i].seq6, l[i].seq6));
                    test(Compare(l2[i].seq7, l[i].seq7));
                    test(Compare(l2[i].seq8, l[i].seq8));
                    test(Compare(l2[i].seq9, l[i].seq9));
                    test(l2[i].d["hi"].Equals(l2[i]));
                }
            }

            {
                var arr = new IObjectPrx[2];
                arr[0] = IObjectPrx.Parse("zero", communicator);
                arr[1] = IObjectPrx.Parse("one", communicator);
                outS = new OutputStream(communicator);
                var l = new List<IObjectPrx>(arr);
                outS.WriteProxySeq(l);
                byte[] data = outS.Finished();
                inS = new InputStream(communicator, data);
                var l2 = inS.ReadProxyList(IObjectPrx.Factory);
                test(Compare(l2, l));
            }

            {
                var arr = new Test.IMyInterfacePrx[2];
                arr[0] = Test.IMyInterfacePrx.Parse("zero", communicator);
                arr[1] = Test.IMyInterfacePrx.Parse("one", communicator);
                outS = new OutputStream(communicator);
                var l = new List<Test.IMyInterfacePrx>(arr);
                outS.WriteProxySeq(l);
                byte[] data = outS.Finished();
                inS = new InputStream(communicator, data);
                var l2 = inS.ReadProxyList(Test.IMyInterfacePrx.Factory);
                test(Compare(l2, l));
            }

            {
                short[] arr = { 0x01, 0x11, 0x12, 0x22 };
                outS = new OutputStream(communicator);
                var l = new LinkedList<short>(arr);
                outS.WriteShortSeq(l);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var l2 = new LinkedList<short>(inS.ReadShortArray());
                test(Compare(l2, l));
            }

            {
                int[] arr = { 0x01, 0x11, 0x12, 0x22 };
                outS = new OutputStream(communicator);
                var l = new LinkedList<int>(arr);
                outS.WriteIntSeq(l);
                byte[] data = outS.Finished();
                inS = new InputStream(communicator, data);
                var l2 = new LinkedList<int>(inS.ReadIntArray());
                test(Compare(l2, l));
            }

            {
                Test.MyEnum[] arr = { Test.MyEnum.enum3, Test.MyEnum.enum2, Test.MyEnum.enum1, Test.MyEnum.enum2 };
                outS = new OutputStream(communicator);
                var l = new LinkedList<Test.MyEnum>(arr);
                outS.WriteEnumSeq(l, Test.MyEnumHelper.OutputStreamWriter);
                byte[] data = outS.Finished();
                inS = new InputStream(communicator, data);
                var l2 = new LinkedList<Test.MyEnum>(inS.ReadEnumCollection(Test.MyEnumHelper.InputStreamReader));
                test(Compare(l2, l));
            }

            {
                outS = new OutputStream(communicator);
                var l = new LinkedList<Test.SmallStruct>(smallStructArray);
                outS.WriteStructSeq(l);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var l2 = new LinkedList<Test.SmallStruct>(inS.ReadCollection(Test.SmallStruct.IceInputStreamReader, Test.SmallStruct.IceMinWireSize));
                test(l2.Count == l.Count);
                var e = l.GetEnumerator();
                var e2 = l2.GetEnumerator();
                while (e.MoveNext() && e2.MoveNext())
                {
                    test(e.Current.Equals(e2.Current));
                }
            }

            {
                long[] arr = { 0x01, 0x11, 0x12, 0x22 };
                outS = new OutputStream(communicator);
                var l = new Stack<long>(arr);
                outS.WriteLongSeq(l);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var l2 = new Stack<long>(inS.ReadLongArray().Reverse());
                test(Compare(l2, l));
            }

            {
                float[] arr = { 1, 2, 3, 4 };
                outS = new OutputStream(communicator);
                var l = new Stack<float>(arr);
                outS.WriteFloatSeq(l);
                byte[] data = outS.Finished();
                inS = new InputStream(communicator, data);
                var l2 = new Stack<float>(inS.ReadFloatArray().Reverse());
                test(Compare(l2, l));
            }

            {
                outS = new OutputStream(communicator);
                var l = new Stack<Test.SmallStruct>(smallStructArray);
                outS.WriteStructSeq(l);
                byte[] data = outS.Finished();
                inS = new InputStream(communicator, data);
                var l2 = new Stack<Test.SmallStruct>(inS.ReadCollection(
                    Test.SmallStruct.IceInputStreamReader, Test.SmallStruct.IceMinWireSize).Reverse());
                test(l2.Count == l.Count);
                var e = l.GetEnumerator();
                var e2 = l2.GetEnumerator();
                while (e.MoveNext() && e2.MoveNext())
                {
                    test(e.Current.Equals(e2.Current));
                }
            }

            {
                var arr = new IObjectPrx[2];
                arr[0] = IObjectPrx.Parse("zero", communicator);
                arr[1] = IObjectPrx.Parse("one", communicator);
                outS = new OutputStream(communicator);
                var l = new Stack<IObjectPrx>(arr);
                outS.WriteProxySeq(l);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var l2 = new Stack<IObjectPrx>(inS.ReadProxyCollection(IObjectPrx.Factory).Reverse());
                test(Compare(l2, l));
            }

            {
                var arr = new Test.IMyInterfacePrx[2];
                arr[0] = Test.IMyInterfacePrx.Parse("zero", communicator);
                arr[1] = Test.IMyInterfacePrx.Parse("one", communicator);
                outS = new OutputStream(communicator);
                var l = new Stack<Test.IMyInterfacePrx>(arr);
                outS.WriteProxySeq(l);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var l2 = new Stack<Test.IMyInterfacePrx>(inS.ReadProxyCollection(Test.IMyInterfacePrx.Factory).Reverse());
                test(Compare(l2, l));
            }

            {
                double[] arr = { 1, 2, 3, 4 };
                outS = new OutputStream(communicator);
                var l = new Queue<double>(arr);
                outS.WriteDoubleSeq(l);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var l2 = new Queue<double>(inS.ReadDoubleArray());
                test(Compare(l2, l));
            }

            {
                string[] arr = { "string1", "string2", "string3", "string4" };
                outS = new OutputStream(communicator);
                var l = new Queue<string>(arr);
                outS.WriteStringSeq(l);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var l2 = new Queue<string>(inS.ReadStringCollection());
                test(Compare(l2, l));
            }

            {
                outS = new OutputStream(communicator);
                var l = new Queue<Test.SmallStruct>(smallStructArray);
                outS.WriteStructSeq(l);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var l2 = new Queue<Test.SmallStruct>(inS.ReadCollection(Test.SmallStruct.IceInputStreamReader, Test.SmallStruct.IceMinWireSize));
                test(l2.Count == l.Count);
                var e = l.GetEnumerator();
                var e2 = l2.GetEnumerator();
                while (e.MoveNext() && e2.MoveNext())
                {
                    test(e.Current.Equals(e2.Current));
                }
            }

            {
                string[] arr = { "string1", "string2", "string3", "string4" };
                string[][] arrS = { arr, new string[0], arr };
                outS = new OutputStream(communicator);
                var l = new List<string[]>(arrS);
                Test.StringSListHelper.OutputStreamWriter(outS, l);
                byte[] data = outS.Finished();
                inS = new InputStream(communicator, data);
                var l2 = Test.StringSListHelper.InputStreamReader(inS);
                test(Compare(l2, l));
            }

            {
                string[] arr = { "string1", "string2", "string3", "string4" };
                string[][] arrS = { arr, new string[0], arr };
                outS = new OutputStream(communicator);
                var l = new Stack<string[]>(arrS);
                Test.StringSStackHelper.OutputStreamWriter(outS, l);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var l2 = Test.StringSStackHelper.InputStreamReader(inS);
                test(Compare(l2, l));
            }

            {
                var dict = new SortedDictionary<string, string>();
                dict.Add("key1", "value1");
                dict.Add("key2", "value2");
                outS = new OutputStream(communicator);
                Test.SortedStringStringDHelper.OutputStreamWriter(outS, dict);
                var data = outS.Finished();
                inS = new InputStream(communicator, data);
                var dict2 = Test.SortedStringStringDHelper.InputStreamReader(inS);
                test(Collections.Equals(dict2, dict));
            }

            output.WriteLine("ok");
            return 0;
        }
    }
}
