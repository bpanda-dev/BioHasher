#define ALPHABETSIZE 4
const char ALPHABET[ALPHABETSIZE] = {'A', 'C', 'G', 'T'};

#define WINDOW_SIZE 30
#define SUBSEQUENCE_LENGTH 25
#define D_PARAM 11

#define MAXK 64
#define MAXD 31

#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>


typedef uint64_t kmer;
typedef uint64_t seed_t;	// remove this in smhasher.
const double INF = 1e15;

struct DPCell{
    double f_max, f_min;	// values
    int g_max, g_min;	// directions for traceback
};

struct genomics_seed
{
    int64_t hashval;
    kmer str;
    size_t start, end;
    uint64_t index = 0;	// This store the 
};

static inline int alphabetIndex(char c)
{
    return 3 & ((c>>2) ^ (c>>1));
}

static inline int dpIndex(int d1, int d2, int d3)
{
    // Assuming dimensions from the original context
    int k = SUBSEQUENCE_LENGTH;
    int d = D_PARAM;
    return d1 * (k+1) * d + d2 * d + d3;
}

void subseqhash(const void* in, const size_t len, const seed_t r_seed, void* out, genomics_seed& gen_seed){
    
    const char* s = (const char*)in;

    int n = WINDOW_SIZE;
    int k = SUBSEQUENCE_LENGTH;
	int d = D_PARAM;
    int dim1 = (n+1) * (k+1) * d;
    int dim2 = d;
    
    //-------------------------------------------------------------------------------------//

    double A[MAXK][ALPHABETSIZE][MAXD];
	int B1[MAXK][ALPHABETSIZE][MAXD];
	int B2[MAXK][ALPHABETSIZE][MAXD];

	int C[MAXK][ALPHABETSIZE];
    
    std::vector<int> pos;
	std::vector<int> possign;

    for(int i = 0; i < 4; i++)
		possign.push_back(i);

	for(int i = 0; i < d; i++)
		pos.push_back(i);
        
	if(d < 4)
		for(int i = d; i < 4; i++)
			pos.push_back(i%d);
    
    // unsigned seed_val;
  	std::default_random_engine generator(r_seed);
	std::uniform_real_distribution<double> distribution((int64_t)1<<30, (int64_t)1<<31);

    for(int i = 0; i < k; i++)
	{
		for(int j = 0; j < 4; j++)
			for(int q = 0; q < d; q++)
				A[i][j][q] = distribution(generator);

		seed_t internal_seed_1 = r_seed + 7*i + 7; // simple way to change seed for different shuffles
		std::shuffle(pos.begin(), pos.end(), std::default_random_engine(internal_seed_1));

		for(int j = 0; j < 4; j++)
			C[i][j] = pos[j];

		for(int q = 0; q < d; q++)
		{	
			seed_t internal_seed_2 = r_seed + 13*q + 13; // simple way to change seed for different shuffles
			std::shuffle(possign.begin(), possign.end(), std::default_random_engine(internal_seed_2));

			for(int j = 0; j < 4; j++)
			{
				B1[i][j][q] = (possign[j] % 2) ? 1: -1;
				B2[i][j][q] = (possign[j] / 2) ? 1: -1;
			}
		}
	}

    //-------------------------------------------------------------------------------------//


    DPCell* dp;
    int* h;

    dp = (DPCell*) malloc(sizeof *dp * (dim1));
	h = (int*) malloc(sizeof *h * dim1);
    
    // If we are processing the input as a single window, n should be its length.
    n = len;
    dim1 = (n+1) * (k+1) * d;

    int dp_index;
    int del = n - k; // 'del' is the max number of characters we can skip

    memset(h, 0, sizeof(*h) * dim1);

    for(int i = 0; i <= n; i++)
	{
	    dp_index = dpIndex(i, 0, 0); //[i][0][0]

	    dp[dp_index].f_max = 0;
	    dp[dp_index].f_min = 0;
	    
	    dp_index += 1;
	    for(int j = 1; j < d; ++j, ++dp_index)
	    {
			dp[dp_index].f_max = -INF;
			dp[dp_index].f_min = INF;
	    }
	}

    // The main loop over windows is removed. We process the sequence as a single window.
    // Let's assume st = 0.
    int st = 0;

    bool skip = 0;
	for(int i = st; i < st + n; i++)
		if(s[i] == 'N')
		{
			skip = 1;
			break;
		}	

    if(!skip)
    {
		for(int i = 0; i <= n; i++)
		{
			dp_index = dpIndex(i, 0, 0);
		    h[dp_index] = st + 1;
		}
	
		int d1 = C[0][alphabetIndex(s[st])];
		dp_index = dpIndex(1, 1, d1);

		dp[dp_index].f_min = dp[dp_index].f_max = B2[0][alphabetIndex(s[st])][d1] * A[0][alphabetIndex(s[st])][d1];
		dp[dp_index].g_min = dp[dp_index].g_max = 0;
		h[dp_index] = st + 1;

		//printf("%d %d %d %.2lf %.2lf %d %d\n", st, d1, dp_index, dp[dp_index].f_max, dp[dp_index].f_min, dp[dp_index].g_max, dp[dp_index].g_min);
		double v1, v2;

		for(int i = 2; i <= n; i++)
		{
		    int minj = std::max(1, i - del);
		    int maxj = std::min(i, k);

		    for(int j = minj; j <= maxj; j++)
		    {
				int now = alphabetIndex(s[st + i - 1]); //Current char
				int v = C[j-1][now];

				for(int q = 0; q < d; q++)
				{
				    int z = (q + v) % d; // q: previous d, z: current d
				    
				    int idx = dpIndex(i, j, z);
				    
				    int laidx1 = dpIndex(i-1, j, z);// = index_cal(i-1,j,z);
				    int laidx2 = dpIndex(i-1, j-1, q);// = index_cal(i-1,j-1,q);

				    if(h[idx] != st + 1)
				    {
						dp[idx].f_min = INF;
						dp[idx].f_max = -INF;
				    }
				    
				    if(h[laidx1] == st + 1)
				    {
						if(dp[laidx1].f_min < dp[idx].f_min)
						{
						    dp[idx].f_min = dp[laidx1].f_min;
						    dp[idx].g_min = dp[laidx1].g_min;
						}
						if(dp[laidx1].f_max > dp[idx].f_max)
						{
						    dp[idx].f_max = dp[laidx1].f_max;
						    dp[idx].g_max = dp[laidx1].g_max;
						}
						h[idx] = st + 1;
				    }

				    if(h[laidx2] == st + 1)
				    {
						if(B1[j-1][now][z] == -1)
						{
						    v1 = -dp[laidx2].f_min;
						    v2 = -dp[laidx2].f_max;

						    if(B2[j-1][now][z] == -1)
						    {
								v1 -= A[j-1][now][z];
								v2 -= A[j-1][now][z];
						    }
						    else
						    {
								v1 += A[j-1][now][z];
								v2 += A[j-1][now][z];
						    }

						    if(v1 > dp[idx].f_max)
						    {
								dp[idx].f_max = v1;
								dp[idx].g_max = i-1;
						    }
						    if(v2 < dp[idx].f_min)
						    {
								dp[idx].f_min = v2;
								dp[idx].g_min = i-1;
						    }
						}

						else
						{
						    v1 = dp[laidx2].f_min;
						    v2 = dp[laidx2].f_max;

						    if(B2[j-1][now][z] == -1)
						    {
								v1 -= A[j-1][now][z];
								v2 -= A[j-1][now][z];
						    }
						    else
						    {
								v1 += A[j-1][now][z];
								v2 += A[j-1][now][z];
						    }

						    if(v1 < dp[idx].f_min)
						    {
								dp[idx].f_min = v1;
								dp[idx].g_min = i-1;
						    }
						    if(v2 > dp[idx].f_max)
						    {
								dp[idx].f_max = v2;
								dp[idx].g_max = i-1;
						    }
						}
						
						h[idx] = st + 1;
				    }			

				    //printf("%d %d %d %d %d %.2lf %.2lf %d %d\n", i,j,z,idx,h[idx],dp[idx].f_max, dp[idx].f_min, dp[idx].g_max, dp[idx].g_min);
				}
			}
		}
        
        genomics_seed tmp;
		int mod;
		dp_index = dpIndex(n, k, 0);

		int i = 0;
		for(i = 0; i < d; i++)
			if(h[dp_index + i] == st + 1)
			{
				if(fabs(dp[dp_index + i].f_min) > dp[dp_index + i].f_max)
					tmp.hashval = uint64_t(dp[dp_index + i].f_min * 32768);
				else
					tmp.hashval = uint64_t(dp[dp_index + i].f_max * 32768);

				mod = i;
                break;
			}

			// If no valid hash was found, we can't proceed.
			if (i == d) {
				printf("No valid hash found for the given input sequence.\n");
				return; // TODO: Need to handle it with much better way. 
			}

			kmer hashval = 0;
			std::vector<size_t> index;

			int x = n;
			int y = k;
			size_t nextpos;
			int nextval, nextd;

			bool z = 1;
			if(tmp.hashval < 0)
				z = 0;

			while(y > 0)
			{
				dp_index = dpIndex(x, y, mod);
				if(z == 1)
				{
					nextpos = st + dp[dp_index].g_max;
					nextval = alphabetIndex(s[nextpos]);

					index.push_back(nextpos);
					hashval = (hashval<<2) | nextval;

					if(B1[y-1][nextval][mod] == -1)
						z = 0;

					x = dp[dp_index].g_max;
					y--;
					mod = (mod + d - C[y][nextval]) % d;
				}
				else
				{
					nextpos = st + dp[dp_index].g_min;
					nextval = alphabetIndex(s[nextpos]);

					index.push_back(nextpos);
					hashval = (hashval<<2) | nextval;

					if(B1[y-1][nextval][mod] == -1)
						z = 1;

					x = dp[dp_index].g_min;
					y--;
					mod = (mod + d - C[y][nextval]) % d;
				}
			}

			tmp.start = index.back();
			tmp.end = index[0];

	    for(size_t a: index)
	    	tmp.index |= ((uint64_t)1)<<(a - tmp.start);

        // The logic for comparing with a vector of seeds is removed.
        // We assign the result to the output parameter 'gen_seed'.
		tmp.str = hashval;
        gen_seed = tmp;
	}
    free(dp);
    free(h);
}


