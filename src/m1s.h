#ifndef _M1S_H
#define _M1S_H

/*
 VERSION 0.0.4
  
 This is a base library made by m1cha1s.
 Features:
 - Nicer types
 - Utility macros
 - Vectors (like in math)
 - Arena allocators
 - Dynamic arrays (STB style)
 - Sized strings
 */

// NOTE(m1cha1s): Is not necessarely required...
#include <stdint.h>
#include <stddef.h>

#if !defined(M1S_ALLOC) || !defined(M1S_FREE)
#include <stdlib.h>
#endif

#ifndef M1S_ALLOC
#define M1S_ALLOC(s) malloc(s)
#endif

#ifndef M1S_FREE
#define M1S_FREE(p) free(p)
#endif

/* --- Some typedefs --- */

typedef  int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef  uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef size_t usize;

typedef float  f32;
typedef double f64;

typedef  s8 b8;
typedef s16 b16;
typedef s32 b32;
typedef s64 b64;

#define true 1
#define false 0

typedef struct v2
{
    f32 x,y;
} Vec2;

typedef struct v3
{
    f32 x,y,z;
} Vec3;

#define V2(x,y) ((Vec2){(x),(y)})
#define V3(x,y,z) ((Vec3){(x),(y),(z)})

#define Vec2Add(a, b) ((Vec2){.x=(a).x+(b).x, .y=(a).y+(b).y})

typedef struct Color
{
    f32 r,g,b,a;
} Color;

Color ColorInvert(Color);

typedef struct Quad
{
    Vec2 p0, p1;
} Quad;

/* --- Utils --- */

#define Abs(val) ((val) < 0 ? -(val) : (val))
#define Min(a,b) ((a) < (b) ? (a) : (b))
#define Max(a,b) ((a) > (b) ? (a) : (b))
#define Len(arr) (sizeof(arr)/sizeof(*(arr)))
#define ToLower(c) (((c) >= 'A' && (c) <= 'Z') ? ((c)+32) : (c))
#define ToUpper(c) (((c) >= 'a' && (c) <= 'z') ? ((c)-32) : (c))
#define IsWhitespace(c) ((c) == ' ' || (c) == '\t' || (c) == '\n')

#define STR2(s) #s
#define STR(s) STR2(s)
#define JOIN2(a,b) a##b
#define JOIN(a,b) JOIN2(a,b)

/* --- Memory --- */

#ifndef M1S_DEFAULT_ARENA_SIZE
#define M1S_DEFAULT_ARENA_SIZE 1024
#endif

typedef struct ArenaBlock ArenaBlock;
struct ArenaBlock {
    u8 *block;
    usize size;
    usize end;
    ArenaBlock *next;
};

typedef struct Arena {
    ArenaBlock *first;
    ArenaBlock *current;
} Arena;

void ArenaFree(Arena *arena);

void *ArenaAlloc(Arena *arena, usize size);
void ArenaReset(Arena *arena);

void MemorySet(void *buff, usize size, u8 val);
void MemoryMove(void *to, void *from, usize size);

/* --- Dynamic array (based on stb_ds) --- */

typedef struct ArrayHeader
{
    usize cap;
    usize len;
} ArrayHeader;

void ArrayGrow(void *arr, usize elemSize, usize addLen, usize minCap);

#define ArrayHdr(arr) ((ArrayHeader*)(arr)-1)
#define ArrayCap(arr) ((arr) ? ArrayHdr(arr)->cap : 0)
#define ArrayLen(arr) ((arr) ? ArrayHdr(arr)->len : 0)
#define ArrayPush(arr, val) (ArrayMaybeGrow(arr, 1), (arr)[ArrayHdr(arr)->len++] = (val))
#define ArrayPop(arr) (ArrayLen(arr) ? ArrayHdr(arr)->len-- : 0, (arr)[ArrayLen(arr)])
#define ArrayAddNPtr(arr, ptr, n) (ArrayMaybeGrow(arr, n), MemoryMove(arr+ArrayLen(arr), ptr, sizeof(*(arr))*n), ArrayHdr(arr)->len+=n)
#define ArrayReset(arr) ((arr) ? ArrayHdr(arr)->len=0 : 0)
#define ArrayInsert(arr, val, idx) (ArrayMaybeGrow(arr, 1), MemoryMove(arr+idx+1, arr+idx, (ArrayLen(arr)-idx)*sizeof(*(arr))), arr[idx]=val, ++ArrayHdr(arr)->len)
#define ArrayRemove(arr, idx) (MemoryMove(arr+idx, arr+idx+1, (ArrayLen(arr)-1-idx)*sizeof(*(arr))),--ArrayHdr(arr)->len)
#define ArrayMaybeGrow(arr, n) (ArrayGrow(&(arr), sizeof(*(arr)), (n), 0),0)
#define ArrayFree(arr) ((arr) ? M1S_FREE(ArrayHdr(arr)) : 0)

