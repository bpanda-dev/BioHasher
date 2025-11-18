#include<iostream>
#include "../build/include/Platform.h"
#include "../util/Random.h"

uint32_t generate_single_random32(uint64_t seed) {
	uint32_t KeyBytes = 4; // 32 bits
	Rand r( seed, KeyBytes );
	RandSeq rs = r.get_seq(SEQ_DIST_1, KeyBytes);
	uint32_t random_value;
	rs.write((void*)&random_value, 0, 1);
	return random_value;
}

int main(){
	uint32_t my_random = generate_single_random32(12345);

	std::cout << "Generated random number: " << my_random << std::endl;
	return 0;
}