// --- Main function to call subseqhash and print the result ---

char* decode(const kmer enc, const int k, char* str)
{
    if(str == NULL)
		str = (char*)malloc(sizeof *str *k);

    kmer enc_copy = enc;

    for(int i=k-1; i>=0; i-=1)
    {
		str[i] = ALPHABET[enc_copy & 3];
		enc_copy >>= 2;
    }

    return str;
}

int main() {
    // 1. Define an input sequence.
    // This sequence must be at least SUBSEQUENCE_LENGTH (25) characters long.
    std::string dna_sequence = "GCCGTAAACACCATATCAACGGTGAAAGGG";

    // 2. Define a genomics_seed object to hold the result.
    genomics_seed result_seed;
    // Initialize to zero to have a clean state.
    memset(&result_seed, 0, sizeof(result_seed));

    // 3. Set a seed for the random number generator.
    seed_t my_seed = 12345;

    // 4. Call subseqhash.
    std::cout << "Calling subseqhash on a sequence of length " << dna_sequence.length() << "..." << std::endl;
    subseqhash(dna_sequence.c_str(), dna_sequence.length(), my_seed, nullptr, result_seed);
    std::cout << "subseqhash call complete." << std::endl << std::endl;

	char decoded_str[100]; // Adjust size as needed
	
	decode(result_seed.str, SUBSEQUENCE_LENGTH, decoded_str); 

    // 5. Print the contents of the resulting genomics_seed structure.
    std::cout << "--- Generated Genomic Seed ---" << std::endl;
    std::cout << "Hash Value: " << result_seed.hashval << std::endl;
    std::cout << "String (kmer): " << result_seed.str << std::endl;
	std::cout << "String (kmer): " << decoded_str << std::endl;

    std::cout << "Start Pos:  " << result_seed.start << std::endl;
    std::cout << "End Pos:    " << result_seed.end << std::endl;
    std::cout << "Index Mask: " << result_seed.index << std::endl;
    std::cout << "----------------------------" << std::endl;

    return 0;
}
