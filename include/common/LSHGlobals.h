#pragma once

// Global variables for LSH runtime communication
extern int g_lsh_token_length;
extern int g_lsh_num_signatures;
extern bool g_lsh_test_active;

// Setter functions (called by LSHCollision test)
void set_lsh_token_length(int length);
void set_lsh_num_signatures(int num);
void set_lsh_test_active(bool active);

// Getter functions (called by hash functions)
int get_lsh_token_length();
int get_lsh_num_signatures();
bool is_lsh_test_active();