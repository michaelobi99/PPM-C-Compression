#pragma once
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <bitset>

namespace stl {
	struct BitFile {
		std::fstream file;
		std::uint32_t rack;
		std::uint32_t mask;
	};

	struct FileError : std::exception {
		FileError(std::string error) : errorMessage{ std::move(error) } {}
		virtual const char* what() const noexcept override {
			return errorMessage.c_str();
		}
	private:
		std::string errorMessage;
	};

	std::unique_ptr<BitFile> OpenOutputBitFile(std::string const& name) {
		auto bitFile = std::make_unique<BitFile>();
		bitFile->file.open(name, std::ios_base::out | std::ios_base::binary);
		if (!bitFile->file.is_open())
			exit(1);
		bitFile->rack = 0;
		bitFile->mask = 0x80;
		return bitFile;
	}
	std::unique_ptr<BitFile> OpenInputBitFile(std::string const& name) {
		auto bitFile = std::make_unique<BitFile>();
		bitFile->file.open(name, std::ios_base::in | std::ios_base::binary);
		bitFile->rack = 0;
		bitFile->mask = 0x80;
		return bitFile;
	}
	void closeOutputBitFile(std::unique_ptr<BitFile>& bitFile) {
		if (bitFile->mask != 0x80) {
			if (!bitFile->file.put(bitFile->rack)) {
				throw FileError("ERROR: An error occurred in closeOutputBitFile\n");
			}
		}
		bitFile->file.close();
	}
	void closeInputBitFile(std::unique_ptr<BitFile>& bitFile) {
		bitFile->file.close();
	}

	void outputBit(std::unique_ptr<BitFile>& bitFile, int bit = 0) {
		static int counter = 0;
		if (bit)
			bitFile->rack |= bitFile->mask;
		bitFile->mask >>= 1;
		if (bitFile->mask == 0) {
			++counter;
			if (!bitFile->file.put(bitFile->rack)) {
				throw FileError("ERROR: An error occurred in outputBits\n");
			}
			bitFile->rack = 0;
			bitFile->mask = 0x80;
		}
	}

	void outputBits(std::unique_ptr<BitFile>& bitFile, std::uint32_t code, std::uint32_t bitCount) {
		std::uint32_t mask;
		mask = 1 << (bitCount - 1);
		while (mask != 0) {
			if (mask & code)
				bitFile->rack |= bitFile->mask;
			bitFile->mask >>= 1;
			if (bitFile->mask == 0) {
				if (!bitFile->file.put(bitFile->rack)) {
					throw FileError("ERROR: An error occurred in outputBits\n");
				}
				bitFile->rack = 0;
				bitFile->mask = 0x80;
			}
			mask >>= 1;
		}
	}
	int inputBit(std::unique_ptr<BitFile>& bitFile) {
		int value{};
		char ch{};
		if (bitFile->mask == 0x80) {
			bitFile->file.get(ch);
			bitFile->rack = ch;
		}
		value = bitFile->rack & bitFile->mask;
		bitFile->mask >>= 1;
		if (bitFile->mask == 0)
			bitFile->mask = 0x80;
		return value ? 1 : 0;
	}
	int inputBits(std::unique_ptr<BitFile>& bitFile, int bitCount) {
		unsigned long mask, returnValue{ 0 };
		char ch{};
		mask = 1L << (bitCount - 1);
		while (mask != 0) {
			if (bitFile->mask == 0x80) {
				bitFile->file.get(ch);
				bitFile->rack = ch;
				if (bitFile->rack == EOF)
					throw FileError("An error occurre in inputBits\n");
			}
			if (bitFile->rack & bitFile->mask)
				returnValue |= mask;
			mask >>= 1;
			bitFile->mask >>= 1;
			if (bitFile->mask == 00)
				bitFile->mask = 0x80;
		}
		return returnValue;
	}
}