#include <filesystem>
#include <chrono>
#include <format>
#include "Decoder.h"
#include "Encoder.h"

namespace fs = std::filesystem;

struct Timer {
public:
	Timer() = default;
	void Start() {
		start = std::chrono::high_resolution_clock::now();
	}
	void Stop() {
		stop = std::chrono::high_resolution_clock::now();
	}
	float time() {
		elapsedTime = std::chrono::duration<float>(stop - start).count();
		return elapsedTime;
	}
private:
	float elapsedTime{};
	std::chrono::time_point<std::chrono::high_resolution_clock> start, stop;
};


uintmax_t fileSize(fs::path const& path) {
	auto lengthInbytes = fs::file_size(path);
	return lengthInbytes;
}


int main() {
	auto timer = Timer();
	
	try {
		std::fstream input(R"(..\PredictionWithPartialMatching(PPM)\testFile.txt)", std::ios_base::in | std::ios_base::binary);
		if (!input.is_open())
			exit(1);
		uint32_t order{ 3 };
		std::cout << "Input the context order: ";
		std::cin >> order;

		auto output = stl::OpenOutputBitFile(R"(..\PredictionWithPartialMatching(PPM)\testFile2.txt)");
		std::cout << "compression started....\n";
		timer.Start();
		compressFile(input, output, order);
		timer.Stop();
		printf("\nFile compression complete\n");
		printf("PPMC coding compression time = %f seconds\n\n", timer.time());
		stl::closeOutputBitFile(output);

		auto input1 = stl::OpenInputBitFile(R"(..\PredictionWithPartialMatching(PPM)\testFile2.txt)");
		std::fstream output1(R"(..\PredictionWithPartialMatching(PPM)\testFile3.txt)", std::ios_base::out | std::ios_base::binary);
		printf("Expansion started....\n");
		timer.Start();
		expandFile(input1, output1, order);
		timer.Stop();
		printf("\nFile expansion complete\n");
		printf("PPMC coding expansion time = %f seconds\n\n", timer.time());
		stl::closeInputBitFile(input1);
		output1.close();

		//print file sizes
		std::cout << std::format("Original file size = {}bytes\n", fileSize(fs::path(R"(..\PredictionWithPartialMatching(PPM)\testFile.txt)")));
		std::cout << std::format("Compressed file size = {} bytes\n", fileSize(fs::path(R"(..\PredictionWithPartialMatching(PPM)\testFile2.txt)")));
		std::cout << std::format("Expanded file size = {} bytes\n", fileSize(fs::path(R"(..\PredictionWithPartialMatching(PPM)\testFile3.txt)")));
	}
	catch (stl::FileError const& error) {
		std::cout << error.what();
		std::cout << "File compression failed\n";
	}
	catch (...) {
		std::cout << "An error occurred during compression or expansion\n";
	}
}