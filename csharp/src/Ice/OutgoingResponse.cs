// Copyright (c) ZeroC, Inc.

#nullable enable

using Ice.Internal;

namespace Ice;

/// <summary>
/// Represents the response to an incoming request. It's returned by <see cref="Object.dispatchAsync" />.
/// </summary>
public sealed class OutgoingResponse
{
    /// <summary>
    /// Gets the exception ID of the response.
    /// </summary>
    /// <value>The exception ID of the response. It's null when replyStatus is <see cref="ReplyStatus.Ok" />. Otherwise,
    /// this ID is the Slice type ID of the exception marshaled into this response if this exception was defined in
    /// Slice or is derived from <see cref="LocalException" />. For other exceptions, this ID is the full name of
    /// the exception's type.</value>
    public string? exceptionId { get; }

    /// <summary>
    /// Gets the full details of the exception marshaled into the response.
    /// </summary>
    /// <value>The exception details, usually produced by calling <see cref="object.ToString" /> on the exception. It's
    /// null when replyStatus is <see cref="ReplyStatus.Ok" />.</value>
    public string? exceptionDetails { get; }

    /// <summary>
    /// Gets the output stream buffer of the response. This output stream should not be written to after construction.
    /// </summary>
    public OutputStream outputStream { get; }

    /// <summary>
    /// Gets the reply status of the response.
    /// </summary>
    public ReplyStatus replyStatus { get; }

    /// <summary>
    /// Gets the number of bytes in the response's payload.
    /// </summary>
    public int size => outputStream.isEmpty() ? 0 : outputStream.size() - Protocol.headerSize - 4;

    /// <summary>
    /// Constructs an OutgoingResponse object.
    /// </summary>
    /// <param name="replyStatus">The reply status.</param>
    /// <param name="exceptionId">The ID of the exception, when the response carries an exception.</param>
    /// <param name="exceptionDetails">The full details of the exception, when the response carries an exception.</param>
    /// <param name="outputStream">The output stream that holds the response.</param>
    public OutgoingResponse(
        ReplyStatus replyStatus,
        string? exceptionId,
        string? exceptionDetails,
        OutputStream outputStream)
    {
        this.replyStatus = replyStatus;
        this.exceptionId = exceptionId;
        this.exceptionDetails = exceptionDetails;
        this.outputStream = outputStream;
    }

    /// <summary>
    /// Constructs an OutgoingResponse object with the <see cref="ReplyStatus.Ok"/> status.
    /// </summary>
    /// <param name="outputStream">The output stream that holds the response.</param>
    public OutgoingResponse(OutputStream outputStream)
        : this(ReplyStatus.Ok, exceptionId: null, exceptionDetails: null, outputStream)
    {
    }
}
