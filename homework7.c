#include <stdio.h>
#include <stdlib.h>
#include <string.h>

    /* --- Configuration --- */
    /* Strings longer than this will NOT be put in the Trie to save memory. 
    They will be searched manually. */
#define MAX_INDEX_LEN 250 
#define BLOCK_SIZE 100000

/* --- Data Structures --- */

typedef struct {
    double freq;
    char *dna;
    int is_indexed; /* 1 if in Trie, 0 if manual search needed */
} DNA_Sample;

typedef struct TrieNode {
    struct TrieNode *children[4]; /* 0=A, 1=C, 2=G, 3=T */
    int *match_indices;           /* Indices into the sorted samples array */
    int match_count;
    int match_capacity;
} TrieNode;

// we will use this to create a memory pool for the trie

typedef struct NodeBlock {
    TrieNode nodes[BLOCK_SIZE];
    struct NodeBlock *next;
} NodeBlock;

typedef struct {
    NodeBlock *head;
    int current_index;
} MemPool;

MemPool pool;

void init_pool() {
    pool.head = (NodeBlock *)malloc(sizeof(NodeBlock));
    if (pool.head) {
        pool.head->next = NULL;
        pool.current_index = 0;
    }
}

TrieNode *new_node() {
    if (!pool.head) return NULL; /* Safety check */
    
    if (pool.current_index >= BLOCK_SIZE) {
        NodeBlock *new_block = (NodeBlock *)malloc(sizeof(NodeBlock));
        if (!new_block) return NULL;
        new_block->next = pool.head;
        pool.head = new_block;
        pool.current_index = 0;
    }
    
    TrieNode *node = &pool.head->nodes[pool.current_index++];
    /* Init Node */
    for (int i = 0; i < 4; i++) node->children[i] = NULL;
    node->match_indices = NULL;
    node->match_count = 0;
    node->match_capacity = 0;
    return node;
}

void free_pool() {
    NodeBlock *curr = pool.head;
    while (curr) {
        NodeBlock *temp = curr;
       
        for (int i = 0; i < pool.current_index; i++) {
            if (curr->nodes[i].match_indices) {
                free(curr->nodes[i].match_indices);
            }
        }
        curr = curr->next;
        free(temp);
        pool.current_index = BLOCK_SIZE; /* Full cleanup for subsequent blocks */
    }
}

// we will use this function to convert the DNA string to an index

int dna_to_int(char c) {
    switch(c) {
        case 'A': return 0;
        case 'C': return 1;
        case 'G': return 2;
        case 'T': return 3;
        default: return -1;
    }
}

int is_valid_dna(const char *str) {
    int len = strlen(str);
    if (len == 0) return 0;
    if (len % 3 != 0) return 0;
    for (int i = 0; i < len; i++) {
        if (dna_to_int(str[i]) == -1) return 0;
    }
    return 1;
}

int compare_samples(const void *a, const void *b) {
    DNA_Sample *sa = (DNA_Sample *)a;
    DNA_Sample *sb = (DNA_Sample *)b;
    if (sa->freq > sb->freq) return -1;
    if (sa->freq < sb->freq) return 1;
    return 0;
}

/* Manual search for long strings that were skipped by Trie */
int search_in_dna_manual(const char *dna, const char *pattern) {
    int d_len = strlen(dna);
    int p_len = strlen(pattern);
    if (p_len > d_len) return 0;
    
    for (int i = 0; i <= d_len - p_len; i += 3) {
        if (strncmp(dna + i, pattern, p_len) == 0) {
            return 1;
        }
    }
    return 0;
}



void add_match(TrieNode *node, int sample_idx) {
    // we will use this to add a match to the node
    if (node->match_count > 0 && 
        node->match_indices[node->match_count - 1] == sample_idx) {
        return; 
    }

    if (node->match_count >= node->match_capacity) {
        int new_cap = (node->match_capacity == 0) ? 4 : node->match_capacity * 2;
        int *new_arr = (int *)realloc(node->match_indices, new_cap * sizeof(int));
        if (!new_arr) return; 
        node->match_indices = new_arr;
        node->match_capacity = new_cap;
    }
    node->match_indices[node->match_count++] = sample_idx;
}

void insert_suffixes(TrieNode *root, const char *dna, int sample_idx) {
    // we will use this to insert the suffixes into the trie
    int len = strlen(dna);
    
    for (int start = 0; start < len; start += 3) {
        TrieNode *curr = root;
        for (int i = start; i < len; i++) {
            int idx = dna_to_int(dna[i]);
            if (!curr->children[idx]) {
                curr->children[idx] = new_node();
                if (!curr->children[idx]) return; // we will use this to return if the memory is full
            }
            curr = curr->children[idx];
            
            if ((i - start + 1) % 3 == 0) {
                add_match(curr, sample_idx);
            }
        }
    }
}


