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

#include <sstream>
#include <climits>
#include <vector>
#include <map>
#include <regex>
#include <algorithm>
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"
#include "llvm/Support/raw_ostream.h"

/*
 *  * Matchers
 *   */

#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

/*
 * Rewriter
 */

#include "clang/Rewrite/Core/Rewriter.h"

#include "analysis_helper_functions.h"
#include "data_structures.h"
#include "ast_common_matchers.h"

/*
 * CallBack handlers for AST matcher calls.
 */
#include "callback_handlers/main_call_expr_handler.h"
#include "callback_handlers/field_inside_forloop_handler.h"
#include "callback_handlers/field_outside_forloop_handler.h"
#include "callback_handlers/field_used_in_forloop_handler.h"
#include "callback_handlers/loop_transform_handler.h"
#include "callback_handlers/sync_pull_call_handler.h"
#include "callback_handlers/operator_split_handler.h"
#include "callback_handlers/add_new_variable_handler.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace llvm;
using namespace std;

namespace {

class GaloisFunctionsVisitor
    : public RecursiveASTVisitor<GaloisFunctionsVisitor> {
private:
  ASTContext* astContext;

public:
  explicit GaloisFunctionsVisitor(CompilerInstance* CI)
      : astContext(&(CI->getASTContext())) {}

  virtual bool VisitCXXRecordDecl(CXXRecordDecl* Dec) {
    // Dec->dump();
    return true;
  }
  virtual bool VisitFunctionDecl(FunctionDecl* func) {
    // std::string funcName = func->getNameInfo().getName().getAsString();
    std::string funcName = func->getNameAsString();
    if (funcName == "foo") {
      llvm::errs() << "Found"
                   << "\n";
    }
    return true;
  }

  virtual bool VisitCallExpr(CallExpr* call) {
    if (call) {
      auto t = call->getType().getTypePtrOrNull();
      if (t) {
        auto func = call->getDirectCallee();
        if (func)
          if (func->getNameAsString() == "getData") {
            llvm::outs() << func->getNameInfo().getName().getAsString() << "\n";
            call->dump();
          }
      }
    }
    return true;
  }
};

class TypedefHandler : public MatchFinder::MatchCallback {
  InfoClass* info;

public:
  TypedefHandler(InfoClass* _info) : info(_info) {}
  virtual void run(const MatchFinder::MatchResult& Results) {
    auto typedefDecl = Results.Nodes.getNodeAs<clang::TypeDecl>("typedefDecl");

    // llvm::outs() << "GlobalID : " << typedefDecl->getGlobalID() << "\n";
    string Graph_name = typedefDecl->getNameAsString();
    llvm::outs() << "--->" << Graph_name << "\n";
    // typedefDecl->dump();
    auto templatePtr =
        typedefDecl->getTypeForDecl()->getAs<TemplateSpecializationType>();
    if (templatePtr) {
      Graph_entry g_entry;
      /**We only need NopeType and EdgeType**/
      // templatePtr->getArg(0).getAsType().dump();
      llvm::outs() << " ARGUMETNS : " << templatePtr->getNumArgs() << "\n";
      g_entry.NODE_TYPE =
          templatePtr->getArg(0).getAsType().getBaseTypeIdentifier()->getName();
      templatePtr->getArg(1).getAsType().dump();
      g_entry.EDGE_TYPE = templatePtr->getArg(1).getAsType().getAsString();

      info->graphs_map[Graph_name].push_back(g_entry);
      llvm::outs() << "node is :" << g_entry.NODE_TYPE
                   << ", edge is : " << g_entry.EDGE_TYPE << "\n";
    }
  }
};

class FindOperatorHandler : public MatchFinder::MatchCallback {
private:
  ASTContext* astContext;

public:
  FindOperatorHandler(CompilerInstance* CI)
      : astContext(&(CI->getASTContext())) {}
  virtual void run(const MatchFinder::MatchResult& Results) {
    llvm::outs() << "I found one operator\n";

    const CXXRecordDecl* recDecl =
        Results.Nodes.getNodeAs<clang::CXXRecordDecl>("GaloisOP");

    if (recDecl) {
      // recDecl->dump();

      for (auto method : recDecl->methods()) {
        if (method->getNameAsString() == "operator()") {
          llvm::outs() << method->getNameAsString() << "\n";
          // method->dump();
          auto funcDecl = dyn_cast<clang::FunctionDecl>(method);
          llvm::outs() << funcDecl->param_size() << "\n";
          if (funcDecl->hasBody() && (funcDecl->param_size() == 1)) {
            auto paramValdecl =
                funcDecl->getParamDecl(0); // This is GNode src to operator.
            auto stmt = funcDecl->getBody();
            if (stmt) {
              // stmt->dumpColor();

              for (auto stmtChild : stmt->children()) {
                stmtChild->dump();
              }
            }
          }
        }
      }
    }
  }
};
class FunctionCallHandler : public MatchFinder::MatchCallback {
private:
  Rewriter& rewriter;
  InfoClass* info;

public:
  FunctionCallHandler(Rewriter& rewriter, InfoClass* _info)
      : rewriter(rewriter), info(_info) {}
  virtual void run(const MatchFinder::MatchResult& Results) {
    const CallExpr* callFS =
        Results.Nodes.getNodeAs<clang::CallExpr>("galoisLoop");

    if (callFS) {
      // callFS->dumpColor();
      // llvm::outs() << "----------------------------------- HERE
      // ------------------------\n";
      auto callRecordDecl =
          Results.Nodes.getNodeAs<clang::CXXRecordDecl>("do_all_recordDecl");
      string struct_name = callRecordDecl->getNameAsString();

      SourceLocation ST_main = callFS->getSourceRange().getBegin();
      clang::LangOptions LangOpts;
      LangOpts.CPlusPlus = true;
      clang::PrintingPolicy Policy(LangOpts);

      // Add text before
      // stringstream SSBefore;
      // SSBefore << "/* Galois:: do_all before  */\n";
      // SourceLocation ST = callFS->getSourceRange().getBegin();
      // rewriter.InsertText(ST, "hello", true, true);

      // Add text after
      for (auto i : info->reductionOps_map) {
        if ((i.first == struct_name) && (i.second.size() > 0)) {
          for (auto j : i.second) {
            // llvm::outs() << "i : " << i.first << ", j : " << j.FIELD_NAME <<
            // "\n";
            stringstream SSAfter;
            // j.READFLAG = "";
            // j.WRITEFLAG = "";
            j.IS_RESET = false;
            for (auto h : info->syncFlags_map[struct_name]) {
              if (h.FIELD_NAME == j.FIELD_NAME) {
                for (auto k : info->syncFlags_map[struct_name]) {
                  // llvm::outs() << "\tk : " << k.FIELD_NAME << "\n";
                  if (k.FIELD_NAME == h.FIELD_NAME) {
                    if (k.RW == "read") {
                      if (j.READFLAG == "")
                        j.READFLAG = k.AT;
                      else if (((j.READFLAG == "readSource") &&
                                (k.AT == "readDestination")) ||
                               ((j.READFLAG == "readDestination") &&
                                (k.AT == "readSource"))) {
                        // llvm::outs() << k.FIELD_NAME << " : " << j.READFLAG
                        // << " : k.AT : " << k.AT << "\n";
                        j.READFLAG = "readAny";
                      }
                    }
                    if (k.RW == "write") {
                      if (j.WRITEFLAG == "") {
                        j.WRITEFLAG = k.AT;
                        j.IS_RESET |= k.IS_RESET;
                      } else if (((j.WRITEFLAG == "writeSource") &&
                                  (k.AT == "writeDestination")) ||
                                 ((j.WRITEFLAG == "writeDestination") &&
                                  (k.AT == "writeSource"))) {
                        j.WRITEFLAG = "writeAny";
                        j.IS_RESET |= k.IS_RESET;
                      }
                    }
                  }
                }
              }
            }

            // llvm::outs() << " j.SYNC_TYPE : " << j.SYNC_TYPE << "\n";
            // TODO: Hack: READ and WRITE flags are not setting correctly.
            if (j.READFLAG == "")
              j.READFLAG = "readAny";
            if (j.WRITEFLAG == "")
              j.WRITEFLAG = "writeAny";

            if (j.SYNC_TYPE == "sync_push") {
              // llvm::outs() << "\t sync_push\n";
              SSAfter << ", galois::write_set(\"" << j.SYNC_TYPE << "\", \""
                      << j.GRAPH_NAME << "\", \"" << j.NODE_TYPE << "\", \""
                      << j.FIELD_TYPE << "\" , \"" << j.FIELD_NAME << "\", \""
                      << j.VAL_TYPE << "\" , \"" << j.OPERATION_EXPR
                      << "\",  \"" << j.RESETVAL_EXPR << "\",  \"" << j.READFLAG
                      << "\",  \"" << j.WRITEFLAG << "\")";
            } else if (j.SYNC_TYPE == "sync_pull")
              SSAfter << ", galois::write_set(\"" << j.SYNC_TYPE << "\", \""
                      << j.GRAPH_NAME << "\", \"" << j.NODE_TYPE << "\", \""
                      << j.FIELD_TYPE << "\", \"" << j.FIELD_NAME << "\" , \""
                      << j.VAL_TYPE << "\" , \"" << j.OPERATION_EXPR
                      << "\",  \"" << j.RESETVAL_EXPR << "\",  \"" << j.READFLAG
                      << "\",  \"" << j.WRITEFLAG << "\",  \""
                      << j.MODIFIED_IN_EDGE_LOOP << "\")";

            llvm::outs() << "Printing SSAfter : " << SSAfter.str() << "\n";
            SourceLocation ST =
                callFS->getSourceRange().getEnd().getLocWithOffset(0);
            rewriter.InsertText(ST, SSAfter.str(), true, true);
          }
        }
      }
    }
  }
};

class GaloisFunctionsConsumer : public ASTConsumer {
private:
  CompilerInstance& Instance;
  std::set<std::string> ParsedTemplates;
  GaloisFunctionsVisitor* Visitor;
  MatchFinder Matchers, Matchers_doall, Matchers_op, Matchers_typedef,
      Matchers_gen, Matchers_gen_field, Matchers_2LT, Matchers_syncpull_field,
      Matchers_splitOperator, Matchers_addNewVariable;
  FunctionCallHandler functionCallHandler;
  FindOperatorHandler findOperator;
  CallExprHandler callExprHandler;
  TypedefHandler typedefHandler;
  FindingFieldHandler f_handler;
  FindingFieldInsideForLoopHandler insideForLoop_handler;
  FieldUsedInForLoop insideForLoopField_handler;
  SyncPullInsideForLoopHandler syncPull_handler;
  SyncPullInsideForEdgeLoopHandler syncPullEdge_handler;
  LoopTransformHandler loopTransform_handler;
  OperatorSplitHandler operatorSplit_handler;
  AddNewVariableHandler addNewVariable_handler;
  InfoClass info;

public:
  GaloisFunctionsConsumer(CompilerInstance& Instance,
                          std::set<std::string> ParsedTemplates, Rewriter& R)
      : Instance(Instance), ParsedTemplates(ParsedTemplates),
        Visitor(new GaloisFunctionsVisitor(&Instance)),
        functionCallHandler(R, &info), findOperator(&Instance),
        callExprHandler(&Instance, &info), typedefHandler(&info),
        f_handler(&Instance, &info, R),
        insideForLoop_handler(&Instance, &info, R),
        insideForLoopField_handler(&Instance, &info),
        loopTransform_handler(R, &info), syncPull_handler(&Instance, &info),
        syncPullEdge_handler(&Instance, &info), operatorSplit_handler(R, &info),
        addNewVariable_handler(R, &info) {

    /**************** Additional matchers ********************/
    // Matchers.addMatcher(callExpr(isExpansionInMainFile(),
    // callee(functionDecl(hasName("getData"))),
    // hasAncestor(binaryOperator(hasOperatorName("=")).bind("assignment"))).bind("getData"),
    // &callExprHandler); //*** WORKS
    // Matchers.addMatcher(recordDecl(hasDescendant(callExpr(isExpansionInMainFile(),
    // callee(functionDecl(hasName("getData")))).bind("getData"))).bind("getDataInStruct"),
    // &callExprHandler); //**** works

    /*MATCHER 1 :*************** Matcher to get the type of node defined by user
     * *************************************/
    // Matchers_typedef.addMatcher(typedefDecl(isExpansionInMainFile()).bind("typedefDecl"),
    // &typedefHandler);
    /****************************************************************************************************************/

    /*MATCHER 2 :************* For matching the nodes in AST with getData
     * function call *****************************/
    StatementMatcher getDataCallExprMatcher =
        callExpr(
            isExpansionInMainFile(), callee(functionDecl(hasName("getData"))),
            hasAncestor(recordDecl().bind("callerInStruct")),
            anyOf(hasAncestor(binaryOperator(hasOperatorName("="),
                                             hasLHS(anyOf(LHSRefVariable,
                                                          LHSNonRefVariable)))
                                  .bind("getDataAssignment")),
                  hasAncestor(declStmt(hasSingleDecl(
                                           varDecl(hasType(references(AnyType)))
                                               .bind("getData_varName")))
                                  .bind("refVariableDecl_getData")),
                  hasAncestor(
                      declStmt(hasSingleDecl(
                                   varDecl(unless(hasType(references(AnyType))))
                                       .bind("getData_varName")))
                          .bind("nonRefVariableDecl_getData"))),
            hasDescendant(memberExpr(
                hasDescendant(memberExpr().bind("getData_memExpr_graph")))),
            unless(hasAncestor(EdgeForLoopMatcher)))
            .bind("calleeName");
    Matchers.addMatcher(getDataCallExprMatcher, &callExprHandler);
    /****************************************************************************************************************/

    /*MATCHER 3 : ********************* For matching the nodes in AST with
     * getEdgeDst function call and with getData inside it ******/

    /** We only care about for loops which have getEdgeDst and getData. **/
    Matchers.addMatcher(
        callExpr(isExpansionInMainFile(),
                 callee(functionDecl(hasName("getEdgeDst"))),
                 hasAncestor(recordDecl().bind("callerInStruct")),
                 hasAncestor(EdgeForLoopMatcher),
                 hasAncestor(declStmt(hasSingleDecl(varDecl(hasType(AnyType))
                                                        .bind("varDeclName")))
                                 .bind("variableDecl_getEdgeDst")))
            .bind("calleeName"),
        &callExprHandler);

    /****************************************************************************************************************/
  }

