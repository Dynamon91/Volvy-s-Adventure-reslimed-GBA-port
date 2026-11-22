/* Standalone C runtime primitives for an experimental ARM7TDMI link. */
#include <stdint.h>
#include <stddef.h>
/* Byte stores to palette/VRAM are unsafe on GBA. Use 32-bit operations for
 * aligned copies/clears; the original engine's hardware callsites are 4B
 * aligned. Fallback byte paths remain for regular EWRAM and small fields. */
void *memcpy(void *dst, const void *src, size_t n) {
    uint8_t *d=dst; const uint8_t *s=src;
    if ((((uintptr_t)d|(uintptr_t)s)&3u)==0) {
        uint32_t *dw=(uint32_t *)d;
        const uint32_t *sw=(const uint32_t *)s;
        while(n>=4) { *dw++=*sw++; n-=4; }
        d=(uint8_t *)dw; s=(const uint8_t *)sw;
    }
    while(n--) *d++=*s++;
    return dst;
}
void *memset(void *dst, int c, size_t n) {
    uint8_t *d=dst;
    if (((uintptr_t)d&3u)==0) {
        uint32_t color=(uint8_t)c;
        color|=color<<8;
        color|=color<<16;
        uint32_t *dw=(uint32_t*)d;
        while(n>=4) { *dw++=color; n-=4; }
        d=(uint8_t*)dw;
    }
    while(n--) *d++=(uint8_t)c;
    return dst;
}
void *memmove(void *dst, const void *src, size_t n) {
    uint8_t *d=dst; const uint8_t *s=src;
    if (d<=s) { while(n--) *d++=*s++; }
    else { d+=n; s+=n; while(n--) *--d=*--s; }
    return dst;
}
void __aeabi_memcpy(void *d,const void *s,size_t n) { memcpy(d,s,n); }
void __aeabi_memcpy4(void *d,const void *s,size_t n) { memcpy(d,s,n); }
void __aeabi_memcpy8(void *d,const void *s,size_t n) { memcpy(d,s,n); }
void __aeabi_memset(void *d,size_t n,int c) { memset(d,c,n); }
void __aeabi_memset4(void *d,size_t n,int c) { memset(d,c,n); }
void __aeabi_memset8(void *d,size_t n,int c) { memset(d,c,n); }
void __aeabi_memclr(void *d,size_t n) { memset(d,0,n); }
void __aeabi_memclr4(void *d,size_t n) { memset(d,0,n); }
void __aeabi_memclr8(void *d,size_t n) { memset(d,0,n); }

/* Pure integer binary long division avoids depending on a newlib/compiler-rt
 * installation for ARMv4T (which has no hardware divide instruction). */
uint32_t __aeabi_uidiv(uint32_t numerator,uint32_t denominator) {
    uint32_t q=0,r=0;
    int shift;
    if (!denominator) return 0;
    for (shift=31;shift>=0;--shift) {
        /* A 33-bit remainder may arise when divisor >= 2^31. Carry out
         * of bit31 guarantees subtracting any 32-bit denominator. */
        uint32_t carry=r>>31;
        r=(r<<1)|((numerator>>(unsigned)shift)&1u);
        if (carry || r>=denominator) {
            r-=denominator;
            q|=1u<<(unsigned)shift;
        }
    }
    return q;
}
int32_t __aeabi_idiv(int32_t numerator,int32_t denominator) {
    uint32_t unsigned_num=(uint32_t)numerator, unsigned_den=(uint32_t)denominator;
    uint32_t q;
    if (!denominator) return 0;
    if (numerator<0) unsigned_num=0u-unsigned_num;
    if (denominator<0) unsigned_den=0u-unsigned_den;
    q=__aeabi_uidiv(unsigned_num,unsigned_den);
    return (numerator<0) != (denominator<0) ? (int32_t)(0u-q) : (int32_t)q;
}
/* ARM EABI divmod returns the quotient in r0 and remainder in r1.
 * Ordinary C aggregate return uses a hidden pointer for 8-byte structs;
 * therefore the ABI-specific divmod entrypoints live in divmod.s. */