/* --- Strings --- */

typedef struct String
{
    u8 *str;
    usize len;
} String;

#define StringLit(lit) ((String){.str=(u8*)(lit), .len=sizeof(lit)-1})

usize CStrLen(u8 *cstr);

String CStrToString(Arena *arena, u8 *cstr);
u8 *StringToCStr(Arena *arena, String string);

String NumToString_s64(Arena *arena, s64 num, s32 digits);
String NumToString_u64(Arena *arena, u64 num, s32 digits);
String NumToString_f64(Arena *arena, f64 num, s32 digits, s32 precision);

String StringConcat(Arena *arena, String a, String b);
String MoveString(Arena *arena, String a);

#ifdef M1S_IMPLS

/* --- Types --- */

Color ColorInvert(Color c)
{
    return (Color){1-c.r, 1-c.g, 1-c.b, c.a};
}

/* --- Memory --- */

static void ArenaBlockDeinit(ArenaBlock *b) 
{
    if (!b) return;
    M1S_FREE(b->block);
    ArenaBlockDeinit(b->next);
    M1S_FREE(b);
}

void ArenaFree(Arena *arena)
{
    ArenaBlockDeinit(arena->first);
}

void *ArenaAlloc(Arena *arena, usize size)
{
    if ((!arena->first) || (!arena->current))
    {
        arena->first = arena->current = M1S_ALLOC(sizeof(ArenaBlock));
        usize size = M1S_DEFAULT_ARENA_SIZE;
        arena->first->block = M1S_ALLOC(size);
        arena->first->size = size;
        arena->first->end = 0;
        arena->first->next = NULL;
    }

    while (arena->current->end+size > arena->current->size)
    {
        if (!arena->current->next)
        {
            arena->current->next = M1S_ALLOC(sizeof(ArenaBlock));
            
            usize s = (size > M1S_DEFAULT_ARENA_SIZE) ? size : M1S_DEFAULT_ARENA_SIZE;
            arena->current->next->block = M1S_ALLOC(s);
            arena->current->next->size = s;
            arena->current->next->end = 0;
            arena->current->next->next = NULL;
        }
        arena->current = arena->current->next;
    }
    
    void *ptr = arena->current->block+arena->current->end;
    arena->current->end += size;
    
    return ptr;
}

void ArenaReset(Arena *arena)
{
    ArenaBlock *b = arena->first;
    while(b)
    {
        b->end = 0;
        b = b->next;
    }
    arena->current = arena->first;
}

void MemorySet(void *Buff, usize size, u8 val)
{
    u8 *buff = (u8*)Buff;
    
    for (usize i = 0; i < size; ++i) buff[i]=val;
}

void MemoryMove(void *To, void *From, usize size)
{
    u8 *to = (u8*)To;
    u8 *from = (u8*)From;
    
    if (to > from)
    {
        for (usize i = 0; i < size; ++i)
        {
            to[size-i-1] = from[size-i-1];
        }
    } 
    else 
    {
        for (usize i = 0; i < size; ++i)
        {
            to[i] = from[i];
        }
    }
}

/* --- Dynamic array (based on STB) --- */

