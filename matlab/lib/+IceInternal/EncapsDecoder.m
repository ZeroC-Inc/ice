%
% Copyright (c) ZeroC, Inc. All rights reserved.
%

classdef (Abstract) EncapsDecoder < handle
    methods
        function obj = EncapsDecoder(is, encaps, sliceValues, valueFactoryManager, classResolver, classGraphDepthMax)
            obj.is = is;
            obj.encaps = encaps;
            obj.sliceValues = sliceValues;
            obj.valueFactoryManager = valueFactoryManager;
            obj.classResolver = classResolver;
            obj.classGraphDepthMax = classGraphDepthMax;
            obj.classGraphDepth = 0;
            obj.patchMap = {};
            obj.patchMapLength = 0;
            obj.unmarshaledMap = {};
            obj.typeIdMap = {};
            obj.valueList = {};
            obj.delayedPostUnmarshal = {};
        end

        function r = readOptional(~, ~, ~)
            r = false;
        end

        function readPendingValues(~)
        end

        function finish(obj)
            %
            % This is our opportunity for unmarshaled values to do some post processing after the initial round
            % of unmarshaling is complete.
            %
            if ~isempty(obj.delayedPostUnmarshal)
                %
                % First call icePostUnmarshal on every instance. This allows the generated code to finish its tasks.
                %
                for i = 1:length(obj.delayedPostUnmarshal)
                    v = obj.delayedPostUnmarshal{i};
                    v.icePostUnmarshal();
                end
                %
                % Then call ice_postUnmarshal on every instance. This is the application's interception point.
                %
                for i = 1:length(obj.delayedPostUnmarshal)
                    v = obj.delayedPostUnmarshal{i};
                    try
                        v.ice_postUnmarshal();
                    catch ex
                        msg = sprintf('exception raised by ice_postUnmarshal:\n%s', getReport(ex, 'extended'));
                        obj.is.getCommunicator().getLogger().warning(msg);
                    end
                end

                obj.delayedPostUnmarshal = {};
            end
        end
    end
    methods(Abstract)
        readValue(obj, cb)
        throwException(obj)
        startInstance(obj, sliceType)
        r = endInstance(obj, preserve)
        r = startSlice(obj)
        endSlice(obj)
        skipSlice(obj)
    end
    methods(Access=protected)
        function r = readTypeId(obj, isIndex)
            if isIndex
                index = obj.is.readSize();
                if index <= length(obj.typeIdMap)
                    r = obj.typeIdMap{index};
                else
                    throw(Ice.UnmarshalOutOfBoundsException());
                end
            else
                r = obj.is.readString();
                obj.typeIdMap{end + 1} = r;
            end
        end

        function r = newInstance(obj, typeId)
            %
            % Try to find a factory registered for the specific type.
            %
            userFactory = obj.valueFactoryManager.find(typeId);
            v = [];
            if ~isempty(userFactory)
                v = userFactory(typeId);
            end

            %
            % Last chance: ask the class resolver to find it.
            %
            if isempty(v)
                constructor = obj.classResolver.resolve(typeId);
                if ~isempty(constructor)
                    try
                        v = constructor(); % Invoke the constructor.
                    catch e
                        reason = sprintf('constructor failed for %s', typeId);
                        ex = Ice.NoValueFactoryException('', reason, reason, typeId);
                        ex.addCause(e);
                        throw(ex);
                    end
                end
            end

            r = v;
        end

        function addPatchEntry(obj, index, cb)
            assert(index > 0);
            %
            % Check if we have already unmarshalled the instance. If that's the case,
            % just invoke the callback and we're done.
            %
            if index <= length(obj.unmarshaledMap)
                v = obj.unmarshaledMap{index};
                if ~isempty(v)
                    cb(v);
                    return;
                end
            end

            %
            % Add patch entry if the instance isn't unmarshaled yet,
            % the callback will be called when the instance is
            % unmarshaled.
            %
            if index <= length(obj.patchMap)
                pl = obj.patchMap{index};
            else
                %
                % We have no outstanding instances to be patched for this
                % index, so make a new entry in the patch map.
                %
                pl = {};
                obj.patchMapLength = obj.patchMapLength + 1;
            end

            %
            % Append a patch entry for this instance.
            %
            e = IceInternal.PatchEntry();
            e.cb = cb;
            e.classGraphDepth = obj.classGraphDepth;
            pl{end + 1} = e;
            obj.patchMap{index} = pl;
        end

        function unmarshal(obj, index, v)
            %
            % Add the instance to the map of unmarshaled instances, this must
            % be done before reading the instances (for circular references).
            %
            obj.unmarshaledMap{index} = v;

            %
            % Read the instance.
            %
            v.iceRead(obj.is);

            if index <= length(obj.patchMap)
                %
                % Patch all instances now that the instance is unmarshaled.
                %
                l = obj.patchMap{index};

                %
                % Patch all pointers that refer to the instance.
                %
                for i = 1:length(l)
                    e = l{i};
                    e.cb(v);
                end

                %
                % Clear out the patch map for that index -- there is nothing left
                % to patch for that index for the time being.
                %
                obj.patchMap{index} = [];
                obj.patchMapLength = obj.patchMapLength - 1;
            end

            if obj.patchMapLength == 0 && isempty(obj.valueList)
                if v.iceDelayPostUnmarshal()
                    obj.delayedPostUnmarshal{end + 1} = v; % See finish()
                else
                    try
                        v.ice_postUnmarshal();
                    catch ex
                        msg = sprintf('exception raised by ice_postUnmarshal:\n%s', getReport(ex, 'extended'));
                        obj.is.getCommunicator().getLogger().warning(msg);
                    end
                end
            else
                obj.valueList{end + 1} = v;

                if obj.patchMapLength == 0
                    %
                    % Iterate over the instance list and invoke ice_postUnmarshal on
                    % each instance. We must do this after all instances have been
                    % unmarshaled in order to ensure that any instance data members
                    % have been properly patched.
                    %
                    for i = 1:length(obj.valueList)
                        p = obj.valueList{i};
                        if p.iceDelayPostUnmarshal()
                            obj.delayedPostUnmarshal{end + 1} = p; % See finish()
                        else
                            try
                                p.ice_postUnmarshal();
                            catch ex
                                msg = sprintf('exception raised by ice_postUnmarshal:\n%s', getReport(ex, 'extended'));
                                obj.is.getCommunicator().getLogger().warning(msg);
                            end
                        end
                    end
                    obj.valueList = {};
                end
            end
        end
    end
    properties(Access=protected)
        is
        encaps
        sliceValues
        valueFactoryManager
        classResolver
        classGraphDepth
        classGraphDepthMax
        patchMap
        patchMapLength
    end
    properties(Access=private)
        unmarshaledMap
        typeIdMap
        valueList
        delayedPostUnmarshal
    end
end
