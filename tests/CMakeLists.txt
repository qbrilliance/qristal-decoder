# Copyright (c) 2022 Quantum Brilliance Pty Ltd

add_gtest(QuantumDecoderAlgorithm)
target_link_libraries(QuantumDecoderAlgorithmTester xacc::xacc xacc::quantum_gate)
