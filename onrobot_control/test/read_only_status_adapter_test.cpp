#include "onrobot_control/read_only_status_adapter.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <diagnostic_msgs/msg/diagnostic_array.hpp>
#include <iostream>
#include <memory>
#include <mutex>
#include <onrobot_msgs/msg/raw_register_snapshot.hpp>
#include <optional>
#include <rclcpp/rclcpp.hpp>
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
  explicit FakeTransportState(std::uint16_t first_register) : first_register(first_register)
  {
  }

  std::mutex mutex;
  std::condition_variable changed;
  std::uint16_t first_register;
  bool read_started{ false };
  bool release_read{ false };
  int disconnect_calls{ 0 };
};

class BlockingFakeTransport final : public onrobot_control::RegisterTransport
{
public:
  explicit BlockingFakeTransport(std::shared_ptr<FakeTransportState> state) : state_(std::move(state))
  {
  }

  onrobot_control::TransportStatus connect() override
  {
    return {};
  }

  void disconnect() noexcept override
  {
    std::lock_guard<std::mutex> lock(state_->mutex);
    ++state_->disconnect_calls;
  }

  onrobot_control::RegisterReadResult read_holding_registers(std::uint16_t, std::uint16_t) override
  {
    std::unique_lock<std::mutex> lock(state_->mutex);
    state_->read_started = true;
    state_->changed.notify_all();
    state_->changed.wait(lock, [this]() { return state_->release_read; });
    return { std::vector<std::uint16_t>{ state_->first_register, 0xabcd }, onrobot_control::TransportError::none, {} };
  }

private:
  std::shared_ptr<FakeTransportState> state_;
};

rclcpp::NodeOptions adapter_options(const std::string& node_namespace, const std::string& model,
                                    std::int64_t start_address)
{
  rclcpp::NodeOptions options;
  options.arguments({ "--ros-args", "-r", "__ns:=" + node_namespace });
  options.parameter_overrides({ rclcpp::Parameter("enabled", true), rclcpp::Parameter("model", model),
                                rclcpp::Parameter("host", "127.0.0.1"), rclcpp::Parameter("port", 1502),
                                rclcpp::Parameter("unit_id", 255), rclcpp::Parameter("start_address", start_address),
                                rclcpp::Parameter("register_count", 2), rclcpp::Parameter("poll_interval_ms", 50),
                                rclcpp::Parameter("response_timeout_ms", 200),
                                rclcpp::Parameter("connect_timeout_ms", 200) });
  return options;
}

bool wait_for_read(const std::shared_ptr<FakeTransportState>& state)
{
  std::unique_lock<std::mutex> lock(state->mutex);
  return state->changed.wait_for(lock, std::chrono::seconds(1), [state]() { return state->read_started; });
}

void release_read(const std::shared_ptr<FakeTransportState>& state)
{
  {
    std::lock_guard<std::mutex> lock(state->mutex);
    state->release_read = true;
  }
  state->changed.notify_all();
}

}  // namespace

