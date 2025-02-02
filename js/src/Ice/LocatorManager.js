// Copyright (c) ZeroC, Inc.

import { HashMap } from "./HashMap.js";
import { LocatorInfo } from "./LocatorInfo.js";
import { Ice as Ice_Locator } from "./Locator.js";
const { LocatorPrx } = Ice_Locator;
import { LocatorTable } from "./LocatorTable.js";

export class LocatorManager {
    constructor(properties) {
        this._background = properties.getIcePropertyAsInt("Ice.BackgroundLocatorCacheUpdates") > 0;
        this._table = new HashMap(HashMap.compareEquals); // Map<Ice.LocatorPrx, LocatorInfo>
        this._locatorTables = new HashMap(HashMap.compareEquals); // Map<Ice.Identity, LocatorTable>
    }

    destroy() {
        for (const locator of this._table.values()) {
            locator.destroy();
        }
        this._table.clear();
        this._locatorTables.clear();
    }

    //
    // Returns locator info for a given locator. Automatically creates
    // the locator info if it doesn't exist yet.
    //
    find(loc) {
        if (loc === null) {
            return null;
        }

        //
        // The locator can't be located.
        //
        const locator = LocatorPrx.uncheckedCast(loc.ice_locator(null));

        //
        // TODO: reap unused locator info objects?
        //
        let info = this._table.get(locator);
        if (info === undefined) {
            //
            // Rely on locator identity for the adapter table. We want to
            // have only one table per locator (not one per locator
            // proxy).
            //
            let table = this._locatorTables.get(locator.ice_getIdentity());
            if (table === undefined) {
                table = new LocatorTable();
                this._locatorTables.set(locator.ice_getIdentity(), table);
            }

            info = new LocatorInfo(locator, table, this._background);
            this._table.set(locator, info);
        }

        return info;
    }
}
