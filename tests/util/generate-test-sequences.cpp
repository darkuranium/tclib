#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>
#include <random>

using namespace std;

void gen_seed_seq()
{
    uint32_t gendata[1];

    seed_seq ss{UINT32_C(0)};
    ss.generate(gendata, gendata + sizeof(gendata) / sizeof(*gendata));
    for(size_t i = 0; i < sizeof(gendata) / sizeof(*gendata); i++)
    {
        if(i && !(i%4)) printf("\n");
        printf("UINT32_C(%" PRIu32 "),", gendata[i]);
    }
    printf("\n");
}

void gen_mt()
{
    minstd_rand mt;
    for(size_t i = 0; i < 32; i++)
    {
        if(i && !(i%4)) printf("\n");
        printf("UINT32_C(%" PRIu32 "),", mt());
    }
    printf("\n");
}

int main()
{
    gen_seed_seq();

    return 0;
}
