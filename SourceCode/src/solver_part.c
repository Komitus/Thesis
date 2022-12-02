#include "gen_configs.h"
#include <string.h>
#include <assert.h>

#define NO_STATUS 0
#define FOREACH_SOLUTION_STATUS_MSG(FUNC) \
	FUNC(NO_STATUS)                       \
	FUNC(GLP_UNDEF)                       \
	FUNC(GLP_OPT)                         \
	FUNC(GLP_FEAS)                        \
	FUNC(GLP_NOFEAS)

#define FILL_ARR_POS(POS) [POS] = #POS,

static const char *const SOLUTION_MSGS[] = {
	FOREACH_SOLUTION_STATUS_MSG(FILL_ARR_POS)};

int solver(ProblemInstance *input, IO_Info *io_info, glp_prob *lp)
{
	int retStatus = FAIL_STATUS;
	if (lp == NULL)
	{
		fprintf(stderr, "LINEAR PROBLEM NOT CREATED\n");
		return retStatus;
	}
	double stocksNeeded;
	int numOfRows = glp_get_num_rows(lp);
	if (numOfRows != input->numOfTypes)
	{
		fprintf(stderr, "NUM OF ROWS ISN'T CORRECT\n");
		return retStatus;
	}
	int numOfCols = glp_get_num_cols(lp);
	int matrixSize = numOfCols * numOfRows;

	fprintf(io_info->outFile, "NUMBER_OF_COLUMNS_(CONFIGS): %d\n", numOfCols);
	fprintf(io_info->outFile, "NUMBER_OF_ROWS: %d\n", numOfRows);
	fprintf(io_info->outFile, "MATRIX_SIZE: %d\n", matrixSize);

	for (int i = 1; i <= numOfRows; i++)
	{
		glp_set_row_bnds(lp, i, GLP_LO, (double)(input->arrOfObjs[i - 1].quantity), 0.0);
	}

	glp_set_obj_dir(lp, GLP_MIN);
	// obj function coef
	// glpk starts indexing from 1
	for (size_t j = 1; j <= numOfCols; j++)
	{
		glp_set_col_bnds(lp, j, GLP_LO, 0.0, 0.0); // ograniczenia dolne na 0
		glp_set_col_kind(lp, j, GLP_IV);
		glp_set_obj_coef(lp, j, 1.0);
	}

	fprintf(io_info->outFile, "NUMBER_OF_INT_VARIABLES_IN_GLPK: %d\n", glp_get_num_int(lp));

	int solutionStatus;
	glp_set_prob_name(lp, "SIMPLEX as presolver for MIP");
	glp_iocp mip_parm;
	glp_init_iocp(&mip_parm);
	mip_parm.presolve = GLP_ON;
	glp_intopt(lp, &mip_parm);
	solutionStatus = glp_mip_status(lp);
	fprintf(io_info->outFile, "SOLUTION STATUS MIP: %s\n", SOLUTION_MSGS[solutionStatus]);
	stocksNeeded = glp_mip_obj_val(lp);
	glp_print_mip(lp, "solver_out.txt");

	/* recover and display results */
	fprintf(io_info->outFile, "-----------------------------------------------\n");
	fprintf(io_info->outFile, "               STOCKS NEEDED: %g          	  \n", stocksNeeded);
	fprintf(io_info->outFile, "-----------------------------------------------\n");

	int *ind = calloc(numOfRows + 1, sizeof(*ind));
	double *val = calloc(numOfRows + 1, sizeof(*val));
	double *config = calloc(numOfRows + 1, sizeof(*config));
	double *objsQuantities = calloc(numOfRows, sizeof(*config));
	double spaceLeft;

	for (int colIdx = 1; colIdx <= numOfCols; colIdx++)
	{
		double x = glp_mip_col_val(lp, colIdx);
		if (x > 0.0)
		{
			memset(config, 0, sizeof(*config) * (numOfRows + 1));
			spaceLeft = input->stockLength;
			int len = glp_get_mat_col(lp, colIdx, ind, val);
			fprintf(io_info->outFile, "C[%3d]", colIdx - 1);

			for (int i = 1; i <= len; i++) // glpk gives column reversed
			{
				int currIdx = ind[i];
				double currVal = val[i];
				config[currIdx] = currVal;
				spaceLeft -= ((double)input->arrOfObjs[currIdx - 1].length * currVal);
			}
			for (int i = 1; i <= numOfRows; i++)
			{
				fprintf(io_info->outFile, "| %g ", config[i]);
				objsQuantities[i - 1] += (config[i] * x);
			}
			fprintf(io_info->outFile, "|%% %3g |x %g\n", spaceLeft, x);
		}
	}

	retStatus = SUCCES_STATUS;

	for (size_t i = 0; i < numOfRows; i++)
	{
		assert(input->arrOfObjs[i].quantity <= (size_t)objsQuantities[i]);
	}
	/* housekeeping */
	free(val);
	free(ind);
	free(config);
	free(objsQuantities);
	return retStatus;
}
