/* Host tests for the experimental bare-metal CRT, not GBA execution. */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>

extern uint32_t __aeabi_uidiv(uint32_t, uint32_t);
extern int32_t __aeabi_idiv(int32_t, int32_t);
extern void *memcpy(void *, const void *, size_t);
extern void *memset(void *, int, size_t);
extern void *memmove(void *, const void *, size_t);

static const uint32_t uvals[] = {
    0u, 1u, 2u, 7u, 15u, 255u, 1024u, 65535u, 0x12345678u,
    0x7fffffffu, 0x80000000u, 0x80000001u, 0xfffffffeu, 0xffffffffu
};
static const int32_t svals[] = {
    INT32_MIN, INT32_MIN+1, -65535, -9, -2, -1, 0, 1, 2, 7, 100, 65535, INT32_MAX
};
int main(void) {
    unsigned int i,j;
    uint32_t a[16]={0},b[16]={0};
    unsigned char *aa=(unsigned char *)a,*bb=(unsigned char *)b;
    for (i=0;i<sizeof(uvals)/sizeof(uvals[0]);++i)
        for (j=0;j<sizeof(uvals)/sizeof(uvals[0]);++j) {
            uint32_t d=uvals[j];
            if (d) assert(__aeabi_uidiv(uvals[i],d)==uvals[i]/d);
        }
    for (i=0;i<sizeof(svals)/sizeof(svals[0]);++i)
        for (j=0;j<sizeof(svals)/sizeof(svals[0]);++j) {
            int32_t d=svals[j];
            if (d && !(svals[i]==INT32_MIN && d==-1))
                assert(__aeabi_idiv(svals[i],d)==svals[i]/d);
        }
    memset(a,0xA5,sizeof(a));
    for (i=0;i<sizeof(a);++i) assert(aa[i]==0xA5);
    memcpy(b,a,sizeof(a));
    for (i=0;i<sizeof(a);++i) assert(bb[i]==0xA5);
    aa[2]=1;
    memmove(aa+4,aa,16);
    assert(aa[6]==1 && aa[2]==1);
    puts("Experimental CRT host division / aligned-copy / clear checks: PASS");
    return 0;
}