int main(int argc, char** argv)
{
  using namespace std::chrono_literals;
  rclcpp::init(argc, argv);

  auto state_a = std::make_shared<FakeTransportState>(0x1234);
  auto state_b = std::make_shared<FakeTransportState>(0x5678);
  const auto factory_for = [](const std::shared_ptr<FakeTransportState>& state) {
    return [state](const onrobot_control::ModbusTcpConfig&) { return std::make_unique<BlockingFakeTransport>(state); };
  };

  auto adapter_a = std::make_shared<onrobot_control::ReadOnlyStatusAdapter>(adapter_options("/cell_a", "rg2", 100),
                                                                            factory_for(state_a));
  auto adapter_b = std::make_shared<onrobot_control::ReadOnlyStatusAdapter>(adapter_options("/cell_b", "vg10", 200),
                                                                            factory_for(state_b));

  std::atomic<int> factory_calls{ 0 };
  rclcpp::NodeOptions invalid_options;
  invalid_options.arguments({ "--ros-args", "-r", "__ns:=/invalid" });
  invalid_options.parameter_overrides({ rclcpp::Parameter("enabled", true), rclcpp::Parameter("host", "127.0.0.1"),
                                        rclcpp::Parameter("register_count", 126) });
  auto invalid_adapter = std::make_shared<onrobot_control::ReadOnlyStatusAdapter>(
      invalid_options,
      [&factory_calls](const onrobot_control::ModbusTcpConfig&) -> std::unique_ptr<onrobot_control::RegisterTransport> {
        ++factory_calls;
        return {};
      });
  expect(factory_calls.load() == 0, "rejects an invalid register count before creating a transport");

  rclcpp::NodeOptions disabled_options;
  disabled_options.arguments({ "--ros-args", "-r", "__ns:=/disabled" });
  auto disabled_adapter = std::make_shared<onrobot_control::ReadOnlyStatusAdapter>(
      disabled_options,
      [&factory_calls](const onrobot_control::ModbusTcpConfig&) -> std::unique_ptr<onrobot_control::RegisterTransport> {
        ++factory_calls;
        return {};
      });
  expect(factory_calls.load() == 0, "does not create a transport unless explicitly enabled");

  auto observer = std::make_shared<rclcpp::Node>("adapter_test_observer");
  std::mutex received_mutex;
  std::optional<onrobot_msgs::msg::RawRegisterSnapshot> snapshot_a;
  std::optional<onrobot_msgs::msg::RawRegisterSnapshot> snapshot_b;
  std::optional<diagnostic_msgs::msg::DiagnosticArray> diagnostics_a;
  std::optional<diagnostic_msgs::msg::DiagnosticArray> diagnostics_invalid;
  std::optional<diagnostic_msgs::msg::DiagnosticArray> diagnostics_disabled;
  auto subscription_a = observer->create_subscription<onrobot_msgs::msg::RawRegisterSnapshot>(
      "/cell_a/raw_registers", 10,
      [&received_mutex, &snapshot_a](onrobot_msgs::msg::RawRegisterSnapshot::ConstSharedPtr message) {
        std::lock_guard<std::mutex> lock(received_mutex);
        snapshot_a = *message;
      });
  auto subscription_b = observer->create_subscription<onrobot_msgs::msg::RawRegisterSnapshot>(
      "/cell_b/raw_registers", 10,
      [&received_mutex, &snapshot_b](onrobot_msgs::msg::RawRegisterSnapshot::ConstSharedPtr message) {
        std::lock_guard<std::mutex> lock(received_mutex);
        snapshot_b = *message;
      });
  auto diagnostics_subscription = observer->create_subscription<diagnostic_msgs::msg::DiagnosticArray>(
      "/cell_a/diagnostics", 10,
      [&received_mutex, &diagnostics_a](diagnostic_msgs::msg::DiagnosticArray::ConstSharedPtr message) {
        std::lock_guard<std::mutex> lock(received_mutex);
        diagnostics_a = *message;
      });
  auto invalid_diagnostics_subscription = observer->create_subscription<diagnostic_msgs::msg::DiagnosticArray>(
      "/invalid/diagnostics", rclcpp::QoS(1).transient_local(),
      [&received_mutex, &diagnostics_invalid](diagnostic_msgs::msg::DiagnosticArray::ConstSharedPtr message) {
        std::lock_guard<std::mutex> lock(received_mutex);
        diagnostics_invalid = *message;
      });
  auto disabled_diagnostics_subscription = observer->create_subscription<diagnostic_msgs::msg::DiagnosticArray>(
      "/disabled/diagnostics", rclcpp::QoS(1).transient_local(),
      [&received_mutex, &diagnostics_disabled](diagnostic_msgs::msg::DiagnosticArray::ConstSharedPtr message) {
        std::lock_guard<std::mutex> lock(received_mutex);
        diagnostics_disabled = *message;
      });
  std::atomic<int> timer_ticks{ 0 };
  auto timer = observer->create_wall_timer(1ms, [&timer_ticks]() { ++timer_ticks; });

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(adapter_a);
  executor.add_node(adapter_b);
  executor.add_node(invalid_adapter);
  executor.add_node(disabled_adapter);
  executor.add_node(observer);

  const bool both_reads_started = wait_for_read(state_a) && wait_for_read(state_b);
  expect(both_reads_started, "starts independent polling workers in multiple namespaces");
  if (both_reads_started)
  {
    const auto responsive_start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - responsive_start < 50ms)
    {
      executor.spin_some();
      std::this_thread::sleep_for(1ms);
    }
    expect(timer_ticks.load() > 5, "executor callbacks continue while endpoint reads are blocked");
  }

  release_read(state_a);
  release_read(state_b);
  const auto message_deadline = std::chrono::steady_clock::now() + 1s;
  while (std::chrono::steady_clock::now() < message_deadline)
  {
    executor.spin_some();
    {
      std::lock_guard<std::mutex> lock(received_mutex);
      if (snapshot_a && snapshot_b && diagnostics_a && diagnostics_invalid && diagnostics_disabled)
      {
        break;
      }
    }
    std::this_thread::sleep_for(1ms);
  }

  {
    std::lock_guard<std::mutex> lock(received_mutex);
    expect(snapshot_a.has_value() && snapshot_b.has_value(), "publishes raw snapshots in both namespaces");
    if (snapshot_a && snapshot_b)
    {
      expect(snapshot_a->model == "rg2" && snapshot_a->start_address == 100 &&
                 snapshot_a->registers == std::vector<std::uint16_t>{ 0x1234, 0xabcd },
             "preserves the RG raw register range and values");
      expect(snapshot_b->model == "vg10" && snapshot_b->start_address == 200 &&
                 snapshot_b->registers == std::vector<std::uint16_t>{ 0x5678, 0xabcd },
             "keeps the second namespace and its raw values independent");
    }
    expect(diagnostics_a.has_value() && !diagnostics_a->status.empty() &&
               diagnostics_a->status.front().level == diagnostic_msgs::msg::DiagnosticStatus::OK,
           "publishes successful diagnostic status");
    expect(diagnostics_invalid.has_value() && !diagnostics_invalid->status.empty() &&
               diagnostics_invalid->status.front().level == diagnostic_msgs::msg::DiagnosticStatus::ERROR,
           "retains invalid-configuration diagnostics for late subscribers");
    expect(diagnostics_disabled.has_value() && !diagnostics_disabled->status.empty() &&
               diagnostics_disabled->status.front().level == diagnostic_msgs::msg::DiagnosticStatus::STALE,
           "retains disabled-state diagnostics for late subscribers");
  }

  auto parameter_result = adapter_a->set_parameter(rclcpp::Parameter("host", "192.0.2.1"));
  expect(!parameter_result.successful, "keeps endpoint parameters read-only at runtime");

  release_read(state_a);
  release_read(state_b);
  executor.remove_node(adapter_a);
  executor.remove_node(adapter_b);
  executor.remove_node(invalid_adapter);
  executor.remove_node(disabled_adapter);
  executor.remove_node(observer);
  adapter_a.reset();
  adapter_b.reset();
  invalid_adapter.reset();
  disabled_adapter.reset();
  observer.reset();
  expect(state_a->disconnect_calls > 0 && state_b->disconnect_calls > 0, "disconnects fake endpoints during orderly "
                                                                         "shutdown");

  rclcpp::shutdown();
  return failures == 0 ? 0 : 1;
}