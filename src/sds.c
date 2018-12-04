//
// Created by YuShuaiLiu on 2018/11/30.
//
#include <stdio.h>

/*
 * 用要保存的字符串和容量创建 SDS sdsnewlen("hello", 5)
 */

sds sdsnewlen(const void *init, size_t initlen) {
  void *sh;
  sds s;
  // 获得合适的 sds 类型
  char type = sdsReqType(initlen);
  // 如果申请的容量是0那么就把 sds 类型设置为 SDS_TYPE_8，因为一般空字符串的值都是用来拼接的，这里算是一种优化
  if (type == SDS_TYPE_5 && initlen == 0 ) type = SDS_TYPE_8;

  int hdrlen = sdsHdrSize(type);

  // flags 字段指针
  unsigned char *fp;

  // 申请内存
  sh = s_malloc(hdrlen + initlen + 1);

  if (init == SDS_NOINIT) {
    init = NULL;
  } else if(!init) {
    memset(sh, 0, hdrlen + initlen + 1);
  }

  if (sh == NULL) return NULL;

  s = (char*)sh + hdrlen;

  fp = ((unsigned char*)s) - 1;

  switch (type) {
    case SDS_TYPE_5: {
      *fp = type | (initlen << SDS_TYPE_BITS);
      break;
    }
    case SDS_TYPE_8: {
      SDS_HDR_VAR(8, s);
      sh->len = initlen;
      sh->alloc = initlen;
      *fp = type;
    }
    case SDS_TYPE_16: {
      SDS_HDR_VAR(16,s);
      sh->len = initlen;
      sh->alloc = initlen;
      *fp = type;
      break;
    }
    case SDS_TYPE_32: {
      SDS_HDR_VAR(32,s);
      sh->len = initlen;
      sh->alloc = initlen;
      *fp = type;
      break;
    }
    case SDS_TYPE_64: {
      SDS_HDR_VAR(64,s);
      sh->len = initlen;
      sh->alloc = initlen;
      *fp = type;
      break;
    }
  }

  if (initlen && init) {
    memcpy(s, init, initlen);
  }
  s[initlen] = '\0';
  return s;
}

// 根据 SDS 编号类型返回响应类型的 大小
static inline int sdsHdrSize(char type) {
  switch (type & SDS_TYPE_MASK) {
    case SDS_TYPE_5:
      return sizeof(struct sdshdr5);
    case SDS_TYPE_8:
      return sizeof(struct sdshdr8);
    case SDS_TYPE_16:
      return sizeof(struct sdshdr16);
    case SDS_TYPE_32:
      return sizeof(struct sdshdr32);
    case SDS_TYPE_64:
      return sizeof(struct sdshdr64);
  }
  return 0;
}

/*
 * 根据容量返回合适的 SDS 类型
 */
static inline char sdsReqType(size_t string_size) {
  if (string_size < 1 << 5) {
    return SDS_TYPE_5;
  }
  if (string_size < 1 << 8) {
    return SDS_TYPE_8;
  }
  if (string_size < 1 << 16) {
    return SDS_TYPE_16;
  }
  /*
   * 这里不懂，LONG_MAX == LLONG_MAX 有什么意义？？我 mac 上的 long 和 long long 都是8字节大小
   */
#if (LONG_MAX == LLONG_MAX)
  if (string_size < 1ll<<32)
    return SDS_TYPE_32;
  return SDS_TYPE_64;
#else
  return SDS_TYPE_32;
#endif
}

// 申请一个值为空的 SDS
sds sdsempty(void) {
  return sdsnewlen("", 0);
}

// 根据一个字符串指针创建一个 SDS
sds sdsnew(const char *init) {
  size_t initlen = (init == NULL) ? 0 : strlen(init);
  return sdsnewlen(init, initlen);
}

// 复制 SDS
sds sdsdup(const sds s) {
  return sdsnewlen(s, sdslen(s));
}

// 释放一个 SDS
void sdsfree(sds s) {
  if (s == NULL) return;
  // 具体释放逻辑
  s_free((char*)s-sdsHdrSize(s[-1]));
}

