#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>

#include "io_handler.h"
#include "approx.h"
#include "gen_configs.h"

static size_t ALGO_ENUM_ARR[] = {APPROX, MIP}; // see structs.h

#define GENERATE_STRING(STRING) #STRING,
static const char *ALGO_STRING[] = {
    FOREACH_ALGO(GENERATE_STRING)};

static int handle_console_args(int argc, char *const *argv, IO_Info *io_info);
static int open_file(IO_Info *io_info, const char *flags);
static int read_input(ProblemInstance *input, FILE *inFile);
static int read_obj_types_to_struct(ProblemInstance *input, FILE *inFile);
static int print_arr_of_objs(ProblemInstance *input, FILE *outFile);
static int print_configs_from_vec(ProblemInstance *input, Vector *v, FILE *outFile);
static const int _qsort_compare(const void *a, const void *b);

int run_program(int argc, char *const *argv)
{
    int retStatus = FAIL_STATUS;
    IO_Info *io_info = malloc(sizeof(*io_info));
    ProblemInstance *input = malloc(sizeof(*input));

    if (io_info != NULL && input != NULL)
    {
        io_info->outFile = stdout;
        io_info->inFile = stdin;
        if (handle_console_args(argc, argv, io_info) != FAIL_STATUS &&
            open_file(io_info, "r") != FAIL_STATUS &&
            read_input(input, io_info->inFile) != FAIL_STATUS &&
            ((input->arrOfObjs = calloc(input->numOfTypes, sizeof(ObjWithQuantity))) != NULL) &&
            read_obj_types_to_struct(input, io_info->inFile) != FAIL_STATUS &&
            fclose(io_info->inFile) == 0 &&
            ((input->minNumbOfStocks = get_min_numb_of_stocks(input)) > 0) &&
            open_file(io_info, "w") != FAIL_STATUS &&
            print_arr_of_objs(input, io_info->outFile) != FAIL_STATUS)
        {
            fprintf(io_info->outFile, "SELECTED ALGO: ");
            if (io_info->options & APPROX)
            {
                fprintf(io_info->outFile, "APPROX\n");
                Vector *vec = malloc(sizeof(*vec));
                if (vec != NULL)
                {
                    vector_init(vec, input->minNumbOfStocks);
                    size_t stocksNeeded = approx(input, vec);
                    if (stocksNeeded != SIZE_MAX)
                    {
                        print_configs_from_vec(input, vec, io_info->outFile);
                        fprintf(io_info->outFile, "STOCKS NEEDED: %lu\n", stocksNeeded);
                    }
                    else
                    {
                        fprintf(stderr, "ERROR OCCURED IN APPROX\n");
                    }
                    retStatus = SUCCES_STATUS;
                    for (size_t i = 0; i < vector_size(vec); i++)
                    {
                        free(vec->items[i]);
                    }
                    vector_free(vec);
                }
                else
                {
                    fprintf(stderr, "MEMORY ERROR\n");
                }
            }
            else    
            {   
                fprintf(io_info->outFile, "MIP\n");
                glp_prob *lp = glp_create_prob(); // freeing in solver
                gen_configs(input, io_info->outFile, lp);
                retStatus = solver(input, io_info, lp);
                glp_delete_prob(lp);
                glp_free_env();
            }
            if (io_info->outFile != stdout)
            {
                fclose(io_info->outFile);
            }
        }
        else
        {
            fprintf(stderr, "ERROR READING INPUT\n");
        }
    }
    // cleanup
    if (input != NULL)
    {
        free(input->arrOfObjs);
        free(input);
    }
    free(io_info);

    return retStatus;
}

