/***
 *** Copyright (c) 2022 Quantum Brilliance Pty Ltd
 ***/

#include "qb/decoder/decoder_kernel.hpp"
#include "qb/decoder/quantum_decoder.hpp"

#include "cppmicroservices/BundleActivator.h"
#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/ServiceProperties.h"

using namespace cppmicroservices;

class US_ABI_LOCAL qbDecoderActivator : public BundleActivator {
public:
  qbDecoderActivator() {}

  void Start(BundleContext context)
  {
      // Register DecoderKernel as both Circuit and Instruction classes.
      // i.e., can use both xacc::getService<xacc::Instruction> and xacc::getService<xacc::quantum::Circuit> to retrieve it.
      context.RegisterService<xacc::quantum::Circuit>(
          std::make_shared<qb::DecoderKernel>());
      context.RegisterService<xacc::Instruction>(
          std::make_shared<qb::DecoderKernel>());
      // Register QuantumDecoder as an Algorithm
      context.RegisterService<xacc::Algorithm>(
          std::make_shared<qb::QuantumDecoder>());
  }

  void Stop(BundleContext) {}
};

CPPMICROSERVICES_EXPORT_BUNDLE_ACTIVATOR(qbDecoderActivator)