// 使用 strlen(s) 更新 s 所对应的 SDS.len，这个redis 源码注释说用于 hack 更新
// SDS 的长度（当手动修改字符串中字符时），目前感觉这个方法没啥用还会存在潜在的风险
// redis 当前源码中还没有地方调用这个函数
void sdsupdatelen(sds s) {
  size_t reallen = strlen(s);
  sdssetlen(s, reallen);
}
// 设置 SDS 为空，但是没有释放已经申请的字符空间，可供后续使用
void sdsclear(sds s) {
  sdssetlen(s, 0);
  s[0] = '\0';
}

// 为 SDS 申请空间以保证可以再容纳 addlen 个字符
sds sdsMakeRoomFor(sds s, size_t addlen) {
  void *sh, *newsh;
  size_t avail = sdsavail(s);

  size_t len, newlen;
  char type, oldtype = s[-1] & SDS_TYPE_MASK;
  int hdrlen;

  if (avail >= addlen) return s;

  len = sdslen(s);
  sh = (char*)s - sdsHdrSize(oldtype);
  newlen = (len + addlen);

  // 字符串分配策略，当字符串长度小于 1M 时，分配内存为需要内存的两倍，当字符串长度大于 1M 时，分配内存比需要内存大 1M
  if (newlen < SDS_MAX_PREALLOC)
    newlen *= 2;
  else
    newlen += SDS_MAX_PREALLOC;

  type = sdsReqType(newlen);

  if (type == SDS_TYPE_5) type = SDS_TYPE_8;

  hdrlen = sdsHdrSize(type);

  if (oldtype == type) {
    // 类型相同那么申请更大的内存，保留原内存的值
    newsh = s_realloc(sh, hdrlen + newlen + 1);
    if (newsh == NULL) return NULL;
    s = (char*)newsh + hdrlen;
  } else {
    newsh = s_malloc(hdrlen + newlen + 1);
    if (newsh == NULL) return NULL;
    memcpy((char*)newsh + hdrlen, s, len + 1);
    s_free(sh);
    s = (char*)newsh + hdrlen;

    s[-1] = type;
    sdssetlen(s, len);
  }
  sdssetalloc(s, newlen);
  return s;
}

// 删除 SDS 中 buf 包含的多余空间
sds sdsRemoveFreeSpace(sds s) {
  void *sh, *newsh;
  char type, oldtype = s[-1] & SDS_TYPE_MASK;

  int hdrlen, oldhdrlen = sdsHdrSize(oldtype);

  size_t len = sdslen(s);
  sh = (char*)s -oldhdrlen;

  type = sdsReqType(len);
  hdrlen = sdsHdrSize(type);

  if (oldtype == type || type > SDS_TYPE_8) {
    newsh = s_realloc(sh, oldhdrlen + len + 1);
  } else {
    newsh = s_malloc(hdrlen + len + 1);
    if (newsh == NULL) return NULL;
    memcpy((char*)newsh+hdrlen, sh, len + 1);
    s_free(sh);

    s = (char*)newsh + hdrlen;
    s[-1] = type;

    sdssetlen(s, len);
  }
  sdssetalloc(s, len);
  return s;
}

// 返回 SDS 占用的所有空间
size_t sdsAllocSize(sds s) {
  size_t alloc = sdsalloc(s);
  return sdsHdrSize(s[-1]) + alloc +1;
}

// 返回 SDS 结构的指针(s实际上是 buf 的指针)
void *sdsAllocPtr(sds s) {
  return (void*)(s - sdsHdrSize(s[-1]));
}

