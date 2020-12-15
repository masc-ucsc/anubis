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

/*
 * PointProduction.hxx
 *
 *  Created on: Sep 12, 2013
 *      Author: dgoik
 */

#ifndef POINTPRODUCTION_HXX_
#define POINTPRODUCTION_HXX_

#include "Production.h"

class PointProduction : public AbstractProduction {

private:
  const int interfaceSize;
  const int leafSize;
  const int a1Size;
  const int anSize;
  const int offset;
  const int a1Offset;
  const int anOffset;

  void generateGraph();

  virtual void recursiveGraphGeneration(int low_range, int high_range,
                                        GraphNode bsSrcNode,
                                        GraphNode mergingDstNode,
                                        Vertex* parent);

  virtual GraphNode addNode(int incomingEdges, int outgoingEdges,
                            int leafNumber, EProduction production,
                            GraphNode src, GraphNode dst, Vertex* v,
                            EquationSystem* system);

  std::vector<EquationSystem*>*
  preprocess(std::vector<EquationSystem*>* input) const;

  /* these methods may be useful in order to parallelize {pre,post}processing */
  EquationSystem* preprocessA1(EquationSystem* input) const;
  EquationSystem* preprocessA(EquationSystem* input) const;
  EquationSystem* preprocessAN(EquationSystem* input) const;

  void postprocessA1(Vertex* leaf, EquationSystem* inputData,
                     std::vector<double>* result, int num) const;
  void postprocessA(Vertex* leaf, EquationSystem* inputData,
                    std::vector<double>* result, int num) const;
  void postprocessAN(Vertex* leaf, EquationSystem* inputData,
                     std::vector<double>* result, int num) const;

  // iterate over leafs and return them in correct order
  std::vector<Vertex*>* collectLeafs(Vertex* p);

public:
  PointProduction(std::vector<int>* productionParameters,
                  std::vector<EquationSystem*>* inputData)
      : AbstractProduction(productionParameters, inputData),
        interfaceSize((*productionParameters)[0]),
        leafSize((*productionParameters)[1]),
        a1Size((*productionParameters)[2]), anSize((*productionParameters)[3]),
        offset((*productionParameters)[1] - 2 * (*productionParameters)[0]),
        a1Offset((*productionParameters)[2] - (*productionParameters)[1]),
        anOffset((*productionParameters)[3] - (*productionParameters)[1]) {
    this->inputData = preprocess(inputData);
    generateGraph();
  };

  /* pull data from leafs and return the solution of MES
         this is sequential code*/
  virtual std::vector<double>* getResult();

  virtual void Execute(EProduction productionToExecute, Vertex* v,
                       EquationSystem* input);
  virtual Vertex* getRootVertex();
  virtual Graph* getGraph();
  void A1(Vertex* v, EquationSystem* inData) const;
  void A(Vertex* v, EquationSystem* inData) const;
  void AN(Vertex* v, EquationSystem* inData) const;
  void A2Node(Vertex* v) const;
  void A2Root(Vertex* v) const;
  void BS(Vertex* v) const;

  int getInterfaceSize() const;
  int getLeafSize() const;
  int getA1Size() const;
  int getANSize() const;
};

#endif /* POINTPRODUCTION_HXX_ */
