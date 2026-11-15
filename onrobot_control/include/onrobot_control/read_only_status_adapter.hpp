#pragma once

#include <cstdint>
#include <diagnostic_msgs/msg/diagnostic_array.hpp>
#include <functional>
#include <memory>
#include <rcl_interfaces/msg/parameter_descriptor.hpp>
#include <rclcpp/rclcpp.hpp>
#include <string>

#include "onrobot_control/read_only_register_poller.hpp"
#include "onrobot_msgs/msg/raw_register_snapshot.hpp"

namespace onrobot_control
{

using RegisterTransportFactory = std::function<std::unique_ptr<RegisterTransport>(const ModbusTcpConfig&)>;

class ReadOnlyStatusAdapter final : public rclcpp::Node
{
public:
  explicit ReadOnlyStatusAdapter(rclcpp::NodeOptions options = rclcpp::NodeOptions(),
                                 RegisterTransportFactory transport_factory = {});
  ~ReadOnlyStatusAdapter() override;

private:
  template <typename Value>
  Value declare_read_only_parameter(const std::string& name, const Value& default_value, const std::string& description)
  {
    rcl_interfaces::msg::ParameterDescriptor descriptor;
    descriptor.description = description;
    descriptor.read_only = true;
    return declare_parameter<Value>(name, default_value, descriptor);
  }

  [[nodiscard]] bool validate_configuration(std::string& error) const;
  void publish_result(const RegisterPollResult& result);
  void publish_diagnostic(std::uint8_t level, const std::string& message, const std::string& state);

  std::string model_;
  std::string host_;
  std::int64_t port_{ 0 };
  std::int64_t unit_id_{ 0 };
  std::int64_t start_address_{ 0 };
  std::int64_t register_count_{ 0 };
  std::int64_t poll_interval_ms_{ 0 };
  std::int64_t response_timeout_ms_{ 0 };
  std::int64_t connect_timeout_ms_{ 0 };
  bool enabled_{ false };
  rclcpp::Publisher<onrobot_msgs::msg::RawRegisterSnapshot>::SharedPtr snapshot_publisher_;
  rclcpp::Publisher<diagnostic_msgs::msg::DiagnosticArray>::SharedPtr diagnostics_publisher_;
  std::unique_ptr<ReadOnlyRegisterPoller> poller_;
};

}  // namespace onrobot_control