  virtual void HandleTranslationUnit(ASTContext& Context) {
    // Visitor->TraverseDecl(Context.getTranslationUnitDecl());
    Matchers_typedef.matchAST(Context);
    Matchers.matchAST(Context);
    llvm::outs() << " MAP SIZE : " << info.getData_map.size() << "\n";
    for (auto i : info.getData_map) {
      llvm::outs() << "\n"
                   << i.first << " has " << i.second.size()
                   << " references of getData \n";

      int width = 30;
      llvm::outs() << center(string("VAR_NAME"), width) << "|"
                   << center(string("VAR_TYPE"), width) << "|"
                   << center(string("SRC_NAME"), width) << "|"
                   << center(string("IS_REFERENCE"), width) << "|"
                   << center(string("IS_REFERENCED"), width) << "||"
                   << center(string("RW_STATUS"), width) << "||"
                   << center(string("GRAPH_NAME"), width) << "\n";
      llvm::outs() << std::string(width * 7 + 2 * 7, '-') << "\n";
      for (auto j : i.second) {
        llvm::outs() << center(j.VAR_NAME, width) << "|"
                     << center(j.VAR_TYPE, width) << "|"
                     << center(j.SRC_NAME, width) << "|"
                     << center(j.IS_REFERENCE, width) << "|"
                     << center(j.IS_REFERENCED, width) << "|"
                     << center(j.RW_STATUS, width) << "|"
                     << center(j.GRAPH_NAME, width) << "\n";
      }
    }

    llvm::outs() << "\n\n Printing for getData in forLoop for all edges\n\n";
    for (auto i : info.edgeData_map) {
      llvm::outs() << "\n"
                   << i.first << " has " << i.second.size()
                   << " references of getData \n";

      int width = 25;
      llvm::outs() << center(string("VAR_NAME"), width) << "|"
                   << center(string("VAR_TYPE"), width) << "|"
                   << center(string("SRC_NAME"), width) << "|"
                   << center(string("IS_REFERENCE"), width) << "|"
                   << center(string("IS_REFERENCED"), width) << "||"
                   << center(string("RW_STATUS"), width) << "||"
                   << center(string("GRAPH_NAME"), width) << "\n";
      llvm::outs() << std::string(width * 7 + 2 * 7, '-') << "\n";
      for (auto j : i.second) {
        llvm::outs() << center(j.VAR_NAME, width) << "|"
                     << center(j.VAR_TYPE, width) << "|"
                     << center(j.SRC_NAME, width) << "|"
                     << center(j.IS_REFERENCE, width) << "|"
                     << center(j.IS_REFERENCED, width) << "|"
                     << center(j.RW_STATUS, width) << "|"
                     << center(j.GRAPH_NAME, width) << "\n";
      }
    }

    /*MATCHER 5: ********************* Match to get fields of NodeData structure
     * being modified  but not in the Galois all edges forLoop
     * *******************/
    for (auto i : info.getData_map) {
      for (auto j : i.second) {
        if (j.IS_REFERENCED && j.IS_REFERENCE) {
          string str_memExpr    = "memExpr_" + j.VAR_NAME + "_" + i.first;
          string str_assignment = "equalOp_" + j.VAR_NAME + "_" + i.first;
          string str_assignment_complexVec =
              "equalOpComplexVec_" + j.VAR_NAME + "_" + i.first;
          string str_plusOp      = "plusEqualOp_" + j.VAR_NAME + "_" + i.first;
          string str_assign_plus = "assignplusOp_" + j.VAR_NAME + "_" + i.first;
          string str_minusOp     = "minusEqualOp_" + j.VAR_NAME + "_" + i.first;
          string str_varDecl     = "varDecl_" + j.VAR_NAME + "_" + i.first;
          string str_varDecl_nonRef =
              "varDeclNonRef_" + j.VAR_NAME + "_" + i.first;

          string str_galoisAdd = "galoisAdd_" + j.VAR_NAME + "_" + i.first;

          string str_atomicAdd = "atomicAdd_" + j.VAR_NAME + "_" + i.first;
          string str_atomicMin = "atomicMin_" + j.VAR_NAME + "_" + i.first;
          string str_min       = "min_" + j.VAR_NAME + "_" + i.first;

          string str_plusOp_vec =
              "plusEqualOpVec_" + j.VAR_NAME + "_" + i.first;
          string str_assignment_vec =
              "equalOpVec_" + j.VAR_NAME + "_" + i.first;

          string str_ifCompare  = "ifCompare_" + j.VAR_NAME + "_" + i.first;
          string str_resetField = "reset_" + j.VAR_NAME + "_" + i.first;
          string str_resetField_ifStmt =
              "reset_ifStmt_" + j.VAR_NAME + "_" + i.first;

          /***Find read set: Read at source for non ref Variables. ***/
          DeclarationMatcher f_1_nonRef =
              varDecl(isExpansionInMainFile(),
                      hasInitializer(expr(anyOf(
                          hasDescendant(
                              memberExpr(hasDescendant(declRefExpr(
                                             to(varDecl(hasName(j.VAR_NAME))))))
                                  .bind(str_memExpr)),
                          memberExpr(hasDescendant(declRefExpr(
                                         to(varDecl(hasName(j.VAR_NAME))))))
                              .bind(str_memExpr)))),
                      unless(hasType(references(AnyType))),
                      hasAncestor(recordDecl(hasName(i.first))))
                  .bind(str_varDecl_nonRef);

          /** Only match references types, ignore read only **/
          DeclarationMatcher f_1 =
              varDecl(isExpansionInMainFile(),
                      hasInitializer(expr(anyOf(
                          hasDescendant(
                              memberExpr(hasDescendant(declRefExpr(
                                             to(varDecl(hasName(j.VAR_NAME))))))
                                  .bind(str_memExpr)),
                          memberExpr(hasDescendant(declRefExpr(
                                         to(varDecl(hasName(j.VAR_NAME))))))
                              .bind(str_memExpr)))),
                      hasType(references(AnyType)),
                      hasAncestor(recordDecl(hasName(i.first))))
                  .bind(str_varDecl);

          StatementMatcher LHS_memExpr =
              memberExpr(
                  hasDescendant(declRefExpr(to(varDecl(hasName(j.VAR_NAME))))))
                  .bind(str_memExpr);
          /** Order matters as the most generic ones should be at the last **/
          StatementMatcher f_2 = expr(isExpansionInMainFile(), anyOf(
                                                                      /** For builtInType : NodeData.field += val **/
                                                                      binaryOperator(hasOperatorName("+="),
                                                                            hasLHS(ignoringParenImpCasts(LHS_memExpr))).bind(str_plusOp),
                                                                      /** For builtInType : NodeData.field = NodeData.field + val **/
                                                                      binaryOperator(hasOperatorName("="),
                                                                            hasLHS(ignoringParenImpCasts(LHS_memExpr)),
                                                                            hasRHS(binaryOperator(hasOperatorName("+"),
                                                                                                  hasLHS(ignoringParenImpCasts(LHS_memExpr))))).bind(str_assign_plus),
                                                                      /** For builtInType : NodeData.field = val **/
                                                                      binaryOperator(hasOperatorName("="),
                                                                            hasLHS(ignoringParenImpCasts(LHS_memExpr))).bind(str_assignment),

                                                                      /** For ComplexType : NodeData.field = val **/
                                                                      operatorCallExpr(hasOverloadedOperatorName("="),
                                                                              hasDescendant(LHS_memExpr)).bind(str_assignment),

                                                                      /** For ComplexType : NodeData.field += val **/
                                                                      operatorCallExpr(hasOverloadedOperatorName("+="),
                                                                              hasDescendant(LHS_memExpr)).bind(str_plusOp),
                                                                      /** For ComplexType : NodeData.field = NodeData.field + val **/
                                                                      operatorCallExpr(hasOverloadedOperatorName("="),
                                                                              hasDescendant(binaryOperator(hasOperatorName("+"),
                                                                                                              hasLHS(hasDescendant(LHS_memExpr))))).bind(str_assign_plus),
                                                                      /** For ComplexType vectors : NodeData.field[i] = val: field is complexType **/
                                                                      operatorCallExpr(hasOverloadedOperatorName("="),
                                                                             hasDescendant(declRefExpr(to(methodDecl(hasName("operator="))))),
                                                                             hasDescendant(declRefExpr(to(methodDecl(hasName("operator[]"))))),
                                                                             hasDescendant(LHS_memExpr)).bind(str_assignment_vec),
                                                                      /** For ComplexType vectors : NodeData.field[i] += val: field is complexType **/
                                                                      operatorCallExpr(hasOverloadedOperatorName("="),
                                                                             hasDescendant(declRefExpr(to(methodDecl(hasName("operator+="))))),
                                                                             hasDescendant(declRefExpr(to(methodDecl(hasName("operator[]"))))),
                                                                             hasDescendant(LHS_memExpr)).bind(str_plusOp_vec),
                                                                      /** For ComplexType : NodeData.field = val **/
                                                                      operatorCallExpr(hasOverloadedOperatorName("="),
                                                                             hasDescendant(LHS_memExpr)).bind(str_assignment),

                                                                      /** For vector with base types **/
                                                                      binaryOperator(hasOperatorName("="), hasLHS(operatorCallExpr(hasDescendant(declRefExpr(to(methodDecl(hasName("operator[]"))))), hasDescendant(LHS_memExpr))), hasAncestor(compoundStmt().bind("cmd"))).bind(str_assignment_vec),
                                                                      binaryOperator(hasOperatorName("+="), hasLHS(operatorCallExpr(hasDescendant(declRefExpr(to(methodDecl(hasName("operator[]"))))), hasDescendant(LHS_memExpr)))).bind(str_plusOp_vec),

                                                                      /** Galois Add **/
                                                                      callExpr(argumentCountIs(2), hasDescendant(declRefExpr(to(functionDecl(hasName("add"))))), hasAnyArgument(LHS_memExpr)).bind(str_galoisAdd),

                                                                      /** Atomic Add **/
                                                                      callExpr(argumentCountIs(2), hasDescendant(declRefExpr(to(functionDecl(hasName("atomicAdd"))))), hasAnyArgument(LHS_memExpr)).bind(str_atomicAdd),
                                                                      /** Atomic min **/
                                                                      callExpr(argumentCountIs(2), hasDescendant(declRefExpr(to(functionDecl(hasName("atomicMin"))))), hasAnyArgument(LHS_memExpr)).bind(str_atomicMin),
                                                                      /** min **/
                                                                      callExpr(argumentCountIs(2), hasDescendant(declRefExpr(to(functionDecl(hasName("min"))))), hasAnyArgument(LHS_memExpr)).bind(str_min),

                                                                      binaryOperator(hasOperatorName("="), hasDescendant(arraySubscriptExpr(hasDescendant(LHS_memExpr)).bind("arraySub"))).bind(str_assignment),
                                                                      binaryOperator(hasOperatorName("+="), hasDescendant(arraySubscriptExpr(hasDescendant(LHS_memExpr)).bind("arraySub"))).bind(str_plusOp),

                                                                     /** For vectors **/
                                                                      binaryOperator(hasOperatorName("+="), hasDescendant(operatorCallExpr().bind("forVector"))).bind("OpforVector"),

                                                                      /** Galois Reset noticed, ASSUMES : reset is enclosed in an If stmt **/
                                                                      callExpr(argumentCountIs(2), hasDescendant(declRefExpr(to(functionDecl(hasName("reset"))))), 
                                                                                                                hasAnyArgument(LHS_memExpr),
                                                                                                                hasAncestor(ifStmt().bind(str_resetField_ifStmt))).bind(str_resetField)

                                                                     ),
                                                                     hasAncestor(recordDecl(hasName(i.first)))/*,
                                                                     unless(hasAncestor(ifStmt(hasCondition(anything()))))*/);

          /** To consider the fields read in comparison : nodeData.field1 >
           * something **/
          StatementMatcher f_3_comparisonLHS =
              ifStmt(isExpansionInMainFile(),
                     hasCondition(allOf(
                         binaryOperator(hasOperatorName(">"),
                                        hasLHS(hasDescendant(LHS_memExpr))),
                         hasAncestor(recordDecl(hasName(i.first))))))
                  .bind(str_ifCompare);
          /** To consider the fields read in comparison : something  >
           * nodeData.field1 **/
          StatementMatcher f_3_comparisonRHS =
              ifStmt(isExpansionInMainFile(),
                     hasCondition(allOf(
                         binaryOperator(hasOperatorName(">"),
                                        hasRHS(hasDescendant(LHS_memExpr))),
                         hasAncestor(recordDecl(hasName(i.first))))))
                  .bind(str_ifCompare);

          /** Non Reduction operations inside the for edge loop ***/
          StatementMatcher f_1_non_reduction = expr(
              isExpansionInMainFile(),
              anyOf(
                  /** For builtInType : NodeData.field += val **/
                  binaryOperator(hasOperatorName("-="),
                                 hasLHS(ignoringParenImpCasts(LHS_memExpr)))
                      .bind(str_minusOp),

                  /** For ComplexType : NodeData.field += val **/
                  operatorCallExpr(hasOverloadedOperatorName("-="),
                                   hasDescendant(LHS_memExpr))
                      .bind(str_minusOp)),
              hasAncestor(recordDecl(hasName(i.first))),
              hasAncestor(EdgeForLoopMatcher));

          Matchers_gen.addMatcher(f_1, &f_handler);
          Matchers_gen.addMatcher(f_2, &f_handler);
          Matchers_gen.addMatcher(f_1_nonRef, &f_handler);
          Matchers_gen.addMatcher(f_3_comparisonLHS, &f_handler);
          Matchers_gen.addMatcher(f_3_comparisonRHS, &f_handler);
          Matchers_gen.addMatcher(f_1_non_reduction, &f_handler);
        }
      }
    }
    /****************************************************************************************************************/

    /*MATCHER 5: *********************Match to get fields of NodeData structure
     * being modified inside the Galois all edges forLoop *******************/
    llvm::outs() << " ********************************** NOW "
                    "***************************\n";
    for (auto i : info.edgeData_map) {
      llvm::outs() << "--> i " << i.first << "\n";
      for (auto j : i.second) {
        if (j.IS_REFERENCED && j.IS_REFERENCE) {
          string str_memExpr     = "memExpr_" + j.VAR_NAME + "_" + i.first;
          string str_assignment  = "equalOp_" + j.VAR_NAME + "_" + i.first;
          string str_plusOp      = "plusEqualOp_" + j.VAR_NAME + "_" + i.first;
          string str_assign_plus = "assignplusOp_" + j.VAR_NAME + "_" + i.first;
          string str_minusOp     = "minusEqualOp_" + j.VAR_NAME + "_" + i.first;
          string str_varDecl     = "varDecl_" + j.VAR_NAME + "_" + i.first;
          string str_varDecl_nonRef =
              "varDeclNonRef_" + j.VAR_NAME + "_" + i.first;

          string str_ifMin    = "ifMin_" + j.VAR_NAME + "_" + i.first;
          string str_ifMinRHS = "ifMinRHS_" + j.VAR_NAME + "_" + i.first;
          string str_Cond_assignment =
              "Cond_equalOp_" + j.VAR_NAME + "_" + i.first;
          string str_Cond_assignmentRHS =
              "Cond_equalOpRHS_" + j.VAR_NAME + "_" + i.first;
          string str_Cond_RHSmemExpr =
              "Cond_RHSmemExpr_" + j.VAR_NAME + "_" + i.first;
          string str_Cond_RHSVarDecl =
              "Cond_RHSVarDecl_" + j.VAR_NAME + "_" + i.first;
          string str_ifCompare = "ifCompare_" + j.VAR_NAME + "_" + i.first;

          string str_whileCAS     = "whileCAS_" + j.VAR_NAME + "_" + i.first;
          string str_whileCAS_RHS = "whileCAS_RHS" + j.VAR_NAME + "_" + i.first;

          string str_atomicAdd = "atomicAdd_" + j.VAR_NAME + "_" + i.first;
          string str_atomicMin = "atomicMin_" + j.VAR_NAME + "_" + i.first;
          string str_min       = "min_" + j.VAR_NAME + "_" + i.first;

          string str_plusOp_vec =
              "plusEqualOpVec_" + j.VAR_NAME + "_" + i.first;
          string str_assignment_vec =
              "equalOpVec_" + j.VAR_NAME + "_" + i.first;

          string str_syncPull_var = "syncPullVar_" + j.VAR_NAME + "_" + i.first;
          string str_resetField   = "reset_" + j.VAR_NAME + "_" + i.first;

          /***Find read set: Read at source for non ref Variables. ***/
          DeclarationMatcher f_1_nonRef =
              varDecl(isExpansionInMainFile(),
                      hasInitializer(expr(anyOf(
                          hasDescendant(
                              memberExpr(hasDescendant(declRefExpr(
                                             to(varDecl(hasName(j.VAR_NAME))))))
                                  .bind(str_memExpr)),
                          memberExpr(hasDescendant(declRefExpr(
                                         to(varDecl(hasName(j.VAR_NAME))))))
                              .bind(str_memExpr)))),
                      unless(hasType(references(AnyType))),
                      hasAncestor(recordDecl(hasName(i.first))))
                  .bind(str_varDecl_nonRef);

          /** Only match references types, ignore read only **/

          DeclarationMatcher f_1 =
              varDecl(isExpansionInMainFile(),
                      hasInitializer(expr(anyOf(
                          hasDescendant(
                              memberExpr(hasDescendant(declRefExpr(
                                             to(varDecl(hasName(j.VAR_NAME))))))
                                  .bind(str_memExpr)),
                          memberExpr(hasDescendant(declRefExpr(
                                         to(varDecl(hasName(j.VAR_NAME))))))
                              .bind(str_memExpr)))),
                      hasType(references(AnyType)),
                      hasAncestor(recordDecl(hasName(i.first))))
                  .bind(str_varDecl);

          StatementMatcher LHS_memExpr =
              memberExpr(
                  hasDescendant(declRefExpr(to(varDecl(hasName(j.VAR_NAME))))),
                  hasAncestor(recordDecl(hasName(i.first))))
                  .bind(str_memExpr);
          StatementMatcher Cond_notMemExpr =
              memberExpr(
                  hasDescendant(declRefExpr(to(
                      varDecl(unless(hasName(j.VAR_NAME))).bind(str_varDecl)))),
                  hasAncestor(recordDecl(hasName(i.first))))
                  .bind(str_Cond_RHSmemExpr);
          StatementMatcher Cond_notDeclExpr =
              declRefExpr(
                  to(varDecl(unless(hasName(j.VAR_NAME))).bind(str_varDecl)),
                  hasAncestor(recordDecl(hasName(i.first))))
                  .bind(str_Cond_RHSmemExpr);

          /** REDUCTIONS : Only need syncPush : Order matters as the most
           * generic ones should be at the last **/
          StatementMatcher f_2 = expr(
              isExpansionInMainFile(),
              anyOf(
                  /** For builtInType : NodeData.field += val **/
                  binaryOperator(hasOperatorName("+="),
                                 hasLHS(ignoringParenImpCasts(LHS_memExpr)))
                      .bind(str_plusOp),
                  /** For builtInType : NodeData.field = NodeData.field + val
                     **/
                  binaryOperator(
                      hasOperatorName("="),
                      hasLHS(ignoringParenImpCasts(LHS_memExpr)),
                      hasRHS(binaryOperator(
                          hasOperatorName("+"),
                          hasLHS(ignoringParenImpCasts(LHS_memExpr)))))
                      .bind(str_assign_plus),
                  /** For builtInType : NodeData.field = val **/
                  binaryOperator(hasOperatorName("="),
                                 hasLHS(ignoringParenImpCasts(LHS_memExpr)))
                      .bind(str_assignment),

                  /** For ComplexType vectors : NodeData.field[i] = val: field
                     is complexType **/
                  operatorCallExpr(hasOverloadedOperatorName("="),
                                   hasDescendant(declRefExpr(
                                       to(methodDecl(hasName("operator="))))),
                                   hasDescendant(declRefExpr(
                                       to(methodDecl(hasName("operator[]"))))),
                                   hasDescendant(LHS_memExpr))
                      .bind(str_assignment_vec),
                  /** For ComplexType vectors : NodeData.field[i] += val: field
                     is complexType **/
                  operatorCallExpr(hasOverloadedOperatorName("="),
                                   hasDescendant(declRefExpr(
                                       to(methodDecl(hasName("operator+="))))),
                                   hasDescendant(declRefExpr(
                                       to(methodDecl(hasName("operator[]"))))),
                                   hasDescendant(LHS_memExpr))
                      .bind(str_plusOp_vec),

                  /** For ComplexType : NodeData.field += val **/
                  operatorCallExpr(hasOverloadedOperatorName("+="),
                                   hasDescendant(LHS_memExpr))
                      .bind(str_plusOp),
                  /** For ComplexType : NodeData.field = NodeData.field + val
                     **/
                  operatorCallExpr(hasOverloadedOperatorName("="),
                                   hasDescendant(binaryOperator(
                                       hasOperatorName("+"),
                                       hasLHS(hasDescendant(LHS_memExpr)))))
                      .bind(str_assign_plus),
                  /** For ComplexType : NodeData.field = val **/
                  operatorCallExpr(hasOverloadedOperatorName("="),
                                   hasDescendant(LHS_memExpr))
                      .bind(str_assignment),

                  /** for vector operations NodeData.field[i] += val **/
                  binaryOperator(hasOperatorName("="),
                                 hasLHS(operatorCallExpr(
                                     hasDescendant(declRefExpr(to(
                                         methodDecl(hasName("operator[]"))))),
                                     hasDescendant(LHS_memExpr))))
                      .bind(str_assignment_vec),
                  binaryOperator(hasOperatorName("+="),
                                 hasLHS(operatorCallExpr(
                                     hasDescendant(declRefExpr(to(
                                         methodDecl(hasName("operator[]"))))),
                                     hasDescendant(LHS_memExpr))))
                      .bind(str_plusOp_vec),

                  /** Atomic Add **/
                  callExpr(argumentCountIs(2),
                           hasDescendant(declRefExpr(
                               to(functionDecl(hasName("atomicAdd"))))),
                           hasAnyArgument(LHS_memExpr))
                      .bind(str_atomicAdd),
                  // callExpr(argumentCountIs(2),
                  // hasDescendant(declRefExpr(to(functionDecl(hasName("atomicAdd")))))).bind(str_atomicAdd),
                  /** Atomic min **/
                  callExpr(argumentCountIs(2),
                           hasDescendant(declRefExpr(
                               to(functionDecl(hasName("atomicMin"))))),
                           hasAnyArgument(LHS_memExpr))
                      .bind(str_atomicMin),
                  /** min **/
                  callExpr(argumentCountIs(2),
                           hasDescendant(
                               declRefExpr(to(functionDecl(hasName("min"))))),
                           hasAnyArgument(LHS_memExpr))
                      .bind(str_min),

                  callExpr(argumentCountIs(2),
                           hasDescendant(
                               declRefExpr(to(functionDecl(hasName("reset"))))),
                           hasAnyArgument(LHS_memExpr))
                      .bind(str_resetField),

                  binaryOperator(hasOperatorName("="),
                                 hasDescendant(arraySubscriptExpr(
                                                   hasDescendant(LHS_memExpr))
                                                   .bind("arraySub")))
                      .bind(str_assignment),
                  binaryOperator(hasOperatorName("+="),
                                 hasDescendant(arraySubscriptExpr(
                                                   hasDescendant(LHS_memExpr))
                                                   .bind("arraySub")))
                      .bind(str_plusOp)),
              hasAncestor(recordDecl(hasName(i.first))),
              /** Only if condition inside EdgeForLoop should be considered**/
              // unless(hasAncestor(ifStmt(hasCondition(anything()),
              // hasAncestor(EdgeForLoopMatcher)))));
              // TODO: Need to find all the variables updated even inside the if
              // statement
              // TODO: This is just a hack right now
              unless(
                  hasAncestor(ifStmt(hasCondition(hasDescendant(
                                         binaryOperator(hasOperatorName(">")))),
                                     hasAncestor(EdgeForLoopMatcher)))));

          /** Conditional modification within forEdgeLoop **/
          StatementMatcher f_3 =
              ifStmt(
                  isExpansionInMainFile(),
                  hasCondition(allOf(
                      binaryOperator(
                          hasOperatorName(">"),
                          hasLHS(hasDescendant(LHS_memExpr)),
                          hasRHS(hasDescendant(
                              declRefExpr(to(decl().bind(str_Cond_RHSVarDecl)))
                                  .bind(str_ifMinRHS)))),
                      hasAncestor(EdgeForLoopMatcher))),
                  hasDescendant(compoundStmt(anyOf(
                      hasDescendant(binaryOperator(
                          hasOperatorName("="),
                          hasLHS(ignoringParenImpCasts(LHS_memExpr)),
                          hasRHS(hasDescendant(declRefExpr(to(
                              decl(equalsBoundNode(str_Cond_RHSVarDecl)))))))),
                      hasDescendant(
                          operatorCallExpr(hasOverloadedOperatorName("="),
                                           hasDescendant(LHS_memExpr),
                                           hasDescendant(Cond_notDeclExpr)))))))
                  .bind(str_ifMin);

          StatementMatcher f_4 =
              whileStmt(
                  isExpansionInMainFile(),
                  hasCondition(binaryOperator(
                      hasOperatorName(">"), hasLHS(hasDescendant(LHS_memExpr)),
                      hasRHS(ignoringParenImpCasts(
                          declRefExpr(to(decl().bind(str_whileCAS_RHS))))))),
                  hasBody(compoundStmt(hasDescendant(memberCallExpr(
                      callee(
                          methodDecl(matchesName(".compare_exchange_strong"))),
                      hasAnyArgument(declRefExpr(
                          to(decl(equalsBoundNode(str_whileCAS_RHS))))))))))
                  .bind(str_whileCAS);

          /** To consider the fields read in comparison : nodeData.field1 >
           * something **/
          StatementMatcher f_5_comparisonLHS =
              ifStmt(isExpansionInMainFile(),
                     hasCondition(allOf(
                         binaryOperator(hasOperatorName(">"),
                                        hasLHS(hasDescendant(LHS_memExpr))),
                         hasAncestor(EdgeForLoopMatcher))))
                  .bind(str_ifCompare);
          /** To consider the fields read in comparison : something  >
           * nodeData.field1 **/
          StatementMatcher f_5_comparisonRHS =
              ifStmt(isExpansionInMainFile(),
                     hasCondition(allOf(
                         binaryOperator(hasOperatorName(">"),
                                        hasRHS(hasDescendant(LHS_memExpr))),
                         hasAncestor(EdgeForLoopMatcher))))
                  .bind(str_ifCompare);

          /** Non Reduction operations inside the for edge loop ***/
          StatementMatcher f_1_non_reduction = expr(
              isExpansionInMainFile(),
              anyOf(
                  /** For builtInType : NodeData.field += val **/
                  binaryOperator(hasOperatorName("-="),
                                 hasLHS(ignoringParenImpCasts(LHS_memExpr)))
                      .bind(str_minusOp),
                  /** For ComplexType : NodeData.field += val **/
                  operatorCallExpr(hasOverloadedOperatorName("-="),
                                   hasDescendant(LHS_memExpr))
                      .bind(str_minusOp)),
              hasAncestor(
                  methodDecl(
                      hasName("operator()"),
                      hasDescendant(
                          varDecl(hasDescendant(memberCallExpr(callee(
                                      functionDecl(hasName("getData"))))))
                              .bind("varDecl_getData_src")))
                      .bind("op")),
              hasAncestor(recordDecl(hasName(i.first))),
              hasAncestor(EdgeForLoopMatcher));

          /** USE but !REDUCTIONS : NodeData.field is used, therefore needs
           * syncPull **/
#if 0
            DeclarationMatcher f_syncPull_1 = varDecl(isExpansionInMainFile(), hasInitializer(expr(anyOf(
                                                                                                hasDescendant(memberExpr(hasDescendant(declRefExpr(to(varDecl(hasName(j.VAR_NAME)))))).bind(str_memExpr)),
                                                                                                memberExpr(hasDescendant(declRefExpr(to(varDecl(hasName(j.VAR_NAME)))))).bind(str_memExpr)
                                                                                                ),unless(f_2))),
                                                                      hasAncestor(recordDecl(hasName(i.first)))
                                                                  ).bind(str_syncPull_var);
#endif

          // StatementMatcher f_5 = callExpr(isExpansionInMainFile(),
          // argumentCountIs(2), (callee(functionDecl(hasName("atomicAdd"))),
          // hasAnyArgument(LHS_memExpr))).bind(str_atomicAdd);
          Matchers_gen.addMatcher(f_1, &insideForLoop_handler);
          Matchers_gen.addMatcher(f_2, &insideForLoop_handler);
          Matchers_gen.addMatcher(f_3, &insideForLoop_handler);
          Matchers_gen.addMatcher(f_4, &insideForLoop_handler);
          Matchers_gen.addMatcher(f_1_nonRef, &insideForLoop_handler);
          Matchers_gen.addMatcher(f_5_comparisonLHS, &insideForLoop_handler);
          Matchers_gen.addMatcher(f_5_comparisonRHS, &insideForLoop_handler);
          Matchers_gen.addMatcher(f_1_non_reduction, &insideForLoop_handler);
          // Matchers_gen.addMatcher(f_syncPull_1, &insideForLoop_handler);
        }
      }
    }

    /****************************************************************************************************************/

    Matchers_gen.matchAST(Context);

    for (auto i : info.fieldData_map) {
      for (auto j : i.second) {
        if (j.IS_REFERENCED && j.IS_REFERENCE) {
          string str_memExpr     = "memExpr_" + j.VAR_NAME + "_" + i.first;
          string str_assignment  = "equalOp_" + j.VAR_NAME + "_" + i.first;
          string str_plusOp      = "plusEqualOp_" + j.VAR_NAME + "_" + i.first;
          string str_assign_plus = "assignplusOp_" + j.VAR_NAME + "_" + i.first;
          string str_minusOp     = "minusEqualOp_" + j.VAR_NAME + "_" + i.first;
          string str_varDecl     = "varDecl_" + j.VAR_NAME + "_" + i.first;

          string str_ifMin    = "ifMin_" + j.VAR_NAME + "_" + i.first;
          string str_ifMinLHS = "ifMinLHS_" + j.VAR_NAME + "_" + i.first;
          string str_ifMinRHS = "ifMinRHS_" + j.VAR_NAME + "_" + i.first;
          string str_Cond_assignment =
              "Cond_equalOp_" + j.VAR_NAME + "_" + i.first;
          string str_Cond_assignmentRHS =
              "Cond_equalOpRHS_" + j.VAR_NAME + "_" + i.first;
          string str_Cond_RHSmemExpr =
              "Cond_RHSmemExpr_" + j.VAR_NAME + "_" + i.first;
          string str_Cond_RHSVarDecl =
              "Cond_RHSVarDecl_" + j.VAR_NAME + "_" + i.first;

          string str_whileCAS     = "whileCAS_" + j.VAR_NAME + "_" + i.first;
          string str_whileCAS_RHS = "whileCAS_RHS" + j.VAR_NAME + "_" + i.first;

          string str_atomicAdd = "atomicAdd_" + j.VAR_NAME + "_" + i.first;
          string str_varDecl_nonRef =
              "varDeclNonRef_" + j.VAR_NAME + "_" + i.first;

          string str_plusOp_vec =
              "plusEqualOpVec_" + j.VAR_NAME + "_" + i.first;
          string str_assignment_vec =
              "equalOpVec_" + j.VAR_NAME + "_" + i.first;
          string str_ifCompare  = "ifCompare_" + j.VAR_NAME + "_" + i.first;
          string str_resetField = "reset_" + j.VAR_NAME + "_" + i.first;

          /***Find read set: Read at source for non ref Variables. ***/
          DeclarationMatcher f_1_nonRef =
              varDecl(isExpansionInMainFile(),
                      hasInitializer(expr(anyOf(
                          hasDescendant(
                              memberExpr(hasDescendant(declRefExpr(
                                             to(varDecl(hasName(j.VAR_NAME))))))
                                  .bind(str_memExpr)),
                          memberExpr(hasDescendant(declRefExpr(
                                         to(varDecl(hasName(j.VAR_NAME))))))
                              .bind(str_memExpr)))),
                      unless(hasType(references(AnyType))),
                      hasAncestor(recordDecl(hasName(i.first))))
                  .bind(str_varDecl_nonRef);

          /** Only match references types, ignore read only **/
          DeclarationMatcher f_1 =
              varDecl(isExpansionInMainFile(),
                      hasInitializer(expr(anyOf(
                          hasDescendant(
                              memberExpr(hasDescendant(declRefExpr(
                                             to(varDecl(hasName(j.VAR_NAME))))))
                                  .bind(str_memExpr)),
                          memberExpr(hasDescendant(declRefExpr(
                                         to(varDecl(hasName(j.VAR_NAME))))))
                              .bind(str_memExpr)))),
                      hasType(references(AnyType)),
                      hasAncestor(recordDecl(hasName(i.first))))
                  .bind(str_varDecl);

          StatementMatcher LHS_memExpr =
              memberExpr(
                  hasDescendant(declRefExpr(to(varDecl(hasName(j.VAR_NAME))))),
                  hasAncestor(recordDecl(hasName(i.first))))
                  .bind(str_memExpr);
          StatementMatcher LHS_declRefExpr =
              declRefExpr(to(varDecl(hasName(j.VAR_NAME)))).bind(str_ifMinLHS);
          StatementMatcher LHS_declRefExprVector = declRefExpr(
              to(varDecl(hasName(j.VAR_NAME), hasType(asString(j.VAR_TYPE)))));
          StatementMatcher Cond_notDeclExpr =
              declRefExpr(
                  to(varDecl(unless(hasName(j.VAR_NAME))).bind(str_varDecl)),
                  hasAncestor(recordDecl(hasName(i.first))))
                  .bind(str_Cond_RHSmemExpr);
          /** Order matters as the most generic ones should be at the last **/
          StatementMatcher f_2 = expr(
              isExpansionInMainFile(),
              anyOf(
                  /** For builtInType : NodeData.field += val **/
                  binaryOperator(hasOperatorName("+="),
                                 hasLHS(ignoringParenImpCasts(LHS_memExpr)))
                      .bind(str_plusOp),
                  /** For builtInType : NodeData.field = NodeData.field + val
                     **/
                  binaryOperator(
                      hasOperatorName("="),
                      hasLHS(ignoringParenImpCasts(LHS_memExpr)),
                      hasRHS(binaryOperator(
                          hasOperatorName("+"),
                          hasLHS(ignoringParenImpCasts(LHS_memExpr)))))
                      .bind(str_assign_plus),
                  /** For builtInType : NodeData.field = val **/
                  binaryOperator(hasOperatorName("="),
                                 hasLHS(ignoringParenImpCasts(LHS_memExpr)))
                      .bind(str_assignment),

                  /** For ComplexType vectors : NodeData.field[i] = val: field
                     is complexType **/
                  operatorCallExpr(hasOverloadedOperatorName("="),
                                   hasDescendant(declRefExpr(
                                       to(methodDecl(hasName("operator="))))),
                                   hasDescendant(declRefExpr(
                                       to(methodDecl(hasName("operator[]"))))),
                                   hasDescendant(LHS_declRefExpr))
                      .bind(str_assignment_vec),
                  /** For ComplexType vectors : NodeData.field[i] += val: field
                     is complexType **/
                  operatorCallExpr(hasOverloadedOperatorName("="),
                                   hasDescendant(declRefExpr(
                                       to(methodDecl(hasName("operator+="))))),
                                   hasDescendant(declRefExpr(
                                       to(methodDecl(hasName("operator[]"))))),
                                   hasDescendant(LHS_declRefExpr))
                      .bind(str_plusOp_vec),

                  /** For ComplexType : NodeData.field += val **/
                  operatorCallExpr(hasOverloadedOperatorName("+="),
                                   hasDescendant(LHS_memExpr))
                      .bind(str_plusOp),
                  /** For ComplexType : NodeData.field = NodeData.field + val
                     **/
                  operatorCallExpr(hasOverloadedOperatorName("="),
                                   hasDescendant(binaryOperator(
                                       hasOperatorName("+"),
                                       hasLHS(hasDescendant(LHS_memExpr)))))
                      .bind(str_assign_plus),
                  /** For ComplexType : NodeData.field = val **/
                  operatorCallExpr(hasOverloadedOperatorName("="),
                                   hasDescendant(LHS_memExpr))
                      .bind(str_assignment),

                  // callExpr(hasAnyArgument(declRefExpr(to(varDecl(hasName(j.VAR_NAME)[>,
                  // hasType(asString("class
                  // std::vector"))<]))))).bind("vectorOp"),
                  /** For vectors: field[i] += val **/
                  binaryOperator(hasOperatorName("+="),
                                 hasLHS(operatorCallExpr(
                                     hasDescendant(declRefExpr(to(
                                         methodDecl(hasName("operator[]"))))),
                                     hasDescendant(LHS_declRefExpr))))
                      .bind(str_plusOp_vec),
                  binaryOperator(hasOperatorName("="),
                                 hasLHS(operatorCallExpr(
                                     hasDescendant(declRefExpr(to(
                                         methodDecl(hasName("operator[]"))))),
                                     hasDescendant(LHS_declRefExpr))))
                      .bind(str_assignment_vec),

                  /** Galois::atomicAdd(field, val) **/
                  callExpr(argumentCountIs(2),
                           hasDescendant(declRefExpr(
                               to(functionDecl(hasName("atomicAdd"))))),
                           hasDescendant(LHS_declRefExpr))
                      .bind(str_atomicAdd),
                  // callExpr(argumentCountIs(2),
                  // hasDescendant(declRefExpr(to(functionDecl(hasName("atomicAdd"))))),
                  // hasAnyArgument(LHS_declRefExprVector)).bind(str_atomicAdd),

                  /** Galois Reset noticed **/
                  callExpr(argumentCountIs(2),
                           hasDescendant(
                               declRefExpr(to(functionDecl(hasName("reset"))))),
                           hasAnyArgument(LHS_memExpr))
                      .bind(str_resetField),

                  /** For arrays: field[i] += val **/
                  binaryOperator(
                      hasOperatorName("="),
                      hasLHS(arraySubscriptExpr(hasDescendant(LHS_declRefExpr))
                                 .bind("arraySub")))
                      .bind(str_assignment),
                  binaryOperator(
                      hasOperatorName("+="),
                      hasLHS(arraySubscriptExpr(hasDescendant(LHS_declRefExpr))
                                 .bind("arraySub")))
                      .bind(str_plusOp),

                  /** For builtInType : field = val **/
                  binaryOperator(hasOperatorName("="),
                                 hasLHS(ignoringParenImpCasts(LHS_declRefExpr)))
                      .bind(str_assignment),

                  /** For builtInType : field += val **/
                  binaryOperator(hasOperatorName("+="),
                                 hasLHS(ignoringParenImpCasts(LHS_declRefExpr)))
                      .bind(str_plusOp)),
              hasAncestor(recordDecl(hasName(i.first))),
              /** Only if condition inside EdgeForLoop should be considered**/
              unless(hasAncestor(ifStmt(hasCondition(anything()),
                                        hasAncestor(EdgeForLoopMatcher)))));

          /** Conditional modification within forEdgeLoop **/
          StatementMatcher f_3 =
              ifStmt(
                  isExpansionInMainFile(),
                  hasCondition(binaryOperator(
                      hasOperatorName(">"),
                      hasLHS(anyOf(hasDescendant(LHS_memExpr),
                                   hasDescendant(LHS_declRefExpr))),
                      hasRHS(hasDescendant(
                          declRefExpr(to(decl().bind(str_Cond_RHSVarDecl)))
                              .bind(str_ifMinRHS))))),
                  anyOf(
                      hasDescendant(compoundStmt(anyOf(
                          hasDescendant(binaryOperator(
                              hasOperatorName("="),
                              hasLHS(hasDescendant(LHS_declRefExpr)),
                              hasRHS(hasDescendant(declRefExpr(to(decl(
                                  equalsBoundNode(str_Cond_RHSVarDecl)))))))),
                          hasDescendant(operatorCallExpr(
                              hasOverloadedOperatorName("="),
                              hasDescendant(LHS_declRefExpr),
                              hasDescendant(declRefExpr(to(decl(
                                  equalsBoundNode(str_Cond_RHSVarDecl)))))))))),
                      hasDescendant(binaryOperator(
                          hasOperatorName("="), hasLHS(LHS_declRefExpr),
                          hasRHS(ignoringParenImpCasts(declRefExpr(to(
                              decl(equalsBoundNode(str_Cond_RHSVarDecl))))))))))
                  .bind(str_ifMin);

          StatementMatcher f_4 =
              whileStmt(
                  isExpansionInMainFile(),
                  hasCondition(binaryOperator(
                      hasOperatorName(">"), hasLHS(hasDescendant(LHS_memExpr)),
                      hasRHS(ignoringParenImpCasts(
                          declRefExpr(to(decl().bind(str_whileCAS_RHS))))))),
                  hasBody(compoundStmt(hasDescendant(memberCallExpr(
                      callee(
                          methodDecl(matchesName(".compare_exchange_strong"))),
                      hasAnyArgument(declRefExpr(
                          to(decl(equalsBoundNode(str_whileCAS_RHS))))))))))
                  .bind(str_whileCAS);

          // StatementMatcher f_5 =
          // forStmt(isExpansionInMainFile()).bind("arraySub"); StatementMatcher
          // f_5 = callExpr(isExpansionInMainFile(), argumentCountIs(2),
          // (callee(functionDecl(hasName("atomicAdd")))[>,
          // hasAnyArgument(LHS_memExpr)<])).bind(str_atomicAdd);
          /** To consider the fields read in comparison : nodeData.field1 >
           * something **/
          StatementMatcher f_3_comparisonLHS =
              ifStmt(isExpansionInMainFile(),
                     hasCondition(allOf(
                         binaryOperator(hasOperatorName(">"),
                                        hasLHS(hasDescendant(LHS_memExpr))),
                         hasAncestor(recordDecl(hasName(i.first))))))
                  .bind(str_ifCompare);
          /** To consider the fields read in comparison : something  >
           * nodeData.field1 **/
          StatementMatcher f_3_comparisonRHS =
              ifStmt(isExpansionInMainFile(),
                     hasCondition(allOf(
                         binaryOperator(hasOperatorName(">"),
                                        hasRHS(hasDescendant(LHS_memExpr))),
                         hasAncestor(recordDecl(hasName(i.first))))))
                  .bind(str_ifCompare);

          Matchers_gen_field.addMatcher(f_1, &insideForLoopField_handler);
          Matchers_gen_field.addMatcher(f_2, &insideForLoopField_handler);
          Matchers_gen_field.addMatcher(f_3, &insideForLoopField_handler);
          Matchers_gen_field.addMatcher(f_4, &insideForLoopField_handler);
          Matchers_gen_field.addMatcher(f_1_nonRef,
                                        &insideForLoopField_handler);
          Matchers_gen_field.addMatcher(f_3_comparisonRHS,
                                        &insideForLoopField_handler);
          Matchers_gen_field.addMatcher(f_3_comparisonLHS,
                                        &insideForLoopField_handler);
          // Matchers_gen_field.addMatcher(f_5, &insideForLoopField_handler);
        }
      }
    }

    Matchers_gen_field.matchAST(Context);

    llvm::outs() << "\n\n Printing for all fields found \n\n";
    for (auto i : info.fieldData_map) {
      llvm::outs() << "\n"
                   << i.first << " has " << i.second.size() << " Fields \n";

      int width = 25;
      llvm::outs() << center(string("VAR_NAME"), width) << "|"
                   << center(string("VAR_TYPE"), width) << "|"
                   << center(string("FIELD_NAME"), width) << "|"
                   << center(string("NODE_NAME"), width) << "|"
                   << center(string("IS_REFERENCE"), width) << "|"
                   << center(string("IS_REFERENCED"), width) << "||"
                   << center(string("RW_STATUS"), width) << "||"
                   << center(string("RESET_VALTYPE"), width) << "||"
                   << center(string("RESET_VAL"), width) << "||"
                   << center(string("GRAPH_NAME"), width) << "\n";
      llvm::outs() << std::string(width * 10 + 2 * 10, '-') << "\n";
      for (auto j : i.second) {
        llvm::outs() << center(j.VAR_NAME, width) << "|"
                     << center(j.VAR_TYPE, width) << "|"
                     << center(j.FIELD_NAME, width) << "|"
                     << center(j.NODE_NAME, width) << "|"
                     << center(j.IS_REFERENCE, width) << "|"
                     << center(j.IS_REFERENCED, width) << "|"
                     << center(j.RW_STATUS, width) << "|"
                     << center(j.RESET_VALTYPE, width) << "|"
                     << center(j.RESET_VAL, width) << "|"
                     << center(j.GRAPH_NAME, width) << "\n";
      }
    }

    llvm::outs() << "\n\n Printing Read and Write sets \n\n";
    for (auto i : info.syncFlags_map) {
      llvm::outs() << "\n"
                   << i.first << " has " << i.second.size() << " Fields \n";

      int width = 25;
      llvm::outs() << center(string("FIELD_NAME"), width) << "|"
                   << center(string("RW"), width) << "|"
                   << center(string("RESET"), width) << "|"
                   << center(string("AT"), width) << "\n";
      llvm::outs() << std::string(width * 10 + 2 * 10, '-') << "\n";
      for (auto j : i.second) {
        llvm::outs() << center(j.FIELD_NAME, width) << "|"
                     << center(j.RW, width) << "|" << center(j.IS_RESET, width)
                     << "|" << center(j.AT, width) << "\n";
      }
    }

    /* MATCHER : TO add the  new variables to NodeData and Initialization
     * *************************/
    for (auto i : info.newVariables_map) {
      if (i.second.size()) {
        for (auto j : i.second) {
          llvm::outs() << "FOIRRR : " << j.FIELD_NAME << " : " << i.first
                       << "\n";
          std::string str_nodeData_struct =
              "nodeData_struct_" + j.FIELD_NAME + "_" + i.first;
          std::string str_varDecl_getData_src =
              "varDecl_getData_src_" + j.FIELD_NAME + "_" + i.first;
          // StatementMatcher nodeData_struct_matcher =
          // expr(isExpansionInMainFile(),
          // hasDescendant(recordDecl(hasName("NodeData")).bind(str_nodeData_struct)));
          auto nodeData_struct_matcher =
              recordDecl(isExpansionInMainFile(), hasName("NodeData"))
                  .bind(str_nodeData_struct);
          auto InitializeGraph_struct_matcher = recordDecl(
              isExpansionInMainFile(), hasName("InitializeGraph"),
              hasDescendant(methodDecl(
                  hasName("operator()"),
                  hasDescendant(varDecl(hasDescendant(memberCallExpr(callee(
                                            functionDecl(hasName("getData"))))))
                                    .bind(str_varDecl_getData_src)))));

          // StatementMatcher nodeData_struct_matcher =
          // recordDecl(isExpansionInMainFile(),
          // hasName("NodeData")).bind(str_nodeData_struct);

          // Matchers_addNewVariable.addMatcher(nodeData_struct_matcher,
          // &addNewVariable_handler);
          Matchers_addNewVariable.addMatcher(InitializeGraph_struct_matcher,
                                             &addNewVariable_handler);
        }
      }
    }

    Matchers_addNewVariable.matchAST(Context);

    /*MATCHER 6.5: *********************Match to get fields of NodeData
     * structure being modified and used to add SYNC_PULL calls inside the
     * Galois all edges forLoop *******************/
    /** For on-demand communication, there is no need to look across operators
     * to see if sync_pull is required. Just look at operator to which variable
     * belongs and marks it dirty if modified. **/
    for (auto i : info.reductionOps_map) {
      for (auto j : i.second) {
        llvm::outs() << " INSIDE LOOP : j.NODE_NAME : " << j.NODE_NAME
                     << " for : " << i.first << "\n";
        // if(j.IS_REFERENCED && j.IS_REFERENCE) {
        string str_memExpr =
            "memExpr_" + j.NODE_NAME + "_" + j.FIELD_NAME + i.first;
        string str_syncPull_var =
            "syncPullVar_" + j.NODE_NAME + "_" + j.FIELD_NAME + "_" + i.first;
        string str_plusOp =
            "plusEqualOp_" + j.NODE_NAME + "_" + j.FIELD_NAME + "_" + i.first;
        string str_binaryOp_lhs =
            "binaryOp_LHS_" + j.NODE_NAME + "_" + j.FIELD_NAME + "_" + i.first;
        string str_atomicMin =
            "atomicMin_" + j.NODE_NAME + "_" + j.FIELD_NAME + "_" + i.first;
        string str_lhs_argument =
            "argument_" + j.NODE_NAME + "_" + j.FIELD_NAME + "_" + i.first;

        string str_modified_insideEdgeLoop = "insideEdgeLoop_LHS_" +
                                             j.NODE_NAME + "_" + j.FIELD_NAME +
                                             "_" + i.first;
        string str_modified_outsideEdgeLoop = "outsideEdgeLoop_LHS_" +
                                              j.NODE_NAME + "_" + j.FIELD_NAME +
                                              "_" + i.first;

        /** Only need sync_pull if modified **/
        // If not going over allNodes or writing different things, then need to
        // sync pull.
        llvm::outs() << "Sync pull is required : " << j.FIELD_NAME << "\n";
        if (j.SYNC_TYPE == "sync_pull_maybe") {
          StatementMatcher LHS_memExpr =
              memberExpr(
                  hasDescendant(declRefExpr(to(varDecl(hasName(j.NODE_NAME))))),
                  hasAncestor(recordDecl(hasName(i.first))))
                  .bind(str_memExpr);
          StatementMatcher stmt_reductionOp =
              makeStatement_reductionOp(LHS_memExpr, i.first);
          // StatementMatcher RHS_memExpr =
          // hasDescendant(memberExpr(member(hasName(j.FIELD_NAME))));
          StatementMatcher RHS_memExpr =
              memberExpr(member(hasName(j.FIELD_NAME)));
          StatementMatcher LHS_memExpr_argument =
              memberExpr(member(hasName(j.FIELD_NAME))).bind(str_lhs_argument);
          StatementMatcher LHS_varDecl =
              declRefExpr(to(varDecl().bind(str_binaryOp_lhs)));

          /** USE but !REDUCTIONS : NodeData.field is used, therefore needs
           * syncPull **/
#if 0
                DeclarationMatcher f_syncPull_1 = varDecl(isExpansionInMainFile(), hasInitializer(expr(anyOf(
                                                                                                    hasDescendant(memberExpr(hasDescendant(declRefExpr(to(varDecl(hasName(j.NODE_NAME)))))).bind(str_memExpr)),
                                                                                                    memberExpr(hasDescendant(declRefExpr(to(varDecl(hasName(j.NODE_NAME)))))).bind(str_memExpr)
                                                                                                    ),unless(stmt_reductionOp))),
                                                                          hasAncestor(recordDecl(hasName(i.first)))
                                                                      ).bind(str_syncPull_var);
#endif

          /** USE: This works across operators to see if sync_pull is required
           * after an operator finishes **/
          DeclarationMatcher f_syncPull_1 =
              varDecl(
                  isExpansionInMainFile(),
                  hasInitializer(expr(
                      hasDescendant(
                          memberExpr(member(hasName(j.FIELD_NAME)),
                                     hasAncestor(recordDecl(hasName(i.first))))
                              .bind(str_memExpr)),
                      unless(stmt_reductionOp))),
                  hasAncestor(EdgeForLoopMatcher))
                  .bind(str_syncPull_var);

          StatementMatcher f_syncPull_2 =
              expr(isExpansionInMainFile(), unless(stmt_reductionOp),
                   anyOf(
                       /** For builtInType: varname = NodeData.fieldName,
                          varname = anything anyOP NodeData.fieldName, varname =
                          NodeData.fieldName anyOP anything **/
                       binaryOperator(hasOperatorName("="), hasLHS(LHS_varDecl),
                                      hasRHS(RHS_memExpr))
                           .bind("binaryOp"),

                       /** For builtInType : varname += NodeData.fieldName **/
                       binaryOperator(hasOperatorName("+="),
                                      hasLHS(LHS_varDecl), hasRHS(RHS_memExpr)),

                       /** For builtInType : varname -= NodeData.fieldName **/
                       binaryOperator(hasOperatorName("-="),
                                      hasLHS(LHS_varDecl), hasRHS(RHS_memExpr))

                       /** Galois Add **/
                       // callExpr(argumentCountIs(2),
                       // hasDescendant(declRefExpr(to(functionDecl(hasName("add"))))),
                       // hasAnyArgument(RHS_memExpr)),

                       /** Any ifStmt which is using the NodeData.fieldName **/
                       // ifStmt(hasCondition(anything()),
                       // hasDescendant(RHS_memExpr))
                       // binaryOperator(hasLHS(RHS_memExpr))

                       ),
                   hasAncestor(EdgeForLoopMatcher))
                  .bind(str_plusOp);

          /**Any operation that has NodeData.fieldName modified at the source
           * inside edge loop, should be marked dirty and should be
           * synchronized**/
          StatementMatcher f_syncPull_modified_insideEdgeLoop =
              expr(isExpansionInMainFile(),
                   anyOf(binaryOperator(hasOperatorName("="),
                                        hasLHS(LHS_memExpr)),

                         /** Galois Add **/
                         callExpr(argumentCountIs(2),
                                  hasDescendant(declRefExpr(
                                      to(functionDecl(hasName("add"))))),
                                  hasAnyArgument(LHS_memExpr_argument))),
                   hasAncestor(EdgeForLoopMatcher))
                  .bind(str_modified_insideEdgeLoop);

          /**Any operation that has NodeData.fieldName modified at the source
           * outside edge loop, should be just marked dirty, there is no need to
           * synchronize for on_demand synchronization **/
          StatementMatcher f_syncPull_modified_outsideEdgeLoop =
              expr(isExpansionInMainFile(),
                   anyOf(

                       /**Any binary operation that has NodeData.fieldName
                          modified at the source, should be marked dirty **/
                       binaryOperator(hasLHS(LHS_memExpr)),

                       /** For ComplexType : NodeData.field = val **/
                       operatorCallExpr(hasOverloadedOperatorName("="),
                                        hasDescendant(LHS_memExpr))))
                  .bind(str_modified_outsideEdgeLoop);

          Matchers_syncpull_field.addMatcher(f_syncPull_1, &syncPull_handler);
          // Matchers_syncpull_field.addMatcher(f_syncPull_2,
          // &syncPull_handler);
          Matchers_syncpull_field.addMatcher(f_syncPull_modified_insideEdgeLoop,
                                             &syncPull_handler);
          Matchers_syncpull_field.addMatcher(
              f_syncPull_modified_outsideEdgeLoop, &syncPull_handler);
        }
      }
    }

    for (auto i : info.fieldData_map) {
      for (auto j : i.second) {
        string str_memExpr     = "memExpr_" + j.VAR_NAME + "_" + i.first;
        string str_assignment  = "equalOp_" + j.VAR_NAME + "_" + i.first;
        string str_plusOp      = "plusEqualOp_" + j.VAR_NAME + "_" + i.first;
        string str_assign_plus = "assignplusOp_" + j.VAR_NAME + "_" + i.first;
        string str_minusOp     = "minusEqualOp_" + j.VAR_NAME + "_" + i.first;
        string str_varDecl     = "varDecl_" + j.VAR_NAME + "_" + i.first;

        string str_ifMin    = "ifMin_" + j.VAR_NAME + "_" + i.first;
        string str_ifMinRHS = "ifMinRHS_" + j.VAR_NAME + "_" + i.first;
        string str_Cond_assignment =
            "Cond_equalOp_" + j.VAR_NAME + "_" + i.first;
        string str_Cond_assignmentRHS =
            "Cond_equalOpRHS_" + j.VAR_NAME + "_" + i.first;
        string str_Cond_RHSmemExpr =
            "Cond_RHSmemExpr_" + j.VAR_NAME + "_" + i.first;
        string str_Cond_RHSVarDecl =
            "Cond_RHSVarDecl_" + j.VAR_NAME + "_" + i.first;

        string str_syncPull_var = "syncPullVar_" + j.VAR_NAME + "_" + i.first;
        string str_binaryOp_lhs = "binaryOp_LHS_" + j.VAR_NAME + "_" + i.first;

        StatementMatcher LHS_memExpr =
            memberExpr(
                hasDescendant(declRefExpr(to(varDecl(hasName(j.VAR_NAME))))),
                hasAncestor(recordDecl(hasName(i.first))))
                .bind(str_memExpr);
        StatementMatcher stmt_reductionOp =
            makeStatement_reductionOp(LHS_memExpr, i.first);
        StatementMatcher LHS_varDecl =
            declRefExpr(to(varDecl().bind(str_syncPull_var)));
        StatementMatcher f_syncPull_1 = expr(isExpansionInMainFile(), unless(stmt_reductionOp),
                                                                      /** For builtInType : varname += NodeData.fieldName **/
                                                                      binaryOperator(hasOperatorName("+="),
                                                                                      hasLHS(LHS_varDecl)/*,
                                                                                      hasRHS(RHS_memExpr2)*/),
                                                                        hasAncestor(recordDecl(hasName(i.first)))
                                                  ).bind(str_plusOp);

        Matchers_syncpull_field.addMatcher(f_syncPull_1, &syncPullEdge_handler);
      }
    }

    Matchers_syncpull_field.matchAST(Context);

    /** PRINTING FINAL REDUCTION OPERATIONS **/

    llvm::outs() << "\n\n";
    for (auto i : info.reductionOps_map) {
      llvm::outs() << "FOR >>>>> " << i.first << "\n";
      for (auto j : i.second) {
        string write_set = "write_set( " + j.SYNC_TYPE + ", " + j.NODE_TYPE +
                           " , " + j.READFLAG + ", " + j.FIELD_NAME + " , " +
                           j.OPERATION_EXPR + " , " + j.RESETVAL_EXPR + " , " +
                           j.MODIFIED_IN_EDGE_LOOP + ")";
        llvm::outs() << write_set << "\n";
      }
    }

    /**
     * Description: After Identifying the final reductions in each operator,
     * this loop writes the galois::write_sets in each operator execution loops,
     * which will then be used in the next compiler phase to generate
     * appropriate synchronization calls.
     *
     */
    for (auto i : info.reductionOps_map) {
      if (i.second.size() > 0) {
        Matchers_doall.addMatcher(
            callExpr(
                callee(functionDecl(anyOf(hasName("galois::do_all"),
                                          hasName("galois::do_all_local"),
                                          hasName("galois::for_each")))),
                unless(isExpansionInSystemHeader()),
                /*unless(hasDescendant(declRefExpr(to(varDecl(hasName("allNodes")))))),*/
                hasAncestor(
                    recordDecl(hasName(i.first)).bind("do_all_recordDecl")))
                .bind("galoisLoop"),
            &functionCallHandler);
      }
    }

    Matchers_doall.matchAST(Context);

    /*MATCHER 7: ******************** Matchers for 2 loop transformations
     * *******************/
    for (auto i : info.edgeData_map) {
      for (auto j : i.second) {
        if (j.IS_REFERENCED && j.IS_REFERENCE) {
          string str_ifGreater_2loopTrans =
              "ifGreater_2loopTrans_" + j.VAR_NAME + "_" + i.first;
          string str_if_RHS_const = "if_RHS_const" + j.VAR_NAME + "_" + i.first;
          string str_if_RHS_nonconst =
              "if_RHS_nonconst" + j.VAR_NAME + "_" + i.first;
          string str_if_LHS_const = "if_LHS_const" + j.VAR_NAME + "_" + i.first;
          string str_if_LHS_nonconst =
              "if_LHS_nonconst" + j.VAR_NAME + "_" + i.first;
          string str_main_struct        = "main_struct_" + i.first;
          string str_forLoop_2LT        = "forLoop_2LT_" + i.first;
          string str_method_operator    = "methodDecl_" + i.first;
          string str_struct_constructor = "constructorDecl_" + i.first;
          string str_struct_constructor_initList =
              "constructorDeclInitList_" + i.first;
          string str_sdata    = "sdata_" + j.VAR_NAME + "_" + i.first;
          string str_for_each = "for_each_" + j.VAR_NAME + "_" + i.first;

          StatementMatcher LHS_memExpr_nonconst =
              memberExpr(
                  hasDescendant(declRefExpr(to(varDecl(hasName(j.VAR_NAME))))),
                  hasAncestor(recordDecl(hasName(i.first))))
                  .bind(str_if_LHS_nonconst);
          StatementMatcher RHS_memExpr_nonconst =
              memberExpr(
                  hasDescendant(declRefExpr(to(varDecl(hasName(j.VAR_NAME))))),
                  hasAncestor(recordDecl(hasName(i.first))))
                  .bind(str_if_RHS_nonconst);

          StatementMatcher callExpr_atomicAdd =
              callExpr(argumentCountIs(2),
                       hasDescendant(
                           declRefExpr(to(functionDecl(hasName("atomicAdd"))))),
                       hasAnyArgument(
                           anyOf(LHS_memExpr_nonconst, RHS_memExpr_nonconst)));
          StatementMatcher callExpr_atomicMin =
              callExpr(argumentCountIs(2),
                       hasDescendant(
                           declRefExpr(to(functionDecl(hasName("atomicMin"))))),
                       hasAnyArgument(
                           anyOf(LHS_memExpr_nonconst, RHS_memExpr_nonconst)));
          StatementMatcher binaryOp_assign = binaryOperator(
              hasOperatorName("="),
              hasLHS(anyOf(LHS_memExpr_nonconst, RHS_memExpr_nonconst)));

          /**Assumptions: 1. first argument is always src in operator() method
           * **/
          /** For 2 loop Transforms: if nodeData.field > TOLERANCE **/
          /** For 2 loop Transforms: if nodeData.field > nodeData.otherField **/
          StatementMatcher f_2loopTrans_1 =
              ifStmt(
                  isExpansionInMainFile(),
                  hasCondition(allOf(
                      binaryOperator(
                          hasLHS(anyOf(hasDescendant(LHS_memExpr_nonconst),
                                       hasDescendant(memberExpr().bind(
                                           str_if_LHS_const)))),
                          /** Order matters: Finds [nodeData.field >
                             nodeData.field2] first then [nodeData.field >
                             constant] **/
                          hasRHS(anyOf(hasDescendant(RHS_memExpr_nonconst),
                                       hasDescendant(memberExpr().bind(
                                           str_if_RHS_const))))),
                      anyOf(hasAncestor(makeStatement_EdgeForStmt(
                                callExpr_atomicAdd, str_forLoop_2LT)),
                            hasAncestor(makeStatement_EdgeForStmt(
                                callExpr_atomicMin, str_forLoop_2LT)),
                            hasAncestor(makeStatement_EdgeForStmt(
                                binaryOp_assign, str_forLoop_2LT))),
                      hasAncestor(
                          recordDecl(
                              hasName(i.first),
                              hasDescendant(
                                  methodDecl(
                                      hasName("operator()"),
                                      hasAncestor(recordDecl(hasDescendant(
                                          callExpr(
                                              callee(functionDecl(
                                                  hasName("for_each"))),
                                              unless(hasDescendant(declRefExpr(
                                                  to(functionDecl(hasName(
                                                      "workList_version")))))))
                                              .bind(str_for_each)))),
                                      hasParameter(0, decl().bind("src_arg")),
                                      hasDescendant(
                                          declStmt(hasDescendant(declRefExpr(to(
                                                       varDecl(equalsBoundNode(
                                                           "src_arg"))))))
                                              .bind(str_sdata)))
                                      .bind(str_method_operator)), /*TODO: Get
                                                                      begin of
                                                                      constructor
                                                                      to add
                                                                      DGAccumulator
                                                                    */
                              hasDescendant(
                                  constructorDecl(
                                      hasDescendant(declRefExpr().bind(
                                          str_struct_constructor_initList)))
                                      .bind(str_struct_constructor)))
                              .bind(str_main_struct)))))
                  .bind(str_ifGreater_2loopTrans);

          Matchers_2LT.addMatcher(f_2loopTrans_1, &loopTransform_handler);
        }
      }
    }

    Matchers_2LT.matchAST(Context);

    /******* Split operator if required ***********/
    for (auto i : info.syncFlags_map) {
      for (auto j : i.second) {
        if (j.IS_RESET && (j.AT == "writeSource")) {
          bool should_split = false;
          for (auto k : info.syncFlags_map[i.first]) {
            if ((k.FIELD_NAME == j.FIELD_NAME)) {
              if (k.AT == "writeDestination") {
                should_split = true;
              }
            }
          }
          if (should_split) {
            llvm::outs() << " GOING TO SPLIT " << j.VAR_NAME << "\n";
            string str_memExpr    = "memExpr_" + j.VAR_NAME + "_" + i.first;
            string str_resetField = "reset_" + j.VAR_NAME + "_" + i.first;
            string str_resetField_ifStmt =
                "reset_ifStmt_" + j.VAR_NAME + "_" + i.first;
            string str_main_struct        = "main_struct_" + i.first;
            string str_method_operator    = "methodDecl_" + i.first;
            string str_struct_constructor = "constructorDecl_" + i.first;
            string str_struct_constructor_initList =
                "constructorDeclInitList_" + i.first;
            string str_sdata = "sdata_" + j.VAR_NAME + "_" + i.first;
            string str_resetField_expr =
                "reset_expr_" + j.VAR_NAME + "_" + i.first;
            string str_do_all = "do_all_" + i.first;

            StatementMatcher LHS_memExpr =
                memberExpr(hasDescendant(
                               declRefExpr(to(varDecl(hasName(j.VAR_NAME))))))
                    .bind(str_memExpr);
            StatementMatcher f_1 =
                expr(
                    isExpansionInMainFile(),
                    /** Galois Reset noticed, ASSUMES : reset is enclosed in an
                       If stmt **/
                    callExpr(argumentCountIs(2),
                             hasDescendant(declRefExpr(
                                 to(functionDecl(hasName("reset"))))),
                             hasAnyArgument(LHS_memExpr),
                             hasAncestor(ifStmt().bind(str_resetField_ifStmt)))
                        .bind(str_resetField),
                    hasAncestor(
                        recordDecl(
                            hasName(i.first),
                            hasDescendant(
                                methodDecl(
                                    hasName("operator()"),
                                    hasParameter(0, decl().bind("src_arg")),
                                    hasDescendant(
                                        declStmt(hasDescendant(declRefExpr(
                                                     to(varDecl(equalsBoundNode(
                                                         "src_arg"))))))
                                            .bind(str_sdata)))
                                    .bind(
                                        str_method_operator)), /*TODO: Get begin
                                                                  of constructor
                                                                  to add
                                                                  DGAccumulator
                                                                */
                            hasDescendant(
                                constructorDecl(
                                    hasDescendant(declRefExpr().bind(
                                        str_struct_constructor_initList)))
                                    .bind(str_struct_constructor)),
                            hasDescendant(
                                callExpr(callee(functionDecl(anyOf(
                                             hasName("galois::do_all"),
                                             hasName("galois::do_all_local"),
                                             hasName("galois::for_each")))))
                                    .bind(str_do_all)))
                            .bind(str_main_struct)))
                    .bind(str_resetField_expr);

            Matchers_splitOperator.addMatcher(f_1, &operatorSplit_handler);
          }
        }
      }
    }

    Matchers_splitOperator.matchAST(Context);
  }
};

class GaloisFunctionsAction : public PluginASTAction {
private:
  std::set<std::string> ParsedTemplates;
  Rewriter TheRewriter;

protected:
  void EndSourceFileAction() override {
    TheRewriter.getEditBuffer(TheRewriter.getSourceMgr().getMainFileID())
        .write(llvm::outs());
    if (!TheRewriter.overwriteChangedFiles()) {
      llvm::outs() << "Successfully saved changes\n";
    }
  }
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& CI,
                                                 llvm::StringRef) override {
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return llvm::make_unique<GaloisFunctionsConsumer>(CI, ParsedTemplates,
                                                      TheRewriter);
  }

  bool ParseArgs(const CompilerInstance& CI,
                 const std::vector<std::string>& args) override {
    return true;
  }
};

} // namespace

static FrontendPluginRegistry::Add<GaloisFunctionsAction>
    X("galois-analysis", "find galois function names");
