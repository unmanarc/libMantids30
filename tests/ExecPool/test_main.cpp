#include <Mantids30/Runtime/ExecPool/pool.h>
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <fstream>
#include <future>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <signal.h>

using Mantids30::Runtime::ExecPool;
using Config = ExecPool::Config;
using Error = ExecPool::Error;
using FailureBehavior = ExecPool::FailureBehavior;
using Request = ExecPool::Request;
using namespace std::chrono_literals;

static void require(bool value, const char *message)
{
    if (!value)
    {
        throw std::runtime_error(message);
    }
}

static int child()
{
    std::cerr << "worker-start\n";
    if (std::getenv("MANTIDS30_TEST_NO_READ") != nullptr)
    {
        std::this_thread::sleep_for(10s);
        return 0;
    }
    std::string line;
    while (std::getline(std::cin, line))
    {
        auto input = boost::json::parse(line);
        auto *action = input.is_object() ? input.as_object().if_contains("action") : nullptr;
        const std::string mode = action ? std::string(action->as_string()) : "echo";
        if (mode == "die")
        {
            return 3;
        }
        if (mode == "hang")
        {
            std::this_thread::sleep_for(10s);
        }
        if (mode == "delay")
        {
            std::this_thread::sleep_for(100ms);
        }
        if (mode == "bad")
        {
            std::cout << "{bad json}\n" << std::flush;
            continue;
        }
        if (mode == "trailing")
        {
            std::cout << boost::json::serialize(input) << " garbage\n" << std::flush;
            continue;
        }
        if (mode == "oversized")
        {
            std::cout << std::string(8192, 'x') << std::flush;
            continue;
        }
        if (mode == "wrong")
        {
            input.as_object()["_exec_id"] = 0;
        }
        if (mode == "double")
        {
            std::cout << boost::json::serialize(input) << '\n';
        }
        if (mode == "missing-id")
        {
            input.as_object().erase("_exec_id");
        }
        if (mode == "string-id")
        {
            input.as_object()["_exec_id"] = "1";
        }
        if (mode == "pid")
        {
            input.as_object()["pid"] = static_cast<int64_t>(::getpid());
        }
        if (mode == "exit-after-response")
        {
            std::cout << boost::json::serialize(input) << '\n' << std::flush;
            return 0;
        }
        if (mode == "partial")
        {
            const auto response = boost::json::serialize(input);
            std::cout << response.substr(0, response.size() / 2) << std::flush;
            std::this_thread::sleep_for(10ms);
            std::cout << response.substr(response.size() / 2) << '\n' << std::flush;
        }
        else
        {
            std::cout << boost::json::serialize(input) << '\n' << std::flush;
        }
    }
    return 0;
}

static Config config(const char *executable)
{
    Config result;
    result.command = {executable, {"--child"}};
    result.workers = 2;
    result.requestTimeout = 700ms;
    return result;
}

static Error run(ExecPool &pool, const char *action)
{
    Request request(boost::json::object{{"action", action}});
    return request.exec(pool);
}

static void waitActive(ExecPool &pool, size_t count)
{
    const auto until = std::chrono::steady_clock::now() + 2s;
    while (pool.statistics().active < count && std::chrono::steady_clock::now() < until)
    {
        std::this_thread::sleep_for(1ms);
    }
    require(pool.statistics().active >= count, "workers did not become active");
}

