// Copyright (c) 2022 Quantum Brilliance Pty Ltd

/*
Quantum Decoder Mark II
Algorithm:
1. Encodes strings with prob. amplitudes proportional to the square root of their
probabilities
2. Perform the measurement

Outcome:
- Both strings and beams should be chosen in proportion to their total probability

Breakdown of algorithm:
A. State Preparation
   1. Apply Hadamard gates to each symbol-related qubit
   2. Entangle each symbol with its metric, which we take to be the
      negative log of its probability from the given probability table
   3. Perform Amplitude Amplification to each register to render the
      square of their amplitudes in the same proportions as their given
      probabilities.

   Note: The metric value entangled with each symbol at each timestep taken to be
         the negative of the logarithm (base 10), rounded to the nearest integer.
         More probable symbols have lower metrics rather than higher. In fact,
         the Trellis data usually has approximately 0.99 for the most probable
         symbol so its metric will be zero. It follows that the good states are
         those with lower metrics rather than higher ones.

B. Amplitude Amplification
   - The final process at each timestep is to amplify the probable symbols at the
     expense of the less probable ones. This requires the proportion of symbols
     with each metric but amplitude estimation is not required to determine this
     because it is available classically from the probability table. From there
     the correct number of Grover iterations can be calculated
   - TODO: For now just do two iterations for each timestep, or similar.

C. Measure results
   - To avoid the measurement simply choosing the highest metric symbol at each
     step it is necessary to calculate and entangle the total metric for each
     string. We take the sum of the symbol metrics which by the above convention
     gives the log of the string metric. This leaves strings entangled with the
     own metrics with probability amplitudes which reproduce their probabilities.
   - The final quantum operation is to measure the register and process the
     obtained string. 
   - This string now requires some classical processing to obtain the correct
     output beam, meaning the process of contracting repetitions and then
     removing null symbols. This is not difficult classically and only needs to
     be done once

Worklog: https://www.notion.so/quantumbrilliance/220815-Worklog-Decoder-Mark-II-01a41031ecee4f5a999986924fb4d528
*/

#pragma once
#include "qbos_circuit_builder.hpp"
#include "CompositeInstruction.hpp"
#include "Circuit.hpp"
#include "IRProvider.hpp"
#include "InstructionIterator.hpp"
#include "Accelerator.hpp"
#include "xacc.hpp"
#include "xacc_service.hpp"
#include <algorithm>
#include <assert.h>
#include <cmath>
#include <math.h>
#include <iostream>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>
#include <bitset>

namespace qbOS {
  class QuantumDecoder2 : public qbOS::CircuitBuilder {
  public:
    QuantumDecoder2() : qbOS::CircuitBuilder() {} ;

    const std::vector<std::string> requiredKeys() {
      return {"probability_table", "qubit_flag", "qubits_metric",
              "qubits_next_metric", "qubits_next_string", "qubits_best_score"};
    }

