#pragma once

#include <modbus/modbus.h>

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace onrobot_control
{

enum class TransportError
{
  none,
  invalid_configuration,
  invalid_request,
  not_connected,
  timeout,
  io_error,
  protocol_error
};

struct TransportStatus
{
  TransportError error{TransportError::none};
  std::string message;

  [[nodiscard]] bool ok() const noexcept
  {
    return error == TransportError::none;
  }
};

struct RegisterReadResult
{
  std::optional<std::vector<std::uint16_t>> registers;
  TransportError error{TransportError::none};
  std::string message;

  [[nodiscard]] bool ok() const noexcept
  {
    return error == TransportError::none;
  }
};

class RegisterTransport
{
public:
  virtual ~RegisterTransport() = default;
  virtual TransportStatus connect() = 0;
  virtual void disconnect() noexcept = 0;
  virtual RegisterReadResult read_holding_registers(
    std::uint16_t address, std::uint16_t count) = 0;
};

struct ModbusTcpConfig
{
  std::string host;
  int port;
  int unit_id;
  std::chrono::milliseconds response_timeout;
};

class ModbusTcpTransport final : public RegisterTransport
{
public:
  explicit ModbusTcpTransport(ModbusTcpConfig config)
  : config_(std::move(config))
  {
  }

  ~ModbusTcpTransport() override
  {
    disconnect();
  }

  ModbusTcpTransport(const ModbusTcpTransport &) = delete;
  ModbusTcpTransport & operator=(const ModbusTcpTransport &) = delete;

  [[nodiscard]] TransportStatus connect() override
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (connected_) {
      return {};
    }
    if (!valid_config()) {
      return {TransportError::invalid_configuration, "Invalid Modbus TCP configuration"};
    }

    context_ = modbus_new_tcp(config_.host.c_str(), config_.port);
    if (context_ == nullptr) {
      return libmodbus_error(TransportError::invalid_configuration);
    }

    const auto timeout_ms = config_.response_timeout.count();
    const auto timeout_seconds = static_cast<std::uint32_t>(timeout_ms / 1000);
    const auto timeout_microseconds = static_cast<std::uint32_t>((timeout_ms % 1000) * 1000);
    if (modbus_set_slave(context_, config_.unit_id) == -1 ||
      modbus_set_response_timeout(context_, timeout_seconds, timeout_microseconds) == -1)
    {
      const auto error = libmodbus_error(TransportError::invalid_configuration);
      free_context(false);
      return error;
    }

    if (modbus_connect(context_) == -1) {
      const auto error = libmodbus_error(TransportError::io_error);
      free_context(true);
      return error;
    }

    connected_ = true;
    return {};
  }

  void disconnect() noexcept override
  {
    std::lock_guard<std::mutex> lock(mutex_);
    free_context(connected_);
  }

  [[nodiscard]] RegisterReadResult read_holding_registers(
    std::uint16_t address, std::uint16_t count) override
  {
    if (count == 0 || count > MODBUS_MAX_READ_REGISTERS ||
      static_cast<std::uint32_t>(address) + count >
      static_cast<std::uint32_t>(std::numeric_limits<std::uint16_t>::max()) + 1U)
    {
      return {std::nullopt, TransportError::invalid_request, "Invalid holding-register range"};
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (!connected_ || context_ == nullptr) {
      return {std::nullopt, TransportError::not_connected, "Modbus TCP is not connected"};
    }

    std::vector<std::uint16_t> registers(count);
    const int result = modbus_read_registers(context_, address, count, registers.data());
    if (result == -1) {
      const auto error = libmodbus_error(TransportError::io_error);
      return {std::nullopt, error.error, error.message};
    }
    if (result != count) {
      return {std::nullopt, TransportError::protocol_error,
        "Modbus read returned an unexpected register count"};
    }

    return {std::move(registers), TransportError::none, {}};
  }

private:
  [[nodiscard]] bool valid_config() const noexcept
  {
    const auto timeout_ms = config_.response_timeout.count();
    return !config_.host.empty() && config_.port > 0 && config_.port <= 65535 &&
           config_.unit_id >= 0 && config_.unit_id <= 255 && timeout_ms > 0 &&
           timeout_ms / 1000 <= std::numeric_limits<std::uint32_t>::max();
  }

  [[nodiscard]] static TransportStatus libmodbus_error(TransportError fallback)
  {
    const int error_number = errno;
    if (error_number == ETIMEDOUT) {
      fallback = TransportError::timeout;
    } else if (error_number >= MODBUS_ENOBASE &&
      error_number < MODBUS_ENOBASE + MODBUS_EXCEPTION_MAX)
    {
      fallback = TransportError::protocol_error;
    }
    return {fallback, modbus_strerror(error_number)};
  }

  void free_context(bool close_socket) noexcept
  {
    if (context_ != nullptr) {
      if (close_socket) {
        modbus_close(context_);
      }
      modbus_free(context_);
      context_ = nullptr;
    }
    connected_ = false;
  }

  ModbusTcpConfig config_;
  modbus_t * context_{nullptr};
  bool connected_{false};
  std::mutex mutex_;
};

}  // namespace onrobot_control