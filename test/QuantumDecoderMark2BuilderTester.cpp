// Copyright (c) 2022 Quantum Brilliance Pty Ltd
#include "Circuit.hpp"
#include "../quantum_decoder_mark2.hpp"
#include "qbos_circuit_builder.hpp"
#include "xacc.hpp"
#include "xacc_service.hpp"
#include <cassert>
#include <cstdlib>
#include <gtest/gtest.h>
#include <iostream>
#include <ostream>
#include <random>
#include <bitset>

TEST(QuantumDecoderMark2CircuitTester, check1) {
  std::vector<std::vector<float>> probability_table = {{0.99999, 0.00001},
                                                       {0.001, 0.999}};
  int qubit_flag = 0;
  std::vector<int> qubits_metric = {1,2,3};
  std::vector<int> qubits_next_metric = {4};//{4,5};
  std::vector<int> qubits_next_letter = {5};//{6,7};
  std::vector<int> qubits_best_score = {6};//{8,9}; // Must be same size as qubits_metric

  qbOS::CircuitBuilder circ;

   //for (int i = 0 ; i < qubits_metric.size(); i++)
     //circ.H(qubits_metric[i]);
   
  const xacc::HeterogeneousMap &map = {
      {"probability_table", probability_table},
      {"qubit_flag", qubit_flag},
      {"qubits_metric", qubits_metric},
      {"qubits_next_metric", qubits_next_metric},
      {"qubits_next_letter", qubits_next_letter},
      {"qubits_best_score", qubits_best_score}};
  qbOS::QuantumDecoder2 build;
  const bool expand_ok = build.expand(map);
  circ.append(build);

  // Measure
  circ.Measure(qubit_flag);
   for (int i = 0; i < qubits_metric.size(); i++)
     circ.Measure(qubits_metric[i]);
  for (int i = 0; i < qubits_next_metric.size(); i++)
    circ.Measure(qubits_next_metric[i]);
   for (int i = 0; i < qubits_next_letter.size(); i++)
     circ.Measure(qubits_next_letter[i]);
  for (int i = 0; i < qubits_best_score.size(); i++)
    circ.Measure(qubits_best_score[i]);

  // Construct the full circuit including preparation of input trial score
  auto circuit = circ.get();
  std::cout << "Final circuit" << std::endl;

  // Run circuit
  //std::cout << circuit->toString() << '\n';
  auto acc = xacc::getAccelerator("qpp", {{"shots", 1024}});
  auto buffer = xacc::qalloc(1 + qubits_metric.size() + qubits_next_metric.size()
                            + qubits_next_letter.size() + qubits_best_score.size());
  acc->execute(buffer, circuit);
  std::cout << "Final circuit" << std::endl;
  buffer->print();
}

int main(int argc, char **argv) {
  xacc::Initialize(argc, argv);
  ::testing::InitGoogleTest(&argc, argv);
  auto ret = RUN_ALL_TESTS();
  xacc::Finalize();
  return ret;
}