void ArrayGrow(void *arr, usize elemSize, usize addLen, usize minCap)
{

    if (!elemSize) return;

    minCap = Max(minCap, 8);

    void *a = (*(void**)arr);

    if (!a)
    {
        usize minLen = addLen;
        if (minLen > minCap) minCap = minLen;

        a = M1S_ALLOC(elemSize*minCap+sizeof(ArrayHeader));
        a = ((ArrayHeader*)a)+1;
        ArrayHdr(a)->cap = minCap;
        ArrayHdr(a)->len = 0;
        (*(void**)arr) = a;
        return;
    }

    u64 minLen = ArrayLen(a)+addLen;
    if (minLen > minCap) minCap = minLen;
    if (minLen <= ArrayCap(a)) return;

    if (minCap < ArrayCap(a)) minCap = ArrayCap(a);

    void *temp = NULL;
    temp = M1S_ALLOC(elemSize*(minCap*2)+sizeof(ArrayHeader));
    temp = ((ArrayHeader*)temp)+1;
    ArrayHdr(temp)->cap = (minCap*2);
    ArrayHdr(temp)->len = (ArrayLen(a));
    MemoryMove(temp, a, elemSize*ArrayLen(a));
    M1S_FREE(ArrayHdr(a));
    (*(void**)arr) = temp;
}

/* --- Strings --- */

usize CStrLen(u8 *cstr)
{
    usize i;
    for (i = 0; *cstr; ++i, ++cstr);
    return i;
}

String CStrToString(Arena *arena, u8 *cstr)
{
    u64 size = CStrLen(cstr);
    String s = {0};
    s.str = (u8*)ArenaAlloc(arena, size);
    s.len = size;
    MemoryMove(s.str, cstr, size);
    return s;
}

u8 *StringToCStr(Arena *arena, String string)
{
    u8 *cstr = (u8*)ArenaAlloc(arena, string.len+1);
    MemorySet(cstr, string.len+1, 0);
    MemoryMove(cstr, string.str, string.len);
    return cstr;
}

String NumToString_s64(Arena *arena, s64 num, s32 digits)
{
    s64 temp = num; 
    
    s64 i = 0;
    while (temp)
    {
        i++;
        temp /= 10;
    }
    
    u64 len = i < digits ? digits : i;
    
    String str = {.str=(u8*)ArenaAlloc(arena, len), .len=len};
    
    temp = num;
    
    i = len-1;
    while (temp)
    {
        str.str[i] = (temp % 10) + '0';
        temp /= 10;
        --i;
    }
    
    while (i >= 0)
    {
        str.str[i--] = ' ';
    }
    
    return str;
}

String NumToString_u64(Arena *arena, u64 num, s32 digits)
{
    u64 temp = num; 
    
    s64 i = 0;
    while (temp)
    {
        i++;
        temp /= 10;
    }
    
    usize len = i < digits ? digits : i;
    
    String str = {.str=(u8*)ArenaAlloc(arena, len), .len=len};
    
    temp = num;
    
    i = len-1;
    while (temp)
    {
        str.str[i] = (temp % 10) + '0';
        temp /= 10;
        --i;
    }
    
    while (i >= 0)
    {
        str.str[i--] = ' ';
    }
    
    return str;
}

String NumToString_f64(Arena *arena, f64 num, s32 digits, s32 precision)
{
    s64 ipart = (s64)num; 
    
    f64 fpart = num - (f64)ipart; 
    
    
    s64 temp = ipart; 
    
    s64 i = 0;
    while (temp)
    {
        i++;
        temp /= 10;
    }
    
    usize len = i < digits ? digits : i;
    //len += precision + 1;
    
    String str = {.str=(u8*)ArenaAlloc(arena, len+precision+1), .len=len+precision+1};
    
    temp = ipart;
    
    i = len-1;
    while (temp)
    {
        str.str[i] = (temp % 10) + '0';
        temp /= 10;
        --i;
    }
    
    while (i >= 0)
    {
        str.str[i--] = ' ';
    }
    
    if (precision != 0) { 
        str.str[len] = '.'; // add dot 
        
        for (usize j = 0; j < precision; ++j)
            fpart *= 10;
        
        temp = (s64)fpart;
        
        i = len+precision;
        while (temp)
        {
            str.str[i] = (temp % 10) + '0';
            temp /= 10;
            --i;
        }
    }
    
    return str;
}

String StringConcat(Arena *arena, String a, String b)
{
    String concated = {0};

    concated.str = (u8*)ArenaAlloc(arena, a.len+b.len);
    concated.len = a.len + b.len;
    MemoryMove(concated.str, a.str, a.len);
    MemoryMove(concated.str+a.len, b.str, b.len);

    return concated;
}

String MoveString(Arena *arena, String a)
{
    String new = {0};
    new.str = (u8*)ArenaAlloc(arena, a.len);
    MemoryMove(new.str, a.str, a.len);
    new.len = a.len;
    return new;
}

#endif

#endif