// 用于手动指定 SDS 的长度，适用于手动修改 SDS 中字符串数组的值，和 sdsupdatelen 场景差不多，
void sdsIncrLen(sds s, ssize_t incr) {
  unsigned char flags = s[-1];
  size_t len;

  switch (flags & SDS_TYPE_MASK) {
    case SDS_TYPE_5: {
      unsigned char *fp = ((unsigned char*)s) - 1;
      unsigned char oldlen = SDS_TYPE_5_LEN(flags);

      assert((incr > 0 && oldlen + incr < 32) || (incr < 0 && oldlen >= (unsigned int)(-incr)));

      *fp = SDS_TYPE_5 | ((oldlen + incr)) << SDS_TYPE_BITS;
      len = oldlen+incr;
      break;
    }
    case SDS_TYPE_8: {
      SDS_HDR_VAR(8,s);
      assert((incr >= 0 && sh->alloc-sh->len >= incr) || (incr < 0 && sh->len >= (unsigned int)(-incr)));
      len = (sh->len += incr);
      break;
    }
    case SDS_TYPE_16: {
      SDS_HDR_VAR(16,s);
      assert((incr >= 0 && sh->alloc-sh->len >= incr) || (incr < 0 && sh->len >= (unsigned int)(-incr)));
      len = (sh->len += incr);
      break;
    }
    case SDS_TYPE_32: {
      SDS_HDR_VAR(32,s);
      assert((incr >= 0 && sh->alloc-sh->len >= (unsigned int)incr) || (incr < 0 && sh->len >= (unsigned int)(-incr)));
      len = (sh->len += incr);
      break;
    }
    case SDS_TYPE_64: {
      SDS_HDR_VAR(64,s);
      assert((incr >= 0 && sh->alloc-sh->len >= (uint64_t)incr) || (incr < 0 && sh->len >= (uint64_t)(-incr)));
      len = (sh->len += incr);
      break;
    }
    default: len = 0;
  }
  s[len] = '\0';
}

// 为 SDS 申请空间并设置新增空间值为 0
sds sdsgrowzero(sds s, size_t len) {
  size_t curlen = sdslen(s);

  if (len <= curlen) return s;

  s = sdsMakeRoomFor(s, len - curlen);

  if (s == NULL) return NULL;

  memset(s + curlen, 0, (len - curlen + 1));
  sdssetlen(s, len);
  return s;
}

// 复制字符串 t 的前 len 字符并拼接到 SDS s 后边
sds sdscatlen(sds s, const void *t, size_t len) {
  size_t curlen = sdslen(s);

  s = sdsMakeRoomFor(s, len);
  if (s == NULL) return NULL;
  memcpy(s + curlen, t, len);
  s[curlen + len] = '\0';
  return s;
}

// 把字符串 t 拼接到 SDS s 后边
sds sdscat(sds s, const char *t) {
  return sdscatlen(s, t, strlen(t));
}

// 把 SDS t 拼接到 s 后边
sds sdscatsds(sds s, const sds t) {
  return sdscatlen(s, t, sdslen(t));
}

// 复制 t 前 len 个字符到 s
sds sdscpylen(sds s, const char *t, size_t len) {
  if (sdsalloc(s) < len) {
    s = sdsMakeRoomFor(s, len - sdslen(s));
    if (s == NULL) return NULL;
  }
  memcpy(s, t, len);

  s[len] = '\0';
  sdssetlen(s, len);
  return s;
}

// 复制字符串 t 到 s
sds sdscpy(sds s, const char *t) {
  return sdscpylen(s, t, strlen(t));
}

// long long 数字转化成字符串最大长度
#define SDS_LLSTR_SIZE 21
// 把 long long 类型的值转换成字符串标识
int sdsll2str(char *s, long long value) {
  char *p, aux;

  unsigned long long v;

  size_t l;

  v = (value < 0) ? -value : value;
  p = s;

  do {
    *p++ = '0' + (v%10);
    v /= 10;
  } while (v);

  if (value < 0) *p++ = '-';

  l = p-s;
  *p = '\0';
  p--;
  // 上面计算的是反向的，这里把字符串反转。
  while (s < p) {
    aux = *s;
    *s = *p;
    *p = aux;
    s++;
    p--;
  }
  return l;
}