static void tests(const char *executable)
{
    namespace JSON = Mantids30::DataFormat::JSON;
    const uint64_t maximum = std::numeric_limits<uint64_t>::max();
    const boost::json::value ids = boost::json::object{
        {"negative", -1}, {"decimal", 1.5}, {"string", "1"},
        {"zero", 0}, {"positive", 42}, {"maximum", maximum}};
    require(JSON::ASUINT64(ids, "negative", 99) == 99, "negative unsigned fallback");
    require(JSON::ASUINT64(ids, "decimal", 99) == 99, "decimal unsigned fallback");
    require(JSON::ASUINT64(ids, "string", 99) == 99, "string unsigned fallback");
    require(JSON::ASUINT64(ids, "missing", 99) == 99, "missing unsigned fallback");
    require(JSON::ASUINT64(ids, "zero", 99) == 0, "unsigned zero");
    require(JSON::ASUINT64(ids, "positive", 99) == 42, "positive signed conversion");
    require(JSON::ASUINT64(ids, "maximum", 99) == maximum, "full unsigned range");

    // Preserve the original AppSpawn output-only use case.
    {
        Mantids30::Core::Helpers::AppSpawn app;
        require(app.setExec("/bin/sh"), "legacy executable");
        app.addArgument("-c");
        app.addArgument("printf stdout; printf stderr >&2");
        require(app.spawnProcess(true, true), "legacy spawn");
        std::string output, errors;
        while (true)
        {
            const auto ready = app.pollResponse();
            if (ready.empty())
            {
                break;
            }
            for (const auto stream : ready)
            {
                char buffer[128];
                const auto count = app.read(stream, buffer, sizeof(buffer));
                require(count >= 0, "legacy read");
                (stream == STDOUT_FILENO ? output : errors).append(buffer, static_cast<size_t>(count));
            }
        }
        app.waitUntilProcessEnds();
        require(app.hasExited() && output == "stdout" && errors == "stderr", "legacy output capture");
    }
    auto cfg = config(executable);
    ExecPool pool(cfg);
    require(run(pool, "echo") == Error::NOT_RUNNING, "submission before start");
    require(pool.start(), "start");
    require(pool.start(), "idempotent start");
    std::vector<std::future<void>> clients;
    for (int index = 0; index < 24; ++index)
    {
        clients.push_back(std::async(std::launch::async, [&, index]
        {
            Request request(boost::json::object{{"value", index}});
            const auto result = request.exec(pool);
            if (result != Error::SUCCESS)
            {
                throw std::runtime_error("concurrent request: " + std::to_string(static_cast<int>(result)));
            }
            require(request.output().as_object().at("value").as_int64() == index, "wrong response");
            require(request.exec(pool) == Error::ALREADY_EXECUTED, "request reuse");
        }));
    }
    for (auto &client : clients)
    {
        client.get();
    }
    require(run(pool, "partial") == Error::SUCCESS, "fragmented response");
    require(run(pool, "bad") == Error::INVALID_JSON, "invalid JSON");
    require(run(pool, "trailing") == Error::SUCCESS, "trailing data after JSON accepted");
    require(run(pool, "wrong") == Error::DESYNCHRONIZED, "wrong ID");
    require(run(pool, "missing-id") == Error::DESYNCHRONIZED, "missing ID");
    require(run(pool, "string-id") == Error::DESYNCHRONIZED, "non-integer ID");
    require(run(pool, "die") == Error::PROCESS_EXITED, "child exit / SIGPIPE survival");
    require(run(pool, "hang") == Error::TIMEOUT, "timeout");
    require(run(pool, "echo") == Error::SUCCESS, "recovery");
    Request scalar(123);
    require(scalar.exec(pool) == Error::INVALID_INPUT, "object required with IDs");
    pool.stop();
    const auto stats = pool.statistics();
    require(stats.submitted == stats.completed && stats.active == 0 && stats.queued == 0, "completion accounting");
    require(stats.failed == 7 && stats.timedOut == 1 && stats.rejected == 1, "failure accounting");
    require(stats.processesRestarted >= 1, "restart statistics");
    require(pool.start(), "start after stop");
    require(run(pool, "echo") == Error::SUCCESS, "request after restart");
    pool.stop();

    cfg = config(executable);
    cfg.workers = 1;
    cfg.maxQueuedRequests = 1;
    cfg.requestTimeout = 3s;
    ExecPool bounded(cfg);
    require(bounded.start(), "bounded start");
    auto active = std::async(std::launch::async, [&] { return run(bounded, "hang"); });
    waitActive(bounded, 1);
    auto queued = std::async(std::launch::async, [&] { return run(bounded, "echo"); });
    const auto until = std::chrono::steady_clock::now() + 1s;
    while (bounded.statistics().queued != 1 && std::chrono::steady_clock::now() < until)
    {
        std::this_thread::sleep_for(1ms);
    }
    require(bounded.statistics().queued == 1, "queued request");
    require(run(bounded, "echo") == Error::QUEUE_FULL, "queue capacity");
    const auto beforeStop = std::chrono::steady_clock::now();
    bounded.stop();
    require(active.get() == Error::STOPPED && queued.get() == Error::STOPPED, "stop cancels requests");
    require(std::chrono::steady_clock::now() - beforeStop < 1s, "bounded stop latency");

    cfg = config(executable);
    cfg.workers = 1;
    cfg.failureBehavior = FailureBehavior::STOP_POOL;
    ExecPool fatal(cfg);
    require(fatal.start(), "fatal pool start");
    require(run(fatal, "wrong") == Error::DESYNCHRONIZED, "fatal failure cause");
    require(!fatal.isRunning(), "stop policy");
    require(run(fatal, "echo") == Error::NOT_RUNNING, "fatal rejects new work");
    fatal.stop();

    cfg = config(executable);
    cfg.workers = 1;
    cfg.idField.clear();
    ExecPool noIds(cfg);
    require(noIds.start(), "no ID start");
    Request scalarEcho(42);
    require(scalarEcho.exec(noIds) == Error::SUCCESS && scalarEcho.output().as_int64() == 42, "scalar without IDs");
    Request nullEcho(nullptr);
    require(nullEcho.exec(noIds) == Error::SUCCESS && nullEcho.output().is_null(), "null without IDs");
    Request booleanEcho(true);
    require(booleanEcho.exec(noIds) == Error::SUCCESS && booleanEcho.output().as_bool(), "boolean without IDs");
    noIds.stop();

    cfg = config(executable);
    cfg.workers = 1;
    cfg.maxLineBytes = 100;
    ExecPool limited(cfg);
    require(limited.start(), "limited start");
    Request tooLong(boost::json::object{{"payload", std::string(200, 'x')}});
    require(tooLong.exec(limited) == Error::INPUT_TOO_LONG, "bounded input line");
    require(run(limited, "oversized") == Error::LINE_TOO_LONG, "bounded response line");
    limited.stop();

    cfg = config(executable);
    cfg.workers = 1;
    cfg.environment = {"MANTIDS30_TEST_NO_READ=1"};
    cfg.maxLineBytes = 4 * 1024 * 1024;
    cfg.requestTimeout = 150ms;
    ExecPool blockedWriter(cfg);
    require(blockedWriter.start(), "blocked writer start");
    Request huge(boost::json::object{{"payload", std::string(2 * 1024 * 1024, 'x')}});
    require(huge.exec(blockedWriter) == Error::TIMEOUT, "write timeout");
    blockedWriter.stop();

    cfg = config(executable);
    cfg.workers = 1;
    ExecPool idle(cfg);
    require(idle.start(), "idle pool start");
    Request pidRequest(boost::json::object{{"action", "pid"}});
    require(pidRequest.exec(idle) == Error::SUCCESS, "get idle PID");
    require(::kill(static_cast<pid_t>(pidRequest.output().as_object().at("pid").as_int64()), SIGKILL) == 0, "kill idle child");
    const auto restartUntil = std::chrono::steady_clock::now() + 2s;
    while (idle.statistics().processesRestarted == 0 && std::chrono::steady_clock::now() < restartUntil)
    {
        std::this_thread::sleep_for(1ms);
    }
    require(idle.statistics().processesRestarted == 1, "idle child restarted");
    require(run(idle, "echo") == Error::SUCCESS, "idle restart request");
    require(run(idle, "exit-after-response") == Error::SUCCESS, "drain response before EOF");
    idle.stop();

    char directory[] = "/tmp/mantids30-execpool-XXXXXX";
    require(::mkdtemp(directory) != nullptr, "temporary log directory");
    cfg = config(executable);
    cfg.workers = 1;
    cfg.stderrFilePrefix = std::string(directory) + "/worker";
    cfg.stderrFileSuffix = ".stderr";
    ExecPool logged(cfg);
    require(logged.start(), "logged start");
    require(run(logged, "bad") == Error::INVALID_JSON, "logged failure");
    require(run(logged, "echo") == Error::SUCCESS, "logged recovery");
    logged.stop();
    std::ifstream log(cfg.stderrFilePrefix + "-1.stderr");
    std::string contents((std::istreambuf_iterator<char>(log)), std::istreambuf_iterator<char>());
    require(contents == "worker-start\nworker-start\n", "stderr appended across restart");
    log.close();
    auto multipleLogs = cfg;
    multipleLogs.workers = 2;
    ExecPool multiLogged(multipleLogs);
    require(multiLogged.start(), "multiple stderr files start");
    multiLogged.stop();
    std::ifstream secondLog(cfg.stderrFilePrefix + "-2.stderr");
    require(secondLog.good(), "separate stderr file for second worker");
    secondLog.close();
    ::unlink((cfg.stderrFilePrefix + "-2.stderr").c_str());
    ::unlink((cfg.stderrFilePrefix + "-1.stderr").c_str());
    ::rmdir(directory);

    cfg.command.arg0 = "/nonexistent/mantids30-worker";
    ExecPool missing(cfg);
    require(!missing.start() && !missing.isRunning(), "startup rollback");
}

int main(int argc, char **argv)
{
    if (argc == 2 && std::string(argv[1]) == "--child")
    {
        return child();
    }
    try
    {
        tests(argv[0]);
        std::cout << "Execution pool integration tests passed\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
