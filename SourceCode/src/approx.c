#include "approx.h"
#include <stdio.h>
#include <stdint.h>
#ifdef TEST_ON
#include <assert.h>
#endif
#include "io_handler.h"

size_t get_min_numb_of_stocks(ProblemInstance *input)
{
    size_t sum = 0;
    for (size_t i = 0; i < input->numOfTypes; i++)
    {
        sum += input->arrOfObjs[i].length * input->arrOfObjs[i].quantity;
    }

    return (1 + ((sum - 1) / input->stockLength));
}

size_t approx(ProblemInstance *input, Vector *v)
{
    size_t stockLength = input->stockLength;
    size_t numOfTypes = input->numOfTypes;

    for (size_t i = 0; i < numOfTypes; i++)
    {
        ObjWithQuantity currObj = input->arrOfObjs[i];

        size_t k = 0;
        for (size_t j = 0; j < currObj.quantity; j++)
        {

            while (k < vector_size(v) && ((StockConfig *)(v->items[k]))->spaceLeft < currObj.length)
            {
                k++;
            }

            if (k < vector_size(v))
            {
                ((StockConfig *)(v->items[k]))->config[i]++;
                ((StockConfig *)(v->items[k]))->spaceLeft -= currObj.length;
            }
            else
            {
                StockConfig *newStock = calloc(1, sizeof(*newStock) + sizeof(*newStock->config) * numOfTypes);
                if (newStock == NULL)
                {
                    fprintf(stderr, "Out of memory\n");
                    exit(EXIT_FAILURE);
                }
                newStock->config[i]++;
                newStock->spaceLeft = stockLength - currObj.length;
                if (vector_push(v, newStock) == FAIL_STATUS)
                {
                    return SIZE_MAX;
                }
            }
        }
    }
    return vector_size(v);
}

void check_approx(Vector *v, size_t numOfTypes, size_t *objQuantities)
{
    for (size_t i = 0; i < vector_size(v); i++)
    {
        StockConfig *col = vector_get(v, i);
        for (size_t j = 0; j < numOfTypes; j++)
        {
            if (objQuantities[j] > col->config[i])
            {
                objQuantities[j] -= col->config[i];
            }
            else
            {
                objQuantities[j] = 0;
            }
        }
    }

    for (size_t i = 0; i < numOfTypes; i++)
    {
        assert(objQuantities[i] == 0);
    }
}