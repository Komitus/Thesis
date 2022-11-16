#include "io_handler.h"
#include "approx.h"
#include "gen_configs.h"
#include <float.h>

int main(int argc, char **argv)
{
    int retVal = run_program(argc, argv);
    if(retVal == FAIL_STATUS){
        fprintf(stderr, "\nERROR OCCURED DURING EXECUTION\n");
    }
    return retVal;
}