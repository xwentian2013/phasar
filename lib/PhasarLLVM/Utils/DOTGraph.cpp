/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * DOTGraph.cpp
 *
 *  Created on: 31.08.2019
 *      Author: rleer
 */

#include <algorithm>
#include <iterator>
#include <ostream>
#include <stdlib.h>

#include <phasar/PhasarLLVM/Utils/DOTGraph.h>

namespace psr {

bool cmpStmtIdByValue::operator()(const std::string &lhs,
                                  const std::string &rhs) const {
  char *endptr1, *endptr2;
  long lhs_val = strtol(lhs.c_str(), &endptr1, 10);
  long rhs_val = strtol(rhs.c_str(), &endptr2, 10);
  if (lhs.c_str() == endptr1 && lhs.c_str() == endptr2) {
    return lhs < rhs;
  } else if (lhs.c_str() == endptr1 && rhs.c_str() != endptr2) {
    return false;
  } else if (lhs.c_str() != endptr1 && rhs.c_str() == endptr2) {
    return true;
  } else {
    return lhs_val < rhs_val;
  }
}

DOTNode::DOTNode(std::string fName, std::string l, std::string sId,
                 unsigned fId, bool isStmt, bool isv)
    : funcName(fName), label(l), stmtId(sId), factId(fId), isVisible(isv) {
  if (isStmt) {
    id = funcName + '_' + stmtId;
  } else {
    id = funcName + '_' + std::to_string(factId) + '_' + stmtId;
  }
}

std::string DOTNode::str(std::string indent) const {
  std::string str =
      indent + id + " [label=\"" + label; // + " | SID: " + stmtId + "\"";
  if (factId) {
    str += " | SID: " + stmtId;
  }
  str += "\"";
  if (!isVisible) {
    str += ", style=invis";
  }
  return str + ']';
}

DOTEdge::DOTEdge(DOTNode src, DOTNode tar, bool isv, std::string efl,
                 std::string vl)
    : source(src), target(tar), isVisible(isv), edgeFnLabel(efl),
      valueLabel(vl) {}

std::string DOTEdge::str(std::string indent) const {
  std::string str = indent + source.id + " -> " + target.id;
  if (isVisible) {
    if (!edgeFnLabel.empty() && !valueLabel.empty()) {
      str += " [headlabel=\"\\r" + edgeFnLabel + "\", taillabel=\"" +
             valueLabel + "\"]";
    } else if (!edgeFnLabel.empty()) {
      str += " [headlabel=\"\\r" + edgeFnLabel + "\"]";
    } else if (!valueLabel.empty()) {
      str += " [taillabel=\"" + valueLabel + "\"]";
    }
  } else if (!isVisible) {
    str += " [style=invis]";
  }
  return str;
}

std::string DOTFactSubGraph::str(std::string indent) const {
  std::string innerIndent = indent + "  ";
  std::string str = indent + "subgraph cluster_" + id + " {\n" + innerIndent +
                    "style=invis\n" + innerIndent + "label=\"" + label +
                    "\"\n\n" + innerIndent + "// Fact nodes in the ESG\n" +
                    innerIndent + DOTConfig::FactNodeAttr() + '\n';
  // Print fact nodes
  for (auto n : nodes) {
    str += n.second.str(innerIndent) + '\n';
  }
  // Print id edges
  str += '\n' + innerIndent + "// Identity edges for this fact\n" +
         innerIndent + DOTConfig::FactIdentityEdgeAttr() + '\n';
  for (DOTEdge e : edges) {
    str += e.str(innerIndent) + '\n';
  }
  return str + indent + '}';
}

std::string DOTFunctionSubGraph::str(std::string indent) const {
  std::string innerIndent = indent + "  ";
  std::string str = indent + "subgraph cluster_" + id + " {\n" + innerIndent +
                    "label=\"" + id + "\"";
  // Print control flow nodes
  str += "\n\n" + innerIndent + "// Control flow nodes\n" + innerIndent +
         DOTConfig::CFNodeAttr() + '\n';
  for (DOTNode stmt : stmts) {
    str += stmt.str(innerIndent) + '\n';
  }

  // Print fact subgraphs
  str += '\n' + innerIndent + "// Fact subgraphs\n";
  for (auto factSG : facts) {
    str += factSG.second.str(innerIndent) + "\n\n";
  }

  // Print lambda subgraph
  str += generateLambdaSG(innerIndent);

  // Print intra control flow edges
  str += "\n\n" + innerIndent + "// Intra-procedural control flow edges\n" +
         innerIndent + DOTConfig::CFIntraEdgeAttr() + '\n';
  for (DOTEdge e : intraCFEdges) {
    str += e.str(innerIndent) + '\n';
  }

  // Print intra cross fact edges
  str += '\n' + innerIndent + "// Intra-procedural cross fact edges\n" +
         innerIndent + DOTConfig::FactCrossEdgeAttr() + '\n';
  for (DOTEdge e : crossFactEdges) {
    str += e.str(innerIndent) + '\n';
  }
  return str + indent + '}';
}

DOTFactSubGraph *DOTFunctionSubGraph::getOrCreateFactSG(unsigned factID,
                                                        std::string &label) {
  DOTFactSubGraph *FuncSG = &facts[factID];
  if (FuncSG->id.empty()) {
    FuncSG->id = id + '_' + std::to_string(factID);
    FuncSG->factId = factID;
    FuncSG->label = label;
  }
  return FuncSG;
}

std::string DOTFunctionSubGraph::generateLambdaSG(std::string indent) const {
  std::string innerIndent = indent + "  ";
  std::string str = indent + "// Auto-generated lambda nodes and edges\n" +
                    indent + "subgraph cluster_" + id + "_lambda {\n" +
                    innerIndent + "style=invis\n" + innerIndent +
                    "label=\"Λ\"\n" + innerIndent + DOTConfig::FactNodeAttr() +
                    '\n';
  // Print lambda nodes
  for (DOTNode stmt : stmts) {
    str += innerIndent + id + "_0_" + stmt.stmtId + " [label=\"Λ\"]\n";
  }
  // Print lambda edges
  str += '\n' + innerIndent + DOTConfig::FactIdentityEdgeAttr() + '\n';
  for (DOTEdge e : intraCFEdges) {
    str += innerIndent + id + "_0_" + e.source.stmtId + " -> " + id + "_0_" +
           e.target.stmtId;
    if (e.isVisible) {
      str += " [headlabel=\"\\rAllBottom\", taillabel=\"BOT\"]\n";
    } else {
      str += " [style=invis]\n";
    }
  }
  return str + indent + '}';
}

void DOTFunctionSubGraph::createLayoutCFNodes() {
  auto last = stmts.empty() ? stmts.end() : std::prev(stmts.end());
  for (auto firstIt = stmts.begin(); firstIt != last; ++firstIt) {
    auto secondIt = std::next(firstIt);
    DOTNode n1 = *firstIt;
    DOTNode n2 = *secondIt;
    intraCFEdges.emplace(n1, n2, false);
  }
}

void DOTFunctionSubGraph::createLayoutFactNodes() {
  for (auto &[key, factSG] : facts) {
    for (auto stmt : stmts) {
      if (factSG.nodes.find(stmt.stmtId) == factSG.nodes.end()) {
        DOTNode factNode(stmt.funcName, factSG.label, stmt.stmtId,
                         factSG.factId, false, false);
        factSG.nodes[stmt.stmtId] = factNode;
      }
    }
  }
}

// TODO: this seems to be buggy
void DOTFunctionSubGraph::createLayoutFactEdges() {
  for (auto &[key, factSG] : facts) {
    auto last = factSG.nodes.empty() ? factSG.nodes.end()
                                     : std::prev(factSG.nodes.end());
    // Check cf edges not only nodes!
    for (auto firstIt = factSG.nodes.begin(); firstIt != last; ++firstIt) {
      auto secondIt = std::next(firstIt);
      DOTNode n1 = firstIt->second;
      DOTNode n2 = secondIt->second;
      factSG.edges.emplace(n1, n2, false);
    }
  }
}

bool operator<(const DOTNode &lhs, const DOTNode &rhs) {
  char *endptr1, *endptr2;
  long lhs_sId = strtol(lhs.stmtId.c_str(), &endptr1, 10);
  long rhs_sId = strtol(rhs.stmtId.c_str(), &endptr2, 10);
  return lhs_sId < rhs_sId;
}

bool operator==(const DOTNode &lhs, const DOTNode &rhs) {
  return !(lhs < rhs) && !(rhs < lhs);
}

std::ostream &operator<<(std::ostream &os, const DOTNode &node) {
  return os << node.str();
}

bool operator<(const DOTEdge &lhs, const DOTEdge &rhs) {
  if (lhs.source == rhs.source) {
    return lhs.target < rhs.target;
  }
  return lhs.source < rhs.source;
}

std::ostream &operator<<(std::ostream &os, const DOTEdge &edge) {
  return os << edge.str();
}

std::ostream &operator<<(std::ostream &os, const DOTFactSubGraph &factSG) {
  return os << factSG.str();
}

std::ostream &operator<<(std::ostream &os,
                         const DOTFunctionSubGraph &functionSG) {
  return os << functionSG.str();
}

// std::ostream &operator<<(std::ostream &os, const DOTGraph &graph) {
//   return os << graph.str();
// }

DOTConfig &DOTConfig::getDOTConfig() {
  static DOTConfig DC;
  return DC;
}

} // namespace psr
