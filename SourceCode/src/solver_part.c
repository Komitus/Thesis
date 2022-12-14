#include "solver_part.h"
#include "approx.h"
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

int solve_with_GLPK(ProblemInstance *input, IO_Info *io_info)
{
	glp_prob *lp = glp_create_prob();
	if (lp == NULL)
	{
		fprintf(stderr, "LINEAR PROBLEM NOT CREATED\n");
		return FAIL_STATUS;
	}
	gen_configs(input, io_info->outFile, lp);
	int numOfCols = glp_get_num_cols(lp);
	int numOfRows = glp_get_num_rows(lp);
	int matrixSize = numOfCols * numOfRows;

	for (int i = 1; i <= numOfRows; i++)
	{
		glp_set_row_bnds(lp, i, GLP_LO, (double)(input->arrOfObjs[i - 1].quantity), 0.0);
	}

	glp_set_obj_dir(lp, GLP_MIN);
	// obj function coef
	// glpk starts indexing from 1
	for (size_t j = 1; j <= numOfCols; j++)
	{
		glp_set_col_bnds(lp, j, GLP_LO, 0.0, 0.0);
		glp_set_obj_coef(lp, j, 1.0);
	}
	assert(numOfRows == input->numOfTypes);

	fprintf(io_info->outFile, "NUMBER_OF_COLUMNS_(CONFIGS): %d\n", numOfCols);
	fprintf(io_info->outFile, "NUMBER_OF_ROWS: %d\n", numOfRows);
	fprintf(io_info->outFile, "MATRIX_SIZE: %d\n", matrixSize);

	// Common part
	glp_smcp simplex_param;
	glp_init_smcp(&simplex_param);
	glp_simplex(lp, &simplex_param);

	if (io_info->options & SM)
	{
		print_SM_solution(input, io_info, lp);
		// lp is freed in print
	}
	else
	{
		for (size_t j = 1; j <= numOfCols; j++)
		{
			glp_set_col_kind(lp, j, GLP_IV);
		}
		glp_iocp mip_parm;
		glp_init_iocp(&mip_parm);
		mip_parm.presolve = GLP_OFF;
		glp_intopt(lp, &mip_parm);
		print_MIP_solution(input, io_info, lp);
		glp_delete_prob(lp);
		glp_free_env();
	}

	return SUCCES_STATUS;
}

void print_SM_solution(ProblemInstance *input, IO_Info *io_info, glp_prob *lp)
{
#ifdef TEST_ON
	size_t *objsQuantities = calloc(input->numOfTypes, sizeof(*objsQuantities));
	for (size_t i = 0; i < input->numOfTypes; i++)
	{
		objsQuantities[i] = input->arrOfObjs[i].quantity;
	}
#endif
	/* recover and display results */
	int numOfRows = glp_get_num_rows(lp);
	int numOfCols = glp_get_num_cols(lp);
	int solutionStatus = glp_get_prim_stat(lp);

	fprintf(io_info->outFile, "NUMBER_OF_INT_VARIABLES_IN_GLPK: %d\n", glp_get_num_int(lp));
	fprintf(io_info->outFile, "SOLUTION_STATUS: %s\n", SOLUTION_MSGS[solutionStatus]);

	int *ind = calloc(numOfRows + 1, sizeof(*ind));
	double *val = calloc(numOfRows + 1, sizeof(*val));
	Vector *v = malloc(sizeof(*v));
	vector_init(v, glp_get_obj_val(lp));

	for (int colIdx = 1; colIdx <= numOfCols; colIdx++)
	{
		size_t x = (size_t)round(glp_get_col_prim(lp, colIdx));
		if (x > 0)
		{
			StockConfig *col = calloc(1, sizeof(*col) + sizeof(*col->config) * numOfRows);
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

	free(val);
	free(ind);
	glp_delete_prob(lp);
	glp_free_env();

	size_t stocksNeeded = approx(input, v);

	if (stocksNeeded != SIZE_MAX)
	{
#ifdef TEST_ON
		check_approx(v, input->numOfTypes, objsQuantities);
		free(objsQuantities);
#endif
		print_configs_from_vec(input, v, io_info->outFile);
	}
	for (size_t i = 0; i < vector_size(v); i++)
	{
		free(v->items[i]);
	}
	vector_free(v);
}

void print_MIP_solution(ProblemInstance *input, IO_Info *io_info, glp_prob *lp)
{
	/* recover and display results */
	int numOfRows = glp_get_num_rows(lp);
	int numOfCols = glp_get_num_cols(lp);
	int solutionStatus = glp_mip_status(lp);
	double stocksNeeded = glp_mip_obj_val(lp);

	fprintf(io_info->outFile, "NUMBER_OF_INT_VARIABLES_IN_GLPK: %d\n", glp_get_num_int(lp));
	fprintf(io_info->outFile, "SOLUTION_STATUS: %s\n", SOLUTION_MSGS[solutionStatus]);
	fprintf(io_info->outFile, "STOCKS_NEEDED: %g\n", stocksNeeded);

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
#ifdef TEST_ON
			for (int i = 1; i <= numOfRows; i++)
			{
				fprintf(io_info->outFile, "| %g ", config[i]);
				objsQuantities[i - 1] += (config[i] * x);
			}
#endif

			fprintf(io_info->outFile, "|%% %3g |x %g\n", spaceLeft, x);
		}
	}

#ifdef TEST_ON
	for (size_t i = 0; i < numOfRows; i++)
	{
		assert(input->arrOfObjs[i].quantity <= (size_t)objsQuantities[i]);
	}
#endif
	/* housekeeping */
	free(val);
	free(ind);
	free(config);
	free(objsQuantities);
}
