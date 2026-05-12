// Lab 3 Exercise 2 — 16-bit Fibonacci LFSR testbench ("Reading Rainbow").
//
// Reading the SV:
//   - Clocked on NEGATIVE edge.
//   - reset is ACTIVE-HIGH.
//   - On reset: out <= ~init  (note: inverse of init).
//   - On normal step: out <= {out[14:0], out[10] ^ out[8] ^ out[3] ^ out[1]}.
//
// Strategy:
//   Exhaustive is infeasible (2^16 init values * many cycles each).
//   Directed inputs cover the corners: all-zero (LFSR locks at 0 — important!),
//   all-one (max walk), alternating patterns, plus some "interesting bits"
//   values. For each init we (1) reset and check out == ~init, (2) run 200
//   cycles comparing against the software LFSR each step, (3) re-assert reset
//   and confirm the latch behavior still works after a long run.

#include <cstdint>
#include <cstdio>
#include <VExercise2.h>

// Negedge step: drive clk high then low.
static void step(VExercise2& model) {
  model.clk = 1; model.eval();
  model.clk = 0; model.eval();
}

struct LFSR {
  uint16_t value;
  void step() {
    uint16_t fb = ((value >> 10) ^ (value >> 8) ^ (value >> 3) ^ (value >> 1)) & 1u;
    value = static_cast<uint16_t>((value << 1) | fb);
  }
};

static int test_init(uint16_t init) {
  VExercise2 model;
  model.init  = init;
  model.reset = 1;
  model.clk   = 0;
  model.eval();
  step(model);  // negedge with reset=1 latches ~init

  uint16_t expected_initial = static_cast<uint16_t>(~init);
  if (model.out != expected_initial) {
    std::printf("FAIL reset: init=0x%04x  expected out=0x%04x  got=0x%04x\n",
                init, expected_initial, model.out);
    return 1;
  }

  // Release reset and walk the LFSR alongside a software reference.
  model.reset = 0;
  LFSR ref{expected_initial};
  for (int cycle = 0; cycle < 200; ++cycle) {
    if (model.out != ref.value) {
      std::printf("FAIL step: init=0x%04x cycle=%d  expected=0x%04x got=0x%04x\n",
                  init, cycle, ref.value, model.out);
      return 1;
    }
    step(model);
    ref.step();
  }

  // Re-assert reset and confirm the latch still wins over the shift.
  model.reset = 1;
  step(model);
  if (model.out != expected_initial) {
    std::printf("FAIL re-reset: init=0x%04x  expected=0x%04x got=0x%04x\n",
                init, expected_initial, model.out);
    return 1;
  }
  return 0;
}

int main() {
  int rc = 0;
  // Corners + a few "random-looking" but deterministic patterns.
  for (uint16_t init : {uint16_t(0x0000), uint16_t(0xFFFF),
                        uint16_t(0x5555), uint16_t(0xAAAA),
                        uint16_t(0x0001), uint16_t(0x8000),
                        uint16_t(0x1234), uint16_t(0xDEAD),
                        uint16_t(0xBEEF), uint16_t(0xCAFE)}) {
    rc |= test_init(init);
  }
  return rc;
}
