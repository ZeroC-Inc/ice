<?php
//
// Copyright (c) ZeroC, Inc. All rights reserved.
//
//
// Ice version 3.7.10
//
// <auto-generated>
//
// Generated from file `ObjectAdapter.ice'
//
// Warning: do not edit this file.
//
// </auto-generated>
//

namespace
{
    require_once 'IceLocal/CommunicatorF.php';
    require_once 'IceLocal/ServantLocatorF.php';
    require_once 'IceLocal/Locator.php';
    require_once 'IceLocal/FacetMap.php';
    require_once 'IceLocal/Endpoint.php';
}

namespace Ice
{
    global $Ice__t_ObjectAdapter;
    interface ObjectAdapter
    {
        public function getName();
        public function getCommunicator();
        public function activate();
        public function hold();
        public function waitForHold();
        public function deactivate();
        public function waitForDeactivate();
        public function isDeactivated();
        public function destroy();
        public function add($servant, $id);
        public function addFacet($servant, $id, $facet);
        public function addWithUUID($servant);
        public function addFacetWithUUID($servant, $facet);
        public function addDefaultServant($servant, $category);
        public function remove($id);
        public function removeFacet($id, $facet);
        public function removeAllFacets($id);
        public function removeDefaultServant($category);
        public function find($id);
        public function findFacet($id, $facet);
        public function findAllFacets($id);
        public function findByProxy($proxy);
        public function addServantLocator($locator, $category);
        public function removeServantLocator($category);
        public function findServantLocator($category);
        public function findDefaultServant($category);
        public function createProxy($id);
        public function createDirectProxy($id);
        public function createIndirectProxy($id);
        public function setLocator($loc);
        public function getLocator();
        public function getEndpoints();
        public function refreshPublishedEndpoints();
        public function getPublishedEndpoints();
        public function setPublishedEndpoints($newEndpoints);
    }
    $Ice__t_ObjectAdapter = IcePHP_defineClass('::Ice::ObjectAdapter', '\\Ice\\ObjectAdapter', -1, false, true, null, null);
}
?>
