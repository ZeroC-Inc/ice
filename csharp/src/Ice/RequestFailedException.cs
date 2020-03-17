//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;

namespace Ice
{
    // The partial classes below extend the partial exception classes generated by the Slice compiler.

    public partial class RequestFailedException
    {
        protected virtual string DefaultMessage
            => Facet.Length == 0 ? $"request for operation `{Operation}' on Ice object `{Id}' failed"
                : $"request for operation `{Operation}' on Ice object `{Id}' with facet `{Facet}' failed";

        public override string Message => HasCustomMessage ? base.Message : DefaultMessage;

        public RequestFailedException(string message, Identity id, string facet, string operation,
                                      Exception innerException)
            : base(message, innerException)
        {
            Id = id;
            Facet = facet;
            Operation = operation;
        }
    }

    public partial class ObjectNotExistException
    {
        protected override string DefaultMessage
            => $"could not find servant for Ice object `{Id}'" + (Facet.Length > 0 ? $" with facet `{Facet}'" : "") +
                $" while attempting to call operation `{Operation}'";
    }

    public partial class OperationNotExistException
    {
        protected override string DefaultMessage
            => $"could not find operation `{Operation}' for Ice object `{Id}'" +
                (Facet.Length > 0 ? $" with facet `{Facet}'" : "");
    }

    public partial class UnhandledException
    {
        public UnhandledException(Identity id, string facet, string operation, Exception innerException)
            : base(CustomMessage(id, facet, operation, innerException), id, facet, operation, innerException)
        {
        }

        private static string CustomMessage(Identity id, string facet, string operation, Exception innerException)
        {
            string message = $"unhandled exception while calling `{operation}' on Ice object `{id}'";
            if (facet.Length > 0)
            {
                message += $" with facet `{facet}'";
            }
            // Since this custom message will be sent "over the wire", we don't include the stack trace of the inner
            // exception since it can include sensitive information. The stack trace is of course available locally.
            message += $":\n{innerException.Message}";
            return message;
        }
    }
}
