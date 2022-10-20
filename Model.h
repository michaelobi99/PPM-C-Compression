#pragma once
#include <optional>
#include "Trie.h"

struct Symbol {
	USHORT lowCount;
	USHORT highCount;
	USHORT scale;
};


uint16_t totals[SYMBOL_COUNT + 2]; //used to get the cumulative totals of any context
std::vector<uint8_t> negativeOneContextTable(SYMBOL_COUNT);
std::vector<std::uint8_t> excludedCharacters(SYMBOL_COUNT);
int escapeContext = 0;


void initializeModel(uint32_t order) {
	trie.root = std::make_unique<Trie::Node>();
	basePtr = trie.root.get();//points to the most recent node of the Trie
	cursor = basePtr;
	trie.maxDepth = order + 1;
	std::memset(excludedCharacters.data(), 0, std::size(excludedCharacters));
	std::memset(negativeOneContextTable.data(), 1, std::size(negativeOneContextTable));
}

void rescaleContextCount(Trie::Node* cursor) {
	for (auto i{ 0u }; i < SYMBOL_COUNT; ++i) {
		if (cursor->children[i]) {
			cursor->children[i]->contextCount = (cursor->children[i]->contextCount + 1 ) /2;
			if (cursor->children[i]->contextCount == 0) {
				cursor->children[i] = nullptr;
			}
		}
	}
}

void initializeTotalsToCurrentTable() {
	int i;
	totals[0] = 0;
	if (cursor) {
		for (i = 0; i < SYMBOL_COUNT; ++i) {
			totals[i + 1] = totals[i] +
				(((cursor->children[i] == nullptr) || (excludedCharacters[i])) ? 0 :
				cursor->children[i]->contextCount);
		}
		totals[ESCAPE + 1] = totals[ESCAPE] + cursor->activeChildren;
	}
	else {
		for (i = 0; i < SYMBOL_COUNT; ++i) {
			totals[i + 1] = totals[i] +
				((excludedCharacters[i]) ? 0 : negativeOneContextTable[i]);
		}
		totals[ESCAPE + 1] = totals[ESCAPE] + 0;
	}
}

void getProbability() {
	initializeTotalsToCurrentTable();
	if (totals[ESCAPE + 1] >= MAX_SIZE) {
		rescaleContextCount(cursor);
		initializeTotalsToCurrentTable();
	}
}

void fillCharactersToBeExcluded() {
	for (int i = 0; i < SYMBOL_COUNT; ++i) {
		if (cursor->children[i]) {
			excludedCharacters[i] = 1;
		}
	}
}

bool convertIntToSymbol(int c, Symbol& s) {
	bool escaped{};
	if (escapeContext >= 0) {
		for (; cursor; --escapeContext, cursor = cursor->vine) {
			if (cursor->activeChildren > 0) break;
		}
	}
	if (!cursor) {//context doesn't exist
		getProbability();
		std::memset(excludedCharacters.data(), 0, std::size(excludedCharacters));
		s.highCount = totals[c + 1];
		s.lowCount = totals[c];
		escaped = false;
	}
	else if (cursor->children[c]) { //current symbol exists in context
		getProbability();
		std::memset(excludedCharacters.data(), 0, std::size(excludedCharacters));
		s.highCount = totals[c + 1];
		s.lowCount = totals[c];
		escaped = false;
	}
	else { //current symbol doesnt exist in context, but context exists. avoid zero probability with escape
		getProbability();
		fillCharactersToBeExcluded();
		s.highCount = totals[ESCAPE + 1];
		s.lowCount = totals[ESCAPE];
		cursor = cursor->vine;
		--escapeContext;
		escaped = true;
	}
	s.scale = totals[ESCAPE + 1];
	return escaped;
}

void getSymbolScale(Symbol& s) {
	while (cursor) {
		if (cursor->activeChildren > 0) break;
		cursor = cursor->vine;
	}
	getProbability();
	s.scale = totals[ESCAPE + 1];
}

int convertSymbolToInt(long index, Symbol& s) {
	int c;
	for (c = ESCAPE; index < totals[c]; --c) {}
	s.highCount = totals[c + 1];
	s.lowCount = totals[c];
	if (c == ESCAPE) {
		fillCharactersToBeExcluded();
		cursor = cursor->vine;
	}
	else {
		std::memset(excludedCharacters.data(), 0, std::size(excludedCharacters));
	}
	return c;
}


void updateModel(int c) {
	//search for parent (parent must be at a lower depth that maximum depth
	Trie::Node* recentlyUpdatedNodePtr{ basePtr };
	Trie::Node* vineUpdater{ nullptr };
	if (recentlyUpdatedNodePtr->depthInTrie == trie.maxDepth) {
		recentlyUpdatedNodePtr = recentlyUpdatedNodePtr->vine;
	}
	if (recentlyUpdatedNodePtr->children[c]){//if not nullptr...Hence, symbol is present
		recentlyUpdatedNodePtr->children[c]->contextCount++;
		if (recentlyUpdatedNodePtr->children[c]->contextCount == 255)
			rescaleContextCount(recentlyUpdatedNodePtr);
	}
	else {
		recentlyUpdatedNodePtr->children[c] = std::make_unique<Trie::Node>();
		recentlyUpdatedNodePtr->children[c]->symbol = c;
		recentlyUpdatedNodePtr->children[c]->depthInTrie = recentlyUpdatedNodePtr->depthInTrie + 1;
		recentlyUpdatedNodePtr->children[c]->contextCount++;
		recentlyUpdatedNodePtr->activeChildren++;
	}
	
	basePtr = recentlyUpdatedNodePtr->children[c].get();
	vineUpdater = basePtr;

	while (recentlyUpdatedNodePtr->depthInTrie > 0) {
		recentlyUpdatedNodePtr = recentlyUpdatedNodePtr->vine;
		if (recentlyUpdatedNodePtr->children[c]) {
			recentlyUpdatedNodePtr->children[c]->contextCount++;
			if (recentlyUpdatedNodePtr->children[c]->contextCount == 255)
				rescaleContextCount(recentlyUpdatedNodePtr);
		}
		else {
			recentlyUpdatedNodePtr->children[c] = std::make_unique<Trie::Node>();
			recentlyUpdatedNodePtr->children[c]->symbol = c;
			recentlyUpdatedNodePtr->children[c]->depthInTrie = recentlyUpdatedNodePtr->depthInTrie + 1;
			recentlyUpdatedNodePtr->children[c]->contextCount++;
			recentlyUpdatedNodePtr->activeChildren++;
		}

		vineUpdater->vine = recentlyUpdatedNodePtr->children[c].get();
		vineUpdater = recentlyUpdatedNodePtr->children[c].get();
	}
	//at this point recentlyUpdatedNodePtr will be pointing to the root. All order-1 context symbols
	//have their vine pointig to the root
	recentlyUpdatedNodePtr->children[c]->vine = recentlyUpdatedNodePtr; 
	cursor = basePtr;
	escapeContext = basePtr->depthInTrie;
}