// 把 unsigned long long 类型的值转换成字符串标识
int sdsull2str(char *s, unsigned long long v) {
  char *p, aux;
  size_t l;

  p = s;
  do {
    *p++ = '0'+(v%10);
    v /= 10;
  } while(v);

  l = p-s;
  *p = '\0';

  p--;
  while(s < p) {
    aux = *s;
    *s = *p;
    *p = aux;
    s++;
    p--;
  }
  return l;
}

// 创建 SDS 值为 value 的字符串表示
sds sdsfromlonglong(long long value) {
  char buf[SDS_LLSTR_SIZE];

  int len = sdsll2str(buf, value);
  return sdsnewlen(buf, len);
}

// 使用 字符串格式化 fmt 和参数列表 ap 解析成新的字符串并拼接到 s 后面
sds sdscatvprintf(sds s, const char *fmt, va_list ap) {
  va_list cpy;
  char staticbuf[1024], *buf = staticbuf, *t;

  size_t buflen = strlen(fmt)*2;

  if (buflen > sizeof(staticbuf)) {
    buf = s_malloc(buflen);
    if (buf == NULL) return NULL;
  } else {
    buflen = sizeof(staticbuf);
  }

  while(1) {
    buf[buflen -2] = '\0';
    va_copy(cpy, ap);
    vsnprintf(buf, buflen, fmt, cpy);
    va_end(cpy);
    if (buf[buflen -2] != '\0') {
      if (buf != staticbuf) s_free(buf);
      buflen *= 2;
      buf = s_malloc(buflen);

      if (buf == NULL) return NULL;
      continue;
    }
    break;
  }
  t = sdscat(s, buf);
  if (buf != staticbuf) s_free(buf);
  return t;
}

// 使用参数列表把格式化字符串拼接到 s 后面
sds sdscatprintf(sds s, const char *fmt, ...) {
  va_list ap;
  char *t;
  va_start(ap, fmt);
  t = sdscatvprintf(s,fmt,ap);
  va_end(ap);
  return t;
}

// 自定义的格式化字符串并拼接到 s 后边
sds sdscatfmt(sds s, char const *fmt, ...) {
  size_t initlen = sdslen(s);

  const char *f = fmt;

  long i;

  va_list ap;

  va_start(ap, fmt);
  f = fmt;
  i = initlen;
  while (*f) {
    char next, *str;
    size_t l;
     long long num;
    unsigned long long unum;

    if (sdsavali(s) == 0) {
      s = sdsMakeRoomFor(s, 1);
    }

    switch (*f) {
      case '%':
        next = *(f+1);
        f++;
        switch (next) {
          case 's':
          case 'S':
            str = va_arg(ap, char*);
            l = (next == 's') ? strlen(str) : sdslen(str);
            if (sdsavail(s) < l) {
              s = sdsMakeRoomFor(s, l);
            }
            memcpy(s + i, str, l);
            sdsinclen(s, l);
            i += l;
            break;
          case 'i':
          case 'I':
            if (next == 'i')
              num = va_arg(ap,int);
            else
              num = va_arg(ap,long long);
            {
              char buf[SDS_LLSTR_SIZE];
              l = sdsll2str(buf,num);
              if (sdsavail(s) < l) {
                s = sdsMakeRoomFor(s,l);
              }
              memcpy(s+i,buf,l);
              sdsinclen(s,l);
              i += l;
            }
            break;
          case 'u':
          case 'U':
            if (next == 'u')
              unum = va_arg(ap,unsigned int);
            else
              unum = va_arg(ap,unsigned long long);
            {
              char buf[SDS_LLSTR_SIZE];
              l = sdsull2str(buf,unum);
              if (sdsavail(s) < l) {
                s = sdsMakeRoomFor(s,l);
              }
              memcpy(s+i,buf,l);
              sdsinclen(s,l);
              i += l;
            }
            break;
          default: /* Handle %% and generally %<unknown>. */
            s[i++] = next;
            sdsinclen(s,1);
            break;
        }
        break;
      default:
        s[i++] = *f;
        sdsinclen(s, 1);
        break;
    }
    f++;
  }
  va_end(ap);
  s[i] = '\0';
  return s;
}

