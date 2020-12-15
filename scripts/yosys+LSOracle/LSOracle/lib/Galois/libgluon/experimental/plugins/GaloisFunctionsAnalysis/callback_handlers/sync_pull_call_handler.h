/*
 * This file belongs to the Galois project, a C++ library for exploiting parallelism.
 * The code is being released under the terms of the 3-Clause BSD License (a
 * copy is located in LICENSE.txt at the top-level directory).
 *
 * Copyright (C) 2018, The University of Texas at Austin. All rights reserved.
 * UNIVERSITY EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES CONCERNING THIS
 * SOFTWARE AND DOCUMENTATION, INCLUDING ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR ANY PARTICULAR PURPOSE, NON-INFRINGEMENT AND WARRANTIES OF
 * PERFORMANCE, AND ANY WARRANTY THAT MIGHT OTHERWISE ARISE FROM COURSE OF
 * DEALING OR USAGE OF TRADE.  NO WARRANTY IS EITHER EXPRESS OR IMPLIED WITH
 * RESPECT TO THE USE OF THE SOFTWARE OR DOCUMENTATION. Under no circumstances
 * shall University be liable for incidental, special, indirect, direct or
 * consequential damages or loss of profits, interruption of business, or
 * related expenses which may arise from use of Software or Documentation,
 * including but not limited to those resulting from defects in Software and/or
 * Documentation, or loss or inaccuracy of data of any kind.
 */

#ifndef _PLUGIN_ANALYSIS_SYNC_PULL_CALL_H
#define _PLUGIN_ANALYSIS_SYNC_PULL_CALL_H

using namespace clang;
using namespace clang::ast_matchers;
using namespace llvm;
using namespace std;

class SyncPullInsideForLoopHandler : public MatchFinder::MatchCallback {
private:
  ASTContext* astContext;
  InfoClass* info;

public:
  SyncPullInsideForLoopHandler(CompilerInstance* CI, InfoClass* _info)
      : astContext(&(CI->getASTContext())), info(_info) {}
  virtual void run(const MatchFinder::MatchResult& Results) {

    clang::LangOptions LangOpts;
    LangOpts.CPlusPlus = true;
    clang::PrintingPolicy Policy(LangOpts);

    for (auto i : info->reductionOps_map) {
      for (auto j : i.second) {
        /** Sync pull variables **/
        string str_syncPull_var =
            "syncPullVar_" + j.NODE_NAME + "_" + j.FIELD_NAME + "_" + i.first;
        string str_memExpr =
            "memExpr_" + j.NODE_NAME + "_" + j.FIELD_NAME + i.first;
        /** Sync pull for += operator. **/
        string str_plusOp =
            "plusEqualOp_" + j.NODE_NAME + "_" + j.FIELD_NAME + "_" + i.first;
        string str_binaryOp_lhs =
            "binaryOp_LHS_" + j.NODE_NAME + "_" + j.FIELD_NAME + "_" + i.first;
        string str_lhs_argument =
            "argument_" + j.NODE_NAME + "_" + j.FIELD_NAME + "_" + i.first;

        /** Is field modified inside or outside edge loop **/
        string str_modified_insideEdgeLoop = "insideEdgeLoop_LHS_" +
                                             j.NODE_NAME + "_" + j.FIELD_NAME +
                                             "_" + i.first;
        string str_modified_outsideEdgeLoop = "outsideEdgeLoop_LHS_" +
                                              j.NODE_NAME + "_" + j.FIELD_NAME +
                                              "_" + i.first;

        /** Sync Pull variables **/
        auto syncPull_var =
            Results.Nodes.getNodeAs<clang::VarDecl>(str_syncPull_var);
        auto syncPull_plusOp = Results.Nodes.getNodeAs<clang::Stmt>(str_plusOp);
        auto syncPull_binary_var =
            Results.Nodes.getNodeAs<clang::VarDecl>(str_binaryOp_lhs);
        auto syncPull_lhs_argument =
            Results.Nodes.getNodeAs<clang::MemberExpr>(str_lhs_argument);
        auto syncPull_memExpr =
            Results.Nodes.getNodeAs<clang::MemberExpr>(str_memExpr);
        auto syncPull_insideEdgeLoop =
            Results.Nodes.getNodeAs<clang::Stmt>(str_modified_insideEdgeLoop);
        auto syncPull_outsideEdgeLoop =
            Results.Nodes.getNodeAs<clang::Stmt>(str_modified_outsideEdgeLoop);

        syncPull_var =
            (syncPull_var != NULL) ? syncPull_var : syncPull_binary_var;

        // Note: FLag to decide if sync pull should be added or not
        bool flag = false;
        // if syncPull_var is not NULL, then it must be referenced
        if (syncPull_var) {
          if (syncPull_var->isReferenced()) {
            flag = true;
          }
        }

        // if syncPull_lhs_argument is not NULL
        if (syncPull_lhs_argument || syncPull_memExpr) {
          flag = true;
        }

        if (flag) {
          llvm::outs() << "ADDING FOR SYNC PULL\n";
          ReductionOps_entry reduceOP_entry;
          reduceOP_entry.GRAPH_NAME = j.GRAPH_NAME;
          reduceOP_entry.NODE_TYPE  = j.NODE_TYPE;
          reduceOP_entry.FIELD_TYPE = j.NODE_TYPE;
          reduceOP_entry.FIELD_NAME = j.FIELD_NAME;
          reduceOP_entry.VAL_TYPE   = j.VAL_TYPE;
          reduceOP_entry.SYNC_TYPE  = "sync_pull";

          reduceOP_entry.OPERATION_EXPR = j.OPERATION_EXPR;
          reduceOP_entry.RESETVAL_EXPR  = j.RESETVAL_EXPR;

          if (syncPull_insideEdgeLoop) {
            reduceOP_entry.MODIFIED_IN_EDGE_LOOP = "__INSIDE";
          } else if (syncPull_outsideEdgeLoop) {
            reduceOP_entry.MODIFIED_IN_EDGE_LOOP = "__OUTSIDE";
          } else {
            reduceOP_entry.MODIFIED_IN_EDGE_LOOP = "__OUTSIDE";
          }

          llvm::outs() << " sync for struct " << i.first
                       << " Field name : " << j.FIELD_NAME
                       << " sync type : " << reduceOP_entry.SYNC_TYPE << "\n";
          /** check for duplicate **/
          if (!syncPull_reduction_exists(reduceOP_entry, i.second)) {
            info->reductionOps_map[i.first].push_back(reduceOP_entry);
          }
          break;
        }
      }
    }
  }
};

