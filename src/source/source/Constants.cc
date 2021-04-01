/**
 * @brief Defines default random number generator and other constants
 *
 * @file Constants.cc
 * @author Mark Fornace
 * @date 2018-05-31
 */
#include <nupack/common/Threading.h>
#include <nupack/common/Constants.h>
#include <nupack/common/Random.h>
#include <nupack/common/Error.h>
#include <nupack/common/Costs.h>

#include <nupack/reflect/Print.h>
#include <nupack/reflect/Serialize.h>

#include <nupack/Version.h>
#include <armadillo>

namespace nupack {

thread_local SharedMode GlobalSharedMode = SharedMode::copy;
thread_local std::vector<std::pair<void const *, nlohmann::json>> GlobalSharedState;

/// This may be used to turn knobs in development code without recompiling completely
string HackHelper = "";

/******************************************************************************************/

std::random_device Static_RD;

#ifndef NUPACK_RANDOM_DEVICE
#   error("NUPACK_RANDOM_DEVICE should be defined")
#elif NUPACK_RANDOM_DEVICE
    bool const RandomDevice = (arma::arma_rng::set_seed_random(), true);
    thread_local std::mt19937 StaticRNG{Static_RD()};
#else
    bool const RandomDevice = false;
    thread_local std::mt19937 StaticRNG{};
#endif

/******************************************************************************************/

// In Kelvin, Tanaka M., Girard, G. et al, Metrologia, 2001 38, 301-309.
real water_molarity(real T) {
   constexpr real const a1 = -3.983035 - ZeroCinK, a2 = 301.797 - ZeroCinK,
                        a3 = 522528.9, a4 = 69.34881 - ZeroCinK, a5 = 999.974950;
   return a5 * (1 - (T + a1) * (T + a1) * (T + a2) / a3 / (T + a4)) / 18.0152;
};

// No correction for RNA since we don't have parameters
real dna_salt_correction(real t, real na, real mg, bool long_helix) {
  // Ignore magnesium for long helix mode (not cited why, for consistency with Mfold)
  NUPACK_REQUIRE(na, >=, 0.05); NUPACK_REQUIRE(na, <=, 1.1);
  NUPACK_REQUIRE(mg, >=, 0);    NUPACK_REQUIRE(mg, <=, 0.2);
  if (long_helix) return -(0.2 + 0.175 * std::log(na)) * t / DefaultTemperature;
  return -0.114 * std::log(na + 3.3 * std::sqrt(mg)) * t / DefaultTemperature;
}

/******************************************************************************************/

std::string ReferenceSequence = "TTCCGTAGCGGAGGTCTATGTCCTCAATGTTTCGCGTCGTATTTATTTGCAAACAGATACGCATTCCCCCCCTGCCTTCCGAGCTGTTGCTACTTCACCAACTCGCGCTTAATGCATGAAACTCTAGTTCACTCACCGATTAGTTATCGATTAAGAAGAGACCAGTTGGGAATTAGCTAACCGCAACAAGAACGACCATATAGAGTTGTCTCCTAGTCTCAGCATTTGGCGAGGTTCAGTCCTTATTGCACGCTGGACCAAACGTCTCCTTGTCTACTAAAAATTCAATGGACTATGAGGAGCTCGTATAGAAGCTCGAATGGGTGCTCTATCCTCCGACTGTTTGAAAACATATGAAGACCAACGGTAATACACACGGTATCTACTTCAAGAAGCTGTGTTTGCCGAGCTCGACGATGTCACTGGCCGGTCCGGTGTGTACACCTATAGGGGGATTTGGTGTCCCCTTGTAGAAGCTAAGTTACCTGTTTGGCTATTAGCGTCGTGTGTAATGTTAATCTGCGATACTTATGAAATCGCATTGGTTTGCAGTTTCTCTACGCTGGTGTAGGACCGAGATAAAGTCGTGCGATAGTTATATAAGTACGAGAGTCAGAGCGCCGTTCAATAAGGTCCCGTGCCGTCCCCCCGTTGTTGCTGTCTCCTTGCGAAATGGATGATGACCAGGTTGGATAGAGAGCGCGACTTCTCGCTGGCTCGGTGATCGCTCGAGACTAGGACAACGGGGGCTATTGAGTGGACCTGACTACTATCCTATTGTCAGAAATGGCCACCTACACGCCTAACTGACTGGACGTTCGTAGTTGATCTGTTAAAACGAGAACTAGCACACTCAACGGCGTGGGGGCTAGTCTTAGGAAAAGTTTGGAGAAGAAAGAAGACCAACGGAACCCATCGATTTGAATTGACGTTGGTGTCTTTTCGTACAAGACAGAGGCAAAATTATTTTGCTTACTTCGTCATACAAAATCATTATCCCTTGACCTGCGGCCCCGCGTAACACCACCTCTCTGATAAGTAGGTTGACTATTCAGGGGGTCCACGAGCTACACGATCGTGCTCAAGAATTCCTTCCGGAATTGACGCGTAATCAACAAAAACCGATATTAGGACGGGCCTGAGTAAGATAGTTGTAGGTGTCCACGGTCTAGTTAGGGTGGTGGGTCCGAGTTCGCGTTACTTTGTTCTGTCAAACTCGGCCTGTGTGCCGAGATAAAGGCCACGATCGTTATATCATGCCAAGCGTCGACAGTCGGAAGGAACGCAATCCGACCGTTCGACCCGTGACCCGTGCCATAAGGACAAACATCGAACATTATTTGCGGGAAAATTCCTACGAAGCGTCGCGCTTCGCAGGAGTGTACTAACTATGTAAGGGAAACCTTACCACAGCTCGCATAGCTGTTTTCAGCTGGTGTGTTCATTTCAAGCGGTAGGTGATTTTCAGATAGTGGCGCGGCCCTGCGGATGCGTTGGTGCTAACACCCTCTATCGACGGGTATGGGTAGGATTGAAACCCTGCTGTATGTGTTAGATATCGATGCCGACTGGAGCTCGGCCATGCTTCGTATAAATTGTTCGATTCGTCACGGGGGGCCCAAGAAATATGGCAAATACACAAATCGGGGTTATCGTGTTTGATTCGACCATCTCCTACCGGCACAATACACATTTTTACGGTTACATTACTCACTGCGTGAACTGACTGGAATCGTCTCTTGTTGGCCCATCACAAAAGCTTTGCGTAAGGCTGTATGAATGCTTTACGTTTCTGGCTGATACTGCTTAGGGGCCGATTATCTAGAGGAATTAACACCCACCGTGTTGTCCGGAGGGTGCGCCGAGTTCCGATTAGACTAAGAAAGCGTGGTCGGATATAGGCAATATCGGTGCCCAGTTCACCTGCTGGATCTCTTGCCCGTGCGCAACGCGGTGATACTTTGATTGATCCTTGATCGTAATCCGTGACCTGGAAGATGTACTCTTACAAACACGCACTGAACCCGGCGTCGCTCTCGAGCGGTGTAGGAAATCTCTATTTCCCCTGTGCTTGTGTCTGTAGATTACCTTACACGTTTTACTGTATGATGCGCATGCCTTTTCGAGGTACCCCGGGCTTGGAACGTAGTGGGGAGCGGGTTGACTTTTCATCTAATCAACCGCTAGGTATTACCACTAAAGGATCATCCAATTAACATCATTTCGGCATTCACCAAATTGTTTGGGTGAGTGATCTCTAGACTAATGTACTGACTAAATCAATCTACATGGGTCTCCAAAAGTGTTCCGTGGTACCCCTACTACCACCCTCCGACCTTGATGGAATAGTAGCGGGAGTCTGGAGTTGATGGGACACAGCATTCCTGGATGGAACAAAATCCGGTCGAACTGGCACGAGCTTAACAATCATACGCATCGACGCGGATAATCGCGGGTTGTTCGTACCAACTAATGCCTTATAAAGAAGCCACGGCAGATGTGACTAGACAGCAACTAGTGAGGTGTGCAGCAGAGCGCCAACACGTTACCAGAGCGAACGTATTAATATAATTATTCATGCTATAACAGTCGCCAACTAGTCTACACATGGAGGCACCTGTGGTGGGGCCATTTAATGCACATGTGGCCGATTGCACAAAGATGGGGCAGACTATCTCAAGTCGGATCGCTATTTATTCTCCTTCACTAAGCCGACAAGCTTATATTAAATCGCCACACTACAACGTAACTGTGGAATACAGCCTGGGTCACTACAGTAGTTGTCTCTTCAGCGGCGATACATAGAGGCATGAGCAATGTAGACGTTGCAAGCCTCAGCATGAAAGACGCTGATTATAAATCTCCCAGAAATTTTCAAGCTTAGTTGCCACATAGCTCGGCTTTTCTAATTATCTCCTCTCCGGCTTCACGTGTGCCGCCCCCAGCATTAAGTTCTTACCCCCATTGAAACGATCCGTCATGTCAATTTGAGTTATCGGC";

string reference_dna(std::size_t length) {
    if (length > ReferenceSequence.size()) throw std::out_of_range("reference_dna");
    return ReferenceSequence.substr(0, length);
}

}