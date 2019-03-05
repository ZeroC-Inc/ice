// **********************************************************************
//
// Copyright (c) 2003-2018 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

public protocol Value: AnyObject {
    init()
    func ice_id() -> String
    func ice_preMarshal()
    func ice_postUnmarshal()
    func ice_getSlicedData() -> SlicedData?

    func _iceReadImpl(from: InputStream) throws
    func _iceWriteImpl(to: OutputStream)

    static func ice_staticId() -> String
}

public class Foo: Value {
    public func _iceReadImpl(from: InputStream) throws {

    }

    public func _iceWriteImpl(to: OutputStream) {

    }

    public static func ice_staticId() -> String {
        return "::FOO:::"
    }

    public required init() {

    }
}

public extension InputStream {
    public func read() throws -> Foo? {
        return nil
    }
}

public extension Value {
    func ice_id() -> String {
        return Self.ice_staticId()
    }

    func ice_preMarshal() {}

    func ice_postUnmarshal() {}

    func ice_getSlicedData() -> SlicedData? {
        return nil
    }

    func _iceRead(from ins: InputStream) throws {
        ins.startValue()
        try _iceReadImpl(from: ins)
        _ = try ins.endValue(preserve: false)

//        let m: Foo? = try ins.read() { m = }
    }

    func _iceWrite(to os: OutputStream) {
        os.startValue(data: nil)
        _iceWriteImpl(to: os)
        os.endValue()
    }
}

//public extension Optional where Wrapped: Value {
//    init(_ callback: ((ValueType?) -> Void)?) {
//        self = Optional.none
//    }
//}
