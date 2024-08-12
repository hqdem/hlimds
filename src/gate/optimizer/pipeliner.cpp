//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "pipeliner.h"

#include <iostream>

namespace eda::gate::optimizer {

Pipeliner::SubnetMarkup Pipeliner::markCascades(
		const SubnetBuilderPtr &builder) const {

	PipeliningState state;
	const size_t builderMaxIdx = builder->getMaxIdx();
	state.layerBounds.resize(builderMaxIdx + 1, {size_t(-1), size_t(-1)});
	state.fanouts.resize(builderMaxIdx + 1);
	initPipeliningState(builder, state);
	divideIntoLayers(builder, state);
	return divideIntoCascades(builder, state);
}

size_t Pipeliner::findSubnetDepth(const SubnetBuilderPtr &builder) const {
	size_t depth = 0;
	for (auto it = builder->rbegin(); it != builder->rend(); ++it) {
		const size_t entryID = *it;
		if (!builder->getCell(entryID).isOut()) {
			break;
		}
		depth = std::max(depth, builder->getDepth(entryID));
	}
	return depth;
}

void Pipeliner::initPipeliningState(
		const SubnetBuilderPtr &builder,
		PipeliningState &state) const {

	const size_t subnetDepth = findSubnetDepth(builder);
	for (auto it = builder->rbegin(); it != builder->rend(); ++it) {
		const size_t entryID = *it;
		// if (builder->isLink(entryID)) { //TODO: fixme
		// 	continue;
		// }

		// Update current cell layer bounds
		updateLeftLayerBound(builder, state, entryID);
		updateRightLayerBound(builder, state, entryID, (size_t)-1, subnetDepth);

		// Update links layer bounds
		const auto &links = builder->getLinks(entryID);
		for (const auto &link : links) {
			updateRightLayerBound(builder, state, link.idx, entryID, subnetDepth);
		}

		// Add links fanouts
		updateLinksFanouts(builder, state, entryID);

		// Add cell delay in the set
		state.delayCellSet.insert({findDelay(builder, entryID), entryID});

		// Update layer delays
		const auto &layerBounds = state.layerBounds;
		const size_t cellLayerLeftBound = layerBounds[entryID].first,
								 cellLayerRightBound = layerBounds[entryID].second;
		if (cellLayerLeftBound == cellLayerRightBound) {
			updateLayerDelay(builder, state, cellLayerLeftBound, entryID);
		}
	}
}

void Pipeliner::divideIntoLayers(
		const SubnetBuilderPtr &builder,
		PipeliningState &state) const {

	auto &delayCellSet = state.delayCellSet;
	auto &layerBounds = state.layerBounds;
  while(!delayCellSet.empty()) {
		const auto delayCell = *(--delayCellSet.end());
		const size_t entryID = delayCell.second;
		size_t &cellLayerLeftBound = layerBounds[entryID].first,
					 &cellLayerRightBound = layerBounds[entryID].second;
		if (cellLayerLeftBound == cellLayerRightBound) {
			updateLayerDelay(builder, state, cellLayerLeftBound, entryID);
			delayCellSet.erase(--delayCellSet.end());
			continue;
		}
		limitLayerBounds(builder, state, entryID);
		updateLayerDelay(builder, state, cellLayerLeftBound, entryID);
		delayCellSet.erase(--delayCellSet.end());
	}

	// Computing the sum of layer delays
	for (const auto &delay : state.layerDelay) {
		state.layerDelaySum += delay;
	}
}

void Pipeliner::updateLeftLayerBound(
		const SubnetBuilderPtr &builder,
		PipeliningState &state,
		const size_t entryID) const {

	state.layerBounds[entryID].first = builder->getDepth(entryID);
}

void Pipeliner::updateRightLayerBound(
		const SubnetBuilderPtr &builder,
		PipeliningState &state,
		const size_t entryID,
		const size_t parEntryID,
		const size_t subnetDepth) const {

	auto &layerBounds = state.layerBounds;
	if (parEntryID == (size_t)-1) {
		if (layerBounds[entryID].second == (size_t)-1) {
			layerBounds[entryID].second = subnetDepth;
		}
		return;
	}

	layerBounds[entryID].second = std::min(
			layerBounds[entryID].second,
			layerBounds[parEntryID].second - 1
	);
}

void Pipeliner::updateLayerDelay(
		const SubnetBuilderPtr &builder,
		PipeliningState &state,
		const size_t layerN,
		const size_t entryID) const {

	auto &layerDelay = state.layerDelay;

	const auto &cell = builder->getCell(entryID);
	float delay;
	if (cell.isIn() || cell.isOut() || cell.isZero() || cell.isOne()) {
		delay = 0.f;
	} else {
		const auto &cellType = cell.getType();
		assert(cellType.hasAttr());
		delay = cellType.getAttr().props.delay;
	}
	if (layerDelay.size() <= layerN) {
		layerDelay.resize(layerN + 1);
	}
	layerDelay[layerN] = std::max(layerDelay[layerN], delay);
}

void Pipeliner::limitLayerBounds(
		const SubnetBuilderPtr &builder,
		PipeliningState &state,
		const size_t entryID) const {

	auto &layerBounds = state.layerBounds;
	const auto &fanouts = state.fanouts;
	std::stack<size_t> toLimLayers;

	toLimLayers.push(entryID);
	while(!toLimLayers.empty()) {
		const size_t curEntryID = toLimLayers.top();
		toLimLayers.pop();
		size_t &layerLeftBound = layerBounds[curEntryID].first;
		size_t &layerRightBound = layerBounds[curEntryID].second;
		layerLeftBound = layerRightBound =
				findMaxDelayLayer(state, layerLeftBound, layerRightBound);

		for (const auto &fanoutIdx : fanouts[curEntryID]) {
			size_t &fanoutLayerLeftBound = layerBounds[fanoutIdx].first;
			const size_t fanoutLayerRightBound = layerBounds[fanoutIdx].second;
			if (fanoutLayerLeftBound == fanoutLayerRightBound) {
				continue;
			}
			fanoutLayerLeftBound = std::max(fanoutLayerLeftBound, layerLeftBound + 1);
			if (fanoutLayerLeftBound == fanoutLayerRightBound) {
				toLimLayers.push(fanoutIdx);
			}
		}
		for (const auto &link : builder->getLinks(curEntryID)) {
			const size_t linkIdx = link.idx;
			const size_t linkLayerLeftBound = layerBounds[linkIdx].first;
			size_t &linkLayerRightBound = layerBounds[linkIdx].second;
			if (linkLayerLeftBound == linkLayerRightBound) {
				continue;
			}
			linkLayerRightBound = std::min(linkLayerRightBound, layerLeftBound - 1);
			if (linkLayerLeftBound == linkLayerRightBound) {
				toLimLayers.push(linkIdx);
			}
		}
	}
}

size_t Pipeliner::findMaxDelayLayer(
		const PipeliningState &state,
		const size_t leftLayer,
		const size_t rightLayer) const {

	size_t maxDelayLayer = leftLayer;
	float maxDelay = state.layerDelay[leftLayer];
	for (size_t i = leftLayer + 1; i <= rightLayer; ++i) {
		const float curDelay = state.layerDelay[i];
		if (curDelay > maxDelay) {
			maxDelayLayer = i;
			maxDelay = curDelay;
		}
	}
	return maxDelayLayer;
}

float Pipeliner::findDelay(
		const SubnetBuilderPtr &builder,
		const size_t entryID) const {

	const auto &cell = builder->getCell(entryID);
	if (cell.isIn() || cell.isOut() || cell.isOne() || cell.isZero()) {
		return 0.f;
	}
	const auto &cellType = cell.getType();
	assert(cellType.hasAttr());
	return cellType.getAttr().props.delay;
}

void Pipeliner::updateLinksFanouts(
		const SubnetBuilderPtr &builder,
		PipeliningState &state,
		const size_t parEntryID) const {
	for (const auto &link : builder->getLinks(parEntryID)) {
		state.fanouts[link.idx].push_back(parEntryID);
	}
}

void Pipeliner::markLayers(
	const PipeliningState &state,
	std::vector<size_t> &cascadePathTriggers,
	std::vector<size_t> &layerCascade) const {

	float delaySum = state.layerDelaySum;
	size_t cascadesN = k;
	float delayLowerBound;
	float curDelay = 0.f, prevDelay = 0.f;
	size_t curCascade = 0;
	for (size_t i = 0; i < state.layerDelay.size(); ++i) {
		layerCascade[i] = curCascade;
		if (i == state.layerDelay.size() - 1 && cascadesN > 1) {
				cascadePathTriggers.push_back(k - 1);
				++curCascade;
		}
		if (cascadesN == 1) {
			layerCascade[i] = curCascade;
			break;
		}

		delayLowerBound = delaySum / static_cast<float>(cascadesN);
		const float delay = state.layerDelay[i];
		prevDelay = curDelay;
		curDelay += delay;
		if (std::fabs(delayLowerBound - curDelay) < floatEps ||
				curDelay - delayLowerBound > floatEps) {
			++curCascade;
			if (!cascadePathTriggers.empty()) {
				cascadePathTriggers.push_back(1 + cascadePathTriggers.back());
			} else {
				cascadePathTriggers.push_back(1);
			}
			if ((curDelay - delayLowerBound) -
					(delayLowerBound - prevDelay) > floatEps) {
				layerCascade[i] = curCascade;
				curDelay = delay;
			} else {
				prevDelay = curDelay;
				curDelay = 0.f;
			}
			cascadesN--;
			delaySum -= prevDelay;
		}
	}
}

Pipeliner::SubnetMarkup Pipeliner::markLinks(
		const SubnetBuilderPtr &builder,
		const PipeliningState &state,
		const std::vector<size_t> &cascadePathTriggers,
		const std::vector<size_t> &layerCascade) const {

	SubnetMarkup subnetMarkup;
	auto &markedLinks = subnetMarkup.markedLinks;
	markedLinks.resize(builder->getMaxIdx() + 1);
	std::vector<size_t> entryPathTriggers(builder->getMaxIdx() + 1, 0);

	for (const auto &entryID : *builder) {
		const auto &curCell = builder->getCell(entryID);
		const size_t entryLayer = state.layerBounds[entryID].first;
		const size_t curCascade = layerCascade[entryLayer];
		const auto &entryLinks = builder->getLinks(entryID);

		for (size_t i = 0; i < entryLinks.size(); ++i) {
			const size_t linkIdx = entryLinks[i].idx;
			const size_t linkLayer = state.layerBounds[linkIdx].first;
			const size_t linkCascade = layerCascade[linkLayer];
			entryPathTriggers[entryID] =
					std::max(entryPathTriggers[entryID], entryPathTriggers[linkIdx]);

			if ((curCell.isOut() && k - 1 > entryPathTriggers[linkIdx]) ||
					curCascade > linkCascade) {
				if (markedLinks[entryID].size() <= entryLinks.size()) {
					markedLinks[entryID].resize(entryLinks.size(), 0);
				}
				if (curCell.isOut()) {
					markedLinks[entryID][i] = k - 1 - entryPathTriggers[linkIdx];
					entryPathTriggers[entryID] = k - 1;
					continue;
				}
				markedLinks[entryID][i] =
						cascadePathTriggers[curCascade - 1] - entryPathTriggers[linkIdx];
				entryPathTriggers[entryID] = cascadePathTriggers[curCascade - 1];
			}
		}
	}
	return subnetMarkup;
}

Pipeliner::SubnetMarkup Pipeliner::divideIntoCascades(
		const SubnetBuilderPtr &builder,
		const PipeliningState &state) const {

	std::vector<size_t> cascadePathTriggers;
	std::vector<size_t> layerCascade(state.layerDelay.size());

	markLayers(state, cascadePathTriggers, layerCascade);

	return markLinks(builder, state, cascadePathTriggers, layerCascade);
}

} // namespace eda::gate::optimizer
