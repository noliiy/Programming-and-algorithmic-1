#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_SIZE 16
#define MAX_WORDS 60
#define MAX_WORD_LEN 17
#define MAX_GRID_SIZE 18
#define MAX_RUNS 120

typedef struct {
    int row, col;
    int length;
    bool horizontal;
    char *constraints;
    int *constraint_set_by;
    int constraint_count; // Number of constraints (for ordering)
    int *intersecting_runs; // Precomputed list of intersecting run indices
    int num_intersections;
} Run;

typedef struct {
    char word[MAX_WORD_LEN];
    int length;
    bool used;
} Word;

char grid[MAX_GRID_SIZE][MAX_GRID_SIZE];
int rows, cols;
Run runs[MAX_RUNS];
int num_runs = 0;
Word words[MAX_WORDS];
int num_words = 0;
int solution_count = 0;
char solution[MAX_GRID_SIZE][MAX_GRID_SIZE];
char first_solution[MAX_GRID_SIZE][MAX_GRID_SIZE];
bool solution_saved = false;

// Word index by length for fast lookup
int word_indices_by_length[MAX_WORD_LEN][MAX_WORDS];
int word_count_by_length[MAX_WORD_LEN];

// Run ordering for better backtracking
int run_order[MAX_RUNS];

// Precomputed intersection data
int intersection_data[MAX_RUNS * MAX_RUNS];
int intersection_offsets[MAX_RUNS];

// we will validate the border of the grid to make sure it is valid
static inline bool validate_border(void) {
    if (rows < 3 || cols < 3) return false;
    
    if (grid[0][0] != '+' || grid[0][cols-1] != '+' ||
        grid[rows-1][0] != '+' || grid[rows-1][cols-1] != '+') {
        return false;
    }
    
    for (int j = 1; j < cols-1; j++) {
        if (grid[0][j] != '-' || grid[rows-1][j] != '-') return false;
    }
    
    for (int i = 1; i < rows-1; i++) {
        if (grid[i][0] != '|' || grid[i][cols-1] != '|') return false;
    }
    
    for (int i = 1; i < rows-1; i++) {
        for (int j = 1; j < cols-1; j++) {
            char c = grid[i][j];
            if (c != ' ' && c != '*') return false;
        }
    }
    
    return true;
}

// Extract runs from the grid to get the runs
static inline void extract_runs(void) {
    num_runs = 0;
    
    // Horizontal runs are runs that go from left to right
    for (int i = 1; i < rows-1; i++) {
        int start = -1;
        for (int j = 1; j < cols; j++) {
            if (grid[i][j] == ' ') {
                if (start == -1) start = j;
            } else {
                if (start != -1) {
                    int len = j - start;
                    if (len >= 1) {
                        Run *r = &runs[num_runs];
                        r->row = i;
                        r->col = start;
                        r->length = len;
                        r->horizontal = true;
                        r->constraints = (char*)calloc(len, sizeof(char));
                        r->constraint_set_by = (int*)calloc(len, sizeof(int));
                        memset(r->constraint_set_by, -1, len * sizeof(int));
                        r->constraint_count = 0;
                        r->num_intersections = 0;
                        num_runs++;
                    }
                    start = -1;
                }
            }
        }
        if (start != -1) {
            int len = cols - 1 - start;
            if (len >= 1) {
                Run *r = &runs[num_runs];
                r->row = i;
                r->col = start;
                r->length = len;
                r->horizontal = true;
                r->constraints = (char*)calloc(len, sizeof(char));
                r->constraint_set_by = (int*)calloc(len, sizeof(int));
                memset(r->constraint_set_by, -1, len * sizeof(int));
                r->constraint_count = 0;
                r->num_intersections = 0;
                num_runs++;
            }
        }
    }
    
    // Vertical runs are runs that go from top to bottom
    for (int j = 1; j < cols-1; j++) {
        int start = -1;
        for (int i = 1; i < rows; i++) {
            if (grid[i][j] == ' ') {
                if (start == -1) start = i;
            } else {
                if (start != -1) {
                    int len = i - start;
                    if (len >= 1) {
                        Run *r = &runs[num_runs];
                        r->row = start;
                        r->col = j;
                        r->length = len;
                        r->horizontal = false;
                        r->constraints = (char*)calloc(len, sizeof(char));
                        r->constraint_set_by = (int*)calloc(len, sizeof(int));
                        memset(r->constraint_set_by, -1, len * sizeof(int));
                        r->constraint_count = 0;
                        r->num_intersections = 0;
                        num_runs++;
                    }
                    start = -1;
                }
            }
        }
        // Handle run that extends to the bottom border
        if (start != -1) {
            // The last inner row is rows-2, so if start is valid, the run goes from start to rows-2
            int len = (rows - 1) - start;  // rows-1 is the border, so rows-2 is last inner row
            if (len >= 1) {
                Run *r = &runs[num_runs];
                r->row = start;
                r->col = j;
                r->length = len;
                r->horizontal = false;
                r->constraints = (char*)calloc(len, sizeof(char));
                r->constraint_set_by = (int*)calloc(len, sizeof(int));
                memset(r->constraint_set_by, -1, len * sizeof(int));
                r->constraint_count = 0;
                r->num_intersections = 0;
                num_runs++;
            }
        }
    }
}