// 根据字符数组删除字符串前后的相应字符
sds sdstrim(sds s, const char *cset) {
  char *start, *end, *sp, *ep;
  size_t len;

  sp = start = s;
  ep = end = s+sdslen(s)-1;
  while(sp <= end && strchr(cset, *sp)) sp++;
  while(ep > sp && strchr(cset, *ep)) ep--;
  len = (sp > ep) ? 0 : ((ep-sp)+1);
  if (s != sp) memmove(s, sp, len);
  s[len] = '\0';
  sdssetlen(s,len);
  return s;
}

// 根据起始位置 start end 截取 SDS
void sdsrange(sds s, ssize_t start, ssize_t end) {
  size_t newlen, len = sdslen(s);

  if (len == 0) return;
  if (start < 0) {
    start = len+start;
    if (start < 0) start = 0;
  }
  if (end < 0) {
    end = len+end;
    if (end < 0) end = 0;
  }
  newlen = (start > end) ? 0 : (end-start)+1;
  if (newlen != 0) {
    if (start >= (ssize_t)len) {
      newlen = 0;
    } else if (end >= (ssize_t)len) {
      end = len-1;
      newlen = (start > end) ? 0 : (end-start)+1;
    }
  } else {
    start = 0;
  }
  if (start && newlen) memmove(s, s+start, newlen);
  s[newlen] = 0;
  sdssetlen(s,newlen);
}

// SDS 各个字母转化为小写字母
void sdstolower(sds s) {
  size_t len = sdslen(s), j;

  for (j = 0; j < len; j++) s[j] = tolower(s[j]);
}

// SDS 各个字母转化为大写字母
void sdstoupper(sds s) {
  size_t len = sdslen(s), j;

  for (j = 0; j < len; j++) s[j] = toupper(s[j]);
}

// 一个字节一个字节的比较两个 SDS 的值
int sdscmp(const sds s1, const sds s2) {
  size_t l1, l2, minlen;
  int cmp;

  l1 = sdslen(s1);
  l2 = sdslen(s2);
  minlen = (l1 < l2) ? l1 : l2;
  cmp = memcmp(s1,s2,minlen);
  if (cmp == 0) return l1>l2? 1: (l1<l2? -1: 0);
  return cmp;
}

// 查找 字符串 sep 的前 seplen 的字符串对字符串 s 的 前 len 个字符组成的字符串进行分割，并返回分割子串列表（SDS 格式）
sds *sdssplitlen(const char *s, ssize_t len, const char *sep, int seplen, int *count) {
  int elements = 0, slots = 5;

  long start = 0, j;
  sds *tokens;

  if (seplen < 1 || len < 0) return NULL;

  tokens = s_malloc(sizeof(sds)*slots);

  if (tokens == NULL) return NULL;

  // 这里分割子串是0 直接返回tokens，没有回收 tokens 申请的空间
  if (len == 0) {
    *count = 0;
    return tokens;
  }

  for (j =0; j< (len - (seplen -1)); j++) {
    if (slots < elements + 2) {
      sds *newtokens;
      slots *= 2;
      newtokens = s_realloc(tokens, sizeof(sds)*slots);
      if (newtokens == NULL) goto cleanup;
      tokens = newtokens;
    }

    if ((seplen == 1 && *(s+j) == sep[0]) || (memcmp(s + j, sep, seplen) == 0)) {
      tokens[elements] = sdsnewlen(s+start, j-start);
      if (tokens[elements] == NULL) goto cleanup;
      elements++;
      start = j + seplen;
      j = j + seplen - 1;
    }
  }
  tokens[elements] = sdsnewlen(s + start, len - start);
  if (tokens[elements] == NULL) goto cleanup;

  elements++;
  *count = elements;
  return tokens;

  cleanup:
  {
    int i;
    for (i = 0; i < elements; i++) sdsfree(tokens[i]);
    s_free(tokens);
    *count = 0;
    return NULL;
  };
}

