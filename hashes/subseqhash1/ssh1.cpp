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


double f_max[100][100][101];
double f_min[100][100][101];
bool h[100][100][101];

std::map<char, int> dict;

double word[100][4][100];
int sign[100][4];
int sign1[100][4][100];
int sign2[100][4][100];

int p=g_subseqHash1_d;   //TODO: Make 'p' configurable by the caller  // This is d in paper.

void init(int blen, seed_t seed){
   dict['A'] = 0;
	dict['C'] = 1;
	dict['G'] = 2;
	dict['T'] = 3;

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
				word[i][j][q] = distribution(generator);

      seed_t internal_seed_1 = seed + 7*i + 7; // simple way to change seed for different shuffles
		std::shuffle(pos.begin(), pos.end(), std::default_random_engine(internal_seed_1));

		for(int j = 0; j < 4; j++)
			sign[i][j] = pos[j];


		for(int q = 0; q < p; q++)
		{  
         seed_t internal_seed_2 = seed + 13*q + 13; // simple way to change seed for different shuffles
			std::shuffle(possign.begin(), possign.end(), std::default_random_engine(internal_seed_2));
			for(int j = 0; j < 4; j++)
			{
				sign1[i][j][q] = (possign[j] % 2) ? 1: -1;
				sign2[i][j][q] = (possign[j] / 2) ? 1: -1;
			}
		}
	}

}

double DP(int blen, std::string s)
{
	int len = s.length();
	int del = len - blen;

	memset(h, 0, sizeof(h));

	double v = 0;

	f_max[0][0][0] = f_min[0][0][0] = v;
	h[0][0][0] = 1;
	for(int i = 1; i <= len; i++)
	{
		f_max[i][0][0] = v;
		f_min[i][0][0] = v;
		h[i][0][0] = 1;
	}

	f_min[1][1][sign[0][dict[s[0]]]] = f_max[1][1][sign[0][dict[s[0]]]] = v * sign1[0][dict[s[0]]][sign[0][dict[s[0]]]] + sign2[0][dict[s[0]]][sign[0][dict[s[0]]]] * word[0][dict[s[0]]][sign[0][dict[s[0]]]];
	h[1][1][sign[0][dict[s[0]]]] = 1;

	double v1, v2;
	for(int i = 2; i <= len; i++)
	{
		int minj = std::max(1, i - del);
		int maxj = std::min(i, blen);

		for(int j = minj; j <= maxj; j++)
		{
			int now = dict[s[i - 1]];
			int v = sign[j-1][now];

			for(int k = 0; k < p; k++)
			{
				int z = (k + v) % p;
				if(h[i][j][z] == 0)
				{
					f_min[i][j][z] = 1e15;
					f_max[i][j][z] = -1e15;
				}

				if(h[i-1][j][z] == 1)
				{
					f_min[i][j][z] = std::min(f_min[i][j][z], f_min[i-1][j][z]);
					f_max[i][j][z] = std::max(f_max[i][j][z], f_max[i-1][j][z]);
					h[i][j][z] = 1;
				}

				if(h[i-1][j-1][k])
				{
					if(sign1[j-1][now][z] == -1)
					{
						v1 = -f_min[i-1][j-1][k];
						v2 = -f_max[i-1][j-1][k];

						if(sign2[j-1][now][z] == -1)
						{
							v1 -= word[j-1][now][z];
							v2 -= word[j-1][now][z];
						}
						else
						{
							v1 += word[j-1][now][z];
							v2 += word[j-1][now][z];
						}

						if(v1 > f_max[i][j][z])
							f_max[i][j][z] = v1;
						if(v2 < f_min[i][j][z])
							f_min[i][j][z] = v2;
					}

					else
					{
						v1 = f_min[i-1][j-1][k];
						v2 = f_max[i-1][j-1][k];

						if(sign2[j-1][now][z] == -1)
						{
							v1 -= word[j-1][now][z];
							v2 -= word[j-1][now][z];
						}
						else
						{
							v1 += word[j-1][now][z];
							v2 += word[j-1][now][z];
						}

						if(v1 < f_min[i][j][z])
							f_min[i][j][z] = v1;
						if(v2 > f_max[i][j][z])
							f_max[i][j][z] = v2;
					}

					h[i][j][z] = 1;
				}
			}
		}
	}

	double ans = 0;
	for(int i = 0; i < p; i++)
		if(h[len][blen][i])
		{
			ans = std::max(ans, fabs(f_min[len][blen][i]));
			ans = std::max(ans, f_max[len][blen][i]);

			if(ans > 0)
				return ans;
		}

	return ans;
}




template <bool bswap>
static void SubseqHash64(const void* in , const size_t len, const seed_t seed, void* out) {

   uint32_t subseq_len = g_subseqHash1_subseq_len;   //TODO: Make 'subseq_len' configurable by the caller

   init(subseq_len, seed);

   //convert the input to string
   std::string s((const char*)in, len);

   double res = DP(subseq_len, s);

   uint64_t double_bits;
   memcpy(&double_bits, &res, sizeof(double));

   // Copy double bits directly to output
    uint64_t hash;
    memcpy(&hash, &res, sizeof(double));

    PUT_U64<bswap>(hash, (uint8_t*)out, 0);
}


REGISTER_FAMILY(SubseqHash,
   $.src_url    = "https://github.com/Shao-Group/subseqhash",
   $.src_status = HashFamilyInfo::SRC_ACTIVE
);

REGISTER_HASH(SubseqHash_64,
   $.desc            = "Subsequence hash with edit distance tolerance (64-bit)",
   $.hash_flags      = FLAG_HASH_LOOKUP_TABLE | FLAG_HASH_LOCALITY_SENSITIVE | FLAG_HASH_EDIT_SIMILARITY,
   $.impl_flags      = FLAG_IMPL_VERY_SLOW,
   $.bits            = 64,
   $.verification_LE = 0x0,
   $.verification_BE = 0x0,
   $.hashfn_native   = SubseqHash64<false>,
   $.hashfn_bswap    = SubseqHash64<true>
);