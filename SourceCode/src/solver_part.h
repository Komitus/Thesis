#ifndef __GEN_CONFIGS__
#define __GEN_CONFIGS__

#include "structs.h"
#include <glpk.h>	/* GNU GLPK linear/mixed integer solver */

int gen_configs(ProblemInstance *input, FILE *outFile, glp_prob *lp);
int solve_with_GLPK(ProblemInstance *input, IO_Info *io_info);
void print_MIP_solution(ProblemInstance *input, IO_Info *io_info, glp_prob *lp);
void print_SM_solution(ProblemInstance *input, IO_Info *io_info, glp_prob *lp);

#endif