// 释放 sds split 返回的结果
void sdsfreesplitres(sds *tokens, int count) {
  if (!tokens) return;
  while(count--)
    sdsfree(tokens[count]);
  s_free(tokens);
}

// 把字符串 p 拼接到 s 后边，对转移字符转化为字面表达，如字符 '\n' 替换成 "\n" 字符串
sds sdscatrepr(sds s, const char *p, size_t len) {
  s = sdscatlen(s,"\"",1);
  while(len--) {
    switch(*p) {
      case '\\':
      case '"':
        s = sdscatprintf(s,"\\%c",*p);
        break;
      case '\n': s = sdscatlen(s,"\\n",2); break;
      case '\r': s = sdscatlen(s,"\\r",2); break;
      case '\t': s = sdscatlen(s,"\\t",2); break;
      case '\a': s = sdscatlen(s,"\\a",2); break;
      case '\b': s = sdscatlen(s,"\\b",2); break;
      default:
        if (isprint(*p))
          s = sdscatprintf(s,"%c",*p);
        else
          s = sdscatprintf(s,"\\x%02x",(unsigned char)*p);
        break;
    }
    p++;
  }
  return sdscatlen(s,"\"",1);
}

// 判断字符 c 是否是 16 进制中有效字符
int is_hex_digit(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F');
}

// 16进制字符转化为 int
int hex_digit_to_int(char c) {
  switch(c) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'a': case 'A': return 10;
    case 'b': case 'B': return 11;
    case 'c': case 'C': return 12;
    case 'd': case 'D': return 13;
    case 'e': case 'E': return 14;
    case 'f': case 'F': return 15;
    default: return 0;
  }
}

// 解析 sdscatrepr 生成的字符串，并返回多个子串
sds *sdssplitargs(const char *line, int *argc) {
  const char *p = line;
  char *current = NULL;
  char **vector = NULL;

  *argc = 0;
  while(1) {
    while(*p && isspace(*p)) p++;
    if (*p) {
      int inq=0;
      int insq=0;
      int done=0;

      if (current == NULL) current = sdsempty();
      while(!done) {
        if (inq) {
          if (*p == '\\' && *(p+1) == 'x' &&
              is_hex_digit(*(p+2)) &&
              is_hex_digit(*(p+3)))
          {
            unsigned char byte;

            byte = (hex_digit_to_int(*(p+2))*16)+
                   hex_digit_to_int(*(p+3));
            current = sdscatlen(current,(char*)&byte,1);
            p += 3;
          } else if (*p == '\\' && *(p+1)) {
            char c;

            p++;
            switch(*p) {
              case 'n': c = '\n'; break;
              case 'r': c = '\r'; break;
              case 't': c = '\t'; break;
              case 'b': c = '\b'; break;
              case 'a': c = '\a'; break;
              default: c = *p; break;
            }
            current = sdscatlen(current,&c,1);
          } else if (*p == '"') {
            if (*(p+1) && !isspace(*(p+1))) goto err;
            done=1;
          } else if (!*p) {
            goto err;
          } else {
            current = sdscatlen(current,p,1);
          }
        } else if (insq) {
          if (*p == '\\' && *(p+1) == '\'') {
            p++;
            current = sdscatlen(current,"'",1);
          } else if (*p == '\'') {
            if (*(p+1) && !isspace(*(p+1))) goto err;
            done=1;
          } else if (!*p) {
            goto err;
          } else {
            current = sdscatlen(current,p,1);
          }
        } else {
          switch(*p) {
            case ' ':
            case '\n':
            case '\r':
            case '\t':
            case '\0':
              done=1;
              break;
            case '"':
              inq=1;
              break;
            case '\'':
              insq=1;
              break;
            default:
              current = sdscatlen(current,p,1);
              break;
          }
        }
        if (*p) p++;
      }
      vector = s_realloc(vector,((*argc)+1)*sizeof(char*));
      vector[*argc] = current;
      (*argc)++;
      current = NULL;
    } else {
      if (vector == NULL) vector = s_malloc(sizeof(void*));
      return vector;
    }
  }

  err:
  while((*argc)--)
    sdsfree(vector[*argc]);
  s_free(vector);
  if (current) sdsfree(current);
  *argc = 0;
  return NULL;
}

