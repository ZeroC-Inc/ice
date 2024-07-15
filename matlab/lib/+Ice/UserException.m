classdef (Abstract) UserException < Ice.Exception
    % UserException   Summary of UserException
    %
    % Base class for exceptions defined in Slice.

    % Copyright (c) ZeroC, Inc. All rights reserved.

    methods(Abstract)
        ice_id(obj)
    end
    methods(Hidden=true)
        function obj = iceRead(obj, is)
            is.startException();
            obj = obj.iceReadImpl(is);
            is.endException(false);
        end
        function obj = icePostUnmarshal(obj)
            %
            % Overridden by subclasses that have class members.
            %
        end
    end
    methods(Abstract,Access=protected)
        obj = iceReadImpl(obj, is)
    end
end
