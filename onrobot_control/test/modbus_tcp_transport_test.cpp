#include "onrobot_control/modbus_tcp_transport.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace
{

int failures = 0;

enum class ResponseMode
{
  normal,
  silent,
  exception,
  malformed
};

void expect(bool condition, const char* description)
{
  if (!condition)
  {
    std::cerr << "FAIL: " << description << '\n';
    ++failures;
  }
}

class LoopbackModbusServer
{
public:
  explicit LoopbackModbusServer(ResponseMode response_mode) : response_mode_(response_mode)
  {
    context_ = modbus_new_tcp("127.0.0.1", 0);
    if (context_ == nullptr)
    {
      throw std::runtime_error("modbus_new_tcp failed");
    }

    mapping_ = modbus_mapping_new(0, 0, 8, 0);
    if (mapping_ == nullptr)
    {
      modbus_free(context_);
      throw std::runtime_error("modbus_mapping_new failed");
    }
    mapping_->tab_registers[0] = 0x1234;
    mapping_->tab_registers[1] = 0xabcd;

    listen_socket_ = modbus_tcp_listen(context_, 1);
    if (listen_socket_ == -1)
    {
      modbus_mapping_free(mapping_);
      modbus_free(context_);
      throw std::runtime_error("modbus_tcp_listen failed");
    }

    sockaddr_in address{};
    socklen_t address_length = sizeof(address);
    if (getsockname(listen_socket_, reinterpret_cast<sockaddr*>(&address), &address_length) == -1)
    {
      close(listen_socket_);
      modbus_mapping_free(mapping_);
      modbus_free(context_);
      throw std::runtime_error("getsockname failed");
    }
    port_ = ntohs(address.sin_port);
    server_thread_ = std::thread(&LoopbackModbusServer::serve, this);
  }

  ~LoopbackModbusServer()
  {
    if (server_thread_.joinable())
    {
      server_thread_.join();
    }
    if (listen_socket_ != -1)
    {
      close(listen_socket_);
    }
    modbus_mapping_free(mapping_);
    modbus_free(context_);
  }

  LoopbackModbusServer(const LoopbackModbusServer&) = delete;
  LoopbackModbusServer& operator=(const LoopbackModbusServer&) = delete;

  [[nodiscard]] int port() const noexcept
  {
    return port_;
  }

private:
  void serve()
  {
    int connection_socket = listen_socket_;
    if (modbus_tcp_accept(context_, &connection_socket) == -1)
    {
      return;
    }
    close(listen_socket_);
    listen_socket_ = -1;

    std::array<std::uint8_t, MODBUS_TCP_MAX_ADU_LENGTH> request{};
    while (true)
    {
      const int request_length = modbus_receive(context_, request.data());
      if (request_length <= 0)
      {
        break;
      }
      if (response_mode_ == ResponseMode::normal)
      {
        if (modbus_reply(context_, request.data(), request_length, mapping_) == -1)
        {
          break;
        }
      }
      else if (response_mode_ == ResponseMode::exception)
      {
        if (modbus_reply_exception(context_, request.data(), MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS) == -1)
        {
          break;
        }
      }
      else if (response_mode_ == ResponseMode::malformed)
      {
        const std::array<std::uint8_t, 2> truncated_response{ 0x00, 0x01 };
        const int socket = modbus_get_socket(context_);
        const auto sent = send(socket, truncated_response.data(), truncated_response.size(), 0);
        if (sent != static_cast<ssize_t>(truncated_response.size()))
        {
          break;
        }
        break;
      }
    }
    modbus_close(context_);
  }

  ResponseMode response_mode_;
  int port_{ 0 };
  int listen_socket_{ -1 };
  modbus_t* context_{ nullptr };
  modbus_mapping_t* mapping_{ nullptr };
  std::thread server_thread_;
};

}  // namespace

int main()
{
  using namespace std::chrono_literals;
  using onrobot_control::ModbusTcpConfig;
  using onrobot_control::ModbusTcpTransport;
  using onrobot_control::TransportError;

  {
    LoopbackModbusServer server(ResponseMode::normal);
    ModbusTcpTransport transport({ "127.0.0.1", server.port(), 255, 500ms });
    expect(transport.connect().ok(), "connects to the loopback Modbus server");

    const auto result = transport.read_holding_registers(0, 2);
    expect(result.ok(), "reads holding registers from the loopback server");
    expect(result.registers && result.registers->size() == 2, "returns the requested register count");
    if (result.registers && result.registers->size() == 2)
    {
      expect((*result.registers)[0] == 0x1234, "preserves the first register value");
      expect((*result.registers)[1] == 0xabcd, "preserves the second register value");
    }

    std::array<bool, 4> concurrent_reads{};
    std::array<std::thread, 4> readers;
    for (std::size_t index = 0; index < readers.size(); ++index)
    {
      readers[index] = std::thread([&transport, &concurrent_reads, index]() {
        const auto read_result = transport.read_holding_registers(0, 2);
        concurrent_reads[index] = read_result.ok() && read_result.registers && read_result.registers->at(0) == 0x1234;
      });
    }
    for (auto& reader : readers)
    {
      reader.join();
    }
    for (const bool read_ok : concurrent_reads)
    {
      expect(read_ok, "serializes concurrent loopback register reads");
    }

    transport.disconnect();
    expect(transport.read_holding_registers(0, 1).error == TransportError::not_connected, "reports disconnected reads "
                                                                                          "explicitly");
  }

  {
    LoopbackModbusServer server(ResponseMode::exception);
    ModbusTcpTransport transport({ "127.0.0.1", server.port(), 255, 500ms });
    expect(transport.connect().ok(), "connects before the protocol-exception test");
    const auto result = transport.read_holding_registers(0, 1);
    expect(result.error == TransportError::protocol_error, "maps a Modbus exception response to a protocol error");
    transport.disconnect();
  }

  {
    LoopbackModbusServer server(ResponseMode::malformed);
    ModbusTcpTransport transport({ "127.0.0.1", server.port(), 255, 500ms });
    expect(transport.connect().ok(), "connects before the malformed-response test");
    const auto result = transport.read_holding_registers(0, 1);
    expect(!result.ok(), "rejects a truncated Modbus TCP response");
    expect(!result.registers, "does not return registers from a truncated response");
    expect(result.error == TransportError::io_error || result.error == TransportError::protocol_error, "reports a "
                                                                                                       "truncated "
                                                                                                       "response as an "
                                                                                                       "I/O or "
                                                                                                       "protocol "
                                                                                                       "error, not a "
                                                                                                       "timeout");
    transport.disconnect();
  }

  {
    LoopbackModbusServer server(ResponseMode::silent);
    ModbusTcpTransport transport({ "127.0.0.1", server.port(), 255, 30ms });
    expect(transport.connect().ok(), "connects before the timeout test");
    const auto result = transport.read_holding_registers(0, 1);
    expect(result.error == TransportError::timeout, "maps a response timeout explicitly");
    transport.disconnect();
  }

  return failures == 0 ? 0 : 1;
}