// **********************************************************************
//
// Copyright (c) 2003-2018 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#pragma once

module Test
{
    sequence<byte> ByteString; /* By default, sequence<byte> is received as a string. */
    ["python:seq:list"] sequence<byte> ByteList;

    sequence<string> StringList; /* By default, a sequence is received as a list. */
    ["python:seq:tuple"] sequence<string> StringTuple;

    ["python:seq:array"] sequence<bool> BoolSeq1;
    ["python:seq:numpyarray"] sequence<bool> BoolSeq2;
    ["python:seq:memoryview:Custom.myBoolSeq"] sequence<bool> BoolSeq3;

    ["python:seq:array"] sequence<byte> ByteSeq1;
    ["python:seq:numpyarray"] sequence<byte> ByteSeq2;
    ["python:seq:memoryview:Custom.myByteSeq"] sequence<byte> ByteSeq3;

    ["python:seq:array"] sequence<short> ShortSeq1;
    ["python:seq:numpyarray"] sequence<short> ShortSeq2;
    ["python:seq:memoryview:Custom.myShortSeq"] sequence<short> ShortSeq3;

    ["python:seq:array"] sequence<int> IntSeq1;
    ["python:seq:numpyarray"] sequence<int> IntSeq2;
    ["python:seq:memoryview:Custom.myIntSeq"] sequence<int> IntSeq3;

    ["python:seq:array"] sequence<long> LongSeq1;
    ["python:seq:numpyarray"] sequence<long> LongSeq2;
    ["python:seq:memoryview:Custom.myLongSeq"] sequence<long> LongSeq3;

    ["python:seq:array"] sequence<float> FloatSeq1;
    ["python:seq:numpyarray"] sequence<float> FloatSeq2;
    ["python:seq:memoryview:Custom.myFloatSeq"] sequence<float> FloatSeq3;

    ["python:seq:array"] sequence<double> DoubleSeq1;
    ["python:seq:numpyarray"] sequence<double> DoubleSeq2;
    ["python:seq:memoryview:Custom.myDoubleSeq"] sequence<double> DoubleSeq3;

    ["python:seq:memoryview:Custom.myComplex128Seq"] sequence<byte> Complex128Seq;

    struct S
    {
        ByteString b1;
        ["python:seq:list"] ByteString b2;
        ["python:seq:default"] ByteList b3;
        ByteList b4;
        StringList s1;
        ["python:seq:tuple"] StringList s2;
        StringTuple s3;
        ["python:seq:default"] StringTuple s4;
    }

    class C
    {
        ByteString b1;
        ["python:seq:list"] ByteString b2;
        ["python:seq:default"] ByteList b3;
        ByteList b4;
        StringList s1;
        ["python:seq:tuple"] StringList s2;
        StringTuple s3;
        ["python:seq:default"] StringTuple s4;
    }

    interface Custom
    {
        ByteString opByteString1(ByteString b1, out ByteString b2);
        ["python:seq:tuple"] ByteString opByteString2(["python:seq:list"] ByteString b1,
                                                      out ["python:seq:list"] ByteString b2);

        ByteList opByteList1(ByteList b1, out ByteList b2);
        ["python:seq:default"] ByteList opByteList2(["python:seq:tuple"] ByteList b1,
                                                    out ["python:seq:tuple"] ByteList b2);

        StringList opStringList1(StringList s1, out StringList s2);
        ["python:seq:tuple"] StringList opStringList2(["python:seq:tuple"] StringList s1,
                                                      out ["python:seq:tuple"] StringList s2);

        StringTuple opStringTuple1(StringTuple s1, out StringTuple s2);
        ["python:seq:list"] StringTuple opStringTuple2(["python:seq:list"] StringTuple s1,
                                                        out ["python:seq:default"] StringTuple s2);

        void sendS(S val);
        void sendC(C val);

        BoolSeq1 opBoolSeq(BoolSeq1 v1, BoolSeq2 v2, BoolSeq3 v3, out BoolSeq2 v4, out BoolSeq3 v5);
        ByteSeq1 opByteSeq(ByteSeq1 v1, ByteSeq2 v2, ByteSeq3 v3, out ByteSeq2 v4, out ByteSeq3 v5);
        ShortSeq1 opShortSeq(ShortSeq1 v1, ShortSeq2 v2, ShortSeq3 v3, out ShortSeq2 v4, out ShortSeq3 v5);
        IntSeq1 opIntSeq(IntSeq1 v1, IntSeq2 v2, IntSeq3 v3, out IntSeq2 v4, out IntSeq3 v5);
        LongSeq1 opLongSeq(LongSeq1 v1, LongSeq2 v2, LongSeq3 v3, out LongSeq2 v4, out LongSeq3 v5);
        FloatSeq1 opFloatSeq(FloatSeq1 v1, FloatSeq2 v2, FloatSeq3 v3, out FloatSeq2 v4, out FloatSeq3 v5);
        DoubleSeq1 opDoubleSeq(DoubleSeq1 v1, DoubleSeq2 v2, DoubleSeq3 v3, out DoubleSeq2 v4, out DoubleSeq3 v5);
        Complex128Seq opComplex128Seq(Complex128Seq v1);

        ["python:seq:memoryview:Custom.myMatrix3x3"] BoolSeq1 opBoolMatrix();
        ["python:seq:memoryview:Custom.myMatrix3x3"] ByteSeq1 opByteMatrix();
        ["python:seq:memoryview:Custom.myMatrix3x3"] ShortSeq1 opShortMatrix();
        ["python:seq:memoryview:Custom.myMatrix3x3"] IntSeq1 opIntMatrix();
        ["python:seq:memoryview:Custom.myMatrix3x3"] LongSeq1 opLongMatrix();
        ["python:seq:memoryview:Custom.myMatrix3x3"] FloatSeq1 opFloatMatrix();
        ["python:seq:memoryview:Custom.myMatrix3x3"] DoubleSeq1 opDoubleMatrix();

        ["python:seq:memoryview:Custom.myBogusArrayNotExistsFactory"] BoolSeq1 opBogusArrayNotExistsFactory();
        ["python:seq:memoryview:Custom.myBogusArrayThrowFactory"]BoolSeq1 opBogusArrayThrowFactory();
        ["python:seq:memoryview:Custom.myBogusArrayType"]BoolSeq1 opBogusArrayType();
        ["python:seq:memoryview:Custom.myBogusNumpyArrayType"]BoolSeq1 opBogusNumpyArrayType();
        ["python:seq:memoryview:Custom.myBogusArrayNoneFactory"]BoolSeq1 opBogusArrayNoneFactory();
        ["python:seq:memoryview:Custom.myBogusArraySignatureFactory"]BoolSeq1 opBogusArraySignatureFactory();
        ["python:seq:memoryview:Custom.myNoCallableFactory"]BoolSeq1 opBogusArrayNoCallableFactory();

        void shutdown();
    }
}
