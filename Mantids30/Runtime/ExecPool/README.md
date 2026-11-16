# JSON-lines execution pool (POSIX)

Link `Mantids30_Runtime_ExecPool` and include `<Mantids30/Runtime/ExecPool/pool.h>`.
Each worker owns one persistent child launched through `Core::Helpers::AppSpawn`.
A shared `Safe::Queue` dispatches requests to one consumer per worker.
JSON parsing, serialization and member insertion use `DataFormat_JSON` helpers;
the request and response values remain `boost::json::value`.

```cpp
using Mantids30::Runtime::ExecPool;

ExecPool::Config config;
config.command = {"/usr/bin/php", {"/path/to/worker.php"}};
config.workers = 4;
config.appLog = std::make_shared<Mantids30::Runtime::Logs::AppLog>(); // Optional diagnostics
config.requestTimeout = std::chrono::milliseconds(5000);
config.failureBehavior = ExecPool::FailureBehavior::RESTART_WORKER;
config.stderrFilePrefix = "/var/log/myapp/worker";
config.stderrFileSuffix = ".log"; // worker-1.log through worker-4.log, append mode

ExecPool pool(config);
std::string startupError;
if (!pool.start(&startupError))
{
    throw std::runtime_error(startupError);
}
ExecPool::Request request(boost::json::object{{"message", "hello"}});
if (request.exec(pool) == ExecPool::Error::SUCCESS)
{
    const auto response = request.output();
    // Consume response here.
}
const auto statistics = pool.statistics();
pool.stop();
```

Example `worker.php`:

```php
<?php
while (($line = fgets(STDIN)) !== false) {
    $request = json_decode($line, true, 512, JSON_THROW_ON_ERROR);
    $response = [
        '_exec_id' => $request['_exec_id'],
        'message' => strtoupper($request['message'])
    ];
    fwrite(STDOUT, json_encode($response, JSON_THROW_ON_ERROR) . "\n");
    fflush(STDOUT);
}
```

The child must read and reply with exactly one JSON value per newline, flushing
each response. Only protocol messages belong on stdout; diagnostic output goes
to stderr. Requests for a given child execute serially. Different workers execute
concurrently; dispatch order does not guarantee completion order.

By default, the pool injects an increasing unsigned `_exec_id` into a copy of the
input object. The child must return it unchanged as a JSON integer. Missing or
incorrect IDs, malformed JSON, extra output lines and oversized lines cause the process
to be killed and recycled. `idField` changes the key; an empty key disables IDs
and permits any JSON value. Caller input is preserved; output retains the ID.
Trailing bytes after the JSON value within the same response line are accepted,
following the shared `DataFormat_JSON::parse` behavior.

`ExecPool::Request` is one-shot; `exec(pool)` enqueues and waits on its completion semaphore
(a condition variable with a protected completion flag). `output()` and `error()`
are synchronized. Requests rejected before enqueue return `NOT_RUNNING` or
`QUEUE_FULL`. The deadline includes queue time, nonblocking writes and response
reads. Timed-out callers return promptly; a worker that already started that
request kills its process, preventing late output from contaminating another
request. Expired requests still waiting in the queue are skipped. A timeout cannot
undo side effects already performed by the child; requests are never retried.

`RESTART_WORKER` replaces failed or unexpectedly exited children, including while
idle, with a configurable `restartDelay` (100 ms by default). `STOP_POOL` stops
all workers and cancels queued requests after a process/protocol failure. Invalid
input types and oversized inputs are request errors and do not stop the pool. Failure details are
available through the `ExecPool::Error` enum. `start(&error)` launches all children before
accepting requests and rolls back if any launch fails. Calling `start()` after
`stop()` or a pool failure joins the old workers before starting a new generation.

`stop()` cancels pending work, kills direct children and joins workers. It is also
called by the destructor. The pool must outlive concurrent `exec()` calls. Child
programs should not daemonize or leave subprocesses behind: termination applies
to direct children. `start()`/`stop()` serialize lifecycle changes. Configuration
is copied at construction and remains immutable.

Statistics accumulate across restarts: accepted/completed/succeeded/failed
requests, timeouts, rejections, process starts/restarts/failures, total request
latency in microseconds, current active workers and queue depth. A worker is
active from dequeue until completion cleanup. `completed = succeeded + failed`;
rejections are counted separately. Queue depth may briefly include expired work
awaiting dequeue. Total latency includes queue time.

An empty stderr prefix inherits the parent's stderr. Otherwise each child opens
`prefix + "-" + workerNumber + suffix` with `O_APPEND`, preserving logs across
restarts and subsequent pool starts. The caller creates the parent directory;
file-open errors make spawning fail. Environment overrides use `NAME=VALUE`.
Executable paths are passed directly to AppSpawn, without a shell or PATH lookup.

Set `Config::appLog` to an existing shared `Runtime::Logs::AppLog` to receive
pool diagnostics under the `ExecPool` module. Null (the default) disables logging.
The pool retains shared ownership; configure the logger before starting workers.
Admission, input and timeout issues use WARNING; process, protocol and internal
failures use ERROR. Startup and worker failures include operation context and
worker numbers where applicable (zero denotes a pool/request event). Normal
shutdown cancellation is silent. Request/response payloads are never logged.
Logging is best effort and does not change request results.
