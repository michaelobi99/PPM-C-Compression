#pragma once
#include <iostream>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <format>
#include <ranges>
#include "BitIO.h"
#include <string>
#include <bitset>
#include "Model.h"


void encodeSymbol(std::unique_ptr<stl::BitFile>& output, Symbol& s, USHORT& low, USHORT& high, USHORT& underflowBits) {
	unsigned long range = (high - low) + 1;
	high = low + static_cast<USHORT>((range * s.highCount) / s.scale - 1);
	low = low + static_cast<USHORT>((range * s.lowCount) / s.scale);
	/*std::cout << std::format("highcount: {},  lowcount: {}  \n", s.highCount, s.lowCount);
	std::cout << std::format("low: {},  high: {}\n", low, high);*/
	//the following loop churns out new bits until high and low are far enough apart to have stabilized
	for (;;) {
		//if their MSBs are the same
		if ((high & 0x8000) == (low & 0x8000)) {
			stl::outputBit(output, high & 0x8000);
			while (underflowBits > 0) {
				stl::outputBit(output, (~high) & 0x8000);
				underflowBits--;
			}
		}
		//if low first and second MSBs are 01 and high first and second MSBs are 10, and underflow is about to occur
		else if ((low & 0x4000) && !(high & 0x4000)) {
			underflowBits++;
			//toggle the second MSB in both low and high.
			//the shifting operation at the end of the loop will set things right
			high |= (1 << 14);
			low &= ~(1 << 14);
		}
		else {
			//std::cout << std::format("low: {},  high: {}\n", low, high);
			return;
		}
		low <<= 1;
		high <<= 1;
		high |= 1;
	}
}

void flushArithmeticEncoder(std::unique_ptr<stl::BitFile>& output, USHORT high, USHORT& underflowBits) {
	stl::outputBit(output, high & 0x8000);
	++underflowBits;
	while (underflowBits > 0) {
		stl::outputBit(output, ~high & 0x8000);
		underflowBits--;
	}
}

size_t readBytes(std::fstream& input, std::string& buffer, size_t limit) {
	buffer.clear();
	buffer.reserve(limit);
	size_t counter = 0;
	int ch{};
	while (counter < limit) {
		ch = input.get();
		if (ch == EOF)
			break;
		buffer.push_back((char)ch);
		++counter;
	}
	buffer.shrink_to_fit();
	return counter;
}

void compressFile(std::fstream& input, std::unique_ptr<stl::BitFile>& output, uint32_t order) {
	int c{};
	USHORT low{ 0 }, high{ 0xffff }, underflowBits{ 0 };
	Symbol s;
	initializeModel(order);
	size_t bufferSize = 4096;
	std::string buffer;
	size_t numBytesRead = 0;
	do {
		numBytesRead = readBytes(input, buffer, bufferSize);
		bool escaped{};
		for (char c : buffer) {
			escaped = convertIntToSymbol(c, s);
			encodeSymbol(output, s, low, high, underflowBits);
			while (escaped) {
				escaped = convertIntToSymbol(c, s);
				encodeSymbol(output, s, low, high, underflowBits);
			}
			updateModel(c);
		}
	} while (numBytesRead == bufferSize);
	convertIntToSymbol(END_OF_STREAM, s);
	flushArithmeticEncoder(output, high, underflowBits);
	stl::outputBits(output, 0L, 16);
}