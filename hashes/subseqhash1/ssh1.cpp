#include "Platform.h"
#include "Hashlib.h"
#include "LSHGlobals.h"

#include <iostream>
#include <vector>
#include <set>
#include <random>
#include <map>
#include <cstring>
#include <algorithm>

const uint32_t subseqHash1_subseq_len = 11; // Default subsequence length for SubseqHash1 This is k   (11,21,31,37)
const uint32_t subseqHash1_d = 1;           // Default 'p' value for SubseqHash1 this is d

const uint32_t DP_array_size = 120;


// Thread-safe structure to hold all computation state
struct SubseqHashState {
    double f_max[DP_array_size][DP_array_size][DP_array_size + 1];
    double f_min[DP_array_size][DP_array_size][DP_array_size + 1];
    bool h[DP_array_size][DP_array_size][DP_array_size + 1];
    double word[DP_array_size][4][DP_array_size];
    int sign[DP_array_size][4];
    int sign1[DP_array_size][4][DP_array_size];
    int sign2[DP_array_size][4][DP_array_size];
	int dict[256];  // Use array instead of map for speed // Memory intensive but faster

    SubseqHashState() {
        memset(dict, 0, sizeof(dict));
        dict[static_cast<unsigned char>('A')] = 0;
		dict[static_cast<unsigned char>('C')] = 1;
		dict[static_cast<unsigned char>('G')] = 2;
		dict[static_cast<unsigned char>('T')] = 3;
    }
};

// struct SubseqHashState {
//     double f_max[100][100][101];
//     double f_min[100][100][101];
//     bool h[100][100][101];
//     double word[100][4][100];
//     int sign[100][4];
//     int sign1[100][4][100];
//     int sign2[100][4][100];
// 	int dict[256];  // Use array instead of map for speed // Memory intensive but faster

//     SubseqHashState() {
//         memset(dict, 0, sizeof(dict));
//         dict['A'] = 0;
//         dict['C'] = 1;
//         dict['G'] = 2;
//         dict['T'] = 3;
//     }
// };

static void init_state(SubseqHashState& state, int blen, int p, seed_t seed){
	std::vector<int> pos;
	std::vector<int> possign;

	for(int i = 0; i < 4; i++)
		possign.push_back(i);

	for(int i = 0; i < p; i++)
		pos.push_back(i);

	std::default_random_engine generator(seed);
	std::uniform_real_distribution<double> distribution((int64_t)1<<30, (int64_t)1<<31);

	for(int i = 0; i < blen; i++)
	{
		for(int j = 0; j < 4; j++)
			for(int q = 0; q < p; q++)
				state.word[i][j][q] = distribution(generator);

      	seed_t internal_seed_1 = seed + 7*i + 7; // simple way to change seed for different shuffles
		std::shuffle(pos.begin(), pos.end(), std::default_random_engine(internal_seed_1));

		for(int j = 0; j < 4; j++)
			state.sign[i][j] = pos[j];

		for(int q = 0; q < p; q++)
		{  
         seed_t internal_seed_2 = seed + 13*q + 13; // simple way to change seed for different shuffles
			std::shuffle(possign.begin(), possign.end(), std::default_random_engine(internal_seed_2));
			for(int j = 0; j < 4; j++)
			{
				state.sign1[i][j][q] = (possign[j] % 2) ? 1: -1;
				state.sign2[i][j][q] = (possign[j] / 2) ? 1: -1;
			}
		}
	}
}

