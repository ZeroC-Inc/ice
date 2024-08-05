%
% Copyright (c) ZeroC, Inc. All rights reserved.
%

classdef AllTests
    methods(Static)
        function allTests(helper)
            communicator = helper.communicator();

            fprintf('testing enum... ');

            members = enumeration('classdef_.break_.bitand');
            for i = 0:int32(classdef_.break_.bitand.LAST) - 1
                % Every enumerator should be escaped and therefore have a trailing underscore.
                name = char(members(i + 1));
                assert(strcmp(name(length(name)), '_'));
                % Ensure ice_getValue is generated correctly.
                assert(members(i + 1) == classdef_.break_.bitand.ice_getValue(i));
            end

            fprintf('ok\n');

            fprintf('testing struct... ');

            s = classdef_.break_.bitor();
            assert(s.case_ == classdef_.break_.bitand.catch_);
            assert(s.continue_ == 1);
            assert(s.eq_ == 2);
            % Exercise the marshaling code.
            os = Ice.OutputStream(communicator);
            classdef_.break_.bitor.ice_write(os, s);
            is = Ice.InputStream(communicator, os.getEncoding(), os.finished());
            s2 = classdef_.break_.bitor.ice_read(is);
            assert(isequal(s, s2));

            fprintf('ok\n');

            fprintf('testing class... ');

            c = classdef_.break_.logical();
            assert(c.else_ == classdef_.break_.bitand.enumeration_);
            assert(c.for_.case_ == classdef_.break_.bitand.catch_);
            assert(c.for_.continue_ == 1);
            assert(c.for_.eq_ == 2);
            assert(c.int64 == true);
            % Exercise the marshaling code.
            os = Ice.OutputStream(communicator);
            os.writeValue(c);
            os.writePendingValues();
            is = Ice.InputStream(communicator, os.getEncoding(), os.finished());
            v = IceInternal.ValueHolder();
            is.readValue(@v.set, 'classdef_.break_.logical');
            is.readPendingValues();
            assert(v.value.else_ == c.else_);
            assert(v.value.for_.case_ == c.for_.case_);
            assert(v.value.for_.continue_ == c.for_.continue_);
            assert(v.value.for_.eq_ == c.for_.eq_);
            assert(v.value.int64 == c.int64);

            d = classdef_.break_.xor();
            assert(d.else_ == classdef_.break_.bitand.enumeration_);
            assert(d.for_.case_ == classdef_.break_.bitand.catch_);
            assert(d.for_.continue_ == 1);
            assert(d.for_.eq_ == 2);
            assert(d.int64 == true);
            assert(d.return_ == 1);
            % Exercise the marshaling code.
            os = Ice.OutputStream(communicator);
            os.writeValue(d);
            os.writePendingValues();
            is = Ice.InputStream(communicator, os.getEncoding(), os.finished());
            v = IceInternal.ValueHolder();
            is.readValue(@v.set, 'classdef_.break_.xor');
            is.readPendingValues();
            assert(v.value.else_ == d.else_);
            assert(v.value.for_.case_ == d.for_.case_);
            assert(v.value.for_.continue_ == d.for_.continue_);
            assert(v.value.for_.eq_ == d.for_.eq_);
            assert(v.value.int64 == d.int64);
            assert(v.value.return_ == d.return_);

            p = classdef_.break_.properties_();
            assert(p.while_ == 1);
            assert(p.delete == 2);
            assert(p.if_ == 2);
            assert(isempty(p.spmd_));
            assert(isempty(p.otherwise_));
            p.catch_ = d;
            % Exercise the marshaling code.
            os = Ice.OutputStream(communicator);
            os.writeValue(p);
            os.writePendingValues();
            is = Ice.InputStream(communicator, os.getEncoding(), os.finished());
            v = IceInternal.ValueHolder();
            is.readValue(@v.set, 'classdef_.break_.properties_');
            is.readPendingValues();
            assert(v.value.while_ == p.while_);
            assert(v.value.delete == p.delete);
            assert(v.value.if_ == p.if_);

            fprintf('ok\n');

            fprintf('testing exception... ');

            e = classdef_.break_.persistent_();
            assert(isempty(e.identifier_));
            assert(isempty(e.message_));
            assert(isempty(e.stack_));
            assert(isempty(e.cause_));
            assert(isempty(e.type_));
            assert(isempty(e.end_));

            g = classdef_.break_.global_();
            assert(isempty(g.identifier_));
            assert(isempty(g.message_));
            assert(isempty(g.stack_));
            assert(isempty(g.cause_));
            assert(isempty(g.type_));
            assert(isempty(g.end_));
            assert(isempty(g.enumeration_));

            fprintf('ok\n');

            fprintf('testing interface... ');

            assert(exist('classdef_.break_.elseifPrx', 'class') ~= 0);
            m = methods('classdef_.break_.elseifPrx');
            assert(ismember('events_', m));
            assert(ismember('eventsAsync', m));
            assert(ismember('function_', m));
            assert(ismember('functionAsync', m));
            assert(ismember('delete_', m));
            assert(ismember('deleteAsync', m));
            assert(ismember('checkedCast_', m));
            assert(ismember('checkedCastAsync', m));

            fprintf('ok\n');

            fprintf('testing constant... ');

            assert(classdef_.break_.methods_.value == 1);

            fprintf('ok\n');
        end
    end
end
