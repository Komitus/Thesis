#ifndef __GEN_CONFIGS__
#define __GEN_CONFIGS__

#include "structs.h"
#include <glpk.h>	/* GNU GLPK linear/mixed integer solver */

int gen_configs(ProblemInstance *input, FILE *outFile, glp_prob *lp);
int solve(ProblemInstance *input, IO_Info *io_info, glp_prob *lp);
void print_MIP_solution(ProblemInstance *input, IO_Info *io_info, glp_prob *lp);
Vector* get_SM_solution(ProblemInstance *input, IO_Info *io_info, glp_prob *lp);

#endif