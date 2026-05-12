// Lab 3 Exercise 1 — exhaustive ALU testbench.
//
// DUT: 2-bit op selects one of {xor, shift-left, modulo, nand} on two 8-bit
// operands. Per the lab, we test all 256x256 = 65,536 (a, b) pairs per opcode.
//
// Verilog quirks the reference model has to mirror:
//   - a % 0 is well-defined in Verilog (Verilator returns 0 in 2-state sim).
//   - a << b truncates to the LHS width; for b >= 8 the 8-bit result is 0.
//     C++ shift by >= int-width is UB, so we guard explicitly.

#include <cstdint>
#include <cstdio>
#include <VExercise1.h>

static uint8_t ref_xor (uint8_t a, uint8_t b) { return a ^ b; }
static uint8_t ref_shl (uint8_t a, uint8_t b) {
  return (b >= 8) ? 0 : static_cast<uint8_t>(a << b);
}
static uint8_t ref_mod (uint8_t a, uint8_t b) {
  return (b == 0) ? 0 : static_cast<uint8_t>(a % b);
}
static uint8_t ref_nand(uint8_t a, uint8_t b) {
  return static_cast<uint8_t>(~(a & b));
}

static int test_op(uint8_t code, uint8_t (*ref)(uint8_t, uint8_t), const char* name) {
  VExercise1 model;
  model.op = code;
  model.a  = 0;
  model.b  = 0;
  do {
    do {
      model.eval();
      uint8_t expected = ref(model.a, model.b);
      if (model.out != expected) {
        std::printf("FAIL op=%s (code=%u) a=%u b=%u  expected=%u got=%u\n",
                    name, code, model.a, model.b, expected, model.out);
        return 1;
      }
    } while (++model.b);
  } while (++model.a);
  return 0;
}

int main() {
  int rc = 0;
  rc |= test_op(0, ref_xor,  "xor");
  rc |= test_op(1, ref_shl,  "shl");
  rc |= test_op(2, ref_mod,  "mod");
  rc |= test_op(3, ref_nand, "nand");
  return rc;
}
