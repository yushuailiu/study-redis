//
// Created by YuShuaiLiu on 2018/11/30.
//

#ifndef REDIS_SDS_H
#define REDIS_SDS_H

#include "stdio.h"
#include <stdint.h>

/* 定义 sdshdr 预分配空间大小 */
#define SDS_MAX_PREALLOC (1024*1024)

/*
 * 空字符串标识
 */
const char *SDS_NOINIT = "SDS_NOINIT";

typedef char *sds;

struct __attribute__ ((__packed__)) sdshdr5 {
  unsigned char flags; /* 低三位标识类型，高五位是字符串长度 */
  char buf[];
};
struct __attribute__ ((__packed__)) sdshdr8 {
  uint8_t len; /* 当前值包含字符串的长度 */
  uint8_t alloc; /* 可容纳字符串的长度 */
  unsigned char flags; /* 低三位标识类型，高五位未使用 */
  char buf[];
};
struct __attribute__ ((__packed__)) sdshdr16 {
  uint16_t len; /* 当前值包含字符串的长度 */
  uint16_t alloc; /* 可容纳字符串的长度 */
  unsigned char flags; /* 低三位标识类型，高五位未使用 */
  char buf[];
};
struct __attribute__ ((__packed__)) sdshdr32 {
  uint32_t len; /* 当前值包含字符串的长度 */
  uint32_t alloc; /* 可容纳字符串的长度 */
  unsigned char flags; /* 低三位标识类型，高五位未使用 */
  char buf[];
};
struct __attribute__ ((__packed__)) sdshdr64 {
  uint64_t len; /* 当前值包含字符串的长度 */
  uint64_t alloc; /* 可容纳字符串的长度 */
  unsigned char flags; /* 低三位标识类型，高五位未使用 */
  char buf[];
};

/* 定义五种字符串类型标识 */
#define SDS_TYPE_5  0
#define SDS_TYPE_8  1
#define SDS_TYPE_16 2
#define SDS_TYPE_32 3
#define SDS_TYPE_64 4

/* 判断 sdshdr 类型的时候用 */
#define SDS_TYPE_MASK 7

#define SDS_TYPE_BITS 3
/* 用于声明 */
#define SDS_HDR_VAR(T,s) struct sdshdr##T *sh = (void*)((s)-(sizeof(struct sdshdr##T)));
#define SDS_HDR(T,s) ((struct sdshdr##T *)((s)-(sizeof(struct sdshdr##T))))
#define SDS_TYPE_5_LEN(f) ((f)>>SDS_TYPE_BITS)

// 返回 SDS len
static inline size_t sdslen(const sds s) {
  unsigned char flags = s[-1];
  switch(flags&SDS_TYPE_MASK) {
    case SDS_TYPE_5:
      return SDS_TYPE_5_LEN(flags);
    case SDS_TYPE_8:
      return SDS_HDR(8,s)->len;
    case SDS_TYPE_16:
      return SDS_HDR(16,s)->len;
    case SDS_TYPE_32:
      return SDS_HDR(32,s)->len;
    case SDS_TYPE_64:
      return SDS_HDR(64,s)->len;
  }
  return 0;
}

// 返回 SDS 还可容下字符数
static inline size_t sdsavail(const sds s) {
  unsigned char flags = s[-1];
  switch(flags&SDS_TYPE_MASK) {
    case SDS_TYPE_5: {
      return 0;
    }
    case SDS_TYPE_8: {
      SDS_HDR_VAR(8,s);
      return sh->alloc - sh->len;
    }
    case SDS_TYPE_16: {
      SDS_HDR_VAR(16,s);
      return sh->alloc - sh->len;
    }
    case SDS_TYPE_32: {
      SDS_HDR_VAR(32,s);
      return sh->alloc - sh->len;
    }
    case SDS_TYPE_64: {
      SDS_HDR_VAR(64,s);
      return sh->alloc - sh->len;
    }
  }
  return 0;
}
// 设置 SDS len
static inline void sdssetlen(sds s, size_t newlen) {
  unsigned char flags = s[-1];
  switch(flags&SDS_TYPE_MASK) {
    case SDS_TYPE_5:
    {
      unsigned char *fp = ((unsigned char*)s)-1;
      *fp = SDS_TYPE_5 | (newlen << SDS_TYPE_BITS);
    }
      break;
    case SDS_TYPE_8:
      SDS_HDR(8,s)->len = newlen;
      break;
    case SDS_TYPE_16:
      SDS_HDR(16,s)->len = newlen;
      break;
    case SDS_TYPE_32:
      SDS_HDR(32,s)->len = newlen;
      break;
    case SDS_TYPE_64:
      SDS_HDR(64,s)->len = newlen;
      break;
  }
}
// 修改 SDS len 增加 inc
static inline void sdsinclen(sds s, size_t inc) {
  unsigned char flags = s[-1];
  switch(flags&SDS_TYPE_MASK) {
    case SDS_TYPE_5:
    {
      unsigned char *fp = ((unsigned char*)s)-1;
      unsigned char newlen = SDS_TYPE_5_LEN(flags)+inc;
      *fp = SDS_TYPE_5 | (newlen << SDS_TYPE_BITS);
    }
      break;
    case SDS_TYPE_8:
      SDS_HDR(8,s)->len += inc;
      break;
    case SDS_TYPE_16:
      SDS_HDR(16,s)->len += inc;
      break;
    case SDS_TYPE_32:
      SDS_HDR(32,s)->len += inc;
      break;
    case SDS_TYPE_64:
      SDS_HDR(64,s)->len += inc;
      break;
  }
}

