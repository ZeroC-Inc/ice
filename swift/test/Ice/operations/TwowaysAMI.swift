// Copyright (c) ZeroC, Inc.

import Ice
import TestCommon

func twowaysAMI(_ helper: TestHelper, _ p: MyClassPrx) async throws {
    func test(_ value: Bool, file: String = #file, line: Int = #line) throws {
        try helper.test(value, file: file, line: line)
    }

    let communicator = helper.communicator()

    try await p.ice_pingAsync()

    do {
        let ret = try await p.ice_isAAsync(id: ice_staticId(MyClassPrx.self))
        try test(ret)
    }

    do {
        let ids = try await p.ice_idsAsync()
        try test(ids.count == 3)
    }

    do {
        let id = try await p.ice_idAsync()
        try test(id == ice_staticId(MyDerivedClassPrx.self))
    }

    try await p.opVoidAsync()

    do {
        let (returnValue, p3) = try await p.opByteAsync(p1: 0xFF, p2: 0x0F)
        try test(returnValue == 0xFF)
        try test(p3 == 0xF0)
    }

    do {
        let (returnValue, p3) = try await p.opBoolAsync(p1: true, p2: false)
        try test(!returnValue)
        try test(p3)
    }

    do {
        let (returnValue, p4, p5, p6) = try await p.opShortIntLongAsync(p1: 10, p2: 11, p3: 12)
        try test(p4 == 10)
        try test(p5 == 11)
        try test(p6 == 12)
        try test(returnValue == 12)
    }

    do {
        let (returnValue, p3, p4) = try await p.opFloatDoubleAsync(p1: 3.14, p2: 1.1e10)
        try test(p3 == 3.14)
        try test(p4 == 1.1e10)
        try test(returnValue == 1.1e10)
    }

    do {
        let (returnValue, p3) = try await p.opStringAsync(p1: "hello", p2: "world")
        try test(p3 == "world hello")
        try test(returnValue == "hello world")
    }

    do {
        let (returnValue, p2) = try await p.opMyEnumAsync(MyEnum.enum2)
        try test(p2 == MyEnum.enum2)
        try test(returnValue == MyEnum.enum3)
    }

    do {
        let (returnValue, p2, p3) = try await p.opMyClassAsync(p)

        try test(p2!.ice_getIdentity() == Ice.stringToIdentity("test"))
        try test(p3!.ice_getIdentity() == Ice.stringToIdentity("noSuchIdentity"))
        try test(returnValue!.ice_getIdentity() == Ice.stringToIdentity("test"))

        //
        // We can't do the callbacks below in connection serialization mode.
        //
        if communicator.getProperties().getPropertyAsInt("Ice.ThreadPool.Client.Serialize") == 0 {
            try returnValue!.opVoid()
            try p2!.opVoid()
            do {
                try p3!.opVoid()
                try test(false)
            } catch is Ice.ObjectNotExistException {}
        }
    }

    do {
        let (returnValue, p3) = try await p.opStructAsync(
            p1: Structure(p: p, e: MyEnum.enum3, s: AnotherStruct(s: "abc")),
            p2: Structure(p: nil, e: MyEnum.enum2, s: AnotherStruct(s: "def")))

        try test(returnValue.p == nil)
        try test(returnValue.e == MyEnum.enum2)
        try test(returnValue.s.s == "def")
        try test(p3.e == MyEnum.enum3)
        try test(p3.s.s == "a new string")
        //
        // We can't do the callbacks below in connection serialization mode.
        //
        if communicator.getProperties().getPropertyAsInt("Ice.ThreadPool.Client.Serialize") == 0 {
            try p3.p!.opVoid()
        }
    }

    do {
        let (returnValue, p3) = try await p.opByteSAsync(
            p1: ByteSeq([0x01, 0x11, 0x12, 0x22]),
            p2: ByteSeq([0xF1, 0xF2, 0xF3, 0xF4]))

        try test(p3.count == 4)
        try test(p3[0] == 0x22)
        try test(p3[1] == 0x12)
        try test(p3[2] == 0x11)
        try test(p3[3] == 0x01)
        try test(returnValue.count == 8)
        try test(returnValue[0] == 0x01)
        try test(returnValue[1] == 0x11)
        try test(returnValue[2] == 0x12)
        try test(returnValue[3] == 0x22)
        try test(returnValue[4] == 0xF1)
        try test(returnValue[5] == 0xF2)
        try test(returnValue[6] == 0xF3)
        try test(returnValue[7] == 0xF4)
    }

    do {
        let (returnValue, p3) = try await p.opBoolSAsync(p1: [true, true, false], p2: [false])

        try test(p3.count == 4)
        try test(p3[0])
        try test(p3[1])
        try test(!p3[2])
        try test(!p3[3])
        try test(returnValue.count == 3)
        try test(!returnValue[0])
        try test(returnValue[1])
        try test(returnValue[2])
    }

    do {
        let (returnValue, p4, p5, p6) = try await p.opShortIntLongSAsync(
            p1: [1, 2, 3],
            p2: [5, 6, 7, 8],
            p3: [10, 30, 20])

        try test(p4.count == 3)
        try test(p4[0] == 1)
        try test(p4[1] == 2)
        try test(p4[2] == 3)
        try test(p5.count == 4)
        try test(p5[0] == 8)
        try test(p5[1] == 7)
        try test(p5[2] == 6)
        try test(p5[3] == 5)
        try test(p6.count == 6)
        try test(p6[0] == 10)
        try test(p6[1] == 30)
        try test(p6[2] == 20)
        try test(p6[3] == 10)
        try test(p6[4] == 30)
        try test(p6[5] == 20)
        try test(returnValue.count == 3)
        try test(returnValue[0] == 10)
        try test(returnValue[1] == 30)
        try test(returnValue[2] == 20)
    }

    do {
        let (returnValue, p3, p4) = try await p.opFloatDoubleSAsync(
            p1: [3.14, 1.11],
            p2: [1.1e10, 1.2e10, 1.3e10])

        try test(p3.count == 2)
        try test(p3[0] == 3.14)
        try test(p3[1] == 1.11)
        try test(p4.count == 3)
        try test(p4[0] == 1.3e10)
        try test(p4[1] == 1.2e10)
        try test(p4[2] == 1.1e10)
        try test(returnValue.count == 5)
        try test(returnValue[0] == 1.1e10)
        try test(returnValue[1] == 1.2e10)
        try test(returnValue[2] == 1.3e10)
        try test(Float(returnValue[3]) == 3.14)
        try test(Float(returnValue[4]) == 1.11)
    }

    do {
        let (returnValue, p3) = try await p.opStringSAsync(
            p1: ["abc", "de", "fghi"],
            p2: ["xyz"])

        try test(p3.count == 4)
        try test(p3[0] == "abc")
        try test(p3[1] == "de")
        try test(p3[2] == "fghi")
        try test(p3[3] == "xyz")
        try test(returnValue.count == 3)
        try test(returnValue[0] == "fghi")
        try test(returnValue[1] == "de")
        try test(returnValue[2] == "abc")
    }

    do {
        let (returnValue, p3) = try await p.opByteSSAsync(
            p1: [ByteSeq([0x01, 0x11, 0x12]), ByteSeq([0xFF])],
            p2: [ByteSeq([0x0E]), ByteSeq([0xF2, 0xF1])])

        try test(p3.count == 2)
        try test(p3[0].count == 1)
        try test(p3[0][0] == 0xFF)
        try test(p3[1].count == 3)
        try test(p3[1][0] == 0x01)
        try test(p3[1][1] == 0x11)
        try test(p3[1][2] == 0x12)
        try test(returnValue.count == 4)
        try test(returnValue[0].count == 3)
        try test(returnValue[0][0] == 0x01)
        try test(returnValue[0][1] == 0x11)
        try test(returnValue[0][2] == 0x12)
        try test(returnValue[1].count == 1)
        try test(returnValue[1][0] == 0xFF)
        try test(returnValue[2].count == 1)
        try test(returnValue[2][0] == 0x0E)
        try test(returnValue[3].count == 2)
        try test(returnValue[3][0] == 0xF2)
        try test(returnValue[3][1] == 0xF1)
    }

    do {
        let (returnValue, p3) = try await p.opBoolSSAsync(
            p1: [[true], [false], [true, true]],
            p2: [[false, false, true]])

        try test(p3.count == 4)
        try test(p3[0].count == 1)
        try test(p3[0][0])
        try test(p3[1].count == 1)
        try test(!p3[1][0])
        try test(p3[2].count == 2)
        try test(p3[2][0])
        try test(p3[2][1])
        try test(p3[3].count == 3)
        try test(!p3[3][0])
        try test(!p3[3][1])
        try test(p3[3][2])
        try test(returnValue.count == 3)
        try test(returnValue[0].count == 2)
        try test(returnValue[0][0])
        try test(returnValue[0][1])
        try test(returnValue[1].count == 1)
        try test(!returnValue[1][0])
        try test(returnValue[2].count == 1)
        try test(returnValue[2][0])
    }

    do {
        let (returnValue, p4, p5, p6) = try await p.opShortIntLongSSAsync(
            p1: [[1, 2, 5], [13], []],
            p2: [[24, 98], [42]],
            p3: [[496, 1729]])

        try test(returnValue.count == 1)
        try test(returnValue[0].count == 2)
        try test(returnValue[0][0] == 496)
        try test(returnValue[0][1] == 1729)
        try test(p4.count == 3)
        try test(p4[0].count == 3)
        try test(p4[0][0] == 1)
        try test(p4[0][1] == 2)
        try test(p4[0][2] == 5)
        try test(p4[1].count == 1)
        try test(p4[1][0] == 13)
        try test(p4[2].count == 0)
        try test(p5.count == 2)
        try test(p5[0].count == 1)
        try test(p5[0][0] == 42)
        try test(p5[1].count == 2)
        try test(p5[1][0] == 24)
        try test(p5[1][1] == 98)
        try test(p6.count == 2)
        try test(p6[0].count == 2)
        try test(p6[0][0] == 496)
        try test(p6[0][1] == 1729)
        try test(p6[1].count == 2)
        try test(p6[1][0] == 496)
        try test(p6[1][1] == 1729)
    }

    do {
        let (returnValue, p3, p4) = try await p.opFloatDoubleSSAsync(
            p1: [[3.14], [1.11], []],
            p2: [[1.1e10, 1.2e10, 1.3e10]])

        try test(p3.count == 3)
        try test(p3[0].count == 1)
        try test(p3[0][0] == 3.14)
        try test(p3[1].count == 1)
        try test(p3[1][0] == 1.11)
        try test(p3[2].count == 0)
        try test(p4.count == 1)
        try test(p4[0].count == 3)
        try test(p4[0][0] == 1.1e10)
        try test(p4[0][1] == 1.2e10)
        try test(p4[0][2] == 1.3e10)
        try test(returnValue.count == 2)
        try test(returnValue[0].count == 3)
        try test(returnValue[0][0] == 1.1e10)
        try test(returnValue[0][1] == 1.2e10)
        try test(returnValue[0][2] == 1.3e10)
        try test(returnValue[1].count == 3)
        try test(returnValue[1][0] == 1.1e10)
        try test(returnValue[1][1] == 1.2e10)
        try test(returnValue[1][2] == 1.3e10)
    }

    do {
        let (returnValue, p3) = try await p.opStringSSAsync(
            p1: [["abc"], ["de", "fghi"]],
            p2: [[], [], ["xyz"]])

        try test(p3.count == 5)
        try test(p3[0].count == 1)
        try test(p3[0][0] == "abc")
        try test(p3[1].count == 2)
        try test(p3[1][0] == "de")
        try test(p3[1][1] == "fghi")
        try test(p3[2].count == 0)
        try test(p3[3].count == 0)
        try test(p3[4].count == 1)
        try test(p3[4][0] == "xyz")
        try test(returnValue.count == 3)
        try test(returnValue[0].count == 1)
        try test(returnValue[0][0] == "xyz")
        try test(returnValue[1].count == 0)
        try test(returnValue[2].count == 0)
    }

    do {
        let (returnValue, p3) = try await p.opStringSSSAsync(
            p1: [[["abc", "de"], ["xyz"]], [["hello"]]],
            p2: [[["", ""], ["abcd"]], [[""]], []])

        try test(p3.count == 5)
        try test(p3[0].count == 2)
        try test(p3[0][0].count == 2)
        try test(p3[0][1].count == 1)
        try test(p3[1].count == 1)
        try test(p3[1][0].count == 1)
        try test(p3[2].count == 2)
        try test(p3[2][0].count == 2)
        try test(p3[2][1].count == 1)
        try test(p3[3].count == 1)
        try test(p3[3][0].count == 1)
        try test(p3[4].count == 0)
        try test(p3[0][0][0] == "abc")
        try test(p3[0][0][1] == "de")
        try test(p3[0][1][0] == "xyz")
        try test(p3[1][0][0] == "hello")
        try test(p3[2][0][0] == "")
        try test(p3[2][0][1] == "")
        try test(p3[2][1][0] == "abcd")
        try test(p3[3][0][0] == "")

        try test(returnValue.count == 3)
        try test(returnValue[0].count == 0)
        try test(returnValue[1].count == 1)
        try test(returnValue[1][0].count == 1)
        try test(returnValue[2].count == 2)
        try test(returnValue[2][0].count == 2)
        try test(returnValue[2][1].count == 1)
        try test(returnValue[1][0][0] == "")
        try test(returnValue[2][0][0] == "")
        try test(returnValue[2][0][1] == "")
        try test(returnValue[2][1][0] == "abcd")
    }

    do {
        let (returnValue, p3) = try await p.opByteBoolDAsync(
            p1: [10: true, 100: false],
            p2: [10: true, 11: false, 101: true])

        try test(p3 == [10: true, 100: false])
        try test(returnValue == [10: true, 11: false, 100: false, 101: true])
    }

    do {
        let (returnValue, p3) = try await p.opShortIntDAsync(
            p1: [110: -1, 1100: 123_123],
            p2: [110: -1, 111: -100, 1101: 0])

        try test(p3 == [110: -1, 1100: 123_123])
        try test(returnValue == [110: -1, 111: -100, 1100: 123_123, 1101: 0])
    }

    do {
        let (returnValue, p3) = try await p.opLongFloatDAsync(
            p1: [999_999_110: -1.1, 999_999_111: 123_123.2],
            p2: [999_999_110: -1.1, 999_999_120: -100.4, 999_999_130: 0.5])

        try test(p3 == [999_999_110: -1.1, 999_999_111: 123_123.2])
        try test(
            returnValue == [
                999_999_110: -1.1, 999_999_120: -100.4, 999_999_111: 123_123.2, 999_999_130: 0.5,
            ])
    }

    do {
        let (returnValue, p3) = try await p.opStringStringDAsync(
            p1: ["foo": "abc -1.1", "bar": "abc 123123.2"],
            p2: ["foo": "abc -1.1", "FOO": "abc -100.4", "BAR": "abc 0.5"])

        try test(p3 == ["foo": "abc -1.1", "bar": "abc 123123.2"])
        try test(
            returnValue == [
                "foo": "abc -1.1",
                "FOO": "abc -100.4",
                "bar": "abc 123123.2",
                "BAR": "abc 0.5",
            ])
    }

    do {
        let (returnValue, p3) = try await p.opStringMyEnumDAsync(
            p1: ["abc": MyEnum.enum1, "": MyEnum.enum2],
            p2: ["abc": MyEnum.enum1, "qwerty": MyEnum.enum3, "Hello!!": MyEnum.enum2])

        try test(p3 == ["abc": MyEnum.enum1, "": MyEnum.enum2])
        try test(
            returnValue == [
                "abc": MyEnum.enum1,
                "qwerty": MyEnum.enum3,
                "": MyEnum.enum2,
                "Hello!!": MyEnum.enum2,
            ])
    }

    do {
        let (returnValue, p3) = try await p.opMyEnumStringDAsync(
            p1: [MyEnum.enum1: "abc"],
            p2: [MyEnum.enum2: "Hello!!", MyEnum.enum3: "qwerty"])

        try test(p3 == [MyEnum.enum1: "abc"])
        try test(
            returnValue == [
                MyEnum.enum1: "abc",
                MyEnum.enum2: "Hello!!",
                MyEnum.enum3: "qwerty",
            ])
    }

    do {
        let (returnValue, p3) = try await p.opMyStructMyEnumDAsync(
            p1: [
                MyStruct(i: 1, j: 1): MyEnum.enum1,
                MyStruct(i: 1, j: 2): MyEnum.enum2,
            ],
            p2: [
                MyStruct(i: 1, j: 1): MyEnum.enum1,
                MyStruct(i: 2, j: 2): MyEnum.enum3,
                MyStruct(i: 2, j: 3): MyEnum.enum2,
            ])

        try test(
            p3 == [
                MyStruct(i: 1, j: 1): MyEnum.enum1,
                MyStruct(i: 1, j: 2): MyEnum.enum2,
            ])

        try test(
            returnValue == [
                MyStruct(i: 1, j: 1): MyEnum.enum1,
                MyStruct(i: 1, j: 2): MyEnum.enum2,
                MyStruct(i: 2, j: 2): MyEnum.enum3,
                MyStruct(i: 2, j: 3): MyEnum.enum2,
            ])
    }

    do {
        let (returnValue, p3) = try await p.opByteBoolDSAsync(
            p1: [[10: true, 100: false], [10: true, 11: false, 101: true]],
            p2: [[100: false, 101: false]])
        try test(
            returnValue == [
                [10: true, 11: false, 101: true],
                [10: true, 100: false],
            ])
        try test(
            p3 == [
                [100: false, 101: false],
                [10: true, 100: false],
                [10: true, 11: false, 101: true],
            ])
    }

    do {
        let (returnValue, p3) = try await p.opShortIntDSAsync(
            p1: [[110: -1, 1100: 123_123], [110: -1, 111: -100, 1101: 0]],
            p2: [[100: -1001]])

        try test(returnValue == [[110: -1, 111: -100, 1101: 0], [110: -1, 1100: 123_123]])
        try test(
            p3 == [
                [100: -1001],
                [110: -1, 1100: 123_123],
                [110: -1, 111: -100, 1101: 0],
            ])
    }

    do {
        let (returnValue, p3) = try await p.opLongFloatDSAsync(
            p1: [
                [999_999_110: -1.1, 999_999_111: 123_123.2],
                [999_999_110: -1.1, 999_999_120: -100.4, 999_999_130: 0.5],
            ],
            p2: [[999_999_140: 3.14]])

        try test(
            returnValue == [
                [999_999_110: -1.1, 999_999_120: -100.4, 999_999_130: 0.5],
                [999_999_110: -1.1, 999_999_111: 123_123.2],
            ])
        try test(
            p3 == [
                [999_999_140: 3.14],
                [999_999_110: -1.1, 999_999_111: 123_123.2],
                [999_999_110: -1.1, 999_999_120: -100.4, 999_999_130: 0.5],
            ])
    }

    do {
        let (returnValue, p3) = try await p.opStringStringDSAsync(
            p1: [
                ["foo": "abc -1.1", "bar": "abc 123123.2"],
                ["foo": "abc -1.1", "FOO": "abc -100.4", "BAR": "abc 0.5"],
            ],
            p2: [["f00": "ABC -3.14"]])

        try test(
            returnValue == [
                ["foo": "abc -1.1", "FOO": "abc -100.4", "BAR": "abc 0.5"],
                ["foo": "abc -1.1", "bar": "abc 123123.2"],
            ])
        try test(
            p3 == [
                ["f00": "ABC -3.14"],
                ["foo": "abc -1.1", "bar": "abc 123123.2"],
                ["foo": "abc -1.1", "FOO": "abc -100.4", "BAR": "abc 0.5"],
            ])
    }

    do {
        let (returnValue, p3) = try await p.opStringMyEnumDSAsync(
            p1: [
                ["abc": MyEnum.enum1, "": MyEnum.enum2],
                ["abc": MyEnum.enum1, "qwerty": MyEnum.enum3, "Hello!!": MyEnum.enum2],
            ],
            p2: [["Goodbye": MyEnum.enum1]])

        try test(
            returnValue == [
                ["abc": MyEnum.enum1, "qwerty": MyEnum.enum3, "Hello!!": MyEnum.enum2],
                ["abc": MyEnum.enum1, "": MyEnum.enum2],
            ])
        try test(
            p3 == [
                ["Goodbye": MyEnum.enum1],
                ["abc": MyEnum.enum1, "": MyEnum.enum2],
                ["abc": MyEnum.enum1, "qwerty": MyEnum.enum3, "Hello!!": MyEnum.enum2],
            ])
    }

    do {
        let (returnValue, p3) = try await p.opMyEnumStringDSAsync(
            p1: [[MyEnum.enum1: "abc"], [MyEnum.enum2: "Hello!!", MyEnum.enum3: "qwerty"]],
            p2: [[MyEnum.enum1: "Goodbye"]])

        try test(
            returnValue == [
                [MyEnum.enum2: "Hello!!", MyEnum.enum3: "qwerty"],
                [MyEnum.enum1: "abc"],
            ])

        try test(
            p3 == [
                [MyEnum.enum1: "Goodbye"],
                [MyEnum.enum1: "abc"],
                [MyEnum.enum2: "Hello!!", MyEnum.enum3: "qwerty"],
            ])
    }

    do {
        let (returnValue, p3) = try await p.opMyStructMyEnumDSAsync(
            p1: [
                [
                    MyStruct(i: 1, j: 1): MyEnum.enum1,
                    MyStruct(i: 1, j: 2): MyEnum.enum2,
                ],
                [
                    MyStruct(i: 1, j: 1): MyEnum.enum1,
                    MyStruct(i: 2, j: 2): MyEnum.enum3,
                    MyStruct(i: 2, j: 3): MyEnum.enum2,
                ],
            ],
            p2: [[MyStruct(i: 2, j: 3): MyEnum.enum3]])

        try test(
            returnValue == [
                [
                    MyStruct(i: 1, j: 1): MyEnum.enum1,
                    MyStruct(i: 2, j: 2): MyEnum.enum3,
                    MyStruct(i: 2, j: 3): MyEnum.enum2,
                ],
                [
                    MyStruct(i: 1, j: 1): MyEnum.enum1,
                    MyStruct(i: 1, j: 2): MyEnum.enum2,
                ],
            ])

        try test(
            p3 == [
                [MyStruct(i: 2, j: 3): MyEnum.enum3],
                [
                    MyStruct(i: 1, j: 1): MyEnum.enum1,
                    MyStruct(i: 1, j: 2): MyEnum.enum2,
                ],
                [
                    MyStruct(i: 1, j: 1): MyEnum.enum1,
                    MyStruct(i: 2, j: 2): MyEnum.enum3,
                    MyStruct(i: 2, j: 3): MyEnum.enum2,
                ],
            ])
    }

    do {
        let (returnValue, p3) = try await p.opByteByteSDAsync(
            p1: [
                0x01: ByteSeq([0x01, 0x11]),
                0x22: ByteSeq([0x12]),
            ],
            p2: [0xF1: ByteSeq([0xF2, 0xF3])])

        try test(p3 == [0xF1: ByteSeq([0xF2, 0xF3])])
        try test(
            returnValue == [
                0x01: ByteSeq([0x01, 0x11]),
                0x22: ByteSeq([0x12]),
                0xF1: ByteSeq([0xF2, 0xF3]),
            ])
    }

    do {
        let (returnValue, p3) = try await p.opBoolBoolSDAsync(
            p1: [false: [true, false], true: [false, true, true]],
            p2: [false: [true, false]])

        try test(p3 == [false: [true, false]])
        try test(returnValue == [false: [true, false], true: [false, true, true]])
    }

    do {
        let (returnValue, p3) = try await p.opShortShortSDAsync(
            p1: [1: [1, 2, 3], 2: [4, 5]],
            p2: [4: [6, 7]])
        try test(p3 == [4: [6, 7]])
        try test(returnValue == [1: [1, 2, 3], 2: [4, 5], 4: [6, 7]])
    }

    do {
        let (returnValue, p3) = try await p.opIntIntSDAsync(
            p1: [100: [100, 200, 300], 200: [400, 500]],
            p2: [400: [600, 700]])
        try test(p3 == [400: [600, 700]])
        try test(returnValue == [100: [100, 200, 300], 200: [400, 500], 400: [600, 700]])
    }

    do {
        let (returnValue, p3) = try await p.opLongLongSDAsync(
            p1: [
                999_999_990: [999_999_110, 999_999_111, 999_999_110],
                999_999_991: [999_999_120, 999_999_130],
            ],
            p2: [999_999_992: [999_999_110, 999_999_120]])

        try test(p3 == [999_999_992: [999_999_110, 999_999_120]])
        try test(
            returnValue == [
                999_999_990: [999_999_110, 999_999_111, 999_999_110],
                999_999_991: [999_999_120, 999_999_130],
                999_999_992: [999_999_110, 999_999_120],
            ])
    }

    do {
        let (returnValue, p3) = try await p.opStringFloatSDAsync(
            p1: [
                "abc": [-1.1, 123_123.2, 100.0],
                "ABC": [42.24, -1.61],
            ],
            p2: ["aBc": [-3.14, 3.14]])

        try test(p3 == ["aBc": [-3.14, 3.14]])
        try test(
            returnValue == [
                "abc": [-1.1, 123_123.2, 100.0],
                "ABC": [42.24, -1.61],
                "aBc": [-3.14, 3.14],
            ])
    }

    do {
        let (returnValue, p3) = try await p.opStringDoubleSDAsync(
            p1: [
                "Hello!!": [1.1e10, 1.2e10, 1.3e10],
                "Goodbye": [1.4e10, 1.5e10],
            ],
            p2: ["": [1.6e10, 1.7e10]])

        try test(p3 == ["": [1.6e10, 1.7e10]])
        try test(
            returnValue == [
                "Hello!!": [1.1e10, 1.2e10, 1.3e10],
                "Goodbye": [1.4e10, 1.5e10],
                "": [1.6e10, 1.7e10],
            ])
    }

    do {
        let (returnValue, p3) = try await p.opStringStringSDAsync(
            p1: [
                "abc": ["abc", "de", "fghi"],
                "def": ["xyz", "or"],
            ],
            p2: ["ghi": ["and", "xor"]])

        try test(p3 == ["ghi": ["and", "xor"]])
        try test(
            returnValue == [
                "abc": ["abc", "de", "fghi"],
                "def": ["xyz", "or"],
                "ghi": ["and", "xor"],
            ])
    }

    do {
        let (returnValue, p3) = try await p.opMyEnumMyEnumSDAsync(
            p1: [
                MyEnum.enum3: [MyEnum.enum1, MyEnum.enum1, MyEnum.enum2],
                MyEnum.enum2: [MyEnum.enum1, MyEnum.enum2],
            ],
            p2: [MyEnum.enum1: [MyEnum.enum3, MyEnum.enum3]])

        try test(p3 == [MyEnum.enum1: [MyEnum.enum3, MyEnum.enum3]])
        try test(
            returnValue == [
                MyEnum.enum3: [MyEnum.enum1, MyEnum.enum1, MyEnum.enum2],
                MyEnum.enum2: [MyEnum.enum1, MyEnum.enum2],
                MyEnum.enum1: [MyEnum.enum3, MyEnum.enum3],
            ])

    }

    do {
        let lengths: [Int32] = [0, 1, 2, 126, 127, 128, 129, 253, 254, 255, 256, 257, 1000]

        for i in 0..<lengths.count {
            var s = [Int32]()
            for j in 0..<lengths[i] {
                s.append(j)
            }

            let r = try await p.opIntSAsync(s)
            for j in 0..<r.count {
                try test(r[j] == -j)
            }
        }
    }

    do {
        let ctx = ["one": "ONE", "two": "TWO", "three": "THREE"]
        try test(p.ice_getContext().count == 0)

        do {
            let r = try await p.opContextAsync()
            try test(r != ctx)
        }

        try test(p.ice_getContext().count == 0)
        do {
            let r = try await p.opContextAsync(context: ctx)
            try test(r == ctx)
        }

        var p2 = try checkedCast(prx: p.ice_context(ctx), type: MyClassPrx.self)!
        try test(p2.ice_getContext() == ctx)
        do {
            let r = try await p2.opContextAsync()
            try test(r == ctx)
        }

        p2 = try checkedCast(prx: p.ice_context(ctx), type: MyClassPrx.self)!
        do {
            let r = try await p2.opContextAsync(context: ctx)
            try test(r == ctx)
        }
    }

    //
    // Test implicit context propagation with async result
    //
    let conn = try await p.ice_getConnection()
    if conn != nil {
        var initData = Ice.InitializationData()
        let properties = communicator.getProperties().clone()
        properties.setProperty(key: "Ice.ImplicitContext", value: "Shared")
        initData.properties = properties
        let ic = try helper.initialize(initData)

        var ctx = ["one": "ONE", "two": "TWO", "three": "THREE"]

        var p3 = try uncheckedCast(
            prx: ic.stringToProxy("test:\(helper.getTestEndpoint(num: 0))")!,
            type: MyClassPrx.self)

        ic.getImplicitContext().setContext(ctx)
        try test(ic.getImplicitContext().getContext() == ctx)

        do {
            let r = try await p3.opContextAsync()
            try test(r == ctx)
        }

        _ = ic.getImplicitContext().put(key: "zero", value: "ZERO")
        ctx = ic.getImplicitContext().getContext()
        do {
            let r = try await p3.opContextAsync()
            try test(r == ctx)
        }

        let prxContext = ["one": "UN", "four": "QUATRE"]
        var combined = prxContext
        for (key, value) in ctx where combined[key] == nil {
            combined[key] = value
        }
        try test(combined["one"] == "UN")

        p3 = uncheckedCast(prx: p3.ice_context(prxContext), type: MyClassPrx.self)

        ic.getImplicitContext().setContext(Ice.Context())
        do {
            let r = try await p3.opContextAsync()
            try test(r == prxContext)
        }

        ic.getImplicitContext().setContext(ctx)
        do {
            let r = try await p3.opContextAsync()
            try test(r == combined)
        }
        ic.destroy()
    }
}
