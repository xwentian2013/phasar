/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Linus Jungemann, and others
 *****************************************************************************/

#include <set>
#include <string>
#include <tuple>

#include "gtest/gtest.h"

#include "llvm/Support/raw_ostream.h"

#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/CallString.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Problems/IntraMonoFullConstantPropagation.h"
#include "phasar/PhasarLLVM/DataFlowSolver/Mono/Solver/IntraMonoSolver.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

using namespace psr;

/* ============== TEST FIXTURE ============== */
class IntraMonoFullConstantPropagationTest : public ::testing::Test {
protected:
  const std::string PathToLlFiles =
      PhasarConfig::getPhasarConfig().PhasarDirectory() +
      "build/test/llvm_test_code/full_constant/";
  const std::set<std::string> EntryPoints = {"main"};

  using IMFCPCompactResult_t =
      std::tuple<std::string, std::size_t, std::string,
                 LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t>>;
  ProjectIRDB *IRDB = nullptr;

  void SetUp() override {
    std::cout << "setup\n";
    boost::log::core::get()->set_logging_enabled(false);
  }
  void TearDown() override { delete IRDB; }

  void
  doAnalysisAndCompareResults(const std::string &LlvmFilePath,
                              const std::set<IMFCPCompactResult_t> &GroundTruth,
                              bool PrintDump = false) {
    IRDB = new ProjectIRDB({PathToLlFiles + LlvmFilePath}, IRDBOptions::WPA);
    if (PrintDump) {
      IRDB->emitPreprocessedIR(std::cout, false);
    }
    ValueAnnotationPass::resetValueID();
    LLVMTypeHierarchy TH(*IRDB);
    LLVMPointsToInfo PT(*IRDB);
    LLVMBasedICFG ICFG(*IRDB, CallGraphAnalysisType::OTF, EntryPoints, &TH,
                       &PT);
    IntraMonoFullConstantPropagation FCP(IRDB, &TH, &ICFG, &PT, EntryPoints);
    IntraMonoSolver_P<IntraMonoFullConstantPropagation> IMSolver(FCP);
    IMSolver.solve();
    if (PrintDump) {
      IMSolver.dumpResults();
    }
    std::cout << "Done analysis!\n";
    // do the comparison
    bool ResultNotEmpty = false;
    for (const auto &Truth : GroundTruth) {
      const auto *Fun = IRDB->getFunctionDefinition(std::get<0>(Truth));
      const auto *Line = getNthInstruction(Fun, std::get<1>(Truth));
      auto ResultSet = IMSolver.getResultsAt(Line);
      for (const auto &[Fact, Value] : ResultSet) {
        std::string FactStr = llvmIRToString(Fact);
        llvm::StringRef FactRef(FactStr);
        if (FactRef.startswith("%" + std::get<2>(Truth) + " ")) {
          std::cout << "Checking variable: " << FactStr << std::endl;
          ResultNotEmpty = true;
          EXPECT_EQ(std::get<3>(Truth), Value);
        }
      }
    }
    EXPECT_TRUE(ResultNotEmpty);
  }

}; // Test Fixture

// Test for Case I of Store
TEST_F(IntraMonoFullConstantPropagationTest, BasicTest_01) {
  std::set<IMFCPCompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string,
                 LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t>>(
          "main", 5, "i", 13));
  doAnalysisAndCompareResults("basic_01_cpp.ll", GroundTruth, true);
}

// Test for Case II of Store and Load Inst
TEST_F(IntraMonoFullConstantPropagationTest, BasicTest_02) {
  std::set<IMFCPCompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string,
                 LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t>>(
          "main", 8, "i", 13));
  doAnalysisAndCompareResults("basic_02_cpp.ll", GroundTruth, true);
}

// Test for Operators
TEST_F(IntraMonoFullConstantPropagationTest, BasicTest_03) {
  std::set<IMFCPCompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string,
                 LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t>>(
          "main", 9, "i", 13));
  doAnalysisAndCompareResults("basic_03_cpp.ll", GroundTruth, true);
}

// Test for Operators
TEST_F(IntraMonoFullConstantPropagationTest, BasicTest_04) {
  std::set<IMFCPCompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string,
                 LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t>>(
          "main", 11, "i", 13));
  doAnalysisAndCompareResults("basic_04_cpp.ll", GroundTruth, true);
}

// Test for Operators
TEST_F(IntraMonoFullConstantPropagationTest, BasicTest_05) {
  std::set<IMFCPCompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string,
                 LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t>>(
          "main", 8, "i", 13));
  doAnalysisAndCompareResults("basic_05_cpp.ll", GroundTruth, true);
}

// Test for Operators
TEST_F(IntraMonoFullConstantPropagationTest, BasicTest_06) {
  std::set<IMFCPCompactResult_t> GroundTruth;
  GroundTruth.emplace(
      std::tuple<std::string, size_t, std::string,
                 LatticeDomain<IntraMonoFullConstantPropagation::plain_d_t>>(
          "main", 7, "i", 9));
  doAnalysisAndCompareResults("basic_06_cpp.ll", GroundTruth, true);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