int main(void) {
    DNA_Sample *samples = NULL;
    int s_count = 0;
    int s_cap = 0;
    
    char *line = NULL;
    size_t line_len = 0;
    ssize_t read;

    printf("DNA database:\n");

    // we will use this to read the samples
    while ((read = getline(&line, &line_len, stdin)) != -1) {
        // we will use this to strip the newline characters
        while (read > 0 && (line[read - 1] == '\n' || line[read - 1] == '\r')) {
            line[--read] = '\0';
        }
        if (read == 0 || line[0] == '\0') break;

        char *colon = strchr(line, ':');
        if (!colon) {
            printf("Invalid input.\n");
            free(line);
            // we will use this to clean up the memory
            for(int i=0; i<s_count; i++) free(samples[i].dna);
            free(samples);
            return 0;
        }

        *colon = '\0';
        char *freq_str = line;
        char *dna_str = colon + 1;
        char *endptr;
        double freq = strtod(freq_str, &endptr);
        
        if (endptr == freq_str || *endptr != '\0' || !is_valid_dna(dna_str)) {
            printf("Invalid input.\n");
            free(line);
            for(int i=0; i<s_count; i++) free(samples[i].dna);
            free(samples);
            return 0;
        }

        if (s_count >= s_cap) {
            s_cap = (s_cap == 0) ? 16 : s_cap * 2;
            DNA_Sample *tmp = (DNA_Sample *)realloc(samples, s_cap * sizeof(DNA_Sample));
            if (!tmp) return 1;
            samples = tmp;
        }
        samples[s_count].freq = freq;
        samples[s_count].dna = strdup(dna_str);
        samples[s_count].is_indexed = 0; 
        s_count++;
    }

    if (s_count == 0) {
        printf("Invalid input.\n");
        free(line);
        free(samples);
        return 0;
    }

    // we will use this to sort and build the index
    qsort(samples, s_count, sizeof(DNA_Sample), compare_samples);
    
    init_pool();
    TrieNode *root = new_node();
    
    for (int i = 0; i < s_count; i++) {
        // we will use this to index the short strings
        if (strlen(samples[i].dna) <= MAX_INDEX_LEN) {
            insert_suffixes(root, samples[i].dna, i);
            samples[i].is_indexed = 1;
        } else {
            samples[i].is_indexed = 0;
        }
    }

    printf("Searches:\n");
    
    // we will use this to search the DNA database
    int *display_indices = (int *)malloc(50 * sizeof(int));

    while ((read = getline(&line, &line_len, stdin)) != -1) {
        while (read > 0 && (line[read - 1] == '\n' || line[read - 1] == '\r')) {
            line[--read] = '\0';
        }
        if (read == 0 || line[0] == '\0') continue;

        if (!is_valid_dna(line)) {
            printf("Invalid input.\n");
            free(display_indices);
            free(line);
            free_pool();
            for(int i=0; i<s_count; i++) free(samples[i].dna);
            free(samples);
            return 0;
        }

        // we will use this to get the matches from the trie
        TrieNode *curr = root;
        int trie_found = 1;
        for (int i = 0; i < read; i++) {
            int idx = dna_to_int(line[i]);
            if (!curr->children[idx]) {
                trie_found = 0;
                break;
            }
            curr = curr->children[idx];
        }

        int *trie_matches = (trie_found) ? curr->match_indices : NULL;
        int trie_count = (trie_found) ? curr->match_count : 0;
        int trie_pos = 0;

        // we will use this to iterate through all the samples and merge the trie results with the manual checks
        int total_found = 0;
        int collected = 0;

        for (int i = 0; i < s_count; i++) {
            int is_match = 0;

            if (samples[i].is_indexed) {
                // we will use this to check if the index exists in the trie result list
                /* Both 'i' and 'trie_matches' are sorted ascendingly */
                if (trie_pos < trie_count && trie_matches[trie_pos] == i) {
                    is_match = 1;
                    trie_pos++;
                }
            } else {
                // we will use this to check if the long string is a match
                if (search_in_dna_manual(samples[i].dna, line)) {
                    is_match = 1;
                }
            }

            if (is_match) {
                total_found++;
                if (collected < 50) {
                    display_indices[collected++] = i;
                }
            }
        }

        printf("Found: %d\n", total_found);
        for (int i = 0; i < collected; i++) {
            printf("> %s\n", samples[display_indices[i]].dna);
        }
    }

    // we will use this to clean up the memory
    free(display_indices);
    free(line);
    free_pool();
    for (int i = 0; i < s_count; i++) free(samples[i].dna);
    free(samples);

    return 0;
}
