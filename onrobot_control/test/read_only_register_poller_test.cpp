#include "onrobot_control/read_only_register_poller.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace
{

int failures = 0;

void expect(bool condition, const char* description)
{
  if (!condition)
  {
    std::cerr << "FAIL: " << description << '\n';
    ++failures;
  }
}

struct FakeTransportState
{
  std::mutex mutex;
  int connect_calls{ 0 };
  int read_calls{ 0 };
  int disconnect_calls{ 0 };
};

class ScriptedTransport final : public onrobot_control::RegisterTransport
{
public:
  explicit ScriptedTransport(std::shared_ptr<FakeTransportState> state) : state_(std::move(state))
  {
  }

  onrobot_control::TransportStatus connect() override
  {
    std::lock_guard<std::mutex> lock(state_->mutex);
    ++state_->connect_calls;
    if (state_->connect_calls == 1)
    {
      return { onrobot_control::TransportError::io_error, "fake endpoint refused connection" };
    }
    return {};
  }

  void disconnect() noexcept override
  {
    std::lock_guard<std::mutex> lock(state_->mutex);
    ++state_->disconnect_calls;
  }

  onrobot_control::RegisterReadResult read_holding_registers(std::uint16_t, std::uint16_t) override
  {
    std::lock_guard<std::mutex> lock(state_->mutex);
    ++state_->read_calls;
    if (state_->read_calls == 1)
    {
      return { std::nullopt, onrobot_control::TransportError::timeout, "fake read timeout" };
    }
    return { std::vector<std::uint16_t>{ 0x1234, 0xabcd }, onrobot_control::TransportError::none, {} };
  }

private:
  std::shared_ptr<FakeTransportState> state_;
};

}  // namespace

int main()
{
  using namespace std::chrono_literals;
  using onrobot_control::ReadOnlyRegisterPoller;
  using onrobot_control::RegisterPollConfig;
  using onrobot_control::RegisterPollResult;

  auto state = std::make_shared<FakeTransportState>();
  std::mutex result_mutex;
  std::condition_variable result_ready;
  std::vector<RegisterPollResult> results;
  const auto start = std::chrono::steady_clock::now();
  {
    ReadOnlyRegisterPoller poller(std::make_unique<ScriptedTransport>(state), RegisterPollConfig{ 10, 2, 5ms },
                                  [&results, &result_mutex, &result_ready](const RegisterPollResult& result) {
                                    {
                                      std::lock_guard<std::mutex> lock(result_mutex);
                                      results.push_back(result);
                                    }
                                    result_ready.notify_all();
                                  });

    std::unique_lock<std::mutex> lock(result_mutex);
    const bool received_snapshot = result_ready.wait_for(lock, 1s, [&results]() {
      return std::any_of(results.begin(), results.end(),
                         [](const auto& result) { return result.registers.has_value(); });
    });
    expect(received_snapshot, "reconnects after a connection error and read timeout");
    if (received_snapshot)
    {
      const auto snapshot =
          std::find_if(results.begin(), results.end(), [](const auto& result) { return result.registers.has_value(); });
      expect(snapshot->registers->at(0) == 0x1234 && snapshot->registers->at(1) == 0xabcd, "publishes the raw words "
                                                                                           "after reconnecting");
    }
    lock.unlock();

    const auto stop_start = std::chrono::steady_clock::now();
    poller.stop();
    expect(std::chrono::steady_clock::now() - stop_start < 200ms, "stop cancels the polling wait promptly");
  }

  {
    std::lock_guard<std::mutex> lock(state->mutex);
    expect(state->connect_calls >= 3, "retries the read-only connection after failures");
    expect(state->read_calls >= 2, "reads again after a timeout");
    expect(state->disconnect_calls >= 2, "disconnects after failure and on shutdown");
  }
  expect(std::chrono::steady_clock::now() - start < 2s, "poller shuts down without waiting for the poll interval");
  return failures == 0 ? 0 : 1;
}