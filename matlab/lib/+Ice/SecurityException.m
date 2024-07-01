% SecurityException   Summary of SecurityException
%
% This exception indicates a failure in a security subsystem, such as the SSL transport.
%
% SecurityException Properties:
%   reason - The reason for the failure.

% Copyright (c) ZeroC, Inc. All rights reserved.
% Generated from LocalException.ice by slice2matlab version 3.7.10

classdef SecurityException < Ice.LocalException
    properties
        % reason - The reason for the failure.
        reason char
    end
    methods
        function obj = SecurityException(errID, msg, reason)
            if nargin <= 2
                reason = '';
            end
            if nargin == 0 || isempty(errID)
                errID = 'Ice:SecurityException';
            end
            if nargin < 2 || isempty(msg)
                msg = 'Ice.SecurityException';
            end
            obj = obj@Ice.LocalException(errID, msg);
            obj.reason = reason;
        end
    end
end
