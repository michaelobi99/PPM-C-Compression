#pragma once
#include <vector>
#include <ranges>
#include <memory>

#define ESCAPE 257
#define END_OF_STREAM 256
#define SYMBOL_COUNT 257 //ascii 256 symbols + EOF symbol
#define MAX_SIZE  ((1 << 14) - (1))

using USHORT = std::uint16_t;


struct Trie {
	struct Node {
		std::vector<std::shared_ptr<Node>> children;
		std::shared_ptr<Node> vine;
		USHORT depthInTrie;
		USHORT contextCount; //keeps track of the frequency of this symbol in a particular context
		USHORT activeChildren; //min 0 , max 256
		char symbol;
		Node() : vine{ nullptr }, depthInTrie{ 0 }, contextCount{ 0 }, activeChildren{ 0 }, symbol{ char() } {
			children.resize(SYMBOL_COUNT);
		}
	};
	Trie() = default;
	std::shared_ptr<Node> root;
	uint32_t maxDepth{ 0 };
};
Trie trie;
std::shared_ptr<Trie::Node> basePtr;//points to the most recent node of the Trie
std::shared_ptr<Trie::Node> cursor;