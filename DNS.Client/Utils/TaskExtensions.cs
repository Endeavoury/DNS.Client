namespace DNS.Client;

internal static class TaskExtensions
{
    public static async Task WithCancellation(this Task task, CancellationToken cancellationToken)
    {
        if (task.IsCompleted || !cancellationToken.CanBeCanceled)
        {
            await task.ConfigureAwait(false);
            return;
        }

        var cancellation = new TaskCompletionSource<bool>(TaskCreationOptions.RunContinuationsAsynchronously);
        using (cancellationToken.Register(state => ((TaskCompletionSource<bool>)state!).TrySetResult(true), cancellation))
        {
            if (task != await Task.WhenAny(task, cancellation.Task).ConfigureAwait(false))
                throw new OperationCanceledException(cancellationToken);
        }
        await task.ConfigureAwait(false);
    }

    public static async Task<T> WithCancellation<T>(this Task<T> task, CancellationToken cancellationToken)
    {
        if (task.IsCompleted || !cancellationToken.CanBeCanceled)
            return await task.ConfigureAwait(false);

        var cancellation = new TaskCompletionSource<bool>(TaskCreationOptions.RunContinuationsAsynchronously);
        using (cancellationToken.Register(state => ((TaskCompletionSource<bool>)state!).TrySetResult(true), cancellation))
        {
            if (task != await Task.WhenAny(task, cancellation.Task).ConfigureAwait(false))
                throw new OperationCanceledException(cancellationToken);
        }
        return await task.ConfigureAwait(false);
    }
}