// Precompute intersections for faster constraint updates to avoid recalculating intersections for each word
static inline void precompute_intersections(void) {
    int offset = 0;
    for (int i = 0; i < num_runs; i++) {
        Run *r1 = &runs[i];
        intersection_offsets[i] = offset;
        int count = 0;
        
        for (int j = 0; j < num_runs; j++) {
            if (i == j) continue;
            Run *r2 = &runs[j];
            
            if (r1->horizontal != r2->horizontal) {
                if (r1->horizontal) {
                    // r1 horizontal, r2 vertical
                    if (r2->col >= r1->col && r2->col < r1->col + r1->length &&
                        r1->row >= r2->row && r1->row < r2->row + r2->length) {
                        intersection_data[offset + count] = j;
                        count++;
                    }
                } else {
                    // r1 vertical, r2 horizontal
                    if (r2->row >= r1->row && r2->row < r1->row + r1->length &&
                        r1->col >= r2->col && r1->col < r2->col + r2->length) {
                        intersection_data[offset + count] = j;
                        count++;
                    }
                }
            }
        }
        
        r1->num_intersections = count;
        r1->intersecting_runs = intersection_data + offset;
        offset += count;
    }
}

// Validate input to make sure the input is valid
static inline bool validate_input(void) {
    int inner_rows = rows - 2;
    int inner_cols = cols - 2;
    if (inner_rows > MAX_SIZE || inner_cols > MAX_SIZE) return false;
    if (num_words > MAX_WORDS) return false;
    
    for (int i = 0; i < num_words; i++) {
        for (int j = 0; j < words[i].length; j++) {
            if (!islower(words[i].word[j])) return false;
        }
    }
    
    // in here we will count the number of words by length
    memset(word_count_by_length, 0, sizeof(word_count_by_length));
    for (int i = 0; i < num_words; i++) {
        int len = words[i].length;
        if (len < MAX_WORD_LEN) {
            word_indices_by_length[len][word_count_by_length[len]++] = i;
        }
    }
    
    // we will count the number of runs by length
    // we will check if the number of runs by length is the same as the number of words by length
    // if not, then the input is invalid
    int length_counts[MAX_WORD_LEN] = {0};
    for (int i = 0; i < num_runs; i++) {
        if (runs[i].length < MAX_WORD_LEN) {
            length_counts[runs[i].length]++;
        }
    }
    
    for (int i = 2; i < MAX_WORD_LEN; i++) {
        if (length_counts[i] != word_count_by_length[i]) return false;
    }
    
    return true;
}

// in here we will count the number of constraints for each run
static inline void count_constraints(void) {
    for (int i = 0; i < num_runs; i++) {
        Run *r = &runs[i];
        int count = 0;
        for (int j = 0; j < r->length; j++) {
            if (r->constraints[j] != 0) count++;
        }
        r->constraint_count = count;
    }
}

// in here we will sort the runs by constraint count (most constrained first) for better pruning
static inline void order_runs(void) {
    for (int i = 0; i < num_runs; i++) {
        run_order[i] = i;
    }
    
    // we will use a simple insertion sort by constraint count (descending)
    for (int i = 1; i < num_runs; i++) {
        int key = run_order[i];
        int j = i - 1;
        while (j >= 0 && runs[run_order[j]].constraint_count < runs[key].constraint_count) {
            run_order[j + 1] = run_order[j];
            j--;
        }
        run_order[j + 1] = key;
    }
}

