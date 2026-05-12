// Lab 3 Exercise 4 — chip-select mux/decoder testbench.
//
// Spec:
//   cs=0           -> out = 0  (sel and the data inputs are don't-cares)
//   cs=1, sel=0    -> alpha
//   cs=1, sel=1    -> beta
//   cs=1, sel=2    -> gamma
//   cs=1, sel=3    -> alpha & (beta | gamma)
//
// Strategy:
//   For the cs=1 cases, drive 16 random (alpha, beta, gamma) triples and verify
//   each sel value picks the right expression. Using random values for each
//   trial ensures the data lines are actually connected (not stuck-at).
//
//   For the cs=0 "don't care" case, the only way to confirm sel and the data
//   inputs truly have no effect is to exercise them: vary sel over every value
//   AND randomize alpha/beta/gamma each trial. If the output is ever non-zero,
//   one of those "don't care" inputs is actually leaking through.

#include <cstdint>
#include <cstdio>
#include <random>
#include <VExercise4.h>

static int check(VExercise4& model, uint8_t expected, const char* label) {
  model.eval();
  if (model.out != expected) {
    std::printf("FAIL %s: cs=%u sel=%u alpha=%u beta=%u gamma=%u  expected=%u got=%u\n",
                label,
                static_cast<unsigned>(model.cs),    static_cast<unsigned>(model.sel),
                static_cast<unsigned>(model.alpha), static_cast<unsigned>(model.beta),
                static_cast<unsigned>(model.gamma),
                expected, static_cast<unsigned>(model.out));
    return 1;
  }
  return 0;
}

int main() {
  VExercise4 model;
  int rc = 0;
  std::mt19937 rng{42u};
  std::uniform_int_distribution<unsigned> rand8{0, 255};

  // ---- cs=1: directed sel sweep across random data triples ----
  model.cs = 1;
  for (int trial = 0; trial < 16; ++trial) {
    uint8_t a = static_cast<uint8_t>(rand8(rng));
    uint8_t b = static_cast<uint8_t>(rand8(rng));
    uint8_t g = static_cast<uint8_t>(rand8(rng));
    model.alpha = a;
    model.beta  = b;
    model.gamma = g;

    model.sel = 0; rc |= check(model, a, "cs=1 sel=0");
    model.sel = 1; rc |= check(model, b, "cs=1 sel=1");
    model.sel = 2; rc |= check(model, g, "cs=1 sel=2");
    model.sel = 3; rc |= check(model, static_cast<uint8_t>(a & (b | g)), "cs=1 sel=3");
  }

  // ---- cs=0: don't-care sweep — out must stay 0 regardless of sel / data ----
  model.cs = 0;
  for (int trial = 0; trial < 256; ++trial) {
    model.alpha = static_cast<uint8_t>(rand8(rng));
    model.beta  = static_cast<uint8_t>(rand8(rng));
    model.gamma = static_cast<uint8_t>(rand8(rng));
    for (uint8_t s = 0; s < 4; ++s) {
      model.sel = s;
      rc |= check(model, 0, "cs=0 don't-care");
    }
  }

  return rc;
}
