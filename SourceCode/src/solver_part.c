#include "solver_part.h"
#include <string.h>
#include <assert.h>
#include <math.h>

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

int solve(ProblemInstance *input, IO_Info *io_info, glp_prob *lp)
{
	if (lp == NULL)
	{
		fprintf(stderr, "LINEAR PROBLEM NOT CREATED\n");
		return FAIL_STATUS;
	}
	int numOfCols = glp_get_num_cols(lp);
	int numOfRows = glp_get_num_rows(lp);

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
		glp_set_obj_coef(lp, j, 1.0);
	}
	assert(numOfRows == input->numOfTypes);

	if (io_info->options & MIP)
	{
		glp_set_prob_name(lp, "MIP");
		for (size_t j = 1; j <= numOfCols; j++)
		{
			glp_set_col_kind(lp, j, GLP_IV);
		}
		fprintf(io_info->outFile, "MIP\n");
		glp_iocp mip_parm;
		glp_init_iocp(&mip_parm);
		mip_parm.presolve = GLP_ON;
		glp_intopt(lp, &mip_parm);
		return SUCCES_STATUS;
	}
	else if (io_info->options & SM)
	{
		glp_set_prob_name(lp, "SIMPLEX WITH ROUNDING");
		fprintf(io_info->outFile, "SM\n");
		glp_smcp simplex_param;
		glp_init_smcp(&simplex_param);
		glp_simplex(lp, &simplex_param);
		return SUCCES_STATUS;
	}

	return SUCCES_STATUS;
}

Vector *get_SM_solution(ProblemInstance *input, IO_Info *io_info, glp_prob *lp)
{

	/* recover and display results */
	int numOfRows = glp_get_num_rows(lp);
	int numOfCols = glp_get_num_cols(lp);
	int matrixSize = numOfCols * numOfRows;
	int solutionStatus = glp_get_prim_stat(lp);
	size_t stocksNeeded = glp_get_obj_val(lp);

#ifdef DEBUG_ON
	glp_print_mip(lp, "solver_out.txt");
#endif

	int *ind = calloc(numOfRows + 1, sizeof(*ind));
	double *val = calloc(numOfRows + 1, sizeof(*val));
	Vector *v = malloc(sizeof(*v));
	vector_init(v, stocksNeeded);
	stocksNeeded = 0;

	for (int colIdx = 1; colIdx <= numOfCols; colIdx++)
	{
		size_t x = (size_t)round(glp_get_col_prim(lp, colIdx));
		if (x > 0)
		{
			StockConfig *col = calloc(1, sizeof(*col) + sizeof(*col->config) * numOfRows);
			stocksNeeded += x;
			col->spaceLeft = input->stockLength;
			int len = glp_get_mat_col(lp, colIdx, ind, val);

			for (int i = 1; i <= len; i++) // glpk gives column reversed
			{
				int currIdx = ind[i];
				size_t currVal = (size_t)val[i];
				if (currIdx > 0)
				{
					col->config[currIdx - 1] = currVal;
					col->spaceLeft -= (input->arrOfObjs[currIdx - 1].length * currVal);
				}
			}
			for (size_t i = 0; i < numOfRows; i++)
			{
				if (input->arrOfObjs[i].quantity > (col->config[i] * x))
				{
					input->arrOfObjs[i].quantity -= (col->config[i] * x);
				}
				else
				{
					input->arrOfObjs[i].quantity = 0;
				}
			}
			vector_push(v, col);
			for (size_t i = 0; i < x - 1; i++)
			{
				StockConfig *newCol = calloc(1, sizeof(*newCol) + sizeof(*newCol->config) * numOfRows);
				memcpy(newCol, col, sizeof(*col) + sizeof(*col->config) * numOfRows);
				vector_push(v, newCol);
			}
		}
	}

	fprintf(io_info->outFile, "NUMBER_OF_COLUMNS_(CONFIGS): %d\n", numOfCols);
	fprintf(io_info->outFile, "NUMBER_OF_ROWS: %d\n", numOfRows);
	fprintf(io_info->outFile, "MATRIX_SIZE: %d\n", matrixSize);
	fprintf(io_info->outFile, "NUMBER_OF_INT_VARIABLES_IN_GLPK: %d\n", glp_get_num_int(lp));
	fprintf(io_info->outFile, "SOLUTION STATUS: %s\n", SOLUTION_MSGS[solutionStatus]);

	free(val);
	free(ind);

	return v;
}

void print_MIP_solution(ProblemInstance *input, IO_Info *io_info, glp_prob *lp)
{

	/* recover and display results */
	int numOfRows = glp_get_num_rows(lp);
	int numOfCols = glp_get_num_cols(lp);
	int matrixSize = numOfCols * numOfRows;
	int solutionStatus = glp_mip_status(lp);
	double stocksNeeded = glp_mip_obj_val(lp);

	fprintf(io_info->outFile, "NUMBER_OF_COLUMNS_(CONFIGS): %d\n", numOfCols);
	fprintf(io_info->outFile, "NUMBER_OF_ROWS: %d\n", numOfRows);
	fprintf(io_info->outFile, "MATRIX_SIZE: %d\n", matrixSize);
	fprintf(io_info->outFile, "NUMBER_OF_INT_VARIABLES_IN_GLPK: %d\n", glp_get_num_int(lp));
	fprintf(io_info->outFile, "SOLUTION STATUS: %s\n", SOLUTION_MSGS[solutionStatus]);
	fprintf(io_info->outFile, "STOCKS NEEDED: %g\n", stocksNeeded);

#ifdef DEBUG_ON
	glp_print_mip(lp, "solver_out.txt");
#endif

	int *ind = calloc(numOfRows + 1, sizeof(*ind));
	double *val = calloc(numOfRows + 1, sizeof(*val));
	double *config = calloc(numOfRows + 1, sizeof(*config));
	double *objsQuantities = calloc(numOfRows, sizeof(*objsQuantities));
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

	for (size_t i = 0; i < numOfRows; i++)
	{
		assert(input->arrOfObjs[i].quantity <= (size_t)objsQuantities[i]);
	}
	/* housekeeping */
	free(val);
	free(ind);
	free(config);
	free(objsQuantities);
}