// SDS 字符替换
sds sdsmapchars(sds s, const char *from, const char *to, size_t setlen) {
  size_t j, i, l = sdslen(s);

  for (j = 0; j < l; j++) {
    for (i = 0; i < setlen; i++) {
      if (s[j] == from[i]) {
        s[j] = to[i];
        break;
      }
    }
  }
  return s;
}

// 用 sep 连接多个 字符串，返回 SDS
sds sdsjoin(char **argv, int argc, char *sep) {
  sds join = sdsempty();
  int j;

  for (j = 0; j < argc; j++) {
    join = sdscat(join, argv[j]);
    if (j != argc-1) join = sdscat(join,sep);
  }
  return join;
}

// 用 sep 连接多个 SDS，返回 SDS
sds sdsjoinsds(sds *argv, int argc, const char *sep, size_t seplen) {
  sds join = sdsempty();
  int j;

  for (j = 0; j < argc; j++) {
    join = sdscatsds(join, argv[j]);
    if (j != argc-1) join = sdscatlen(join,sep,seplen);
  }
  return join;
}

// 封装 SDS 内存申请
void *sds_malloc(size_t size) { return s_malloc(size); }
// 封装 SDS 内存重新申请
void *sds_realloc(void *ptr, size_t size) { return s_realloc(ptr,size); }
// 封装 SDS 内存释放
void sds_free(void *ptr) { s_free(ptr); }


// 以下为 测试代码
#if defined(SDS_TEST_MAIN)
#include <stdio.h>
#include "testhelp.h"
#include "limits.h"

