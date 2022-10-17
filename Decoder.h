#pragma once

#include <iostream>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <ranges>
#include <format>
#include "BitIO.h"
#include "Model.h"

void initializeArithmeticDecoder(std::unique_ptr<stl::BitFile>& input, USHORT& code) {
	for (int i{ 0 }; i < 16; ++i) {
		code <<= 1;
		code |= stl::inputBit(input);
	}
}

long getCurrentIndex(Symbol& s, USHORT low, USHORT high, USHORT code) {
	long range{ high - low + 1 };
	long index = (long)(((code - low) + 1) * s.scale - 1) / range;
	return index;
}

void removeSymbolFromStream(std::unique_ptr<stl::BitFile>& input, Symbol& s, USHORT& low, USHORT& high, USHORT& code) {
	long range{ (high - low) + 1 };
	high = low + (USHORT)((range * s.highCount) / s.scale - 1);
	low = low + (USHORT)((range * s.lowCount) / s.scale);
	for (;;) {
		if ((high & 0x8000) == (low & 0x8000)) {
			//do nothing
		}
		else if ((low & 0x4000) && !(high & 0x4000)) {
			code ^= 0x4000;
			high |= (1 << 14);//set bit
			low &= ~(1 << 14);//clear bit
		}
		else
			return;
		low <<= 1;
		high <<= 1;
		high |= 1;
		code <<= 1;
		code |= stl::inputBit(input);
	}
}


void expandFile(std::unique_ptr<stl::BitFile>& input, std::fstream& output, uint32_t order) {
	Symbol s;
	int c{};
	USHORT low{ 0 }, high{ 0xffff }, code{ 0 };
	long index{ 0 };
	initializeModel(order);
	initializeArithmeticDecoder(input, code);
	for (;;) {
		do {
			getSymbolScale(s);
			index = getCurrentIndex(s, low, high, code);
			c = convertSymbolToInt(index, s);
			removeSymbolFromStream(input, s, low, high, code);
		} while (c == ESCAPE);
		if (c == END_OF_STREAM)
			break;
		output.put(c);
		updateModel(c);
	}
}