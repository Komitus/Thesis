#ifndef __GEN_CONFIGS__
#define __GEN_CONFIGS__

#include "structs.h"
#include <glpk.h>	/* GNU GLPK linear/mixed integer solver */

int gen_configs(ProblemInstance *input, FILE *outFile, glp_prob *lp);
int solver(ProblemInstance *input, IO_Info *io_info, glp_prob *lp);

#endif