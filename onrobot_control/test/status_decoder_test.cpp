#include "onrobot_control/status_decoder.hpp"

#include <iostream>

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

}  // namespace

int main()
{
  const auto status = onrobot_control::decode_rg_status(0xfff6, 250, 0x45, 240);
  expect(status.fingertip_offset_tenths_mm == -10, "negative offset uses signed two's-complement decoding");
  expect(status.width_tenths_mm == 250 && status.width_with_offset_tenths_mm == 240, "width registers are preserved in "
                                                                                     "tenths of a millimeter");
  expect(status.busy && !status.grip_detected && status.safety_switch_1 && !status.safety_circuit_1 &&
             !status.safety_switch_2 && !status.safety_circuit_2 && status.safety_error,
         "RG status flags map to their documented bit positions");

  const auto clear = onrobot_control::decode_rg_status(0, 0, 0, 0);
  expect(clear.fingertip_offset_tenths_mm == 0 && !clear.busy && !clear.grip_detected && !clear.safety_switch_1 &&
             !clear.safety_circuit_1 && !clear.safety_switch_2 && !clear.safety_circuit_2 && !clear.safety_error,
         "zero registers decode to a clear status");
  return failures == 0 ? 0 : 1;
}