    bool expand(const xacc::HeterogeneousMap &runtimeOptions) {
      //////////////////////////////////////////////////////////////////////////////////////

      // Load qubits
      if (!runtimeOptions.keyExists<std::vector<std::vector<float>>>("probability_table")) {
        return false;
      }
      std::vector<std::vector<float>> probability_table = runtimeOptions.get<std::vector<std::vector<float>>>("probability_table");

      if (!runtimeOptions.keyExists<std::vector<int>>("qubits_next_metric")) {
        return false;
      }
      std::vector<int> qubits_next_metric = runtimeOptions.get<std::vector<int>>("qubits_next_metric");

      if (!runtimeOptions.keyExists<std::vector<int>>("qubits_next_letter")) {
        return false;
      }
      std::vector<int> qubits_next_letter = runtimeOptions.get<std::vector<int>>("qubits_next_letter");

      if (!runtimeOptions.keyExists<std::vector<int>>("qubits_metric")) {
        return false;
      }
      std::vector<int> qubits_metric = runtimeOptions.get<std::vector<int>>("qubits_metric");

      if (!runtimeOptions.keyExists<int>("qubit_flag")) {
        return false;
      }
      int qubit_flag = runtimeOptions.get<int>("qubit_flag");

      if (!runtimeOptions.keyExists<std::vector<int>>("qubits_best_score")) {
        return false;
      }
      std::vector<int> qubits_best_score = runtimeOptions.get<std::vector<int>>("qubits_best_score");

      //////////////////////////////////////////////////////////////////////////////////////

      // Load qpu accelerator parameter
      xacc::Accelerator *qpu_ = nullptr; // WHAT CIRCUIT BUILDER COMMAND TO USE IN ORDER TO AVOID CALLING XACC::ACCELERATOR???
      if (runtimeOptions.stringExists("qpu")) {
        static auto acc = xacc::getAccelerator(runtimeOptions.getString("qpu"), {{"shots", 1}});
        qpu_ = acc.get();
      } else if (runtimeOptions.pointerLikeExists<xacc::Accelerator>("qpu")) {
        qpu_ = runtimeOptions.getPointerLike<xacc::Accelerator>("qpu");
      }

      if (!qpu_) { // Default to qpp if none provided
        static auto qpp = xacc::getAccelerator("qpp", {{"shots", 1}});
        qpu_ = qpp.get();
      }

      //////////////////////////////////////////////////////////////////////////////////////

      std::vector<float> probability_column;

      // Scoring function: Converts elements of the probability table into binary.
      std::function<int(int)> scoring_function = [&](int symbol) {
        int temp = qubits_next_letter.size();
        float flt = probability_column[symbol];
        int num_scoring_qubits = qubits_next_metric.size();

        std::string binary;
        if (flt == 1.0) {
          for (int i = 0; i < num_scoring_qubits; i++)
            binary.push_back('1');
        } else if (flt == 0.0) {
          for (int i = 0; i < num_scoring_qubits; i++)
            binary.push_back('0');
        } else {
          while (temp) {
            flt *= 2;
            int fract_bit = flt;
            if (fract_bit == 1) {
              flt -= fract_bit;
              binary.push_back('1');
            } else {
              binary.push_back('0');
            }
            --temp;
          }
        }
        int score = std::stoi(binary, 0, 2);
        return score;
      };

      //////////////////////////////////////////////////////////////////////////////////////

      // Binary to gray code function: Converts binary value to gray code.
      std::function<std::string(std::string)> gray_code = [&](std::string binary) {
        char first_bit = binary[0];
        std::string gray_code;
        if (first_bit == '1') {
          gray_code = "1";
        }
        if (first_bit == '0') {
          gray_code = '0';
        }
        for (int i = 0; i < binary.size() - 1; i++) {
          char ex_or = ((binary[i] + binary[i + 1]) % 2 == 1) ? '1' : '0';
          gray_code = gray_code + ex_or;
        }
        return gray_code;
      };

      //////////////////////////////////////////////////////////////////////////////////////

      // Integer to string binary function: Converts an integer number to its
      // binary value. Input is an integer. Output is a string.
      std::function<std::string(int, int, bool)> binary = [&](int i, int num_qubits, bool is_LSB) {
        std::string i_binary = std::bitset<8*sizeof(i)>(i).to_string();
        std::string i_binary_n = i_binary.substr(
            i_binary.size() < num_qubits ? 0 : i_binary.size() - num_qubits);

        if (is_LSB)
          reverse(i_binary_n.begin(), i_binary_n.end());
        return i_binary_n;
      };

      //////////////////////////////////////////////////////////////////////////////////////

      // Determine the number of Grover iteration to use in the Amplitude Amplification.
      int precision = qubits_next_metric.size(); // metrics > 2^(precision) - 2 all given maximum metric
      int max_metric = pow(2, precision) - 1;
      int nb_alphabet = pow(2, qubits_metric.size());
      std::map<int, int> metric_count; // Used for metric_proportions later
      for (int i = 0; i <= max_metric; i++)
        metric_count[i] = 0;

      // Take the *negative* square root logarithm (i.e. -log2(sqrt(probability)))
      // of the probability table 
      std::vector<std::vector<float>> negative_log_prob_table;
      for (auto i : probability_table) {
        std::vector<float> log_i;
        for (auto j : i) {
          int log_j = -std::floor(std::log(sqrt(j))); // Base 10 reflects Trellis data better
          log_j = std::min(max_metric, log_j);
          metric_count[log_j]++ ; // Construct data to calculate the number of Grover iterations
          log_i.push_back(-std::log(sqrt(j)));
std::cout<<"j:"<<j<<", -log(sqrt(j)):"<<-std::log(sqrt(j))<<
", -floor(log(sqrt(j))):"<<-std::floor(std::log(sqrt(j)))<<", log_j:"<<log_j<<
", metric_count:"<<metric_count[log_j]<<std::endl;
        }
        negative_log_prob_table.push_back(log_i);
      }
std::cout<<std::endl;

      std::map<int, float> metric_proportions;
      std::map<int, int> :: iterator iter;
      for (iter = metric_count.begin(); iter != metric_count.end(); iter++)
        metric_proportions[iter->first] = static_cast<float>(iter->second)/nb_alphabet;

      //////////////////////////////////////////////////////////////////////////////////////

      const int string_length = probability_table.size(); // Number of rows of probability_table. The rows represent the string length or time steps.
      const int alphabet_size = probability_table[0].size(); //Number of columns of probability_table. Columns represent alphabets size.

      // Parameters for EfficientEncoding()
      bool is_LSB = true;
      bool use_ancilla = false;
      std::vector<int> qubits_init_flag = {};
      int flag_integer = 0;

      // Parameters for EqualityChecker()
      std::vector<int> qubits_ancilla;
      std::vector<int> controls_on;
      std::vector<int> controls_off;

      // Apply Hadamard gate to each symbol-related qubit
      for (int i = 0; i < qubits_next_letter.size(); i++)
        this->H(qubits_next_letter[i]);

      for (int it = 0; it < 1 ; it++) {  //string_length; it++) {
        /*std::vector<float>*/ probability_column = negative_log_prob_table[it]; // This should be passed into scoring_function() when it is called in EfficientEncoding().
        //this->EfficientEncoding(scoring_function, qubits_next_letter.size(),
        //        qubits_next_metric.size(), qubits_next_letter, qubits_next_metric,
        //        is_LSB, use_ancilla, qubits_init_flag, flag_integer);

        //auto state_prep = loop_builder.get() ; // USE THE STATE PREP CIRCUIT USED IN STATE ENCODING?
        CircuitBuilder state_builder = CircuitBuilder();
        state_builder.EfficientEncoding(scoring_function, qubits_next_letter.size(),
                qubits_next_metric.size(), qubits_next_letter, qubits_next_metric,
                is_LSB, use_ancilla, qubits_init_flag, flag_integer);

        // Perform Amplitude Amplification on the state prep circuit
        // Check value in qubits_metric is less successively diminishing possible values from maximum down to one
        // Possible values are zero to maximum inclusive
        int max_metric_int = std::floor(max_metric);
        for (int best_score = max_metric_int; best_score >= 0; best_score--) {// TODO: ADJUSTABLE LOOP INTERVALS
          // Number of Grover iterations
          int nb_grovers = ceil(M_PI / (4 * sqrt(metric_proportions[best_score])));
std::cout<<"it:"<<it<<", best_score:"<<best_score<<", metric_proportions:"<<metric_proportions[best_score]<<", nb_grovers:"<<nb_grovers<<std::endl;

          if (nb_grovers < 0) {
              std::cout<<"nb_grovers had negative value " << nb_grovers << ", now it is one" << std::endl; 
              nb_grovers = 1;
          }

          if (nb_grovers > 0) {
std::cout<<"Performing Amplitude Amplification!"<<std::endl;
            // Convert best_score's integer value to binary
            std::string best_score_binary = binary(best_score, qubits_best_score.size(), false);
            // Convert best_score's binary value to gray code
            std::string best_score_gray = gray_code(best_score_binary);
            // Assign best_score_gray to qubits_best_score
            for (int i = 0; i < best_score_gray.length(); i++) {
              if (best_score_gray[i] == '1') {
                this->X(qubits_best_score[i]);
              }
            }

            std::cout << "Oracle: checks whether qubits_next_metric is equal to the current best score" << std::endl;
            CircuitBuilder oracle_equal = CircuitBuilder();
            oracle_equal.EqualityChecker(qubits_next_metric, qubits_best_score,
              qubit_flag, use_ancilla, qubits_ancilla, controls_on, controls_off);

            std::cout << "Amplitude Amplification. " << std::endl;
            CircuitBuilder AA = CircuitBuilder();
            AA.AmplitudeAmplification(oracle_equal, state_builder, nb_grovers);
            this->append(AA);
            // Perform Amplitude Amplification if qubits_metric == qubits_best_score, i.e. if qubit_flag = 1.
          //   CircuitBuilder c_aa = CircuitBuilder();
          //   c_aa.CU(AA, std::vector<int>{qubit_flag});
          //   this->append(c_aa);

            std::cout << "// Undo the oracle to reset qubit_flag to 0." << std::endl;
            CircuitBuilder oracle_equal_undo = CircuitBuilder();
            oracle_equal_undo.EqualityChecker(qubits_next_metric, qubits_best_score,
              qubit_flag, use_ancilla, qubits_ancilla, controls_on, controls_off);
            this->append(oracle_equal_undo);
            
            //std::cout << "Where does reversal happen?" << std::endl;
            // Undo assigning qubits_best_score
            for (int i = 0; i < best_score_gray.length(); i++) {
              if (best_score_gray[i] == '1' and false) {
                  std::cout << "this->X" << std::endl;
                this->X(qubits_best_score[i]);
              }
            }
          }
          std::cout << "Best Score:" << best_score << std::endl;
          //break;
        }
std::cout<<std::endl;
      }

/*
  this->H(qubits_next_metric[0]);
  this->H(qubits_best_score[0]);

  //bool use_ancilla = false;
  //std::vector<int> qubits_ancilla;
  //std::vector<int> controls_on;
  //std::vector<int> controls_off;
  this->EqualityChecker(qubits_next_metric, qubits_best_score,
            qubit_flag, use_ancilla, qubits_ancilla, controls_on, controls_off);
*/

      return true;
    }; // bool expand
  }; // class QuantumDecoder2
} // namespace qbOS