// 返回 SDS alloc
static inline size_t sdsalloc(const sds s) {
  unsigned char flags = s[-1];
  switch(flags&SDS_TYPE_MASK) {
    case SDS_TYPE_5:
      return SDS_TYPE_5_LEN(flags);
    case SDS_TYPE_8:
      return SDS_HDR(8,s)->alloc;
    case SDS_TYPE_16:
      return SDS_HDR(16,s)->alloc;
    case SDS_TYPE_32:
      return SDS_HDR(32,s)->alloc;
    case SDS_TYPE_64:
      return SDS_HDR(64,s)->alloc;
  }
  return 0;
}

// 设置 SDS alloc 值
static inline void sdssetalloc(sds s, size_t newlen) {
  unsigned char flags = s[-1];
  switch(flags&SDS_TYPE_MASK) {
    case SDS_TYPE_5:
      break;
    case SDS_TYPE_8:
      SDS_HDR(8,s)->alloc = newlen;
      break;
    case SDS_TYPE_16:
      SDS_HDR(16,s)->alloc = newlen;
      break;
    case SDS_TYPE_32:
      SDS_HDR(32,s)->alloc = newlen;
      break;
    case SDS_TYPE_64:
      SDS_HDR(64,s)->alloc = newlen;
      break;
  }
}

sds sdsnewlen(const void *init, size_t initlen);
// 根据一个字符串指针创建一个 SDS
sds sdsnew(const char *init);
// 申请一个值为空的 SDS
sds sdsempty(void);
// 复制 SDS
sds sdsdup(const sds s);
// 释放一个 SDS
void sdsfree(sds s);
// 为 SDS 申请空间并设置新增空间值为 0
sds sdsgrowzero(sds s, size_t len);
// 复制字符串 t 的前 len 字符并拼接到 SDS s 后边
sds sdscatlen(sds s, const void *t, size_t len);
// 把字符串 t 拼接到 SDS s 后边
sds sdscat(sds s, const char *t);
// 把 SDS t 拼接到 s 后边
sds sdscatsds(sds s, const sds t);
// 复制 t 前 len 个字符到 s
sds sdscpylen(sds s, const char *t, size_t len);
// 复制字符串 t 到 s
sds sdscpy(sds s, const char *t);

// 使用 字符串格式化 fmt 和参数列表 ap 解析成新的字符串并拼接到 s 后面
sds sdscatvprintf(sds s, const char *fmt, va_list ap);
#ifdef __GNUC__
// 使用参数列表把格式化字符串拼接到 s 后面
sds sdscatprintf(sds s, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
#else
// 使用参数列表把格式化字符串拼接到 s 后面
sds sdscatprintf(sds s, const char *fmt, ...);
#endif
// 自定义的格式化字符串并拼接到 s 后边
sds sdscatfmt(sds s, char const *fmt, ...);
// 根据字符数组删除字符串前后的相应字符
sds sdstrim(sds s, const char *cset);
// 根据起始位置 start end 截取 SDS
void sdsrange(sds s, ssize_t start, ssize_t end);
// 使用 strlen(s) 更新 s 所对应的 SDS.len，这个redis 源码注释说用于 hack 更新
// SDS 的长度（当手动修改字符串中字符时），目前感觉这个方法没啥用还会存在潜在的风险
// redis 当前源码中还没有地方调用这个函数
void sdsupdatelen(sds s);
// 设置 SDS 为空，但是没有释放已经申请的字符空间，可供后续使用
void sdsclear(sds s);
// 一个字节一个字节的比较两个 SDS 的值
int sdscmp(const sds s1, const sds s2);
// 查找 字符串 sep 的前 seplen 的字符串对字符串 s 的 前 len 个字符组成的字符串进行分割，并返回分割子串列表（SDS 格式）
sds *sdssplitlen(const char *s, ssize_t len, const char *sep, int seplen, int *count);
// 释放 sds split 返回的结果
void sdsfreesplitres(sds *tokens, int count);
// SDS 各个字母转化为小写字母
void sdstolower(sds s);
// SDS 各个字母转化为大写字母
void sdstoupper(sds s);
sds sdsfromlonglong(long long value);
// 把字符串 p 拼接到 s 后边，对 p 中转义字符转化为字面表达，如字符 '\n' 替换成 "\n" 字符串
sds sdscatrepr(sds s, const char *p, size_t len);
// 解析 sdscatrepr 生成的字符串，并返回多个子串
sds *sdssplitargs(const char *line, int *argc);
// SDS 字符替换
sds sdsmapchars(sds s, const char *from, const char *to, size_t setlen);
// 用 sep 连接多个 字符串，返回 SDS
sds sdsjoin(char **argv, int argc, char *sep);
// 用 sep 连接多个 SDS，返回 SDS
sds sdsjoinsds(sds *argv, int argc, const char *sep, size_t seplen);

// 为 SDS 申请空间以保证可以再容纳 addlen 个字符
sds sdsMakeRoomFor(sds s, size_t addlen);
// 用于手动指定 SDS 的长度，适用于手动修改 SDS 中字符串数组的值，和 sdsupdatelen 场景差不多，
void sdsIncrLen(sds s, ssize_t incr);
// 删除 SDS 中 buf 包含的多余空间
sds sdsRemoveFreeSpace(sds s);
// 返回 SDS 占用的所有空间
size_t sdsAllocSize(sds s);
// 返回 SDS 结构的指针(s实际上是 buf 的指针)
void *sdsAllocPtr(sds s);

// 封装 SDS 内存申请
void *sds_malloc(size_t size);
// 封装 SDS 内存重新申请
void *sds_realloc(void *ptr, size_t size);
// 封装 SDS 内存释放
void sds_free(void *ptr);

#ifdef REDIS_TEST
int sdsTest(int argc, char *argv[]);
#endif

#endif //REDIS_SDS_H
