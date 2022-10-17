#pragma once
#include <optional>
#include "Trie.h"

struct Symbol {
	USHORT lowCount;
	USHORT highCount;
	USHORT scale;
};


uint16_t totals[SYMBOL_COUNT + 2]; //used to get the cumulative totals of any context
uint16_t negativeOneContextTable[SYMBOL_COUNT + 1];
char excludedCharacters[256];
int escapeContext = 0;


void initializeModel(uint32_t order) {
	trie.root = std::make_shared<Trie::Node>();
	basePtr = trie.root;//points to the most recent node of the Trie
	cursor = basePtr;
	trie.maxDepth = order + 1;
	negativeOneContextTable[0] = 0;
	for (int i = 0; i < SYMBOL_COUNT; ++i) {
		negativeOneContextTable[i + 1] = negativeOneContextTable[i] + 1;
	}
}

void rescaleContextCount(std::shared_ptr<Trie::Node> cursor) {
	for (auto i{ 0u }; i < SYMBOL_COUNT; ++i) {
		if (cursor->children[i]) {
			cursor->children[i]->contextCount = (cursor->children[i]->contextCount + 1 ) /2;
			if (cursor->children[i]->contextCount == 0) {}
		}
	}
}

void initializeTotalsToCurrentTable() {
	int i;
	totals[0] = 0;
	for (i = 0; i < SYMBOL_COUNT; ++i) {
		if (cursor->children[i]) {
			totals[i + 1] = totals[i] + cursor->children[i]->contextCount;
		}
		else {
			totals[i + 1] = totals[i] + 0;
		}
	}
	totals[ESCAPE + 1] = cursor->activeChildren + totals[ESCAPE];
	
}

void getProbability() {
	initializeTotalsToCurrentTable();
	if (totals[ESCAPE + 1] >= MAX_SIZE) {
		rescaleContextCount(cursor);
		initializeTotalsToCurrentTable();
	}
}

bool convertIntToSymbol(int c, Symbol& s) {
	if (escapeContext >= 0) {
		for (; cursor; --escapeContext, cursor = cursor->vine) {
			if (cursor->activeChildren > 0) break;
		}
	}
	if (!cursor) {
		s.highCount = negativeOneContextTable[c + 1];
		s.lowCount = negativeOneContextTable[c];
		s.scale = negativeOneContextTable[END_OF_STREAM + 1];
		return false;
	}
	bool escaped{};
	getProbability();
	if (cursor->children[c]) { //current symbol exists in context
		s.highCount = totals[c + 1];
		s.lowCount = totals[c];
		escaped = false;
	}
	else { //current symbol doesnt exist in context. To avoid zero probability, we use escape
		s.highCount = totals[ESCAPE + 1];
		s.lowCount = totals[ESCAPE];
		escaped = true;
		cursor = cursor->vine;
	}
	s.scale = totals[ESCAPE + 1];
	return escaped;
}

void getSymbolScale(Symbol& s) {
	while (cursor) {
		if (cursor->activeChildren > 0) break;
		cursor = cursor->vine;
	}
	if (!cursor) {
		s.scale = negativeOneContextTable[END_OF_STREAM + 1]; 
	}
	else {
		getProbability();
		s.scale = totals[ESCAPE + 1];
	}
}

int convertSymbolToInt(long index, Symbol& s) {
	int c;
	if (!cursor) {
		for (c = END_OF_STREAM; index < negativeOneContextTable[c]; --c) {}
		s.highCount = negativeOneContextTable[c + 1];
		s.lowCount = negativeOneContextTable[c];
		return c;
	}
	else {
		for (c = ESCAPE; index < totals[c]; --c) {}
		s.highCount = totals[c + 1];
		s.lowCount = totals[c];
		if (c == ESCAPE) {
			cursor = cursor->vine;
		}
		return c;
	}
}

void updateModel(int c) {
	//search for parent (parent must be at a lower depth that maximum depth
	std::shared_ptr<Trie::Node> recentlyUpdatedNodePtr{ basePtr }, vineUpdater{ nullptr };
	if (recentlyUpdatedNodePtr->depthInTrie == trie.maxDepth) {
		recentlyUpdatedNodePtr = recentlyUpdatedNodePtr->vine;
	}
	if (recentlyUpdatedNodePtr->children[c]){//if not nullptr...Hence, symbol is present
		recentlyUpdatedNodePtr->children[c]->contextCount++;
		if (recentlyUpdatedNodePtr->children[c]->contextCount == 255)
			rescaleContextCount(recentlyUpdatedNodePtr);
	}
	else {
		recentlyUpdatedNodePtr->children[c] = std::make_shared<Trie::Node>();
		recentlyUpdatedNodePtr->children[c]->symbol = c;
		recentlyUpdatedNodePtr->children[c]->depthInTrie = recentlyUpdatedNodePtr->depthInTrie + 1;
		recentlyUpdatedNodePtr->children[c]->contextCount++;
		recentlyUpdatedNodePtr->activeChildren++;
	}
	
	basePtr = recentlyUpdatedNodePtr->children[c];
	vineUpdater = basePtr;

	while (recentlyUpdatedNodePtr->depthInTrie > 0) {
		recentlyUpdatedNodePtr = recentlyUpdatedNodePtr->vine;
		if (recentlyUpdatedNodePtr->children[c]) {
			recentlyUpdatedNodePtr->children[c]->contextCount++;
			if (recentlyUpdatedNodePtr->children[c]->contextCount == 255)
				rescaleContextCount(recentlyUpdatedNodePtr);
		}
		else {
			recentlyUpdatedNodePtr->children[c] = std::make_shared<Trie::Node>();
			recentlyUpdatedNodePtr->children[c]->symbol = c;
			recentlyUpdatedNodePtr->children[c]->depthInTrie = recentlyUpdatedNodePtr->depthInTrie + 1;
			recentlyUpdatedNodePtr->children[c]->contextCount++;
			recentlyUpdatedNodePtr->activeChildren++;
		}
		

		vineUpdater->vine = recentlyUpdatedNodePtr->children[c];
		vineUpdater = recentlyUpdatedNodePtr->children[c];
	}
	//at this point recentlyUpdatedNodePtr will be pointing to the root. All order-1 context symbols
	//have their vine pointig to the root
	recentlyUpdatedNodePtr->children[c]->vine = recentlyUpdatedNodePtr; 
	cursor = basePtr;
	escapeContext = basePtr->depthInTrie;
}