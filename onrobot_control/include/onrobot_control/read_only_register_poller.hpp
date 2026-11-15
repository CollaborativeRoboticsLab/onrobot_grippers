#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "onrobot_control/modbus_tcp_transport.hpp"

namespace onrobot_control
{

struct RegisterPollResult
{
  std::optional<std::vector<std::uint16_t>> registers;
  TransportError error{ TransportError::none };
  std::string message;
};

struct RegisterPollConfig
{
  std::uint16_t start_address;
  std::uint16_t register_count;
  std::chrono::milliseconds interval;
};

class ReadOnlyRegisterPoller final
{
public:
  using Callback = std::function<void(const RegisterPollResult&)>;

  ReadOnlyRegisterPoller(std::unique_ptr<RegisterTransport> transport, RegisterPollConfig config, Callback callback)
    : transport_(std::move(transport)), config_(config), callback_(std::move(callback))
  {
    if (!transport_ || config_.register_count == 0 || config_.interval.count() <= 0 || !callback_)
    {
      throw std::invalid_argument("Invalid read-only register poller configuration");
    }
    worker_ = std::thread(&ReadOnlyRegisterPoller::run, this);
  }

  ~ReadOnlyRegisterPoller()
  {
    stop();
  }

  ReadOnlyRegisterPoller(const ReadOnlyRegisterPoller&) = delete;
  ReadOnlyRegisterPoller& operator=(const ReadOnlyRegisterPoller&) = delete;

  void stop() noexcept
  {
    stopping_.store(true);
    wake_.notify_all();
    if (worker_.joinable())
    {
      worker_.join();
    }
  }

private:
  void emit(RegisterPollResult result) noexcept
  {
    try
    {
      callback_(result);
    }
    catch (...)
    {
      stopping_.store(true);
      wake_.notify_all();
    }
  }

  bool wait_for_interval()
  {
    std::unique_lock<std::mutex> lock(wait_mutex_);
    wake_.wait_for(lock, config_.interval, [this]() { return stopping_.load(); });
    return !stopping_.load();
  }

  void run() noexcept
  {
    bool connected = false;
    while (!stopping_.load())
    {
      if (!connected)
      {
        const auto status = transport_->connect();
        if (!status.ok())
        {
          emit({ std::nullopt, status.error, status.message });
          if (!wait_for_interval())
          {
            break;
          }
          continue;
        }
        connected = true;
      }

      const auto read = transport_->read_holding_registers(config_.start_address, config_.register_count);
      if (read.ok() && read.registers)
      {
        emit({ read.registers, TransportError::none, {} });
      }
      else
      {
        transport_->disconnect();
        connected = false;
        emit({ std::nullopt, read.error, read.message });
      }

      if (!wait_for_interval())
      {
        break;
      }
    }
    transport_->disconnect();
  }

  std::unique_ptr<RegisterTransport> transport_;
  RegisterPollConfig config_;
  Callback callback_;
  std::atomic<bool> stopping_{ false };
  std::mutex wait_mutex_;
  std::condition_variable wake_;
  std::thread worker_;
};

}  // namespace onrobot_control