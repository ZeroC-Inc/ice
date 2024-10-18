// Copyright (c) ZeroC, Inc.

import Foundation

/// Creates a new class instance from the Slice type ID of a class.
///
/// - parameter _: `String` The Slice type ID of a class.
///
/// - returns: `Value?` - The class instance created for the given type ID, or nil if the factory is unable to find
/// the corresponding class.
public typealias ValueFactory = (String) -> Value?

/// A value factory manager maintains a collection of value factories. An application can supply a custom
/// implementation during communicator initialization, otherwise Ice provides a default implementation.
public protocol ValueFactoryManager: AnyObject {
    /// Add a value factory. Attempting to add a factory with an id for which a factory is already registered throws
    /// AlreadyRegisteredException.
    /// When unmarshaling an Ice value, the Ice run time reads the most-derived type id off the wire and attempts to
    /// create an instance of the type using a factory. If no instance is created, either because no factory was found,
    /// or because all factories returned nil, the behavior of the Ice run time depends on the format with which the
    /// value was marshaled:
    /// If the value uses the "sliced" format, Ice ascends the class hierarchy until it finds a type that is recognized
    /// by a factory, or it reaches the least-derived type. If no factory is found that can create an instance, the run
    /// time throws a MarshalException.
    /// If the value uses the "compact" format, Ice immediately raises a MarshalException.
    /// The following order is used to locate a factory for a type:
    ///
    /// The Ice run-time looks for a factory registered specifically for the type.
    /// If no instance has been created, the Ice run-time looks for the default factory, which is registered with
    /// an empty type id.
    /// If no instance has been created by any of the preceding steps, the Ice run-time looks for a factory that
    /// may have been statically generated by the language mapping for non-abstract classes.
    ///
    /// - parameter factory: `@escaping ValueFactory` The factory to add.
    ///
    /// - parameter id: `String` The type id for which the factory can create instances, or an empty string for
    /// the default factory.
    func add(factory: @escaping ValueFactory, id: String) throws

    /// Find an value factory registered with this communicator.
    ///
    /// - parameter _: `String` The type id for which the factory can create instances, or an empty string for
    /// the default factory.
    ///
    /// - returns: `ValueFactory?` - The value factory, or null if no value factory was found for the given id.
    func find(_ id: String) -> ValueFactory?
}
