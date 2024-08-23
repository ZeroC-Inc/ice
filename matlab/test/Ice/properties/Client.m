%
% Copyright (c) ZeroC, Inc. All rights reserved.
%

function client(args)
    if ~libisloaded('ice')
        loadlibrary('ice', @iceproto)
    end

    fprintf("testing load properties exception... ");
    props = Ice.createProperties();
    try
        props.load('./config/xxxx.config')
        assert(false)
    catch ex
        % We don't define Ice.FileException in MATLAB. This allows us to test the conversion of
        % unmapped C++ local exceptions to Ice.LocalException.
        assert(strcmp(ex.identifier, 'Ice:FileException'));
        assert(isa(ex, 'Ice.LocalException'));
    end
    fprintf('ok\n');

    fprintf('testing ice properties with set default values...');
    props = Ice.createProperties();
    toStringMode = props.getIceProperty('Ice.ToStringMode');
    assert(strcmp(toStringMode, 'Unicode'));
    closeTimeout = props.getIcePropertyAsInt('Ice.Connection.Client.CloseTimeout');
    assert(closeTimeout == 10);
    retryIntervals = props.getIcePropertyAsList('Ice.RetryIntervals');
    assert(length(retryIntervals) == 1);
    assert(strcmp(retryIntervals{1}, '0'));
    fprintf('ok\n');

    fprintf('testing ice properties with unset default values...');
    stringValue = props.getIceProperty('Ice.Admin.Router');
    assert(strcmp(stringValue, ''));
    intValue = props.getIcePropertyAsInt('Ice.Admin.Router');
    assert(intValue == 0);
    listValue = props.getIcePropertyAsList('Ice.Admin.Router');
    assert(length(listValue) == 0);
    fprintf('ok\n');

    fprintf('testing that getting an unknown ice property throws an exception...');
    try
        props.getIceProperty('Ice.UnknownProperty');
        assert(false);
    catch ex
        assert(strcmp(ex.identifier, 'Ice:CppException'));
    end
    fprintf('ok\n');

    clear('classes'); % Avoids conflicts with tests that define the same symbols.
end
