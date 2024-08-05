//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "associative_reordering.h"
#include "gate/analyzer/probabilistic_estimate.h"
#include "gate/model/subnetview.h"

#include <algorithm>
#include <functional>
#include <map>
#include <set>
#include <stack>

namespace eda::gate::optimizer::synthesis {

using Subnet = eda::gate::model::Subnet;
using SubnetID = eda::gate::model::SubnetID;
using SubnetBuilder = eda::gate::model::SubnetBuilder;
using Link = Subnet::Link;
using LinkList = Subnet::LinkList;
using Cell = Subnet::Cell;
using CellSymbol = eda::gate::model::CellSymbol;
using Effect = SubnetBuilder::Effect;
using Estimator = eda::gate::analyzer::ProbabilityEstimator;
using InOutMapping = eda::gate::model::InOutMapping;
using CellTypeID = eda::gate::model::CellTypeID;
using SubnetView = eda::gate::model::SubnetView;
using SubnetViewWalker = eda::gate::model::SubnetViewWalker;

bool AssociativeReordering::isAssociative(const SubnetBuilder &builder) const {

  size_t last = *builder.end().prev();
  size_t depth = builder.getDepth(last);
  return (depth > 2);
}

bool AssociativeReordering::isOpen(SubnetView &view) const {
  
  SubnetViewWalker walker(view);
  bool open = false;
  auto func = [&open](SubnetBuilder &builder,
                      const bool isIn,
                      const bool isOut,
                      const size_t entryID) ->bool { 
      
      if (builder.getCell(entryID).refcount > 1) {
        open = true;
        return false;
      } 
      return true;
  };

  walker.run(func);
  return open;
}

void AssociativeReordering::combination(std::vector<int> &permutation,
                                        float &maxEffect,
                                        std::vector<size_t> &inEl,
                                        size_t value,
                                        std::map<float, size_t> &pos, 
                                        FragmentInfo &info) const {  

  size_t width = 1 << (info.depth - 1);
  size_t inputNum = info.builder->getInNum();
  
  if (value < inputNum) {    
    float curWght = info.weights[value];
    size_t j = 0;
    auto elmnt = pos.find(curWght);
    
    if (elmnt != pos.end()) {
      j = elmnt->second;
    }
    for (size_t i = j; i < width; ++i) {
      
      if (inEl[i] < info.arity) {
        
        permutation[i * info.arity + inEl[i]] = value;
        inEl[i]++;
        pos[curWght] = i;
        combination(permutation, maxEffect, inEl, value + 1, pos, info); 
        inEl[i] -= 1;
        permutation[i * info.arity + inEl[i]] = -1; 

        if (inEl[i] == 0) {
          break;
        }
      }
    }
    pos[curWght] = j;   
  } else {
    float newEffect = getEffect(info, permutation);

    if (((newEffect - epsilon) > epsilon) && 
        ((newEffect - maxEffect) > epsilon)) {
      maxEffect = newEffect;
      info.goodPermutation = permutation;
    }  
  }
}

std::vector<std::set<int>> AssociativeReordering::createSet(
      const std::vector<int> &vec,
      const size_t arity) const {
  std::vector<std::set<int>> vecSet;
  size_t i = 0;

  while (i < vec.size()) {
    std::set<int> s;
    
    for (size_t j = 0; j < arity; ++j) {
      s.insert(vec[i + j]);
    }
    vecSet.push_back(s);
    i += arity;
  }
  return vecSet;
}

float AssociativeReordering::getEffect(
      FragmentInfo &info,
      const std::vector<int> &permutation) const {
  
  std::vector<std::set<int>> permut = createSet(permutation, info.arity); 
  auto last = info.builder->end().prev();
  auto prelast = *(last.prev());
  
  const CellSymbol symbol = info.builder->getCell(prelast).getSymbol();
  size_t numInputs = info.builder->getInNum();
  std::shared_ptr<SubnetBuilder> newBuilder = createBuilder(numInputs, 
                                                            info.depth, 
                                                            info.arity, 
                                                            permut, 
                                                            symbol);

  InOutMapping simpleMap;
  simpleMap.inputs.resize(info.builder->getInNum());
  
  for (size_t i = 0; i < info.builder->getInNum(); ++i) {
    simpleMap.inputs[i] = i;
  }
  simpleMap.outputs.push_back(prelast);
  setWeights(*newBuilder, *info.builder);

  std::function<float(float)> cellWeightModifier = 
      [](float w) -> float {return w * (1 - w) * 2;};
  Effect effect = info.builder->evaluateReplace(*newBuilder, 
                                                simpleMap, 
                                                &cellWeightModifier);
  return effect.weight;
}

std::shared_ptr<SubnetBuilder> AssociativeReordering::createBuilder(
    const size_t numInputs,
    size_t depth,
    const size_t arity,
    std::vector<std::set<int>> &permut, 
    const CellSymbol cellSymbol, 
    const std::set<size_t> &negInputs) const {
  
  std::shared_ptr<SubnetBuilder> newBuilder = std::make_shared<SubnetBuilder>();
  LinkList inputs = newBuilder->addInputs(numInputs);
  
  for (auto el : negInputs) {
    inputs[el] = ~inputs[el];
  }
  
  while (depth > 1) {
    std::vector<std::set<int>> newPermut;
    LinkList newInputs;
    size_t i = 0;

    while (i < permut.size()) {
      std::set<int> newSet;
      
      for (size_t j = 0; j < arity; ++j) {
        std::set<int> s = permut[i];
        size_t idx = -1;
        
        LinkList ll;
        
        for (auto ID : s) {
          if (ID != -1) {
            auto pred = [ID](Link l) { return l.idx == ID; };
            auto l = std::find_if(inputs.begin(), inputs.end(), pred);
            ll.push_back(*l); 
          } 
        }
        
        if (ll.size() == 1) {
          newInputs.push_back(ll[0]); 
          idx = (ll[0]).idx;
        } 
        
        if (ll.size() > 1) {
          Link newLink = newBuilder->addCell(cellSymbol, ll);
          newInputs.push_back(newLink);
          idx = newLink.idx;
        }
        
        ++i;
        newSet.insert(idx);
      }

      newPermut.push_back(newSet);
    }

    inputs = newInputs;
    permut = newPermut;

    depth--;
  }

  Link l = inputs.size() > 1 ? 
      newBuilder->addCell(cellSymbol, inputs) : inputs[0];
  
  newBuilder->addOutput(l);

  return newBuilder;
}

std::shared_ptr<SubnetBuilder> AssociativeReordering::makeBuilder(
    SubnetView &view, 
    const std::set<size_t> &negInpts) const {
  
  std::shared_ptr<SubnetBuilder> newBuilder = 
      std::make_shared<SubnetBuilder>(view.getSubnet().make());
  
  const auto rootCell = newBuilder->end().prev().prev();
  CellTypeID typeId = newBuilder->getCell(*rootCell).getTypeID();
  auto curCell = rootCell;
  
  for (; curCell != newBuilder->begin(); --curCell) {
    
    size_t numCell = *curCell;
    bool replace = false;
    LinkList links = newBuilder->getLinks(numCell);
    LinkList replaceLinks;
    
    for (auto l : links) {      
      if ((l.inv && !negInpts.count(l.idx)) ||
          (!l.inv && negInpts.count(l.idx))) {
        
        replace = true;
        replaceLinks.push_back(~l);
      } else {
        replaceLinks.push_back(l);
         
        if (newBuilder->getCell(numCell).getTypeID() != 
            newBuilder->getCell(*rootCell).getTypeID()) {
          replace = true;
        }
      }
    }
    
    if (replace) {
      newBuilder->replaceCell(numCell, typeId, replaceLinks);
    }
  }
  return newBuilder;
}

void AssociativeReordering::dfsBuilder(const SubnetBuilder &builder, 
                                       size_t start, 
                                       std::vector<size_t> &mapInputs, 
                                       std::set<size_t> &negLinks) const {

  std::stack<size_t> st;
  st.push(start);
  bool rootAnd = builder.getCell(start).isAnd();
  bool rootOr = builder.getCell(start).isOr();
  bool rootXor = builder.getCell(start).isXor();

  while (!st.empty()) {

    size_t curNum = st.top(); 
    st.pop();
    LinkList links = builder.getLinks(curNum);

    for (auto link : links) {
      
      const Cell prev = builder.getCell(link.idx);
      LinkList beforePrev = builder.getLinks(link.idx);
      bool neg = (link.inv && !negLinks.count(link.idx)) || 
          (!link.inv && negLinks.count(link.idx));
      
      if (neg) {
        if ((prev.isAnd() && rootOr) ||
            (prev.isOr() && rootAnd)) {
          
          negLinks.erase(link.idx);
          st.push(link.idx);
          
          for (auto l : beforePrev) {
            negLinks.insert(l.idx);
          }
        } else if ((prev.isXor() && rootXor)) {

          negLinks.erase(link.idx);
          negLinks.insert(beforePrev[0].idx);
          st.push(link.idx);
        } else {

          negLinks.insert(link.idx);
          mapInputs.push_back(link.idx);
        }
      
      } else {

        negLinks.erase(link.idx);
        if ((prev.isAnd() && rootAnd) ||
            (prev.isOr() && rootOr) ||
            (prev.isXor() && rootXor)) {

          st.push(link.idx);
        } else {
          mapInputs.push_back(link.idx);
        }
      }
    }
  }
}

void AssociativeReordering::setWeights(SubnetBuilder &newBuilder, 
                                       SubnetBuilder &parent) const {
  
  std::vector<float> inpProb;
  
  for (auto iter = parent.begin(); parent.getCell(*iter).isIn(); ++iter) {
    
    float weight = parent.getWeight(*iter);
    inpProb.push_back(weight);
  }

  Estimator probEst;
  std::vector<float> prob = probEst.estimateProbs(newBuilder, inpProb);
  
  for (auto iter = newBuilder.begin(); iter != newBuilder.end(); ++iter) {
    newBuilder.setWeight(*iter, prob[*iter]);
  }
}

void AssociativeReordering::setWeights(SubnetView &view, 
                                       SubnetBuilder &newBuilder, 
                                       const std::set<size_t> &negLinks) const {
  
  std::vector<float> inpProb;
  
  for (auto iter = view.getParent().begin(); 
      view.getParent().getCell(*iter).isIn(); ++iter) {
    
    float weight = view.getParent().getWeight(*iter);
    inpProb.push_back(weight);
  }

  Estimator probEst;
  std::vector<float> prob = probEst.estimateProbs(view.getParent(), inpProb);

  for (auto iter = newBuilder.begin(); 
      newBuilder.getCell(*iter).isIn(); ++iter) {
    
    size_t posInPrnt = view.getInputs()[*iter];
    float p = prob[posInPrnt];
    p = (negLinks.count(posInPrnt)) ? 1 - p : p;
    newBuilder.setWeight(*iter, p);
  }
  setWeights(newBuilder, newBuilder);
}

model::SubnetObject AssociativeReordering::synthesize(
    const SubnetBuilder &builder, 
    uint16_t maxArity) const {                                             

  auto last = (builder.end().prev()); 
  auto curCell = last.prev(); 

  float max = 0.0;
  std::shared_ptr<SubnetBuilder> rhs = std::make_shared<SubnetBuilder>();
  InOutMapping goodRhsToLhs;
  
  for (; builder.getDepth(*curCell) > 1; --curCell) {
  
    size_t curNum = *curCell; 
    std::vector<size_t> mapInp;
    std::set<size_t> negLinks;
    
    dfsBuilder(builder, curNum, mapInp, negLinks);
    model::InOutMapping map(mapInp, {curNum});
    SubnetView view(builder, map);
    
    if (isOpen(view)) {
      continue;
    }
    std::shared_ptr<SubnetBuilder> builderPtr = makeBuilder(view);
    
    if (!isAssociative(*builderPtr)) {
      continue;
    }
    setWeights(view, *builderPtr, negLinks);
    std::set<size_t> negInputs;
    
    for (size_t i = 0; i < mapInp.size(); ++i) {
      if (negLinks.count(mapInp[i])) {
        negInputs.insert(i);
      }
    }

    std::vector<float> weights(builderPtr->getInNum());
    for (auto iter = builderPtr->begin(); 
        builderPtr->getCell(*iter).isIn(); ++iter) {
      
      weights[*iter] = builderPtr->getWeight(*iter);
    }

    const auto frLast = builderPtr->end().prev();
    auto  frPreLast = frLast.prev();

    size_t depth = builderPtr->getDepth(*frPreLast);
    size_t width = 1 << (depth - 1);
    std::vector<int> permut (maxArity * width, -1);
    std::vector<size_t> inEl (width, 0);
    float maxEffect = 0.0;
    
    FragmentInfo info(builderPtr, weights, depth, maxArity);
    std::map<float, size_t> wMap;
    combination(permut, maxEffect, inEl, 0, wMap, info);

    if ((maxEffect - epsilon) > max) {

      max = maxEffect;
      const CellSymbol symbol = builderPtr->getCell(*frPreLast).getSymbol();
      std::vector<std::set<int>> goodPermut = createSet(info.goodPermutation, 
                                                        maxArity);
      goodRhsToLhs = map;
      rhs = createBuilder(mapInp.size(), 
                          depth, 
                          maxArity, 
                          goodPermut, 
                          symbol, 
                          negInputs);
    }
  }
 
  if (std::fabs(max) <= epsilon) {
    return model::SubnetObject{model::OBJ_NULL_ID};
  }

  SubnetView copyView(builder);
  copyView.getSubnet().builder().replace(*rhs, goodRhsToLhs);
  return model::SubnetObject(copyView.getSubnet().builder().make());
}
} // namespace eda::gate::optimizer::synthesis