// we will check if the word can be placed in the run
static inline bool can_place(int run_idx, int word_idx) {
    if (words[word_idx].used) return false;
    Run *r = &runs[run_idx];
    if (r->length != words[word_idx].length) return false;
    
    // we will check if the constraints are met
    const char *word = words[word_idx].word;
    for (int i = 0; i < r->length; i++) {
        char c = r->constraints[i];
        if (c != 0 && word[i] != c) return false;
    }
    
    return true;
}

// we will place the word in the run
static inline void place_word(int run_idx, int word_idx) {
    Run *r = &runs[run_idx];
    words[word_idx].used = true;
    const char *word = words[word_idx].word;
    
    for (int i = 0; i < r->length; i++) {
        int row = r->row;
        int col = r->col;
        if (r->horizontal) col += i;
        else row += i;
        
        solution[row][col] = word[i];
        
        // we will use the precomputed intersections to update the constraints
        for (int k = 0; k < r->num_intersections; k++) {
            int other_idx = r->intersecting_runs[k];
            Run *other = &runs[other_idx];
            
            int other_row = other->row;
            int other_col = other->col;
            int pos;
            
            if (r->horizontal) {
                // r is horizontal, other is vertical
                if (other_col == col && row >= other_row && row < other_row + other->length) {
                    pos = row - other_row;
                    if (other->constraints[pos] == 0) {
                        other->constraints[pos] = word[i];
                        other->constraint_set_by[pos] = run_idx;
                        other->constraint_count++;
                    } else if (other->constraints[pos] != word[i]) {
                        // This should not happen if can_place worked correctly
                        // But we check anyway for safety
                    }
                }
            } else {
                // r is vertical, other is horizontal
                if (other_row == row && col >= other_col && col < other_col + other->length) {
                    pos = col - other_col;
                    if (other->constraints[pos] == 0) {
                        other->constraints[pos] = word[i];
                        other->constraint_set_by[pos] = run_idx;
                        other->constraint_count++;
                    } else if (other->constraints[pos] != word[i]) {
                        // This should not happen if can_place worked correctly
                        // But we check anyway for safety
                    }
                }
            }
        }
    }
}

// we will remove the word from the run to backtrack
static inline void remove_word(int run_idx, int word_idx) {
    Run *r = &runs[run_idx];
    words[word_idx].used = false;
    
    for (int i = 0; i < r->length; i++) {
        int row = r->row;
        int col = r->col;
        if (r->horizontal) col += i;
        else row += i;
        
        solution[row][col] = ' ';
        
        // we will use the precomputed intersections to update the constraints
        for (int k = 0; k < r->num_intersections; k++) {
            int other_idx = r->intersecting_runs[k];
            Run *other = &runs[other_idx];
            
            int other_row = other->row;
            int other_col = other->col;
            int pos;
            
            if (r->horizontal) {
                if (other_col == col && row >= other_row && row < other_row + other->length) {
                    pos = row - other_row;
                    if (other->constraint_set_by[pos] == run_idx) {
                        other->constraints[pos] = 0;
                        other->constraint_set_by[pos] = -1;
                        other->constraint_count--;
                    }
                }
            } else {
                if (other_row == row && col >= other_col && col < other_col + other->length) {
                    pos = col - other_col;
                    if (other->constraint_set_by[pos] == run_idx) {
                        other->constraints[pos] = 0;
                        other->constraint_set_by[pos] = -1;
                        other->constraint_count--;
                    }
                }
            }
        }
    }
}

// we will solve the crossword using backtracking to avoid duplicate words
static void solve(int order_idx) {
    if (order_idx == num_runs) {
        solution_count++;
        if (solution_count == 1) {
            memcpy(first_solution, solution, sizeof(solution));
            solution_saved = true;
        }
        return;
    }
    
    // Reorder remaining runs by constraint count for better pruning
    // This helps us try the most constrained runs first
    for (int i = order_idx + 1; i < num_runs; i++) {
        int key = run_order[i];
        int j = i - 1;
        while (j >= order_idx && runs[run_order[j]].constraint_count < runs[key].constraint_count) {
            run_order[j + 1] = run_order[j];
            j--;
        }
        run_order[j + 1] = key;
    }
    
    int run_idx = run_order[order_idx];
    Run *r = &runs[run_idx];
    
    // we will get the candidate words for this length
    int len = r->length;
    if (len >= MAX_WORD_LEN) return;
    
    int *candidates = word_indices_by_length[len];
    int num_candidates = word_count_by_length[len];
    
    // we will try each candidate word
    // duplicate avoidance: only try first unused instance of each unique word value
    for (int i = 0; i < num_candidates; i++) {
        int word_idx = candidates[i];
        
        if (can_place(run_idx, word_idx)) {
            // we will check if we've already tried an earlier unused word with the same value
            // This ensures we don't generate duplicate solutions when we have identical words
            bool skip = false;
            for (int j = 0; j < i; j++) {
                int prev_idx = candidates[j];
                if (!words[prev_idx].used &&
                    words[prev_idx].length == words[word_idx].length &&
                    memcmp(words[prev_idx].word, words[word_idx].word, len) == 0) {
                    // If there's an earlier unused identical word, skip this one
                    // This ensures canonical ordering: always use the first available instance
                    skip = true;
                    break;
                }
            }
            
            if (!skip) {
                place_word(run_idx, word_idx);
                solve(order_idx + 1);
                remove_word(run_idx, word_idx);
            }
        }
    }
}

