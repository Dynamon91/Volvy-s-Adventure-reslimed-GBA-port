#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "core/result_text.h"

int main(void)
{
    char value[11];
    result_text_uint32(0u, value);
    assert(strcmp(value,"0")==0);
    result_text_uint32(9u, value);
    assert(strcmp(value,"9")==0);
    result_text_uint32(1009u, value);
    assert(strcmp(value,"1009")==0);
    result_text_uint32(UINT32_MAX, value);
    assert(strcmp(value,"4294967295")==0);
    assert(strcmp(result_text_heading(4u),"SHREWY DEFEATED")==0);
    assert(strcmp(result_text_heading(1u),"STAGE CLEAR")==0);
    assert(strcmp(result_text_heading(3u),"STAGE CLEAR")==0);
    assert(strcmp(result_text_heading(5u),"STAGE CLEAR")==0);
    result_text_uint32(8u, 0);
    puts("Result text: PASS (exact integer totals and source-confirmed boss heading)");
    return 0;
}