class SyncPullInsideForEdgeLoopHandler : public MatchFinder::MatchCallback {
private:
  ASTContext* astContext;
  InfoClass* info;

public:
  SyncPullInsideForEdgeLoopHandler(CompilerInstance* CI, InfoClass* _info)
      : astContext(&(CI->getASTContext())), info(_info) {}
  virtual void run(const MatchFinder::MatchResult& Results) {

    clang::LangOptions LangOpts;
    LangOpts.CPlusPlus = true;
    clang::PrintingPolicy Policy(LangOpts);

    for (auto i : info->fieldData_map) {
      for (auto j : i.second) {
        /** Sync pull variables **/
        string str_syncPull_var = "syncPullVar_" + j.VAR_NAME + "_" + i.first;
        /** Sync pull for += operator. **/
        string str_plusOp       = "plusEqualOp_" + j.VAR_NAME + "_" + i.first;
        string str_binaryOp_lhs = "binaryOp_LHS_" + j.VAR_NAME + "_" + i.first;

        /** Sync Pull variables **/
        auto syncPull_var =
            Results.Nodes.getNodeAs<clang::VarDecl>(str_syncPull_var);
        auto syncPull_plusOp = Results.Nodes.getNodeAs<clang::Stmt>(str_plusOp);
        auto syncPull_binary_var =
            Results.Nodes.getNodeAs<clang::VarDecl>(str_binaryOp_lhs);

        if (syncPull_var) {
          syncPull_var->dumpColor();
          llvm::outs() << "BINARY IS referenced : "
                       << syncPull_var->isReferenced() << "  " << i.first
                       << "\n";

          if (syncPull_var->isReferenced()) {
            llvm::outs() << "ADDING FOR SYNC PULL\n";
#if 0
              ReductionOps_entry reduceOP_entry;
              reduceOP_entry.GRAPH_NAME = j.GRAPH_NAME;
              reduceOP_entry.NODE_TYPE = j.NODE_TYPE;
              reduceOP_entry.FIELD_TYPE = j.NODE_TYPE;
              reduceOP_entry.FIELD_NAME = j.FIELD_NAME;
              reduceOP_entry.VAL_TYPE = j.VAL_TYPE;
              reduceOP_entry.SYNC_TYPE = "sync_pull";
#endif
            llvm::outs() << " sync for struct " << i.first << "\n";
            /** check for duplicate **/
            // if(!syncPull_reduction_exists(reduceOP_entry, i.second)){
            // llvm::outs() << " Adding for " << i.first << "\n";
            // info->reductionOps_map[i.first].push_back(reduceOP_entry);
            //}
          }
          break;
        }
        /*
        else if(syncPull_binary_var){
          syncPull_binary_var->dumpColor();
          llvm::outs() << "BINARY IS referenced : " <<
        syncPull_binary_var->isReferenced() <<"  " <<i.first << "\n"; break;
        }
        */
      }
    }
  }
};

#endif //_PLUGIN_ANALYSIS_SYNC_PULL_CALL_H
