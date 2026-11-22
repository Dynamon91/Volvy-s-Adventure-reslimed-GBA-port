#include <assert.h>
#include "core/fixed.h"

int main(void)
{
    assert(fx_from_int(1) == FX_ONE);
    assert(fx_to_int(fx_from_int(7)) == 7);
    assert(fx_mul(fx_from_int(2), fx_from_int(3)) == fx_from_int(6));
    assert(fx_mul(fx_from_int(-2), fx_from_int(3)) == fx_from_int(-6));
    assert(fx_clamp(fx_from_int(12), fx_from_int(0), fx_from_int(10))
           == fx_from_int(10));
    assert(fx_clamp(fx_from_int(-2), fx_from_int(0), fx_from_int(10))
           == fx_from_int(0));
    return 0;
}