static int handle_console_args(int argc, char *const *argv, IO_Info *io_info)
{
    io_info->options = NO_OPTION;
    io_info->inFilename = NULL;
    io_info->outFilename = NULL;
    io_info->inFile = stdin;
    io_info->outFile = stdout;

    int c;
    opterr = 0;
    while ((c = getopt(argc, argv, "f:o:a:hq")) != -1)
    {
        switch (c)
        {
        case 'f':
            io_info->options |= FROM_FILE;
            io_info->inFilename = optarg;
            break;
        case 'o':
            io_info->options |= TO_FILE;
            io_info->outFilename = optarg;
            break;
        case 'a': // encoding
            for (size_t i = 0; i < sizeof(ALGO_ENUM_ARR) / sizeof(ALGO_ENUM_ARR[0]); i++)
            {
                size_t len = strcspn(ALGO_STRING[i], "=") - 1;
                if (strncmp(optarg, ALGO_STRING[i], len) == 0)
                {
                    io_info->options |= ALGO_ENUM_ARR[i];
                    break;
                }
            }
            break;
        case 'q':
            io_info->options |= QUIET_MODE;
            break;
        case 'h':
            fprintf(stderr, "Avail flags:\n-f <in_filename.txt>\n-o <out_filename.txt>-q (quiet mode - no configs)\n");
            fprintf(stderr, "-a <ALGO> ALGOS: \n");
            for (size_t i = 0; i < sizeof(ALGO_STRING) / sizeof(ALGO_STRING[0]); i++)
            {
                int len = strcspn(ALGO_STRING[i], "=");
                fprintf(stderr, "\t%.*s\n", len, ALGO_STRING[i]);
            }
            return FAIL_STATUS;
            break;
        case '?':
            if (optopt == 'f')
                fprintf(stderr, "Option -%c requires an input filename\n", optopt);
            else if (optopt == 'o')
                fprintf(stderr, "Option -%c requires output filename.\n", optopt);
            else if (optopt == 'a')
                fprintf(stderr, "Option -%c requires ALGO type.\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
            return FAIL_STATUS;
        default:
            abort();
        }
    }
    return 0;
}

static int read_input(ProblemInstance *input, FILE *inFile)
{
    if (input == NULL || inFile == NULL)
    {
        return FAIL_STATUS;
    }

    char line[256];

    const size_t numOfParams = 2;
    size_t params[numOfParams];
    const char *const paramsMsgs[] = {"stock length", "number of different objs"};

    for (size_t i = 0; i < numOfParams; i++)
    {
        while (1)
        {

            if (inFile == stdin)
            {
                printf("Type %s: \n", paramsMsgs[i]);
            }

            if (fgets(line, sizeof(line), inFile))
            {
                if (1 != sscanf(line, "%lu", &(params[i])))
                {
                    fprintf(stderr, "Wrong input, type again\n");
                    if (inFile == stdin)
                    {
                        continue;
                    }
                    else
                    {
                        return FAIL_STATUS;
                    }
                }
            }
            else
            {
                return FAIL_STATUS;
            }
            break;
        }
    }
    input->stockLength = params[0];
    input->numOfTypes = params[1];
    return SUCCES_STATUS;
}

/**
 * @brief reads speace separated length and quantity of object
 * each object in new line
 *
 * @param arrOfObjs has to be allocated before, is after filling
 * @param stockLength
 * @param numOfTypes
 * @return n reuturns n - number of all objects
 */
static int read_obj_types_to_struct(ProblemInstance *input, FILE *inFile)
{
    if (input == NULL || inFile == NULL || input->arrOfObjs == NULL)
    {
        return FAIL_STATUS;
    }
    char line[256];
    size_t p_i, n_i, n = 0;
    if (inFile == stdin)
    {
        printf("object lenght (p_i) *space* number of objects\n");
    }
    size_t i = 0;

    while (i < input->numOfTypes)
    {
        if (fgets(line, sizeof(line), inFile))
        {
            if (2 == sscanf(line, "%lu %lu", &p_i, &n_i))
            {
                if (p_i > input->stockLength)
                {

                    fprintf(stderr, "Item longer than stock length: %lu, type again\n", p_i);
                    if (inFile == stdin)
                    {
                        continue;
                    }
                    else
                    {
                        return FAIL_STATUS;
                    }
                }
                for (size_t j = 0; j < i; j++)
                {
                    if (input->arrOfObjs[j].length == p_i)
                    {
                        fprintf(stderr, "This length already exists: %lu\n", p_i);
                        if (inFile == stdin)
                        {
                            continue;
                        }
                        else
                        {
                            return FAIL_STATUS;
                        }
                    }
                }
                input->arrOfObjs[i].length = p_i;
                input->arrOfObjs[i].quantity = n_i;
                n += n_i;
                i++;
            }
            else
            {
                fprintf(stderr, "Wrong input, type again\n");
                if (inFile == stdin)
                {
                    continue;
                }
                else
                {
                    return FAIL_STATUS;
                }
            }
        }
        else
        {
            return FAIL_STATUS;
        }
    }
    qsort(input->arrOfObjs, input->numOfTypes, sizeof(ObjWithQuantity), _qsort_compare); // sort for distinction on big and small
    input->numOfAllObjs = n;
    return SUCCES_STATUS;
}

static int print_arr_of_objs(ProblemInstance *input, FILE *outFile)
{
    if (input == NULL || outFile == NULL)
    {
        return FAIL_STATUS;
    }

    fprintf(outFile, "STOCK_LENGTH: %lu\n", input->stockLength);
    fprintf(outFile, "NUM_OF_TYPES: %lu\n", input->numOfTypes);
 
    const char *const strs[] = {"LENGTH|", "QUANTITY|"};

    ObjWithQuantity *arrOfObjs = input->arrOfObjs;
    size_t n = sizeof(strs) / sizeof(strs[0]);
    for (size_t paramIdx = 0; paramIdx < n; paramIdx++)
    {
        fprintf(outFile, "%s", strs[paramIdx]);
        for (size_t i = 0; i < input->numOfTypes; i++)
        {
            size_t val;
            if (paramIdx == 0)
            {
                val = arrOfObjs[i].length;
            }
            else
            {
                val = arrOfObjs[i].quantity;
            }
            fprintf(outFile, "%lu|", val);
        }
        fprintf(outFile, "\n");
        for (size_t i = 0; i < input->numOfTypes * 6; i++)
        {
            fprintf(outFile, "-");
        }
        fprintf(outFile, "\n");
    }
    fprintf(outFile, "\n");
    return SUCCES_STATUS;
}

static int print_configs_from_vec(ProblemInstance *input, Vector *v, FILE *outFile)
{
    if (input == NULL || outFile == NULL)
    {
        return FAIL_STATUS;
    }

    for (size_t i = 0; i < vector_size(v); i++)
    {
        size_t filled = 0;
        fprintf(outFile, "C[%3lu]", i);
        const StockConfig *stock = (const StockConfig *)v->items[i];
        for (size_t j = 0; j < input->numOfTypes; j++)
        {
            filled = filled + (input->arrOfObjs[j].length * stock->config[j]);
            fprintf(outFile, "| %lu ", stock->config[j]);
        }
        fprintf(outFile, "|%% %4lu\n", stock->spaceLeft);
        assert(stock->spaceLeft == (input->stockLength - filled));
    }
    return SUCCES_STATUS;
}

static int open_file(IO_Info *io_info, const char *flags)
{
    if (strcmp(flags, "r") == 0 && (io_info->options & FROM_FILE))
    {
        if ((io_info->inFile = fopen(io_info->inFilename, "r")) == NULL)
        {
            return FAIL_STATUS;
        }
    }
    if (strcmp(flags, "w") == 0 && (io_info->options & TO_FILE))
    {
        if ((io_info->outFile = fopen(io_info->outFilename, "w")) == NULL)
        {
            return FAIL_STATUS;
        }
    }
    return SUCCES_STATUS;
}

static const int _qsort_compare(const void *a, const void *b)
{
    ObjWithQuantity _a = *(ObjWithQuantity *)a;
    ObjWithQuantity _b = *(ObjWithQuantity *)b;
    if (_a.length < _b.length)
        return 1;
    else if (_a.length == _b.length)
        return 0;
    else
        return -1;
}