static double DP_state(SubseqHashState& state, int blen, int p, const char* s, size_t len)
{
	int del = (int)len - blen;

	memset(state.h, 0, sizeof(state.h));

	double v = 0;

	state.f_max[0][0][0] = state.f_min[0][0][0] = v;
	state.h[0][0][0] = 1;
	for(size_t i = 1; i <= len; i++)
	{
		state.f_max[i][0][0] = v;
		state.f_min[i][0][0] = v;
		state.h[i][0][0] = 1;
	}

	int char0 = state.dict[(unsigned char)s[0]];
    int sign0 = state.sign[0][char0];
    state.f_min[1][1][sign0] = state.f_max[1][1][sign0] = 
        v * state.sign1[0][char0][sign0] + 
        state.sign2[0][char0][sign0] * state.word[0][char0][sign0];
    state.h[1][1][sign0] = 1;

	// state.f_min[1][1][state.sign[0][state.dict[s[0]]]] = state.f_max[1][1][state.sign[0][state.dict[s[0]]]] = v * state.sign1[0][state.dict[s[0]]][state.sign[0][state.dict[s[0]]]] + state.sign2[0][state.dict[s[0]]][state.sign[0][state.dict[s[0]]]] * state.word[0][state.dict[s[0]]][state.sign[0][state.dict[s[0]]]];
	// state.h[1][1][state.sign[0][state.dict[s[0]]]] = 1;

	double v1, v2;
	for(size_t i = 2; i <= len; i++)
	{
		int minj = std::max(1, (int)i - del);
		int maxj = std::min((int)i, blen);

		for(int j = minj; j <= maxj; j++)
		{
			int now = state.dict[(unsigned char)s[i - 1]];
			int vv = state.sign[j-1][now];

			for(int k = 0; k < p; k++)
			{
				int z = (k + vv) % p;
				if(state.h[i][j][z] == 0)
				{
					state.f_min[i][j][z] = 1e15;
					state.f_max[i][j][z] = -1e15;
				}

				if(state.h[i-1][j][z] == 1)
				{
					state.f_min[i][j][z] = std::min(state.f_min[i][j][z], state.f_min[i-1][j][z]);
					state.f_max[i][j][z] = std::max(state.f_max[i][j][z], state.f_max[i-1][j][z]);
					state.h[i][j][z] = 1;
				}

				if(state.h[i-1][j-1][k])
				{
					if(state.sign1[j-1][now][z] == -1)
					{
						v1 = -state.f_min[i-1][j-1][k];
						v2 = -state.f_max[i-1][j-1][k];

						if(state.sign2[j-1][now][z] == -1)
						{
							v1 -= state.word[j-1][now][z];
							v2 -= state.word[j-1][now][z];
						}
						else
						{
							v1 += state.word[j-1][now][z];
							v2 += state.word[j-1][now][z];
						}

						if(v1 > state.f_max[i][j][z])
							state.f_max[i][j][z] = v1;
						if(v2 < state.f_min[i][j][z])
							state.f_min[i][j][z] = v2;
					}
					else
					{
						v1 = state.f_min[i-1][j-1][k];
						v2 = state.f_max[i-1][j-1][k];

						if(state.sign2[j-1][now][z] == -1)
						{
							v1 -= state.word[j-1][now][z];
							v2 -= state.word[j-1][now][z];
						}
						else
						{
							v1 += state.word[j-1][now][z];
							v2 += state.word[j-1][now][z];
						}

						if(v1 < state.f_min[i][j][z])
							state.f_min[i][j][z] = v1;
						if(v2 > state.f_max[i][j][z])
							state.f_max[i][j][z] = v2;
					}

					state.h[i][j][z] = 1;
				}
			}
		}
	}

	double ans = 0;
	for(int i = 0; i < p; i++)
		if(state.h[len][blen][i])
		{
			ans = std::max(ans, fabs(state.f_min[len][blen][i]));
			ans = std::max(ans, state.f_max[len][blen][i]);

			if(ans > 0)
				return ans;
		}

	return ans;
}

template <bool bswap>
static void SubseqHash64(const void* in, const size_t len, const seed_t seed, void* out) {

   uint32_t subseq_len = subseqHash1_subseq_len;  //TODO: Make 'subseq_len' configurable by the caller
   int p = subseqHash1_d; //TODO: Make 'p' configurable by the caller  // This is d in paper.

    // Thread-local heap allocation - persists across calls, no stack overflow
    thread_local std::unique_ptr<SubseqHashState> state_ptr;
    if (!state_ptr) {
        state_ptr = std::make_unique<SubseqHashState>();
    }
    SubseqHashState& state = *state_ptr;

   init_state(state, subseq_len, p, seed);

   double res = DP_state(state, subseq_len, p, (const char*)in, len);

   // Copy double bits directly to output
    uint64_t hash;
    memcpy(&hash, &res, sizeof(double));

    PUT_U64<bswap>(hash, (uint8_t*)out, 0);
}

REGISTER_FAMILY(SubseqHash,
   $.src_url    = "https://github.com/Shao-Group/subseqhash",
   $.src_status = HashFamilyInfo::SRC_STABLEISH
);

REGISTER_HASH(SubseqHash_64,
   $.desc            = "Subsequence hash with edit distance tolerance (64-bit)",
   $.hash_flags      = FLAG_HASH_LOOKUP_TABLE | FLAG_HASH_LOCALITY_SENSITIVE | FLAG_HASH_EDIT_SIMILARITY,
   $.impl_flags      = FLAG_IMPL_VERY_SLOW | FLAG_IMPL_SMALL_SEQUENCE_LENGTH,
   $.bits            = 64,
   $.verification_LE = 0x0,
   $.verification_BE = 0x0,
   $.hashfn_native   = SubseqHash64<false>,
   $.hashfn_bswap    = SubseqHash64<true>,
   $.parameterNames  = {"subseqlen(k)", "d"},
   $.parameterDescriptions  = {"Subsequence length (k)", "Parameter 'd' from the paper"},
   $.parameterDefaults = {subseqHash1_subseq_len, subseqHash1_d}
);
