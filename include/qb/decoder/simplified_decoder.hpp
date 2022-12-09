// Copyright (c) 2022 Quantum Brilliance Pty Ltd

#include "qb/core/circuit_builders/ry_encoding.hpp"

#include "Algorithm.hpp"
#include "IRProvider.hpp"
#include "InstructionIterator.hpp"
#include "xacc.hpp"
#include "xacc_plugin.hpp"
#include "xacc_service.hpp"

#include <assert.h>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <random>
#include <sstream>
#include <vector>

#pragma once

namespace qb {
class SimplifiedDecoder : public xacc::Algorithm {
private:
  // Function performs kernel operation post-measurement
  std::function<std::string(std::string)> f_kernel_; //Converts selected ionput string to form of corresponding output string (beam)
  xacc::Accelerator *qpu_;          //Accelerator, optional

  //Qubit registers
  std::vector<int> qubits_best_score;

  //Choose which method to encode strings and probabilities. Currently supported methods are:
  //"ry" - ry-rotations (default)
  //"aa" - using Amplitude Amplification
  std::string method;

  //Probability table
  std::vector<std::vector<float>> probability_table;

  //qubits encoding string
  std::vector<int> qubits_string;

  // Qubit registers for decoder kernel
  //std::vector<int> qubits_ancilla_pool;
  //std::vector<int> qubits_beam_metric;
  //std::vector<int> evaluation_bits;
  //std::vector<int> precision_bits;

private:
public:
  bool initialize(const xacc::HeterogeneousMap &parameters) override;
  const std::vector<std::string> requiredParameters() const override;

  void
  execute(const std::shared_ptr<xacc::AcceleratorBuffer> buffer) const override;

  int nb_timesteps;
  int nq_string;
  int nq_symbol;


  const std::string name() const override {
    return "simplified-decoder";
  }
  const std::string description() const override {
    return "Simplified Quantum Decoder";
  }

  DEFINE_ALGORITHM_CLONE(SimplifiedDecoder)
};
} // namespace qb
