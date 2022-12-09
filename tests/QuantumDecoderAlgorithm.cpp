// Copyright (c) 2022 Quantum Brilliance Pty Ltd
#include "Circuit.hpp"
#include "xacc.hpp"
#include "xacc_service.hpp"
#include <gtest/gtest.h>
#include <numeric>

TEST(QuantumDecoderCanonicalAlgorithm, checkSimple) {
  //Initial state parameters:
  std::vector<std::string> alphabet = {"-","a"};

  //Rows represent time step, while columns represent alphabets.
  //For each row, the sum of probabilities across the columns must add up to 1.
  std::vector<std::vector<float>> probability_table{{0.7, 0.3},
                                                    {0.2, 0.8}};

  int L = probability_table.size(); // string length = number of rows of probability_table (number of columns is probability_table[0].size())
  int S = 1; // number of qubits per letter, ceiling(log2(|Sigma|))
  int ml = 3; // metric letter precision
  int ms = std::round(0.49999999 + std::log2(1 + L*(std::pow(2,ml) - 1))); // metric string precision
  int p = ms*(ms+1)/2; // number of precision qubits needed for ae
  int mb = std::round(0.49999999 + std::log2(1 + std::pow(probability_table[0].size(), L)*(std::pow(2,ms) - 1)));; // metric beam precision

  //First, the qubits that aren't ancilla / can't be re-used:
  std::vector<int> qubits_metric;
  for (int i = 0; i < L*ml; i++) {qubits_metric.emplace_back(i);} // L*ml
  std::vector<int> qubits_string;
  for (int i = L*ml; i < L*ml + L*S; i++) {qubits_string.emplace_back(i);} // L*S
  std::vector<int> qubits_init_null;
  for (int i = L*ml+L*S; i < L*(ml+S+1); i++) {qubits_init_null.emplace_back(i);} // L
  std::vector<int> qubits_init_repeat;
  for (int i = L*(ml+S+1); i < L*(ml+S+2); i++) {qubits_init_repeat.emplace_back(i);} // L
  std::vector<int> qubits_superfluous_flags;
  for (int i = L*(ml+S+2); i < L*(ml+S+3); i++) {qubits_superfluous_flags.emplace_back(i);} // L

  std::vector<int> qubits_total_metric_buffer;
  for (int i = L*(ml+S+3); i < L*(ml+S+3) + ms - ml; i++) {qubits_total_metric_buffer.emplace_back(i);} //We need total_metric to have k = log2(1 + L*(2**m - 1)) qubits, so qubits_ancilla_adder.size() = k - m
  std::vector<int> qubits_beam_metric;
  for (int i = L*(ml+S+3) + ms - ml; i < L*(ml+S+3) + ms - ml + mb; i++) {qubits_beam_metric.emplace_back(i);} // k + L
  std::vector<int> qubits_best_score;
  for (int i = L*(ml+S+3) + ms - ml + mb; i < L*(ml+S+3) + ms - ml + 2*mb; i++) {qubits_best_score.emplace_back(i);} //Same size as qubits_beam_metric

  //The remaining qubits can be drawn from an 'ancilla pool'. The size of this ancilla
  //pool needs to be the maximum number of ancilla required at any one time.
  int last_qubit_non_ancilla = qubits_best_score[qubits_best_score.size() - 1] + 1;
  std::vector<int> qubits_ancilla_pool;
  int num_qubits_ancilla_pool = std::max({ml+S, ms-ml, 4+5*ms+2*p+ms+S+L*S+L, 4+p+mb+2*ms+L*S+L});
  for (int i = 0; i < num_qubits_ancilla_pool; i++) {
  qubits_ancilla_pool.push_back(last_qubit_non_ancilla + i);
  }
  int total_num_qubits = qubits_ancilla_pool[qubits_ancilla_pool.size()-1] + 1;
  std::cout<<"num qubits = " << total_num_qubits << "\n";

  const int BestScore = 0; //Initial best score
  int N_TRIALS = 4; //Number of decoder iterations

//  const int iteration = probability_table[0].size(); //Number of columns of probability_table
  const int iteration = probability_table.size(); //Number of rows of probability_table
  const int num_next_letter_qubits = S;
  const int num_next_metric_qubits = ml;

  ////////////////////////////////////////////////////////////////////////////////////////

  //Exponential search parameters
  //Set inputs
  const int num_scoring_qubits = qubits_metric.size();
  auto acc = xacc::getAccelerator("sparse-sim", {{"shots", 1}});
  auto gateRegistry = xacc::getService<xacc::IRProvider>("quantum");
  auto quantum_decoder_algo = xacc::getAlgorithm(
    "quantum-decoder", {{"iteration", iteration},
                        {"probability_table", probability_table},
                        {"qubits_metric", qubits_metric},
                        {"qubits_string", qubits_string},
                        {"method", "canonical"},
                        {"BestScore", BestScore},
                        {"N_TRIALS", N_TRIALS},
                        {"qubits_total_metric_buffer", qubits_total_metric_buffer},
                        {"qubits_init_null", qubits_init_null},
                        {"qubits_init_repeat", qubits_init_repeat},
                        {"qubits_superfluous_flags", qubits_superfluous_flags},
                        {"qubits_beam_metric", qubits_beam_metric},
                        {"qubits_ancilla_pool", qubits_ancilla_pool},
                        {"qubits_best_score", qubits_best_score},
                        {"qpu", acc}});

  auto buffer = xacc::qalloc(total_num_qubits);
  quantum_decoder_algo->execute(buffer);
  auto info = buffer->getInformation();
//  buffer->print();
//  EXPECT_GT(BestScore, 0);

}
