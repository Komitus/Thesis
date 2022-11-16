#include <string.h>
#include <assert.h>
#include <float.h>
#include "gen_configs.h"

static inline int gen_configs_tree(ProblemInstance *input, size_t *maxQuantities, glp_prob *lp);
static inline void get_max_quantities(ProblemInstance *input, size_t *maxQuantitiesInStock);
static inline void print_max_quantities(ProblemInstance *input, size_t *maxQuantitiesInStock, FILE *outFile);

int gen_configs(ProblemInstance *input, FILE *outFile, glp_prob *lp)
{
    int retStatus = SUCCES_STATUS;
    size_t *maxQuantitiesInStock = calloc(input->numOfTypes, sizeof(*maxQuantitiesInStock));
    if (maxQuantitiesInStock == NULL || input == NULL || outFile == NULL)
    {
        retStatus = FAIL_STATUS;
        fprintf(stderr, "Out of memory\n");
    }
    else
    {
        get_max_quantities(input, maxQuantitiesInStock);
        print_max_quantities(input, maxQuantitiesInStock, outFile);
        retStatus = gen_configs_tree(input, maxQuantitiesInStock, lp);
    }

    free(maxQuantitiesInStock);

    return retStatus;
}

static inline int gen_configs_tree(ProblemInstance *input, size_t *maxQuantities, glp_prob *lp)
{
    int retStatus = SUCCES_STATUS;
    size_t numOfTypes = input->numOfTypes;
    size_t stockLength = input->stockLength;
    StockConfig *col = malloc(sizeof(*col) + sizeof(*col->config) * numOfTypes);
    size_t sConfigSize = sizeof(*col) + sizeof(*col->config) * numOfTypes;
    StockConfig *prevCol = malloc(sConfigSize);
    size_t *lengths = calloc(numOfTypes, sizeof(*lengths));
    int *ind = malloc(sizeof(*ind) * (numOfTypes+1));
    double *val = malloc(sizeof(*val) * (numOfTypes+1));

    if ((col == NULL) || (prevCol == NULL) || (lengths == NULL) || (ind == NULL) || (val == NULL))
    {
        retStatus = FAIL_STATUS;
        fprintf(stderr, "Out of memory\n");
    }
    else
    {
        for (size_t i = 0; i < numOfTypes; i++)
        {
            lengths[i] = input->arrOfObjs[i].length;
        }

        // Fill first col of matrix
        col->spaceLeft = stockLength;
        for (size_t i = 0; i < numOfTypes; i++) // from 1 bcs of glpk
        {
            col->config[i] = col->spaceLeft / lengths[i];
            if (col->config[i] > maxQuantities[i])
                col->config[i] = maxQuantities[i];
            col->spaceLeft = col->spaceLeft - col->config[i] * lengths[i];
        }

        // add firtst column to solver
        glp_add_rows(lp, input->numOfTypes);
        for (size_t idx = 1; idx <= numOfTypes; idx++)
        {
            ind[idx] = idx;
            val[idx] = (double)(col->config[idx - 1]);
        }
        int j = glp_add_cols(lp, 1);
        glp_set_mat_col(lp, 1, numOfTypes, ind, val);


        size_t i = numOfTypes - 2;

        while (1)
        {
            if (col->config[i] == 0)
            {
                if (i == 0)
                    break;
                i--;
                continue;
            }
            memcpy(prevCol, col, sConfigSize);

            if (i > 0) // bcs of size_t wrapping
            {
                for (size_t k = 0; k <= i - 1; k++)
                {
                    col->config[k] = prevCol->config[k];
                }
            }

            col->config[i] = prevCol->config[i] - 1;
            // calculate cut loss
            col->spaceLeft = stockLength;
            for (size_t idx = 0; idx <= i; idx++)
            {
                col->spaceLeft = col->spaceLeft - (lengths[idx] * col->config[idx]);
            }

            for (i = i + 1; i < numOfTypes; i++)
            {
                col->config[i] = col->spaceLeft / lengths[i];
                if (col->config[i] > maxQuantities[i])
                    col->config[i] = maxQuantities[i];
                col->spaceLeft = col->spaceLeft - col->config[i] * lengths[i];
            }

            // insert to solver -- it uses sparse matrixes, so it doesn't store zero values
            size_t len = 0;

            for (size_t idx = 1; idx <= numOfTypes; idx++)
            {
                if (col->config[idx - 1] != 0)
                {
                    len++;
                    ind[len] = idx;
                    val[len] = (double)(col->config[idx - 1]);
                }
            }

            j = glp_add_cols(lp, 1);
            glp_set_mat_col(lp, j, len, ind, val);
            i = numOfTypes - 2;
        }
    }

    free(lengths);
    free(col);
    free(prevCol);
    free(ind);
    free(val);

    return retStatus;
}

static inline void get_max_quantities(ProblemInstance *input, size_t *maxQuantitiesInStock)
{
    // check max values
    size_t numOfTypes = input->numOfTypes;
    size_t stockLength = input->stockLength;
    for (size_t i = 0; i < numOfTypes; i++)
    {
        ObjWithQuantity currObj = input->arrOfObjs[i];
        size_t filled = 0;
        size_t cnt = 0;

        while (filled <= stockLength && cnt <= currObj.quantity)
        {
            filled += currObj.length;
            cnt++;
        }
        maxQuantitiesInStock[i] = cnt - 1;
    }
}

static inline void print_max_quantities(ProblemInstance *input, size_t *maxQuantitiesInStock, FILE *outFile)
{
    size_t sumOfCombinations = 1;
    size_t numOfTypes = input->numOfTypes;
    fprintf(outFile, "Max number of elements to fit in stock for every object\n");
    fprintf(outFile, "OBJ IDX:      |");
    for (size_t i = 0; i < input->numOfTypes; i++)
    {
        fprintf(outFile, " %3lu |", i);
        if (maxQuantitiesInStock[i] != 0)
        {
            sumOfCombinations *= (maxQuantitiesInStock[i] + 1);
        }
    }
    fprintf(outFile, "\n");
    for (size_t i = 0; i < numOfTypes * 8; i++)
    {
        fprintf(outFile, "-");
    }
    fprintf(outFile, "\n");
    fprintf(outFile, "MAX IN STOCK: |");
    for (size_t i = 0; i < numOfTypes; i++)
    {
        fprintf(outFile, " %3lu |", maxQuantitiesInStock[i]);
    }
    fprintf(outFile, "\n");
    for (size_t i = 0; i < numOfTypes * 8; i++)
    {
        fprintf(outFile, "-");
    }
    fprintf(outFile, "\n");
    fprintf(outFile, "NUM OF (CUSTOM) PERMUTATIONS: %lu\n", sumOfCombinations);
}