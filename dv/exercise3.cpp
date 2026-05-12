// Lab 3 Exercise 3 — composition testbench (two Mystery1's feeding Mystery2).
//
// Mystery1 (combinational), 2-bit a select:
//   a=0: d = {2'b00, b[2:0], c[2:0]}    (top two bits are a, which is 0)
//   a=1: d = {2'b01, c[2:0], b[2:0]}    (top two bits are a, which is 1)
//   a=2: d = b
//   a=3: d = c
//
// Mystery2 (clocked NEGEDGE, reset active-HIGH):
//   reset:           out = {b_in, a_in}        (b_in is high byte)
//   state 0:         out = {a_in, out[7:0]}
//   state 1:         out = {out[15:8], b_in}
//   state 2:         out = {out[7:0], out[15:8]}     (byte swap)
//   state 3:         out = {out[3:0], out[7:4], out[11:8], out[15:12]}  (nibble reverse)
//   state 4:         out = {15'b0, ^out}             (xor reduction)
//   count cycles 0..4 (mod 5).
//
// Top wiring (per the schematic in the lab PDF):
//   a_in <- Mystery1(a[1:0], b[7:0],  c[7:0])
//   b_in <- Mystery1(a[3:2], b[15:8], c[15:8])
//   out  <- Mystery2(clk, reset, a_in, b_in)
//
// Strategy: build a software reference mirroring the schematic, then drive the
// DUT with a fixed-seed PRNG so failures reproduce.

#include <cstdint>
#include <cstdio>
#include <random>
#include <VExercise3.h>

static uint8_t mystery1(uint8_t a, uint8_t b, uint8_t c) {
  a &= 0x3;
  switch (a) {
    case 0: return static_cast<uint8_t>((0u << 6) | ((b & 0x7) << 3) | (c & 0x7));
    case 1: return static_cast<uint8_t>((1u << 6) | ((c & 0x7) << 3) | (b & 0x7));
    case 2: return b;
    default: return c;  // case 3
  }
}

struct Mystery2Ref {
  uint8_t  count{0};
  uint16_t out{0};

  uint16_t reset(uint8_t a_in, uint8_t b_in) {
    out   = static_cast<uint16_t>((b_in << 8) | a_in);
    count = 0;
    return out;
  }

  uint16_t step(uint8_t a_in, uint8_t b_in) {
    switch (count) {
      case 0:
        out = static_cast<uint16_t>((a_in << 8) | (out & 0xFF));
        break;
      case 1:
        out = static_cast<uint16_t>((out & 0xFF00) | b_in);
        break;
      case 2:
        out = static_cast<uint16_t>(((out & 0xFF) << 8) | (out >> 8));
        break;
      case 3: {
        uint16_t n0 = (out >>  0) & 0xF;
        uint16_t n1 = (out >>  4) & 0xF;
        uint16_t n2 = (out >>  8) & 0xF;
        uint16_t n3 = (out >> 12) & 0xF;
        out = static_cast<uint16_t>((n0 << 12) | (n1 << 8) | (n2 << 4) | n3);
        break;
      }
      case 4: {
        uint16_t parity = 0;
        for (int i = 0; i < 16; ++i) parity ^= (out >> i) & 1u;
        out = parity;
        break;
      }
    }
    count = (count + 1) % 5;
    return out;
  }
};

struct Exercise3Ref {
  Mystery2Ref state;

  uint16_t reset(uint8_t a, uint16_t b, uint16_t c) {
    uint8_t a_in = mystery1( a       & 0x3,  b        & 0xFF,  c        & 0xFF);
    uint8_t b_in = mystery1((a >> 2) & 0x3, (b >> 8)  & 0xFF, (c >> 8)  & 0xFF);
    return state.reset(a_in, b_in);
  }

  uint16_t step(uint8_t a, uint16_t b, uint16_t c) {
    uint8_t a_in = mystery1( a       & 0x3,  b        & 0xFF,  c        & 0xFF);
    uint8_t b_in = mystery1((a >> 2) & 0x3, (b >> 8)  & 0xFF, (c >> 8)  & 0xFF);
    return state.step(a_in, b_in);
  }
};

// Negedge step: clk high then low.
static void step(VExercise3& model) {
  model.clk = 1; model.eval();
  model.clk = 0; model.eval();
}

int main() {
  VExercise3   model;
  Exercise3Ref ref;

  std::mt19937 rng{0xC0FFEEu};  // fixed seed -> reproducible failures
  std::uniform_int_distribution<unsigned> rand4 {0,    15};
  std::uniform_int_distribution<unsigned> rand16{0, 0xFFFF};

  // Reset for a few cycles with varying inputs.
  model.clk   = 0;
  model.reset = 1;
  for (int i = 0; i < 3; ++i) {
    model.a = rand4(rng);
    model.b = rand16(rng);
    model.c = rand16(rng);
    uint16_t expected = ref.reset(model.a, model.b, model.c);
    step(model);
    if (model.out != expected) {
      std::printf("FAIL reset cycle=%d  expected=0x%04x got=0x%04x\n",
                  i, expected, model.out);
      return 1;
    }
  }

  // Release reset and run a long randomized sweep covering many state-machine loops.
  model.reset = 0;
  for (int cycle = 0; cycle < 500; ++cycle) {
    model.a = rand4(rng);
    model.b = rand16(rng);
    model.c = rand16(rng);
    uint16_t expected = ref.step(model.a, model.b, model.c);
    step(model);
    if (model.out != expected) {
      std::printf("FAIL step cycle=%d  a=0x%x b=0x%04x c=0x%04x  expected=0x%04x got=0x%04x\n",
                  cycle, model.a, model.b, model.c, expected, model.out);
      return 1;
    }
  }
  return 0;
}
