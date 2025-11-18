#include<iostream>
#include <cstdint>
#include <cstring>
#include <vector>
#include "../build/include/Platform.h"
// #include "Hashlib.h"
//LE or BE. It matters. 
#include <bitset>

bool ExtractBitsMinhash(const uint8_t *src, size_t startBit, size_t bitLen, uint8_t *dst){
    // Allocate destination buffer
	memset(dst, 0, (bitLen + 7) / 8); // Clear destination buffer.

	for (size_t i = 0; i < bitLen; ++i) {
		size_t srcBitIndex  = startBit + i;
		size_t srcByteIndex = srcBitIndex / 8;
		size_t srcBitOffset = 7 - (srcBitIndex % 8);  // // LSB-first (0 = rightmost bit)
		printf("srcBitIndex: %zu, srcByteIndex: %zu, srcBitOffset: %zu\n", srcBitIndex, srcByteIndex, srcBitOffset);

		uint8_t bit = (src[srcByteIndex] >> srcBitOffset) & 1;

		size_t dstByteIndex = i / 8;
		size_t dstBitOffset = 7 - (i % 8); // LSB-first in destination too

		dst[dstByteIndex] |= (bit << dstBitOffset);
	}
	return true
}

int main() {
	int bytes_in_token = 2; // 16 bits
	const uint8_t* data = (const uint8_t*)"BCAC10";
	printf("Data byte: %x\n", data[0]);
	printf("Data byte: %x\n", data[1]);
	printf("Data byte: %x\n", data[2]);
	printf("Data byte: %x\n", data[3]);

	std::cout << "0 = " << std::bitset<8>(data[0])  << std::endl;
	std::cout << "1 = " << std::bitset<8>(data[1])  << std::endl;
	std::cout << "2 = " << std::bitset<8>(data[2])  << std::endl;
	std::cout << "3 = " << std::bitset<8>(data[3])  << std::endl;

	std::vector<uint8_t> token(bytes_in_token, 0);
	std::fill(token.begin(), token.end(), 0);

	printf("%x\n",token.data()[0]);
	
	ExtractBitsMinhash(data, 3, 16, token.data());

	std::cout << "Extracted token: " << std::bitset<8>(token.data()[0])  << std::endl;
	std::cout << "Extracted token: " << std::bitset<8>(token.data()[1])  << std::endl;

	return 0;
}