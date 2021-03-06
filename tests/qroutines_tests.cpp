#include <fmt/core.h>
#include <lp3/sims/coroutine.hpp>
#include <lp3/sims/qroutines.hpp>
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

namespace sims = lp3::sims;

TEST_CASE("Run a few simple routines.", "[basic_runs]") {
    sims::QRoutineRunner runner;
    std::vector<char> history;
    runner.run(
            [&history]() {
                fmt::print("Running routine 1");
                history.push_back('2');
                return std::nullopt;
            },
            30);

    runner.run(
            [&history]() {
                fmt::print("Running routine 2");
                history.push_back('1');
                return std::nullopt;
            },
            20);

    REQUIRE(history.size() == 0);
    REQUIRE(runner.proc_count() == 2);

    fmt::print("Pulse 1");
    runner.process_events(16);
    REQUIRE(history.size() == 0);
    REQUIRE(runner.proc_count() == 2);

    fmt::print("Pulse 2");
    runner.process_events(16);

    REQUIRE(history.size() == 2);
    REQUIRE(history[0] == '1');
    REQUIRE(history[1] == '2');
    REQUIRE(runner.proc_count() == 0);

    fmt::print("THE END");
}

TEST_CASE("Cancel a routine before it runs", "[cancel_run]") {
    sims::QRoutineRunner runner;
    sims::QRoutineId id = runner.run(
            []() {
                fmt::print("Running routine 1");
                return std::nullopt;
            },
            3);

    REQUIRE(runner.proc_count() == 1);
    runner.cancel(id);
    REQUIRE(runner.proc_count() == 0);
    runner.process_events(0);
    REQUIRE(runner.proc_count() == 0);
    runner.process_events(1000);
    REQUIRE(runner.proc_count() == 0);
    fmt::print("THE END 2");
}

TEST_CASE("Call a routine with arguments.", "[calling_with_args]") {
    sims::QRoutineRunner runner;
    std::string arg_holder("I am a crazy string. Wow! Crazy!");

    auto some_crazy_func = [](const std::string & hi) {
        fmt::print("I got message: %s", hi);
        return std::nullopt;
    };

    runner.run([arg_holder,
                &some_crazy_func]() { return some_crazy_func(arg_holder); },
               0);
    runner.process_events(16);
    fmt::print("did you see signal that was string?");
}

TEST_CASE("Sleeping routines.", "[sleepy_q]") {
    sims::QRoutineRunner runner;
    std::vector<char> history;
    sims::CoroutineState coro1;
    runner.run(
            [&history, &coro1]() -> std::optional<sims::SleepTime> {
                LP3_COROUTINE_BEGIN(coro1)
                fmt::print("Running routine 1");
                history.push_back('1');
                LP3_YIELD(10);
                fmt::print("Running routine 1");
                history.push_back('3');
                LP3_YIELD(std::nullopt);
                LP3_COROUTINE_END()
                return std::nullopt;
            },
            30);

    runner.run(
            [&history]() {
                fmt::print("Running routine 2");
                history.push_back('2');
                return std::nullopt;
            },
            35);

    REQUIRE(history.size() == 0);
    REQUIRE(runner.proc_count() == 2);

    for (int i = 0; i < 12; ++i) {
        runner.process_events(5);
    }

    REQUIRE(history.size() == 3);
    REQUIRE(history[0] == '1');
    REQUIRE(history[1] == '2');
    REQUIRE(history[2] == '3');
    REQUIRE(runner.proc_count() == 0);

    fmt::print("THE END");
}

TEST_CASE("Kicking off routines from routines.", "[basic_runs]") {
    sims::QRoutineRunner runner;
    std::vector<char> history;
    sims::CoroutineState coro1;
    int spawn = 80;
    runner.run(
            [&]() -> std::optional<sims::SleepTime> {
                LP3_COROUTINE_BEGIN(coro1)
                for (spawn = 0; spawn < 100; ++spawn) {
                    fmt::print("Main proc, spawn %i", spawn);
                    runner.run(
                            [&history, spawn]() {
                                (void)spawn; // silence unused var error
                                fmt::print("Child routine (%i)", spawn);
                                history.push_back('c');
                                return std::nullopt;
                            },
                            50);
                    LP3_YIELD(10);
                }
                LP3_COROUTINE_END()
                return std::nullopt;
            },
            10);

    // int step = 1;
    // for (int i = 0; i < 1000; i = i + step) {
    //     std::cout << i << " .. " << runner.proc_count() << " hist=" <<
    //     history.size() << "\n"; runner.process_events(step);
    // }

    REQUIRE(history.size() == 0);
    REQUIRE(runner.proc_count() == 1);

    // Sleep for 5- the single queued routine won't have fired yet.
    runner.process_events(5);
    REQUIRE(history.size() == 0);
    REQUIRE(runner.proc_count() == 1);

    // Sleep for another five- now a second process will be queued, but not run
    runner.process_events(5);
    REQUIRE(history.size() == 0);
    REQUIRE(runner.proc_count() == 2);

    // Wait for the queued routine to run- by this point four more
    // processes will have been queued, but none will have run yet.
    runner.process_events(49);
    REQUIRE(history.size() == 0);
    REQUIRE(runner.proc_count() == 6);

    // for (int i = 0; i < 50; i += 5)
    //    runner.process_events(5);

    // Run for one more second- the first queued process runs and goes away,
    // but is replaced by yet another spawned process.
    runner.process_events(1);
    REQUIRE(history.size() == 1);
    REQUIRE(runner.proc_count() == 6);

    // Run routines until we're all out
    while (runner.process_events(1000)) {
        fmt::print("Processing events.");
    }

    REQUIRE(history.size() == 100);
    REQUIRE(runner.proc_count() == 0);

    fmt::print("THE END");
}
