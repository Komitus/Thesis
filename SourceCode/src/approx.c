#include "approx.h"
#include <stdio.h>
#include <stdint.h>

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

    for (size_t i = 0; i < input->numOfTypes; i++)
    {
        ObjWithQuantity currObj = input->arrOfObjs[i];

        size_t idxOfConfigToStart = 0;
        for (size_t j = 0; j < currObj.quantity; j++)
        {
            uint8_t fitted = 0;
            for (size_t k = idxOfConfigToStart; k < vector_size(v); k++)
            {
                StockConfig *item = (StockConfig *)(v->items[k]);
                if (item->spaceLeft >= currObj.length)
                {
                    item->config[i]++;
                    item->spaceLeft -= currObj.length;
                    fitted = 1;
                    break;
                }
            }

            if (!fitted) // new variable bcs i want to have cached iterator "k"
            {
                StockConfig *newConfig = calloc(1, sizeof(*newConfig) + sizeof(*newConfig->config) * numOfTypes);
                if (newConfig == NULL)
                {
                    fprintf(stderr, "Out of memory\n");
                    exit(EXIT_FAILURE);
                }
                newConfig->config[i]++;
                newConfig->spaceLeft = stockLength - currObj.length;
                if (vector_push(v, newConfig) == FAIL_STATUS)
                {
                    return SIZE_MAX;
                }
                idxOfConfigToStart = vector_size(v) - 1;
            }
        }
    }
    return vector_size(v);
}
