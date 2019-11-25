/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * InterMonoTaintAnalysis.h
 *
 *  Created on: 22.08.2018
 *      Author: richard leer
 */

#ifndef PHASAR_PHASARLLVM_MONO_PROBLEMS_INTERMONOTAINTANALYSIS_H_
#define PHASAR_PHASARLLVM_MONO_PROBLEMS_INTERMONOTAINTANALYSIS_H_

#include <initializer_list>
#include <map>
#include <set>
#include <string>

#include <phasar/PhasarLLVM/DataFlowSolver/Mono/InterMonoProblem.h>
#include <phasar/PhasarLLVM/Utils/TaintConfiguration.h>

namespace llvm {
class Instruction;
class Value;
class Function;
class StructType;
} // namespace llvm

namespace psr {

class LLVMBasedICFG;

class InterMonoTaintAnalysis
    : public InterMonoProblem<const llvm::Instruction *, const llvm::Value *,
                              const llvm::Function *, const llvm::StructType *, const llvm::Value *, LLVMBasedICFG> {
private:
  typedef const llvm::Instruction *n_t;
  typedef const llvm::Value *d_t;
  typedef const llvm::Function *m_t;
  typedef const llvm::StructType *t_t;
  typedef const llvm::Value *v_t;
  typedef LLVMBasedICFG i_t;

  TaintConfiguration<const llvm::Value *> TSF;
  std::map<const llvm::Instruction *, std::set<const llvm::Value *>> Leaks;

public:
  using ConfigurationTy = TaintConfiguration<const llvm::Value *>;

  InterMonoTaintAnalysis(const ProjectIRDB *IRDB, const TypeHierarchy<t_t, m_t> *TH,
                         const LLVMBasedICFG *ICF, const PointsToInfo<v_t> *PT,
                         std::initializer_list<std::string> EntryPoints = {});
  ~InterMonoTaintAnalysis() override = default;

  MonoSet<const llvm::Value *>
  join(const MonoSet<const llvm::Value *> &Lhs,
       const MonoSet<const llvm::Value *> &Rhs) override;

  bool sqSubSetEqual(const MonoSet<const llvm::Value *> &Lhs,
                     const MonoSet<const llvm::Value *> &Rhs) override;

  MonoSet<const llvm::Value *>
  normalFlow(const llvm::Instruction *Stmt,
             const MonoSet<const llvm::Value *> &In) override;

  MonoSet<const llvm::Value *>
  callFlow(const llvm::Instruction *CallSite, const llvm::Function *Callee,
           const MonoSet<const llvm::Value *> &In) override;

  MonoSet<const llvm::Value *>
  returnFlow(const llvm::Instruction *CallSite, const llvm::Function *Callee,
             const llvm::Instruction *ExitStmt,
             const llvm::Instruction *RetSite,
             const MonoSet<const llvm::Value *> &In) override;

  MonoSet<const llvm::Value *>
  callToRetFlow(const llvm::Instruction *CallSite,
                const llvm::Instruction *RetSite,
                MonoSet<const llvm::Function *> Callees,
                const MonoSet<const llvm::Value *> &In) override;

  MonoMap<const llvm::Instruction *, MonoSet<const llvm::Value *>>
  initialSeeds() override;

  void printNode(std::ostream &os, const llvm::Instruction *n) const override;

  void printDataFlowFact(std::ostream &os, const llvm::Value *d) const override;

  void printMethod(std::ostream &os, const llvm::Function *m) const override;

  const std::map<const llvm::Instruction *, std::set<const llvm::Value *>> &
  getAllLeaks() const;
};

} // namespace psr

#endif