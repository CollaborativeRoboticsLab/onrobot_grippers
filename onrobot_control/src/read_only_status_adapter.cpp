#include "onrobot_control/read_only_status_adapter.hpp"

#include <arpa/inet.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace onrobot_control
{

ReadOnlyStatusAdapter::ReadOnlyStatusAdapter(rclcpp::NodeOptions options, RegisterTransportFactory transport_factory)
  : Node("onrobot_read_only_status_adapter", options)
{
  model_ = declare_read_only_parameter<std::string>("model", "rg2",
                                                    "Device label only; no model-specific register map is assumed");
  host_ =
      declare_read_only_parameter<std::string>("host", "", "Numeric IPv4 address of the read-only Modbus TCP endpoint");
  port_ = declare_read_only_parameter<std::int64_t>("port", 502, "Modbus TCP port");
  unit_id_ = declare_read_only_parameter<std::int64_t>("unit_id", 255, "Modbus unit identifier");
  start_address_ = declare_read_only_parameter<std::int64_t>("start_address", 0,
                                                             "Explicit holding-register start address; verify against "
                                                             "the exact firmware manual");
  register_count_ = declare_read_only_parameter<std::int64_t>("register_count", 1,
                                                              "Number of holding registers to read, from 1 through "
                                                              "125");
  poll_interval_ms_ =
      declare_read_only_parameter<std::int64_t>("poll_interval_ms", 1000, "Polling interval in milliseconds");
  response_timeout_ms_ = declare_read_only_parameter<std::int64_t>("response_timeout_ms", 500,
                                                                   "Maximum Modbus response wait in milliseconds");
  connect_timeout_ms_ = declare_read_only_parameter<std::int64_t>("connect_timeout_ms", 500,
                                                                  "Maximum TCP connection wait in milliseconds");
  enabled_ = declare_read_only_parameter<bool>("enabled", false,
                                               "Enable read-only polling; disabled by default to prevent automatic "
                                               "network access");

  snapshot_publisher_ = create_publisher<onrobot_msgs::msg::RawRegisterSnapshot>("raw_registers", 10);
  diagnostics_publisher_ =
      create_publisher<diagnostic_msgs::msg::DiagnosticArray>("diagnostics", rclcpp::QoS(1).transient_local());

  if (!enabled_)
  {
    publish_diagnostic(diagnostic_msgs::msg::DiagnosticStatus::STALE, "Disabled; no network access is attempted",
                       "disabled");
    return;
  }

  std::string validation_error;
  if (!validate_configuration(validation_error))
  {
    RCLCPP_ERROR(get_logger(), "Read-only adapter configuration rejected: %s", validation_error.c_str());
    publish_diagnostic(diagnostic_msgs::msg::DiagnosticStatus::ERROR, validation_error, "invalid_configuration");
    return;
  }

  if (!transport_factory)
  {
    transport_factory = [](const ModbusTcpConfig& config) { return std::make_unique<ModbusTcpTransport>(config); };
  }

  const ModbusTcpConfig transport_config{ host_, static_cast<int>(port_), static_cast<int>(unit_id_),
                                          std::chrono::milliseconds(response_timeout_ms_),
                                          std::chrono::milliseconds(connect_timeout_ms_) };
  RegisterPollConfig poll_config{ static_cast<std::uint16_t>(start_address_),
                                  static_cast<std::uint16_t>(register_count_),
                                  std::chrono::milliseconds(poll_interval_ms_) };
  poller_ =
      std::make_unique<ReadOnlyRegisterPoller>(transport_factory(transport_config), poll_config,
                                               [this](const RegisterPollResult& result) { publish_result(result); });
}

ReadOnlyStatusAdapter::~ReadOnlyStatusAdapter()
{
  if (poller_)
  {
    poller_->stop();
  }
}

bool ReadOnlyStatusAdapter::validate_configuration(std::string& error) const
{
  constexpr std::array<const char*, 4> supported_models{ "rg2", "rg6", "vg10", "vgc10" };
  const bool supported_model = std::any_of(supported_models.begin(), supported_models.end(),
                                           [this](const char* model) { return model_ == model; });
  in_addr address{};
  if (!supported_model)
  {
    error = "model must be one of rg2, rg6, vg10, or vgc10";
  }
  else if (host_.empty() || inet_pton(AF_INET, host_.c_str(), &address) != 1)
  {
    error = "host must be a numeric IPv4 address";
  }
  else if (port_ < 1 || port_ > 65535)
  {
    error = "port must be between 1 and 65535";
  }
  else if (unit_id_ < 0 || unit_id_ > 255)
  {
    error = "unit_id must be between 0 and 255";
  }
  else if (start_address_ < 0 || start_address_ > 65535)
  {
    error = "start_address must be between 0 and 65535";
  }
  else if (register_count_ < 1 || register_count_ > MODBUS_MAX_READ_REGISTERS ||
           start_address_ + register_count_ > 65536)
  {
    error = "register_count or holding-register range is invalid";
  }
  else if (poll_interval_ms_ < 10 || poll_interval_ms_ > 60000)
  {
    error = "poll_interval_ms must be between 10 and 60000";
  }
  else if (response_timeout_ms_ < 1 || response_timeout_ms_ > 60000)
  {
    error = "response_timeout_ms must be between 1 and 60000";
  }
  else if (connect_timeout_ms_ < 1 || connect_timeout_ms_ > 10000)
  {
    error = "connect_timeout_ms must be between 1 and 10000";
  }
  else
  {
    return true;
  }
  return false;
}

void ReadOnlyStatusAdapter::publish_result(const RegisterPollResult& result)
{
  if (result.registers)
  {
    onrobot_msgs::msg::RawRegisterSnapshot snapshot;
    snapshot.header.stamp = now();
    snapshot.model = model_;
    snapshot.endpoint = host_ + ":" + std::to_string(port_);
    snapshot.unit_id = static_cast<std::uint8_t>(unit_id_);
    snapshot.start_address = static_cast<std::uint16_t>(start_address_);
    snapshot.registers = *result.registers;
    snapshot_publisher_->publish(std::move(snapshot));
    publish_diagnostic(diagnostic_msgs::msg::DiagnosticStatus::OK, "Holding registers read", "ok");
    return;
  }

  const auto level = result.error == TransportError::timeout ? diagnostic_msgs::msg::DiagnosticStatus::WARN :
                                                               diagnostic_msgs::msg::DiagnosticStatus::ERROR;
  publish_diagnostic(level, result.message, "read_error");
}

void ReadOnlyStatusAdapter::publish_diagnostic(std::uint8_t level, const std::string& message, const std::string& state)
{
  diagnostic_msgs::msg::DiagnosticArray array;
  array.header.stamp = now();
  diagnostic_msgs::msg::DiagnosticStatus status;
  status.level = level;
  status.name = std::string(get_fully_qualified_name()) + "/modbus_read_only";
  status.hardware_id = model_ + "@" + host_ + ":" + std::to_string(port_);
  status.message = message;
  const auto add_value = [&status](const std::string& key, const std::string& value) {
    diagnostic_msgs::msg::KeyValue item;
    item.key = key;
    item.value = value;
    status.values.push_back(std::move(item));
  };
  add_value("state", state);
  add_value("model", model_);
  add_value("endpoint", host_ + ":" + std::to_string(port_));
  add_value("unit_id", std::to_string(unit_id_));
  add_value("start_address", std::to_string(start_address_));
  add_value("register_count", std::to_string(register_count_));
  array.status.push_back(std::move(status));
  diagnostics_publisher_->publish(std::move(array));
}

}  // namespace onrobot_control