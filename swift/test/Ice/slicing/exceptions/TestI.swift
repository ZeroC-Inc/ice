// Copyright (c) ZeroC, Inc.

import Ice
import TestCommon

class TestI: TestIntf {
    var _helper: TestHelper

    init(_ helper: TestHelper) {
        _helper = helper
    }

    func baseAsBase(current _: Current) async throws {
        throw Base(b: "Base.b")
    }

    func unknownDerivedAsBase(current _: Current) async throws {
        throw UnknownDerived(b: "UnknownDerived.b", ud: "UnknownDerived.ud")
    }

    func knownDerivedAsBase(current _: Current) async throws {
        throw KnownDerived(b: "KnownDerived.b", kd: "KnownDerived.kd")
    }

    func knownDerivedAsKnownDerived(current _: Current) async throws {
        throw KnownDerived(b: "KnownDerived.b", kd: "KnownDerived.kd")
    }

    func unknownIntermediateAsBase(current _: Current) async throws {
        throw UnknownIntermediate(b: "UnknownIntermediate.b", ui: "UnknownIntermediate.ui")
    }

    func knownIntermediateAsBase(current _: Current) async throws {
        throw KnownIntermediate(b: "KnownIntermediate.b", ki: "KnownIntermediate.ki")
    }

    func knownMostDerivedAsBase(current _: Current) async throws {
        throw KnownMostDerived(
            b: "KnownMostDerived.b", ki: "KnownMostDerived.ki", kmd: "KnownMostDerived.kmd")
    }

    func knownIntermediateAsKnownIntermediate(current _: Current) async throws {
        throw KnownIntermediate(b: "KnownIntermediate.b", ki: "KnownIntermediate.ki")
    }

    func knownMostDerivedAsKnownIntermediate(current _: Current) async throws {
        throw KnownMostDerived(
            b: "KnownMostDerived.b", ki: "KnownMostDerived.ki", kmd: "KnownMostDerived.kmd")
    }

    func knownMostDerivedAsKnownMostDerived(current _: Current) async throws {
        throw KnownMostDerived(
            b: "KnownMostDerived.b",
            ki: "KnownMostDerived.ki",
            kmd: "KnownMostDerived.kmd")
    }

    func unknownMostDerived1AsBase(current _: Current) async throws {
        throw UnknownMostDerived1(
            b: "UnknownMostDerived1.b",
            ki: "UnknownMostDerived1.ki",
            umd1: "UnknownMostDerived1.umd1")
    }

    func unknownMostDerived1AsKnownIntermediate(current _: Current) async throws {
        throw UnknownMostDerived1(
            b: "UnknownMostDerived1.b",
            ki: "UnknownMostDerived1.ki",
            umd1: "UnknownMostDerived1.umd1")
    }

    func unknownMostDerived2AsBase(current _: Current) async throws {
        throw UnknownMostDerived2(
            b: "UnknownMostDerived2.b",
            ui: "UnknownMostDerived2.ui",
            umd2: "UnknownMostDerived2.umd2")
    }

    func shutdown(current: Current) async throws {
        current.adapter.getCommunicator().shutdown()
    }
}
