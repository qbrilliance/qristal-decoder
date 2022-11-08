#include "decoder_kernel.hpp"
#include <CompositeInstruction.hpp>
#include <Instruction.hpp>
#include <memory>
#include "xacc.hpp"
#include "qb/core/circuit_builder.hpp"
namespace qbOS {

bool DecoderKernel::expand(const xacc::HeterogeneousMap &runtimeOptions) {

  ////////////////////////////////////////////////////////
  // Define helper functions
  ////////////////////////////////////////////////////////

  // Flip a bitstring
  std::function<std::string(std::string)> flip_bitstring =
      [&](std::string bitstring) {
        std::string flipped;
        for (int i = 0; i < bitstring.size(); i++) {
          if (bitstring[i] == '1') {
            flipped.push_back('0');
          }
          if (bitstring[i] == '0') {
            flipped.push_back('1');
          }
        }
        return flipped;
      };

  // int to binary function
  std::function<std::string(int, int)> binary = [&](int i, int num_qubits) {
    std::string i_binary = std::bitset<8 * sizeof(i)>(i).to_string();
    std::string i_binary_n = i_binary.substr(
        i_binary.size() < num_qubits ? 0 : i_binary.size() - num_qubits);
    return i_binary_n;
  };

  // binary to gray code function
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

  // Find the bit that differs
  std::function<int(std::string, std::string)> different_bit_index =
      [&](std::string bitstring1, std::string bitstring2) {
        assert(bitstring1.size() == bitstring2.size());
        for (int i = 0; i < bitstring1.size(); i++) {
          if (bitstring1[i] != bitstring2[i]) {
            return i;
          }
        }
        return -1;
      };

  ////////////////////////////////////////////////////////
  // Get expected inputs or define any required variables
  ////////////////////////////////////////////////////////

  if (!runtimeOptions.keyExists<std::vector<int>>("qubits_string")) {
    return false;
  }
  std::vector<int> qubits_string =
      runtimeOptions.get<std::vector<int>>("qubits_string");

  if (!runtimeOptions.keyExists<std::vector<int>>("qubits_metric")) {
    return false;
  }
  std::vector<int> qubits_metric =
      runtimeOptions.get<std::vector<int>>("qubits_metric");

  if (!runtimeOptions.keyExists<std::vector<int>>("qubits_total_metric_buffer")) {
    return false;
  }
  std::vector<int> qubits_total_metric_buffer =
      runtimeOptions.get<std::vector<int>>("qubits_total_metric_buffer");

  if (!runtimeOptions.keyExists<std::vector<int>>("qubits_init_null")) {
    return false;
  }
  std::vector<int> qubits_init_null =
      runtimeOptions.get<std::vector<int>>("qubits_init_null");

  if (!runtimeOptions.keyExists<std::vector<int>>("qubits_init_repeat")) {
    return false;
  }
  std::vector<int> qubits_init_repeat =
      runtimeOptions.get<std::vector<int>>("qubits_init_repeat");

  if (!runtimeOptions.keyExists<std::vector<int>>("qubits_superfluous_flags")) {
    return false;
  }
  std::vector<int> qubits_superfluous_flags =
      runtimeOptions.get<std::vector<int>>("qubits_superfluous_flags");

  if (!runtimeOptions.keyExists<std::vector<int>>("qubits_beam_metric")) {
    return false;
  }
  std::vector<int> qubits_beam_metric =
      runtimeOptions.get<std::vector<int>>("qubits_beam_metric");

  if (!runtimeOptions.keyExists<std::vector<int>>("qubits_ancilla_pool")) {
    return false;
  }
  std::vector<int> qubits_ancilla_pool =
      runtimeOptions.get<std::vector<int>>("qubits_ancilla_pool");

 std::vector<int> total_metric_exponent;
  if (runtimeOptions.keyExists<std::vector<int>>("total_metric_exponent")) {
      total_metric_exponent = runtimeOptions.get<std::vector<int>>("total_metric_exponenet");
  }

  if (!runtimeOptions.pointerLikeExists<xacc::CompositeInstruction>("metric_state_prep")) {
    return false;
  }
  auto metric_state_prep = runtimeOptions.getPointerLike<xacc::CompositeInstruction>("metric_state_prep");

  int L = qubits_init_null.size();
  int S = qubits_string.size()/L;
  int ml = qubits_metric.size()/L;

  std::vector<int> qubits_total_metric;
  for (int i = 0; i < ml; i++) {
    qubits_total_metric.push_back(qubits_metric[i]);
  }
  for (int i = 0; i < qubits_total_metric_buffer.size(); i++) {
    qubits_total_metric.push_back(qubits_total_metric_buffer[i]);
  }

  ////////////////////////////////////////////////////////
  // Add instructions
  ////////////////////////////////////////////////////////

  auto gateRegistry = xacc::getService<xacc::IRProvider>("quantum");

  /// 
  // Take the exponenet of the string total metric register
  ///

//   if (total_metric_exponent.size() > 0) {
//     const xacc::HeterogeneousMap &map_exp = {
//         {"qubits_log", qubits_total_metric},
//         {"qubits_exponent", total_metric_exponent}};
//     qbOS::Exponent build_exp;
//     const bool expand_ok = build_exp.expand(map_exp);
//     auto exponent = build_exp.get();
//     addInstructions(exponent->getInstructions());
//   }

  ///
  // Form equivalence classes
  ///

  // flag superfluous symbols and mark for swap

  for (int i = L - 1; i >= 0; i--) {
    std::vector<int> letter;
    for (int j = 0; j < S; j++) {
      letter.push_back(qubits_string[i * S + j]);
    }

    // flag last symbol if it is null or repeat
    if (i == L - 1) {
      addInstruction(
          gateRegistry->createInstruction("X", qubits_superfluous_flags[i]));
      metric_state_prep->addInstruction(
          gateRegistry->createInstruction("X", qubits_superfluous_flags[i]));
      auto untoffoli = std::dynamic_pointer_cast<xacc::CompositeInstruction>(
          xacc::getService<xacc::Instruction>("GeneralisedMCX"));
      std::vector<int> off;
      off.push_back(qubits_init_null[i]);
      off.push_back(qubits_init_repeat[i]);
      untoffoli->expand(
          {{"controls_off", off}, {"target", qubits_superfluous_flags[i]}});
      addInstruction(untoffoli);
      metric_state_prep->addInstruction(untoffoli);
    }

    // loop from second last symbol to first symbol
    else {
      // flag if it is a repeat or a null
      addInstruction(
          gateRegistry->createInstruction("X", qubits_superfluous_flags[i]));
      metric_state_prep->addInstruction(
          gateRegistry->createInstruction("X", qubits_superfluous_flags[i]));
      auto untoffoli = std::dynamic_pointer_cast<xacc::CompositeInstruction>(
          xacc::getService<xacc::Instruction>("GeneralisedMCX"));
      std::vector<int> off;
      off.push_back(qubits_init_null[i]);
      off.push_back(qubits_init_repeat[i]);
      untoffoli->expand(
          {{"controls_off", off}, {"target", qubits_superfluous_flags[i]}});
      addInstruction(untoffoli);
      metric_state_prep->addInstruction(untoffoli);

      // flip control-swap qubit according to whether that symbol is a repeat or
      // a null
      int qubit_control_swap = qubits_ancilla_pool[0];
      addInstruction(gateRegistry->createInstruction(
          "CX", {static_cast<unsigned long>(qubits_superfluous_flags[i]),
                 static_cast<unsigned long>(qubit_control_swap)}));
      metric_state_prep->addInstruction(gateRegistry->createInstruction(
          "CX", {static_cast<unsigned long>(qubits_superfluous_flags[i]),
                 static_cast<unsigned long>(qubit_control_swap)}));

      // loop from current symbol to the end
      for (int j = i; j < L - 1; j++) {
        std::vector<int> current_letter;
        std::vector<int> next_letter;
        std::vector<int> current_flag = {qubits_superfluous_flags[j]};
        std::vector<int> next_flag = {qubits_superfluous_flags[j + 1]};

        for (int k = 0; k < S; k++) {
          current_letter.push_back(
              qubits_string[j * S + k]);
        }
        for (int k = 0; k < S; k++) {
          next_letter.push_back(
              qubits_string[(j + 1) * S + k]);
        }

        // swap flagged symbol (swap conditional on control-swap) with next one
        auto c_swap_letter =
            std::dynamic_pointer_cast<xacc::CompositeInstruction>(
                xacc::getService<xacc::Instruction>("ControlledSwap"));
        std::vector<int> flags_on = {qubit_control_swap};
        xacc::HeterogeneousMap options_letter{{"qubits_a", current_letter},
                                              {"qubits_b", next_letter},
                                              {"flags_on", flags_on}};
        const bool expand_ok_letter = c_swap_letter->expand(options_letter);
        assert(expand_ok_letter);
        addInstruction(c_swap_letter);
        metric_state_prep->addInstruction(c_swap_letter);

        // swap superfluous flag (swap conditional on control-swap) with next
        // one
        auto c_swap_flag =
            std::dynamic_pointer_cast<xacc::CompositeInstruction>(
                xacc::getService<xacc::Instruction>("ControlledSwap"));
        xacc::HeterogeneousMap options_flag{{"qubits_a", current_flag},
                                            {"qubits_b", next_flag},
                                            {"flags_on", flags_on}};
        const bool expand_ok_flag = c_swap_flag->expand(options_flag);
        assert(expand_ok_flag);
        addInstruction(c_swap_flag);
        metric_state_prep->addInstruction(c_swap_flag);
      }

      // flip control-swap qubit back according to whether that symbol is a
      // repeat or a null
      addInstruction(gateRegistry->createInstruction("X", qubit_control_swap));
      metric_state_prep->addInstruction(gateRegistry->createInstruction("X", qubit_control_swap));
      auto untoffoli2 = std::dynamic_pointer_cast<xacc::CompositeInstruction>(
          xacc::getService<xacc::Instruction>("GeneralisedMCX"));
      std::vector<int> off2;
      off2.push_back(qubits_init_null[i]);
      off2.push_back(qubits_init_repeat[i]);
      untoffoli2->expand(
          {{"controls_off", off2}, {"target", qubit_control_swap}});
      addInstruction(untoffoli2);
      metric_state_prep->addInstruction(untoffoli2);
    }
  }

  int q0 = qubits_ancilla_pool[0];
  int q1 = qubits_ancilla_pool[1];
  int q2 = qubits_ancilla_pool[2];

  std::shared_ptr<xacc::CompositeInstruction> sp_c =
      xacc::ir::asComposite(metric_state_prep->clone());
  auto state_prep_set = qbOS::uniqueBitsQD(sp_c);
  std::vector<int> sp_qubits;
  for (auto bit : state_prep_set) {
      sp_qubits.push_back(bit);
  }

  std::vector<int> qubits_ancilla;
  for (int i = 3; i < qubits_ancilla_pool.size(); i++) {
    if (std::find(sp_qubits.begin(), sp_qubits.end(), qubits_ancilla_pool[i]) ==
        sp_qubits.end()) {
      qubits_ancilla.push_back(qubits_ancilla_pool[i]);
    }
  }

  auto add_metrics = std::dynamic_pointer_cast<xacc::CompositeInstruction>(
      xacc::getService<xacc::Instruction>("SuperpositionAdder"));
  xacc::HeterogeneousMap options_adder{
      {"q0", q0}, {"q1", q1}, {"q2", q2},
      {"qubits_flags", qubits_superfluous_flags},
      {"qubits_string", qubits_string},
      {"qubits_metric", qubits_total_metric},
      {"ae_state_prep_circ", metric_state_prep},
      {"qubits_ancilla", qubits_ancilla},
      {"qubits_beam_metric", qubits_beam_metric}};
  const bool expand_ok_add = add_metrics->expand(options_adder);
  assert(expand_ok_add);
  addInstruction(add_metrics);
  return true;
}

const std::vector<std::string> DecoderKernel::requiredKeys() { return {}; }

} // namespace qbOS
