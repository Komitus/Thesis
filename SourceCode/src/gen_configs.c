#include <string.h>
#include <assert.h>
#include <float.h>
#include "solver_part.h"

static inline int gen_configs_tree(ProblemInstance *input, glp_prob *lp);

int gen_configs(ProblemInstance *input, FILE *outFile, glp_prob *lp)
{
    int retStatus = SUCCES_STATUS;
    size_t *maxQuantitiesInStock = calloc(input->numOfTypes, sizeof(*maxQuantitiesInStock));
    if (input == NULL || outFile == NULL)
    {
        retStatus = FAIL_STATUS;
        fprintf(stderr, "Out of memory\n");
    }
    else
    {
        retStatus = gen_configs_tree(input, lp);
    }

    return retStatus;
}

static inline int gen_configs_tree(ProblemInstance *input, glp_prob *lp)
{
    int retStatus = SUCCES_STATUS;
    size_t numOfTypes = input->numOfTypes;
    size_t stockLength = input->stockLength;
    StockConfig *col = malloc(sizeof(*col) + sizeof(*col->config) * numOfTypes);
    size_t sConfigSize = sizeof(*col) + sizeof(*col->config) * numOfTypes;
    StockConfig *prevCol = malloc(sConfigSize);
    size_t *lengths = calloc(numOfTypes, sizeof(*lengths));
    size_t *quantities = calloc(numOfTypes, sizeof(*lengths));
    int *ind = malloc(sizeof(*ind) * (numOfTypes + 1));
    double *val = malloc(sizeof(*val) * (numOfTypes + 1));

    if ((col == NULL) || (prevCol == NULL) || (lengths == NULL) ||
        (quantities == NULL) || (ind == NULL) || (val == NULL))
    {
        retStatus = FAIL_STATUS;
        fprintf(stderr, "Out of memory\n");
    }
    else
    {
        for (size_t i = 0; i < numOfTypes; i++)
        {
            lengths[i] = input->arrOfObjs[i].length;
            quantities[i] = input->arrOfObjs[i].quantity;
        }

        // Fill first col of matrix
        col->spaceLeft = stockLength;
        for (size_t i = 0; i < numOfTypes; i++)
        {
            col->config[i] = col->spaceLeft / lengths[i];
            if (col->config[i] > quantities[i])
            {
                col->config[i] = quantities[i];
            }
            col->spaceLeft = col->spaceLeft - col->config[i] * lengths[i];
        }

        // add firtst column to solver
        for (size_t idx = 1; idx <= numOfTypes; idx++)
        {
            ind[idx] = idx;
            val[idx] = (double)(col->config[idx - 1]);
        }
        glp_add_rows(lp, input->numOfTypes);
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
                if (col->config[i] > quantities[i])
                {
                    col->config[i] = quantities[i];
                }
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
    free(quantities);
    free(col);
    free(prevCol);
    free(ind);
    free(val);

    return retStatus;
}