// i will print the solution to the console
static inline void print_solution(void) {
    for (int i = 0; i < rows; i++) {
        fwrite(first_solution[i], 1, cols, stdout);
        putchar('\n');
    }
}

// we will check if a line looks like a grid line
static inline bool is_grid_line(const char *line) {
    for (int i = 0; line[i] != '\0'; i++) {
        char c = line[i];
        if (c == '+' || c == '-' || c == '|' || c == '*' || c == ' ') {
            return true;
        }
    }
    return false;
}

int main(void) {
    printf("Enter the crossword:\n");
    
    // read all input lines
    char all_lines[200][MAX_GRID_SIZE + 2];
    int num_lines = 0;
    char line[MAX_GRID_SIZE + 2];
    
    while (fgets(line, sizeof(line), stdin) != NULL) {
        int len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
            len--;
        }
        
        if (num_lines >= 200) {
            printf("Invalid input.\n");
            return 1;
        }
        
        strcpy(all_lines[num_lines], line);
        num_lines++;
    }
    
    // find where grid ends to separate the grid from the words
    int grid_end = 0;
    for (int i = 0; i < num_lines; i++) {
        if (strlen(all_lines[i]) > 0 && !is_grid_line(all_lines[i])) {
            grid_end = i;
            break;
        }
        if (i == num_lines - 1) {
            grid_end = num_lines;
        }
    }
    
    // read the grid from the input to get the rows and columns
    rows = 0;
    cols = 0;
    for (int i = 0; i < grid_end; i++) {
        int len = strlen(all_lines[i]);
        if (len == 0) continue;
        
        if (rows == 0) cols = len;
        else if (len != cols) {
            printf("Invalid input.\n");
            return 1;
        }
        
        if (rows >= MAX_GRID_SIZE) {
            printf("Invalid input.\n");
            return 1;
        }
        
        strcpy(grid[rows], all_lines[i]);
        rows++;
    }
    
    if (!validate_border()) {
        printf("Invalid input.\n");
        return 1;
    }
    
    extract_runs();
    
    // read the words from the input to get the words
    for (int i = grid_end; i < num_lines; i++) {
        int len = strlen(all_lines[i]);
        if (len == 0) continue;
        
        if (num_words >= MAX_WORDS) {
            printf("Invalid input.\n");
            return 1;
        }
        
        strcpy(words[num_words].word, all_lines[i]);
        words[num_words].length = len;
        words[num_words].used = false;
        num_words++;
    }
    
    if (!validate_input()) {
        printf("Invalid input.\n");
        return 1;
    }
    
    // initialize the solution grid and set the borders to the grid
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (i == 0 || i == rows-1 || j == 0 || j == cols-1) {
                solution[i][j] = grid[i][j];
            } else if (grid[i][j] == '*') {
                solution[i][j] = '*';
            } else {
                solution[i][j] = ' ';
            }
        }
    }
    
    // Precompute intersections
    precompute_intersections();
    
    // Count constraints and order runs
    count_constraints();
    order_runs();
    
    // solve the crossword using backtracking
    solution_count = 0;
    solution_saved = false;
    solve(0);
    
    // thats how we will output the result
    if (solution_count == 0) {
        printf("No solution.\n");
    } else if (solution_count == 1) {
        printf("There is one solution of the crossword:\n");
        print_solution();
    } else {
        printf("Unique solutions: %d\n", solution_count);
    }
    
    // Cleanup
    for (int i = 0; i < num_runs; i++) {
        free(runs[i].constraints);
        free(runs[i].constraint_set_by);
    }
    
    return 0;
}
