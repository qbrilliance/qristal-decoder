// Copyright (c) 2022 Quantum Brilliance Pty Ltd
#include "Circuit.hpp"
#include "xacc.hpp"
#include "xacc_service.hpp"
#include <cmath>
#include <gtest/gtest.h>
#include <math.h>
#include <numeric>
#include <string>
#include <type_traits>

//std::map<std::string, int>
void check_output_strings(std::vector<std::vector<float>> prob_table, std::vector<int> qubits_string,
                           std::vector<std::string> best_beams, std::vector<std::string> included_beams, std::vector<std::string> excluded_beams) {
  //Set inputs
  auto acc = xacc::getAccelerator("sparse-sim", {{"shots", 1000}});
  auto gateRegistry = xacc::getService<xacc::IRProvider>("quantum");
  std::cout << "getAlgorithm()" << std::endl;
  auto simplified_decoder_algo = xacc::getAlgorithm(
    "simplified-decoder", {{"probability_table", prob_table},
                        {"qubits_string", qubits_string},
                        {"qpu", acc}});

  auto buffer = xacc::qalloc((int)qubits_string.size());
  std::cout << "execute()" << std::endl;
  simplified_decoder_algo->execute(buffer);

  // Verify best beam
  std::cout << "best beam:" ;
  auto info = buffer->getInformation();
  std::string max_beam = info.at("best_beam").as<std::string>();
  std::cout << max_beam <<  std::endl;
  if (best_beams.size() > 0) {
      assert(("Vector best_beams does not contain the best beam " + max_beam,std::find(best_beams.begin(),best_beams.end(),max_beam) != best_beams.end()));
  }

  // Verify returned beams
  // Excluded beams
  int nb_beams = info.at("nb_beams").as<int>();
  std::vector<std::string> found_beams;
  std::cout << "nb_beams:" << nb_beams << std::endl;
  for (int count = 0; count < nb_beams; count++) {
      std::string label_ = "beam_" + std::to_string(count) ;
      std::string beam_ = info.at(label_).as<std::string>();
      found_beams.push_back(beam_);
      assert(("Found excluded beam " + beam_,std::find(excluded_beams.begin(),excluded_beams.end(),beam_) == excluded_beams.end()));
      std::cout << label_ << ": " << beam_ ;
      label_ = "beam_count_" + std::to_string(count) ;
      std::cout << "   " << (int) info.at(label_).as<int>() << std::endl;
  }
  // Included beams
  for (std::string beam_ : included_beams) {
      assert(std::find(found_beams.begin(),found_beams.end(),beam_) != found_beams.end());
  }

  // Verify number of beams
  int nb_timesteps_ = prob_table.size();
  int nb_symbols_ = prob_table[0].size();
  int nq_symbol_ = std::ceil(std::log2(nb_symbols_)) ;

  int upper_nb_beams = std::pow(nb_symbols_ - 1,nb_timesteps_);    // String repetitions and leading nulls removed
  // Repetitions and leading nulls gone, remove remaining nulls
  std::cout << "upper_nb_beams: " << upper_nb_beams << " ";

  std::cout << upper_nb_beams << " ";
  // Remove non-leading nulls but not ending nulls
  // Subtract number of leading non-null followed by null followed by non-null different from leading (non-null) symbol
  if (nb_timesteps_ > 2) {
      upper_nb_beams -= (nb_symbols_ - 1) * std::pow(nb_symbols_-2,nb_timesteps_-2) * (nb_timesteps_-2);    // Leading non-null, null , nb_timesteps_-2 non-repetitive non-null, for null at nb_timesteps_-2
      std::cout << upper_nb_beams << " " ;
  }
  if (nb_timesteps_ > 4) {
      upper_nb_beams -= (nb_symbols_ - 1) * std::pow(nb_symbols_-2,nb_timesteps_-3) * (nb_timesteps_-4);    // Two nulls minimal distance apart
  }
  // More generally, second null can be from 2 to nb_timesteps_-3 places after first one//. 0.5 * ((nb_timesteps-4)*(nb_timesteps-3)
  std::cout << std::endl;

  assert(("Wrong number of beams",nb_beams < upper_nb_beams));

}



TEST(SimplifiedDecoderAlgorithm, checkSimple) {
  //Initial state parameters:
  std::vector<int> qubits_string ;
  std::vector<std::string> alphabet = {"-","a","b","c"};
  int nb_symbols = alphabet.size();
  int nq_symbol = std::ceil(std::log2(nb_symbols)) ;

  //Rows represent time step, while columns represent alphabets.
  //For each row, the sum of probabilities across the columns must add up to 1.
  std::vector<std::vector<float>> probability_table = {{0.0, 0.5, 0.5, 0.0}, {0.25, 0.25, 0.25, 0.25}};

  int nb_timesteps = probability_table.size();  //Number of columns of probability_table // string length = number of rows of probability_table (number of columns is probability_table[0].size())
  //int nq_symbol = 2; // number of qubits per letter, ceiling(log2(|Sigma|))


  for (int i = 0; i < nb_timesteps*nq_symbol; i++) {
      qubits_string.emplace_back(i);
  }

  std::cout<<"num qubits = " << nb_timesteps*nq_symbol << " " ;
  std::cout << " " << qubits_string.size() << "\n";

  int N_TRIALS = 1; //Number of decoder iterations

  ////////////////////////////////////////////////////////////////////////////////////////


  check_output_strings(probability_table, qubits_string,
                           {"01", "10"}, {}, {}) ;


}

