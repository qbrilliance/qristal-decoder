# Copyright (c) 2022 Quantum Brilliance Pty Ltd

#add_qbos_test(ExponentBuilder)
#target_link_libraries(ExponentBuilderTester xacc::xacc xacc::quantum_gate)

#add_qbos_test(QuantumDecoderMark2Builder)
#target_link_libraries(QuantumDecoderMark2BuilderTester xacc::xacc xacc::quantum_gate)

add_qbos_test(RyEncoding)
target_link_libraries(RyEncodingTester xacc::xacc xacc::quantum_gate)
