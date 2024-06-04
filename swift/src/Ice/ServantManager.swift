//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

import PromiseKit

class ServantManager: Dispatcher {
    private let adapterName: String
    private let communicator: Communicator

    private var servantMapMap = [Identity: [String: Disp]]()
    private var defaultServantMap = [String: Disp]()
    private var locatorMap = [String: ServantLocator]()

    // This is used to distinguish between ObjectNotExistException and FacetNotExistException
    // when a servant is not found on a Swift Admin OA.
    private var adminId: Identity?

    private var mutex = Mutex()

    init(adapterName: String, communicator: Communicator) {
        self.adapterName = adapterName
        self.communicator = communicator
    }

    func addServant(servant: Disp, id ident: Identity, facet: String) throws {
        try mutex.sync {
            if var m = servantMapMap[ident] {
                if m[facet] != nil {
                    var id = communicator.identityToString(ident)
                    if !facet.isEmpty {
                        id += try " - f " + escapeString(string: facet, special: "", communicator: communicator)
                    }
                    throw AlreadyRegisteredException(kindOfObject: "servant", id: id)
                }
                m[facet] = servant
                servantMapMap[ident] = m
            } else {
                servantMapMap[ident] = [facet: servant]
            }
        }
    }

    func addDefaultServant(servant: Disp, category: String) throws {
        try mutex.sync {
            guard defaultServantMap[category] == nil else {
                throw AlreadyRegisteredException(kindOfObject: "default servant", id: category)
            }

            defaultServantMap[category] = servant
        }
    }

    func removeServant(id ident: Identity, facet: String) throws -> Disp {
        return try mutex.sync {
            guard var m = servantMapMap[ident], let obj = m.removeValue(forKey: facet) else {
                var id = communicator.identityToString(ident)
                if !facet.isEmpty {
                    id += try " - f " + escapeString(string: facet, special: "", communicator: communicator)
                }
                throw NotRegisteredException(kindOfObject: "servant", id: id)
            }

            if m.isEmpty {
                servantMapMap.removeValue(forKey: ident)
            } else {
                servantMapMap[ident] = m
            }
            return obj
        }
    }

    func removeDefaultServant(category: String) throws -> Disp {
        return try mutex.sync {
            guard let obj = defaultServantMap.removeValue(forKey: category) else {
                throw NotRegisteredException(kindOfObject: "default servant", id: category)
            }

            return obj
        }
    }

    func removeAllFacets(id: Identity) throws -> FacetMap {
        return try mutex.sync {
            guard let m = servantMapMap.removeValue(forKey: id) else {
                throw NotRegisteredException(kindOfObject: "servant", id: identityToString(id: id))
            }

            return m
        }
    }

    func findServant(id: Identity, facet: String) -> Disp? {
        return mutex.sync {
            guard let m = servantMapMap[id] else {
                guard let obj = defaultServantMap[id.category] else {
                    return defaultServantMap[""]
                }

                return obj
            }

            return m[facet]
        }
    }

    func findDefaultServant(category: String) -> Disp? {
        return mutex.sync {
            defaultServantMap[category]
        }
    }

    func findAllFacets(id: Identity) -> FacetMap {
        return mutex.sync {
            guard let m = servantMapMap[id] else {
                return FacetMap()
            }

            return m
        }
    }

    func hasServant(id: Identity) -> Bool {
        return mutex.sync {
            servantMapMap[id] != nil
        }
    }

    func addServantLocator(locator: ServantLocator, category: String) throws {
        return try mutex.sync {
            guard locatorMap[category] == nil else {
                let id = try escapeString(string: category, special: "", communicator: communicator)
                throw AlreadyRegisteredException(kindOfObject: "servant locator", id: id)
            }

            locatorMap[category] = locator
        }
    }

    func removeServantLocator(category: String) throws -> ServantLocator {
        return try mutex.sync {
            guard let l = locatorMap.removeValue(forKey: category) else {
                let id = try escapeString(string: category, special: "", communicator: communicator)
                throw NotRegisteredException(kindOfObject: "servant locator", id: id)
            }

            return l
        }
    }

    func findServantLocator(category: String) -> ServantLocator? {
        return mutex.sync {
            locatorMap[category]
        }
    }

    func setAdminId(_ id: Identity) {
        mutex.sync {
            adminId = id
        }
    }

    func isAdminId(_ id: Identity) -> Bool {
        return mutex.sync {
            adminId == id
        }
    }

    func destroy() {
        var m = [String: ServantLocator]()
        mutex.sync {
            servantMapMap.removeAll()
            defaultServantMap.removeAll()
            m = locatorMap
            locatorMap.removeAll()
        }

        for (category, locator) in m {
            locator.deactivate(category)
        }
    }

    func dispatch(_ request: IncomingRequest) -> Promise<OutgoingResponse> {
        let current = request.current
        var servant = findServant(id: current.id, facet: current.facet)

        if let servant = servant {
            // the simple, common path
            return servant.dispatch(request)
        }

        // Else, check servant locators

        var locator = findServantLocator(category: current.id.category)
        if locator == nil, !current.id.category.isEmpty {
            locator = findServantLocator(category: "")
        }

        if let locator = locator {
            do {
                var cookie: AnyObject?
                (servant, cookie) = try locator.locate(current)

                if let servant = servant {
                    // If locator returned a servant, we must execute finished once no matter what.
                    return servant.dispatch(request).map(on: nil) { response in
                        do {
                            try locator.finished(curr: current, servant: servant, cookie: cookie)
                        } catch {
                            // Can't return a rejected promise here; otherwise recover will execute finished a second
                            // time.
                            return current.makeOutgoingResponse(error: error)
                        }
                        return response
                    }.recover(on: nil) { error in
                        // This can throw and return a rejected promise.
                        try locator.finished(curr: current, servant: servant, cookie: cookie)
                        return Promise<OutgoingResponse>(error: error)
                    }
                }
            } catch {
                return Promise(error: error)
            }
        }

        if hasServant(id: current.id) || isAdminId(current.id) {
            return Promise(error: FacetNotExistException())
        } else {
            return Promise(error: ObjectNotExistException())
        }
    }
}