#define UNUSED(x) (void)(x)
int sdsTest(void) {
    {
        sds x = sdsnew("foo"), y;

        test_cond("Create a string and obtain the length",
            sdslen(x) == 3 && memcmp(x,"foo\0",4) == 0)

        sdsfree(x);
        x = sdsnewlen("foo",2);
        test_cond("Create a string with specified length",
            sdslen(x) == 2 && memcmp(x,"fo\0",3) == 0)

        x = sdscat(x,"bar");
        test_cond("Strings concatenation",
            sdslen(x) == 5 && memcmp(x,"fobar\0",6) == 0);

        x = sdscpy(x,"a");
        test_cond("sdscpy() against an originally longer string",
            sdslen(x) == 1 && memcmp(x,"a\0",2) == 0)

        x = sdscpy(x,"xyzxxxxxxxxxxyyyyyyyyyykkkkkkkkkk");
        test_cond("sdscpy() against an originally shorter string",
            sdslen(x) == 33 &&
            memcmp(x,"xyzxxxxxxxxxxyyyyyyyyyykkkkkkkkkk\0",33) == 0)

        sdsfree(x);
        x = sdscatprintf(sdsempty(),"%d",123);
        test_cond("sdscatprintf() seems working in the base case",
            sdslen(x) == 3 && memcmp(x,"123\0",4) == 0)

        sdsfree(x);
        x = sdsnew("--");
        x = sdscatfmt(x, "Hello %s World %I,%I--", "Hi!", LLONG_MIN,LLONG_MAX);
        test_cond("sdscatfmt() seems working in the base case",
            sdslen(x) == 60 &&
            memcmp(x,"--Hello Hi! World -9223372036854775808,"
                     "9223372036854775807--",60) == 0)
        printf("[%s]\n",x);

        sdsfree(x);
        x = sdsnew("--");
        x = sdscatfmt(x, "%u,%U--", UINT_MAX, ULLONG_MAX);
        test_cond("sdscatfmt() seems working with unsigned numbers",
            sdslen(x) == 35 &&
            memcmp(x,"--4294967295,18446744073709551615--",35) == 0)

        sdsfree(x);
        x = sdsnew(" x ");
        sdstrim(x," x");
        test_cond("sdstrim() works when all chars match",
            sdslen(x) == 0)

        sdsfree(x);
        x = sdsnew(" x ");
        sdstrim(x," ");
        test_cond("sdstrim() works when a single char remains",
            sdslen(x) == 1 && x[0] == 'x')

        sdsfree(x);
        x = sdsnew("xxciaoyyy");
        sdstrim(x,"xy");
        test_cond("sdstrim() correctly trims characters",
            sdslen(x) == 4 && memcmp(x,"ciao\0",5) == 0)

        y = sdsdup(x);
        sdsrange(y,1,1);
        test_cond("sdsrange(...,1,1)",
            sdslen(y) == 1 && memcmp(y,"i\0",2) == 0)

        sdsfree(y);
        y = sdsdup(x);
        sdsrange(y,1,-1);
        test_cond("sdsrange(...,1,-1)",
            sdslen(y) == 3 && memcmp(y,"iao\0",4) == 0)

        sdsfree(y);
        y = sdsdup(x);
        sdsrange(y,-2,-1);
        test_cond("sdsrange(...,-2,-1)",
            sdslen(y) == 2 && memcmp(y,"ao\0",3) == 0)

        sdsfree(y);
        y = sdsdup(x);
        sdsrange(y,2,1);
        test_cond("sdsrange(...,2,1)",
            sdslen(y) == 0 && memcmp(y,"\0",1) == 0)

        sdsfree(y);
        y = sdsdup(x);
        sdsrange(y,1,100);
        test_cond("sdsrange(...,1,100)",
            sdslen(y) == 3 && memcmp(y,"iao\0",4) == 0)

        sdsfree(y);
        y = sdsdup(x);
        sdsrange(y,100,100);
        test_cond("sdsrange(...,100,100)",
            sdslen(y) == 0 && memcmp(y,"\0",1) == 0)

        sdsfree(y);
        sdsfree(x);
        x = sdsnew("foo");
        y = sdsnew("foa");
        test_cond("sdscmp(foo,foa)", sdscmp(x,y) > 0)

        sdsfree(y);
        sdsfree(x);
        x = sdsnew("bar");
        y = sdsnew("bar");
        test_cond("sdscmp(bar,bar)", sdscmp(x,y) == 0)

        sdsfree(y);
        sdsfree(x);
        x = sdsnew("aar");
        y = sdsnew("bar");
        test_cond("sdscmp(bar,bar)", sdscmp(x,y) < 0)

        sdsfree(y);
        sdsfree(x);
        x = sdsnewlen("\a\n\0foo\r",7);
        y = sdscatrepr(sdsempty(),x,sdslen(x));
        test_cond("sdscatrepr(...data...)",
            memcmp(y,"\"\\a\\n\\x00foo\\r\"",15) == 0)

        {
            unsigned int oldfree;
            char *p;
            int step = 10, j, i;

            sdsfree(x);
            sdsfree(y);
            x = sdsnew("0");
            test_cond("sdsnew() free/len buffers", sdslen(x) == 1 && sdsavail(x) == 0);

            /* Run the test a few times in order to hit the first two
             * SDS header types. */
            for (i = 0; i < 10; i++) {
                int oldlen = sdslen(x);
                x = sdsMakeRoomFor(x,step);
                int type = x[-1]&SDS_TYPE_MASK;

                test_cond("sdsMakeRoomFor() len", sdslen(x) == oldlen);
                if (type != SDS_TYPE_5) {
                    test_cond("sdsMakeRoomFor() free", sdsavail(x) >= step);
                    oldfree = sdsavail(x);
                }
                p = x+oldlen;
                for (j = 0; j < step; j++) {
                    p[j] = 'A'+j;
                }
                sdsIncrLen(x,step);
            }
            test_cond("sdsMakeRoomFor() content",
                memcmp("0ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ",x,101) == 0);
            test_cond("sdsMakeRoomFor() final length",sdslen(x)==101);

            sdsfree(x);
        }
    }
    test_report()
    return 0;
}
#endif

#ifdef SDS_TEST_MAIN
int main(void) {
    return sdsTest();
}
#endif