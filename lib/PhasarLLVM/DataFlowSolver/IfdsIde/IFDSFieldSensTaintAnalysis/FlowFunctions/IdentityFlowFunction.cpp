/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/IdentityFlowFunction.h"

namespace psr {

std::set<ExtendedValue>
IdentityFlowFunction::computeTargetsExt(ExtendedValue &Fact) {
  return {Fact};
}

} // namespace psr
