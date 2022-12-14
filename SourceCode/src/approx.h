#include "structs.h"
#include "vector.h"

/**
 * @brief Function that performs the approximation algorithm
 *
 * @param input
 * @return size_t SIZE_MAX indicates error
 */
size_t approx(ProblemInstance *input, Vector *v);
int print_configs_from_vec(ProblemInstance *input, Vector *v, FILE *outFile);
size_t get_min_numb_of_stocks(ProblemInstance *input);
void check_approx(Vector *v, size_t numOfTypes, size_t *objQuantities);