#include <fstream>
#include <random>

#include "benchmark/benchmark.h"
#include "mpc_utils/openssl_uniform_bit_generator.hpp"

namespace mpc_utils {

namespace {

static void BM_OpenSSLUniformBitGenerator(benchmark::State& state) {
  int64_t size = state.range(0);
  std::vector<uint8_t> buf(size);
  OpenSSLUniformBitGenerator rng;
  constexpr int block_size = sizeof(OpenSSLUniformBitGenerator::result_type);
  for (auto _ : state) {
    for (int64_t i = 0; i + block_size < size; i += block_size) {
      *(reinterpret_cast<OpenSSLUniformBitGenerator::result_type*>(&buf[i])) =
          rng();
    }
    benchmark::DoNotOptimize(buf);
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * size);
}
BENCHMARK(BM_OpenSSLUniformBitGenerator)->Range(1, 1 << 24);

static void BM_OpenSSLRaw(benchmark::State& state) {
  int64_t size = state.range(0);
  std::vector<uint8_t> buf(size);
  for (auto _ : state) {
    RAND_bytes(buf.data(), size);
    benchmark::DoNotOptimize(buf);
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * size);
}
BENCHMARK(BM_OpenSSLRaw)->Range(1, 1 << 24);

static void BM_RandomDevice(benchmark::State& state) {
  int64_t size = state.range(0);
  std::vector<uint8_t> buf(size);
  std::random_device rng;
  constexpr int block_size = sizeof(std::random_device::result_type);
  for (auto _ : state) {
    for (int64_t i = 0; i + block_size < size; i += block_size) {
      *(reinterpret_cast<std::random_device::result_type*>(&buf[i])) = rng();
    }
    benchmark::DoNotOptimize(buf);
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * size);
}
BENCHMARK(BM_RandomDevice)->Range(1, 1 << 24);

static void BM_Urandom(benchmark::State& state) {
  int64_t size = state.range(0);
  std::vector<char> buf(size);
  std::ifstream urandom("/dev/urandom");
  for (auto _ : state) {
    urandom.read(buf.data(), size);
    benchmark::DoNotOptimize(buf);
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * size);
}
BENCHMARK(BM_Urandom)->Range(1, 1 << 24);

}  // namespace
}  // namespace mpc_utils