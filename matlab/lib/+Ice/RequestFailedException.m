% RequestFailedException   Summary of RequestFailedException
%
% This exception is raised if a request failed. This exception, and all exceptions derived from
% RequestFailedException, are transmitted by the Ice protocol, even though they are declared
% local.
%
% RequestFailedException Properties:
%   id - The identity of the Ice Object to which the request was sent.
%   facet - The facet to which the request was sent.
%   operation - The operation name of the request.

% Copyright (c) ZeroC, Inc. All rights reserved.

classdef RequestFailedException < Ice.LocalException
    properties
        % id - The identity of the Ice Object to which the request was sent.
        id Ice.Identity
        % facet - The facet to which the request was sent.
        facet char
        % operation - The operation name of the request.
        operation char
    end
    methods
        function obj = RequestFailedException(id, facet, operation, errID, msg)
            assert(nargin == 5); % always created from a derived class
            obj = obj@Ice.LocalException(errID, msg);
            obj.id = id;
            obj.facet = facet;
            obj.operation = operation;
        end
    end
end
