#pragma once
#include <memory>
#include <vector>
#include <ranges>
#include <optional>

using USHORT = uint16_t;

struct Symbol {
	USHORT lowCount;
	USHORT highCount;
	USHORT scale;
};

#define ESCAPE 257
#define END_OF_STREAM 256
#define SYMBOL_COUNT 257
uint16_t totals[SYMBOL_COUNT + 2]; //used to ge the cumulative totals of any context
uint16_t negativeOneContextTable[SYMBOL_COUNT + 1]; //normal symbols + END_OF_STREAM symbol
char excludedCharacters[256];
const int MAX_SIZE = (1 << 14) - 1;

struct Trie {
	struct Node {
		std::vector<std::shared_ptr<Node>> children;
		std::shared_ptr<Node> vine;
		USHORT depthInTrie;
		USHORT contextCount; //keeps track of the frequency of this symbol in a particular context
		USHORT activeChildren; //min 0 , max 256
		char symbol;
		Node() : vine{ nullptr }, depthInTrie{ 0 }, contextCount{ 0 }, activeChildren{ 0 }, symbol { char() }{
			children.resize(SYMBOL_COUNT);
			std::ranges::for_each(children, [](std::shared_ptr<Node>& elem) {elem = nullptr; });
		}
	};
	Trie() : root{ std::make_shared<Node>() } {}
	std::shared_ptr<Node> root = nullptr;
	uint32_t maxDepth{ 0 };
};

Trie trie;
std::shared_ptr<Trie::Node> basePtr = trie.root;//points to the most recent node of the Trie
std::shared_ptr<Trie::Node> cursor{basePtr};


void initializeModel(uint32_t order) {
	trie.maxDepth = order + 1;
	//escapeContext = order;
	negativeOneContextTable[0] = 0;
	for (int i = 0; i < SYMBOL_COUNT; ++i) {
		negativeOneContextTable[i + 1] = negativeOneContextTable[i] + 1;
	}
}

void rescaleContextCount(std::shared_ptr<Trie::Node>& cursor) {
	for (auto i{ 0u }; i < SYMBOL_COUNT; ++i) {
		if (cursor->children[i]) {
			cursor->children[i]->contextCount /= 2;
			if (cursor->children[i]->contextCount == 0)
				cursor->children[i] = nullptr;
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
	int escapeContext{0};
	if (cursor)
		escapeContext = cursor->depthInTrie;
	if (escapeContext > 0)
		for (; escapeContext > 0; --escapeContext, cursor = cursor->vine) {
			if (cursor->activeChildren > 0) break;
		}
	else{
		s.highCount = negativeOneContextTable[c + 1];
		s.lowCount = negativeOneContextTable[c];
		s.scale = negativeOneContextTable[END_OF_STREAM + 1];
		return false;
	}
	//std::cout << (char)c << "\n";
	bool escaped{};
	getProbability();
	if (cursor->children[c] != nullptr) { //current symbol exists in context
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

void updateModel(int c) {
	//code to update Trie
	//search for parent (parent must be at a lower depth that maximum depth
	std::shared_ptr<Trie::Node> recentlyUpdatedNodePtr{ basePtr }, vineUpdater{ nullptr };
	if (recentlyUpdatedNodePtr->depthInTrie == trie.maxDepth) {
		recentlyUpdatedNodePtr = recentlyUpdatedNodePtr->vine;
	}
	if (recentlyUpdatedNodePtr->children[c]) //if not nullptr...Hence, symbol is present
		recentlyUpdatedNodePtr->children[c]->contextCount++;
	else {
		recentlyUpdatedNodePtr->children[c] = std::make_shared<Trie::Node>();
		recentlyUpdatedNodePtr->children[c]->symbol = c;
		recentlyUpdatedNodePtr->children[c]->depthInTrie = recentlyUpdatedNodePtr->depthInTrie + 1;
		recentlyUpdatedNodePtr->children[c]->contextCount++;
		if (recentlyUpdatedNodePtr->children[c]->contextCount == 255)
			rescaleContextCount(recentlyUpdatedNodePtr);
		recentlyUpdatedNodePtr->activeChildren++;
	}
	basePtr = recentlyUpdatedNodePtr->children[c];
	vineUpdater = basePtr;
	while (recentlyUpdatedNodePtr->depthInTrie > 0) {
		recentlyUpdatedNodePtr = recentlyUpdatedNodePtr->vine;
		if (recentlyUpdatedNodePtr->children[c])
			recentlyUpdatedNodePtr->children[c]->contextCount++;
		else {
			recentlyUpdatedNodePtr->children[c] = std::make_shared<Trie::Node>();
			recentlyUpdatedNodePtr->children[c]->symbol = c;
			recentlyUpdatedNodePtr->children[c]->depthInTrie = recentlyUpdatedNodePtr->depthInTrie + 1;
			recentlyUpdatedNodePtr->children[c]->contextCount++;
			if (recentlyUpdatedNodePtr->children[c]->contextCount == 255)
				rescaleContextCount(recentlyUpdatedNodePtr);
			recentlyUpdatedNodePtr->activeChildren++;
		}
		vineUpdater->vine = recentlyUpdatedNodePtr->children[c];
		vineUpdater = recentlyUpdatedNodePtr->children[c];
	}
	//at this point recentlyUpdatedNodePtr will be pointing to the root, all order 1 context symbols
	//have their vine pointig to the root
	recentlyUpdatedNodePtr->children[c]->vine = recentlyUpdatedNodePtr; 
	cursor = basePtr;
}