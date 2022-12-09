// Copyright (c) 2022 Quantum Brilliance Pty Ltd
#include "qb/decoder/simplified_decoder.hpp"

#include "Algorithm.hpp"
#include "xacc.hpp"
#include "xacc_service.hpp"

#include <assert.h>
#include <bitset>
#include <iomanip>
#include <memory>
#include <string>

namespace qb {
bool SimplifiedDecoder::initialize(const xacc::HeterogeneousMap &parameters) {

  //std::vector<std::vector<float>> probability_table;
  if (!parameters.keyExists<std::vector<std::vector<float>>>("probability_table")) {
      return false;
  }
  probability_table = parameters.get<std::vector<std::vector<float>>>("probability_table");

  //std::vector<int> qubits_string;
  if (!parameters.keyExists<std::vector<int>>("qubits_string")) {
      return false;
  }
  qubits_string = parameters.get<std::vector<int>>("qubits_string");

  nb_timesteps = probability_table.size();
  nq_string = qubits_string.size() ;
  nq_symbol = nq_string/nb_timesteps;
  assert(nb_timesteps*nq_symbol == nq_string);

  //////////////////////////////////////////////////////////////////////////////////////

  //Parameters for comparator oracle in exponential search
  int BestScore = parameters.get_or_default("BestScore", 0);

  qubits_best_score = {};
  if (parameters.keyExists<std::vector<int>>("qubits_best_score")) {
    qubits_best_score = parameters.get<std::vector<int>>("qubits_best_score");
  }

  //////////////////////////////////////////////////////////////////////////////////////

  // Which algorithm to use for the simplified decoder
  method = "ry";
  if (parameters.keyExists<std::string>("method")) {
      method = parameters.get<std::string>("method");
      // Assert that given builder_name is acceptable and
  }

  //////////////////////////////////////////////////////////////////////////////////////

  //Initialize qpu accelerator
  qpu_ = nullptr;
  if (parameters.stringExists("qpu")) {
    static auto acc =
        xacc::getAccelerator(parameters.getString("qpu"), {{"shots", 1}});
    qpu_ = acc.get();
  } else if (parameters.pointerLikeExists<xacc::Accelerator>("qpu")) {
    qpu_ = parameters.getPointerLike<xacc::Accelerator>("qpu");
  }

  if (!qpu_) {
    static auto qpp = xacc::getAccelerator("qpp", {{"shots", 1}});
    // Default to qpp if none provided
    qpu_ = qpp.get();
  }

  return true;
} //SimplifiedDecoder::initialize

/////////////////////////////////////////////////////////////////////////////////////////////

const std::vector<std::string> SimplifiedDecoder::requiredParameters() const {
  return {"probability_table", "qubits_string"};
          //"method", "BestScore", "iteration", "qubits_metric", "qubits_beam_metric", "qubits_superfluous_flags",
          //"num_scoring_qubits", "qubits_init_null", "qubits_init_repeat",
          //"qubits_best_score", "qubits_ancilla_oracle", "N_TRIALS"};
}

/////////////////////////////////////////////////////////////////////////////////////////////

void SimplifiedDecoder::execute(
    const std::shared_ptr<xacc::AcceleratorBuffer> buffer) const {

    qb::CircuitBuilder circ;

    const int nq_symbol_const = nq_symbol;

    if ("ry" == method) {
        const xacc::HeterogeneousMap &map = {
            {"probability_table", probability_table},
            {"qubits_string", qubits_string}};

        qb::RyEncoding build;
        const bool expand_ok = build.expand(map);
        circ.append(build);
    }

    // Measure
    for (int qubit : qubits_string) {
        circ.Measure(qubit);
    }

    // Construct the full circuit including preparation of input trial score
    //std::cout << "\ncirc.get()" << std::endl;
    auto circuit = circ.get();

    // Run circuit
    //std::cout << circuit->toString() << '\n';
    qpu_->execute(buffer, circuit);  // acc
    std::map<std::string, int> measurements = buffer->getMeasurementCounts();
    std::string output_string = buffer->toString() ;
    std::cout << "output_string:\n" << output_string << std::endl;

  /////////////////////////////////////////////////////////////////////////////////////////////

  //Initiate kernel function
  const std::function<std::string(std::string)> f_kernel_ = [&](std::string string_) {
      std::string no_repeats_;
      std::string beam_;
      std::string null_char_ ;
      //std::cout << "f_kernel_  " << string_ << std::endl;
      // nq_symbol cannot be known at compile time
      switch (nq_symbol) {
          case 1:
              null_char_ = std::bitset<1>(0).to_string();
              break;
          case 2:
              null_char_ = std::bitset<2>(0).to_string();
              break;
      }

      // Contract repeats in string_
      std::string current_char_ = string_.substr(0,nq_symbol);
      int current_place_ = 0;  // Last new symbol
      int next_place_ = current_place_;
      std::string next_char_ = current_char_;    // Next non-repeat symbol
      int no_repeat_length = 0;
      while (current_place_ < nb_timesteps) {
          no_repeats_ += current_char_;
          no_repeat_length++;
          next_place_++;
          next_char_ = string_.substr(next_place_*nq_symbol,nq_symbol);
          while ((next_char_ == current_char_) and (current_place_ < nb_timesteps)) {
              current_place_ = next_place_;
              next_place_++;
              next_char_ = string_.substr(next_place_*nq_symbol,nq_symbol);
          }
          current_char_ = next_char_;
          current_place_ = next_place_;
      }

      // Remove nulls from no_repeats_.
      current_place_ = -1;
      next_char_ = "";
      while (current_place_ < no_repeat_length) {
          beam_ += next_char_;  // no_repeats_.substr(current_place_, next_place_-(nq_symbol*current_place_));
          current_place_++;
          next_char_ = no_repeats_.substr(current_place_*nq_symbol,nq_symbol);
          if ((next_char_ == null_char_) & (current_place_ < no_repeat_length)){
              current_place_++;
              next_char_ = no_repeats_.substr(current_place_*nq_symbol,nq_symbol);
          }
          //std::cout << "beam: " << beam_ << std::endl;
      }
      //std::cout << "final beam:" << beam_ << std::endl;

      //buffer->addExtraInfo("output string",beam_);

      return beam_; };


  /////////////////////////////////////////////////////////////////////////////////////////////


    std::map<std::string, int>::iterator iter;
    std::map<std::string, int> beams;
    std::string input_string;
    std::string beam;
    std::cout << "beams" << std::endl;
    for (iter = measurements.begin(); iter != measurements.end(); iter++){
        input_string = iter->first;
        beam = f_kernel_(input_string);
        if (beams.count(beam) == 0) {
            beams[beam] = 0;
        }

        beams[beam] += iter->second;
    }

    //buffer->addExtraInfo("output_strings", (std::map<std::string, int>) beams);
    std::cout << "max beam:" ;
    auto max_beam_entry = std::max_element(beams.begin(),beams.end(), [](const auto &x, const auto &y){
        return x.second < y.second;
    });
    std::string max_beam = max_beam_entry->first;
    std::cout << max_beam << std::endl;
    buffer->addExtraInfo("best_beam", max_beam);  //->first);
    // Output beams and their shot counts
    int nb_beams = 0;
    for (iter = beams.begin(); iter != beams.end(); iter++ ) {
        std::cout << iter->first << ": " << iter->second << std::endl;
        std::string beam_label = "beam_" + std::to_string(nb_beams);
        buffer->addExtraInfo(beam_label,(std::string) iter->first);
        std::string beam_count_label = "beam_count_" + std::to_string(nb_beams);
        buffer->addExtraInfo(beam_count_label,(int) iter->second);
        nb_beams++;
    }
    buffer->addExtraInfo("nb_beams",nb_beams);


} // SimplifiedDecoder::execute
} // namespace qb
REGISTER_PLUGIN(qb::SimplifiedDecoder, xacc::Algorithm)
