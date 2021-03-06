// a super slow event system which handles dispatching to arbitrary functions
// using integral codes as well as causing code to fire at a specific time

#ifdef FILE_LP3_SIMS_EVENTSIMULATOR_HPP
#define FILE_LP3_SIMS_EVENTSIMULATOR_HPP

#include "config.hpp"
#include "dispatcher.hpp"
#include "qroutines.hpp"

namespace lp3::sims {

class EventSimulator {
  public:
    EventSimulator();

    inline void cancel(const QRoutineId id) { return runner.cancel(id); }

    template <typename T>
    QRoutineId emit(EventType id, T args, SleepTime sleep_time) {
        return runner.run(
                [id, args, this]() -> std::optional<SleepTime> {
                    dispatcher.send(id, args);
                    return std::nullopt;
                },
                sleep_time);
    }

    inline std::size_t proc_count() const { return runner.proc_count(); }

    inline void process_events(SleepTime time) {
        runner.process_events(time);
        dispatcher.prune();
    }

    // Runs the given function in some given amount of time.
    inline QRoutineId run(QRoutineDelegate rd, SleepTime sleep_time) {
        return runner.run(rd, sleep_time);
    }

    // Sets a function to receive events when the given EventType happens.
    template <typename T>
    SubscriptionId subscribe(const EventType id,
                             std::function<void(T args)> func) {
        return dispatcher.subscribe(id, func);
    }

  private:
    EventDispatcher dispatcher;
    QRoutineRunner runner;
};

} // namespace lp3::sims

#endif
