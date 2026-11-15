#pragma once

#include <arpa/inet.h>
#include <fcntl.h>
#include <modbus/modbus.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
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
  TransportError error{ TransportError::none };
  std::string message;

  [[nodiscard]] bool ok() const noexcept
  {
    return error == TransportError::none;
  }
};

struct RegisterReadResult
{
  std::optional<std::vector<std::uint16_t>> registers;
  TransportError error{ TransportError::none };
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
  virtual RegisterReadResult read_holding_registers(std::uint16_t address, std::uint16_t count) = 0;
};

struct ModbusTcpConfig
{
  std::string host;
  int port;
  int unit_id;
  std::chrono::milliseconds response_timeout;
  std::chrono::milliseconds connect_timeout{ 1000 };
};

class ModbusTcpTransport final : public RegisterTransport
{
public:
  explicit ModbusTcpTransport(ModbusTcpConfig config) : config_(std::move(config))
  {
  }

  ~ModbusTcpTransport() override
  {
    disconnect();
  }

  ModbusTcpTransport(const ModbusTcpTransport&) = delete;
  ModbusTcpTransport& operator=(const ModbusTcpTransport&) = delete;

  [[nodiscard]] TransportStatus connect() override
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (connected_)
    {
      return {};
    }
    if (!valid_config())
    {
      return { TransportError::invalid_configuration, "Invalid Modbus TCP configuration" };
    }

    context_ = modbus_new_tcp(config_.host.c_str(), config_.port);
    if (context_ == nullptr)
    {
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

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<std::uint16_t>(config_.port));
    if (inet_pton(AF_INET, config_.host.c_str(), &address.sin_addr) != 1)
    {
      free_context(false);
      return { TransportError::invalid_configuration, "Host must be a numeric IPv4 address" };
    }

    const int socket_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
      const auto error = system_error(TransportError::io_error);
      free_context(false);
      return error;
    }

    const int socket_flags = fcntl(socket_fd, F_GETFL, 0);
    if (socket_flags == -1 || fcntl(socket_fd, F_SETFL, socket_flags | O_NONBLOCK) == -1)
    {
      const auto error = system_error(TransportError::io_error);
      close(socket_fd);
      free_context(false);
      return error;
    }

    if (::connect(socket_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1)
    {
      if (errno != EINPROGRESS)
      {
        const auto error = system_error(TransportError::io_error);
        close(socket_fd);
        free_context(false);
        return error;
      }

      pollfd descriptor{ socket_fd, POLLOUT, 0 };
      const int ready = poll(&descriptor, 1, static_cast<int>(config_.connect_timeout.count()));
      if (ready <= 0)
      {
        if (ready == 0)
        {
          errno = ETIMEDOUT;
        }
        const auto error = system_error(TransportError::io_error);
        close(socket_fd);
        free_context(false);
        return error;
      }

      int socket_error = 0;
      socklen_t socket_error_length = sizeof(socket_error);
      if (getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &socket_error, &socket_error_length) == -1)
      {
        const auto error = system_error(TransportError::io_error);
        close(socket_fd);
        free_context(false);
        return error;
      }
      if (socket_error != 0)
      {
        errno = socket_error;
        const auto error = system_error(TransportError::io_error);
        close(socket_fd);
        free_context(false);
        return error;
      }
    }

    if (fcntl(socket_fd, F_SETFL, socket_flags) == -1)
    {
      const auto error = system_error(TransportError::io_error);
      close(socket_fd);
      free_context(false);
      return error;
    }

    modbus_set_socket(context_, socket_fd);
    connected_ = true;
    return {};
  }

  void disconnect() noexcept override
  {
    std::lock_guard<std::mutex> lock(mutex_);
    free_context(connected_);
  }

  [[nodiscard]] RegisterReadResult read_holding_registers(std::uint16_t address, std::uint16_t count) override
  {
    if (count == 0 || count > MODBUS_MAX_READ_REGISTERS ||
        static_cast<std::uint32_t>(address) + count >
            static_cast<std::uint32_t>(std::numeric_limits<std::uint16_t>::max()) + 1U)
    {
      return { std::nullopt, TransportError::invalid_request, "Invalid holding-register range" };
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (!connected_ || context_ == nullptr)
    {
      return { std::nullopt, TransportError::not_connected, "Modbus TCP is not connected" };
    }

    std::vector<std::uint16_t> registers(count);
    const int result = modbus_read_registers(context_, address, count, registers.data());
    if (result == -1)
    {
      const auto error = libmodbus_error(TransportError::io_error);
      return { std::nullopt, error.error, error.message };
    }
    if (result != count)
    {
      return { std::nullopt, TransportError::protocol_error, "Modbus read returned an unexpected register count" };
    }

    return { std::move(registers), TransportError::none, {} };
  }

private:
  [[nodiscard]] bool valid_config() const noexcept
  {
    const auto timeout_ms = config_.response_timeout.count();
    const auto connect_timeout_ms = config_.connect_timeout.count();
    in_addr address{};
    return !config_.host.empty() && config_.port > 0 && config_.port <= 65535 &&
           inet_pton(AF_INET, config_.host.c_str(), &address) == 1 && config_.unit_id >= 0 && config_.unit_id <= 255 &&
           timeout_ms > 0 && timeout_ms / 1000 <= std::numeric_limits<std::uint32_t>::max() && connect_timeout_ms > 0 &&
           connect_timeout_ms <= std::numeric_limits<int>::max();
  }

  [[nodiscard]] static TransportStatus system_error(TransportError fallback)
  {
    const int error_number = errno;
    if (error_number == ETIMEDOUT)
    {
      fallback = TransportError::timeout;
    }
    return { fallback, std::strerror(error_number) };
  }

  [[nodiscard]] static TransportStatus libmodbus_error(TransportError fallback)
  {
    const int error_number = errno;
    if (error_number == ETIMEDOUT)
    {
      fallback = TransportError::timeout;
    }
    else if (error_number >= MODBUS_ENOBASE && error_number < MODBUS_ENOBASE + MODBUS_EXCEPTION_MAX)
    {
      fallback = TransportError::protocol_error;
    }
    return { fallback, modbus_strerror(error_number) };
  }

  void free_context(bool close_socket) noexcept
  {
    if (context_ != nullptr)
    {
      if (close_socket)
      {
        modbus_close(context_);
      }
      modbus_free(context_);
      context_ = nullptr;
    }
    connected_ = false;
  }

  ModbusTcpConfig config_;
  modbus_t* context_{ nullptr };
  bool connected_{ false };
  std::mutex mutex_;
};

}  // namespace onrobot_control