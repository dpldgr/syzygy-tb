/*
  Standalone tbdecompress, C++20 edition.

  Copyright (c) 2011-2018 Ronald de Man
  Copyright (c) 2026

  This file is distributed under the terms of the GNU GPL, version 2.

  Build (Linux, macOS, or MinGW-w64):
    c++ -std=c++20 -O3 -pthread tbdecompress.cpp -o tbdecompress
*/
#define REGULAR
#define DECOMPRESSION_ONLY
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <new>
#include <string>
#include <sys/stat.h>
#include <sys/time.h>
#include <thread>
#include <unistd.h>
#include <fcntl.h>
#ifndef _WIN32
#include <sys/mman.h>
#endif
#include <vector>


/*
  Copyright (c) 2011-2018 Ronald de Man

  This file is distributed under the terms of the GNU GPL, version 2.
*/

#ifndef DEFS_H
#define DEFS_H

#include <inttypes.h>

#if defined(REGULAR) || defined(SHATRANJ)
#define SMALL
#endif

#ifndef SHATRANJ
#define DRAW_RULE (2 * 50)
#else
#define DRAW_RULE (2 * 70)
#endif

#if TBPIECES < 7
#define MAX_STATS 1536
#else
#define MAX_STATS 2560
#endif

#ifndef COMPRESSION_THREADS
#define COMPRESSION_THREADS 1
#endif

#ifndef ZSTD_LEVEL
#define ZSTD_LEVEL 1
#endif

#define MAX_VALS (((MAX_STATS / 2) - DRAW_RULE) / 2)

enum { MAXSYMB = 4095 + 8 };

#define LOOKUP
#define LUBITS 12

// GIVEAWAY is a variation on SUICIDE
#ifdef GIVEAWAY
#define SUICIDE
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
#define assume(x) do { if (!(x)) __builtin_unreachable(); } while (0)
#else
#define assume(x) do { } while (0)
#endif

#if 0
#define likely(x) (x)
#define unlikely(x) (x)
#else
#define likely(x) __builtin_expect(!!(x),1)
#define unlikely(x) __builtin_expect(!!(x),0)
#endif

#define PASTER(x,y) x##_##y
#define EVALUATOR(x,y) PASTER(x,y)

#endif
#ifndef TYPES_H
#define TYPES_H

#include <inttypes.h>

typedef uint64_t bitboard;
typedef uint16_t Move;

typedef uint8_t u8;
typedef uint16_t u16;

enum { PAWN = 1, KNIGHT, BISHOP, ROOK, QUEEN, KING };

enum {
  WPAWN = 1, WKNIGHT, WBISHOP, WROOK, WQUEEN, WKING,
  BPAWN = 9, BKNIGHT, BBISHOP, BROOK, BQUEEN, BKING,
};

struct dtz_map {
  uint16_t map[4][MAX_VALS];
  uint16_t inv_map[4][MAX_VALS];
  uint16_t num[4];
  uint16_t max_num;
  uint8_t side;
  uint8_t ply_accurate_win;
  uint8_t ply_accurate_loss;
  uint8_t wide;
  uint8_t high_freq_max;
};

#endif
#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef _WIN32
typedef HANDLE map_t;
typedef HANDLE FD;
#define FD_ERR INVALID_HANDLE_VALUE
#define SEP_CHAR ';'

#else
typedef size_t map_t;
typedef int FD;
#define FD_ERR -1
#define SEP_CHAR ':'

#endif

#undef min
#define min(a,b) ((a) < (b) ? (a) : (b))

FD open_file(const char *name);
void close_file(FD fd);

size_t file_size(FD fd);

void *map_file(FD fd, bool shared, map_t *map);
void unmap_file(void *data, map_t map);

void *alloc_aligned(uint64_t size, uintptr_t alignment);
void *alloc_huge(uint64_t size);

void write_u32(FILE *F, uint32_t v);
void write_u16(FILE *F, uint16_t v);
void write_u8(FILE *F, uint8_t v);

void write_bits(FILE *F, uint32_t bits, int n);

void copy_data(FILE *F, FILE *G, uint64_t num);
void write_data(FILE *F, uint8_t *src, uint64_t offset, uint64_t size,
    uint8_t *v);
void read_data_u8(FILE *F, uint8_t *dst, uint64_t size, uint8_t *v);
void read_data_u16(FILE *F, uint16_t *dst, uint64_t size, uint16_t *v);

#endif
/*
  Copyright (c) 2011-2016, 2018 Ronald de Man

  This file is distributed under the terms of the GNU GPL, version 2.
*/

#ifndef PROBE_H
#define PROBE_H


#define TBPIECES 7

#if defined(SUICIDE)
#if !defined(GIVEAWAY)
#define WDLSUFFIX ".stbw"
#define DTZSUFFIX ".stbz"
#define TBPATH "STBPATH"
#define STATSDIR "STBSTATSDIR"
#define LOGFILE "stblog.txt"
#else
#define WDLSUFFIX ".gtbw"
#define DTZSUFFIX ".gtbz"
#define TBPATH "GTBPATH"
#define STATSDIR "GTBSTATSDIR"
#define LOGFILE "gtblog.txt"
#endif
#elif defined(LOSER)
#define WDLSUFFIX ".ltbw"
#define DTZSUFFIX ".ltbz"
#define TBPATH "LTBPATH"
#define STATSDIR "LTBSTATSDIR"
#define LOGFILE "ltblog.txt"
#elif defined(GIVEAWAY)
#define WDLSUFFIX ".gtbw"
#define DTZSUFFIX ".gtbz"
#define TBPATH "GTBPATH"
#define STATSDIR "GTBSTATSDIR"
#define LOGFILE "gtblog.txt"
#elif defined(ATOMIC)
#define WDLSUFFIX ".atbw"
#define DTZSUFFIX ".atbz"
#define TBPATH "ATBPATH"
#define STATSDIR "ATBSTATSDIR"
#define LOGFILE "atblog.txt"
#elif defined(SHATRANJ)
#define WDLSUFFIX ".jtbw"
#define DTZSUFFIX ".jtbz"
#define TBPATH "JTBPATH"
#define STATSDIR "JTBSTATSDIR"
#define LOGFILE "jtblog.txt"
#else
#define WDLSUFFIX ".rtbw"
#define DTZSUFFIX ".rtbz"
#define TBPATH "RTBPATH"
#define STATSDIR "RTBSTATSDIR"
#define LOGFILE "rtblog.txt"
#endif

#define MAX_TBPIECES 8

#if defined(REGULAR)
#define WDL_MAGIC 0x5d23e871
#define DTZ_MAGIC 0xa50c66d7
#elif defined(ATOMIC)
#define WDL_MAGIC 0x49a48d55
#define DTZ_MAGIC 0xeb5ea991
#elif defined(SHATRANJ)
#define WDL_MAGIC 0xb4e9b3b7
#define DTZ_MAGIC 0x87c126fc
#elif defined(SUICIDE) && !defined(GIVEAWAY)
#define WDL_MAGIC 0x1593f67b
#define DTZ_MAGIC 0x23e7cfe4
#define OTHER_MAGIC 0x21bc55bc
#elif defined(GIVEAWAY)
#define WDL_MAGIC 0x21bc55bc
#define DTZ_MAGIC 0x501bf5d6
#define OTHER_MAGIC 0x1593f67b
#endif

#ifdef SUICIDE
#define TBHASHBITS 12
#else
#define TBHASHBITS 10
#endif

struct TBHashEntry;

struct PairsData {
  char *indextable;
  uint16_t *sizetable;
  uint8_t *data;
  uint16_t *offset;
  uint8_t *symlen;
  uint8_t *sympat;
#ifdef LOOKUP
  uint16_t *lookup_len;
  uint8_t *lookup_bits;
#endif
  int blocksize;
  int idxbits;
  int min_len;
  int max_len; // to allow checking max_len with rtbver/rtbverp
  uint64_t base[];
};

struct TBEntry {
  uint8_t *data;
  uint32_t key;
  uint8_t ready;
  uint8_t num;
  uint8_t symmetric;
  uint8_t has_pawns;
} __attribute__((__may_alias__));

struct TBEntry_piece {
  uint8_t *data;
  uint32_t key;
  uint8_t ready;
  uint8_t num;
  uint8_t symmetric;
  uint8_t has_pawns;
  uint8_t enc_type;
  struct PairsData *precomp[2];
  uint64_t factor[2][TBPIECES];
  uint8_t pieces[2][TBPIECES];
  uint8_t norm[2][TBPIECES];
  uint8_t order[2];
};

struct TBEntry_pawn {
  uint8_t *data;
  uint32_t key;
  uint8_t ready;
  uint8_t num;
  uint8_t symmetric;
  uint8_t has_pawns;
  uint8_t pawns[2];
  struct {
    struct PairsData *precomp[2];
    uint64_t factor[2][TBPIECES];
    uint8_t pieces[2][TBPIECES];
    uint8_t norm[2][TBPIECES];
    uint8_t order[2];
    uint8_t order2[2];
  } file[4];
};

struct TBHashEntry {
  uint64_t key;
  struct TBEntry *ptr;
};

int probe_tb(int *pieces, int *pos, int wtm, bitboard occ, int alpha, int beta);

uint64_t encode_piece(struct TBEntry_piece *ptr, uint8_t *norm, int *pos,
    uint64_t *factor);
void decode_piece(struct TBEntry_piece *ptr, uint8_t *norm, int *pos,
    uint64_t *factor, int *order, uint64_t idx);
uint64_t encode_pawn(struct TBEntry_pawn *ptr, uint8_t *norm, int *pos,
    uint64_t *factor);
void decode_pawn(struct TBEntry_pawn *ptr, uint8_t *norm, int *pos,
    uint64_t *factor, int *order, uint64_t idx, int file);

void set_norm_piece(struct TBEntry_piece *ptr, uint8_t *norm, uint8_t *pieces,
    int order);
void set_norm_pawn(struct TBEntry_pawn *ptr, uint8_t *norm, uint8_t *pieces,
    int order, int order2);
uint64_t calc_factors_piece(uint64_t *factor, int num, int order,
    uint8_t *norm, uint8_t enc_type);
uint64_t calc_factors_pawn(uint64_t *factor, int num, int order, int order2,
    uint8_t *norm, int file);

void calc_order_piece(int num, int ord, int *order, uint8_t *norm);
void calc_order_pawn(int num, int ord, int ord2, int *order, uint8_t *norm);

#endif
/*
  Copyright (c) 2011-2016, 2018 Ronald de Man

  This file is distributed under the terms of the GNU GPL, version 2.
*/

#ifndef DECOMPRESS_H
#define DECOMPRESS_H


#ifdef COMPRESS_H
#error decompress.h conflicts with compress.h
#endif

struct tb_handle {
  FILE *F;
  map_t mmap;
  uint8_t *data;
  uint64_t data_size;
  int wdl;
  int num_files;
  int split;
  int has_pawns;
  struct {
    uint64_t idx[2];
    uint64_t size[2];
  } file[4];
  union {
    struct TBEntry entry;
    struct TBEntry_piece entry_piece;
    struct TBEntry_pawn entry_pawn;
  };
  uint8_t dtz_flags[4];
  uint8_t (*map[4])[256];
  uint16_t (*map16[4])[MAX_VALS];
};

void decomp_init_piece(int *pcs);
void decomp_init_pawn(int *pcs, int *pt);
struct tb_handle *open_tb_handle(char *tablename, int wdl);
struct tb_handle *open_tb_file(const char *name, int wdl);
void decomp_init_table(struct tb_handle *H);
uint8_t *decompress_table(struct tb_handle *H, int bside, int f);
void close_tb(struct tb_handle *H);
void set_perm(struct tb_handle *H, int bside, int f, int *perm, int *pt);
struct TBEntry *get_entry(struct tb_handle *H);
int get_ply_accurate_win(struct tb_handle *H, int f);
int get_ply_accurate_loss(struct tb_handle *H, int f);
int get_dtz_side(struct tb_handle *H, int f);
uint8_t (*get_dtz_map(struct tb_handle *H, int f))[256];
uint16_t (*get_dtz_map16(struct tb_handle *H, int f))[MAX_VALS];

#endif


struct alignas(64) thread_data {
  uint64_t begin;
  uint64_t end;
  bitboard occ;
  uint64_t *stats;
  int *p;
  int thread;
  int affinity;
};
static int numthreads;
static int total_work;
static timeval cur_time;

static uint64_t *alloc_work(int n) {
  return static_cast<uint64_t *>(std::malloc((n + 1) * sizeof(uint64_t)));
}
static void fill_work(int n, uint64_t size, uint64_t mask, uint64_t *work) {
  work[0] = 0;
  work[n] = size;
  for (int i = 1; i < n; ++i)
    work[i] = ((static_cast<uint64_t>(i) * size) / static_cast<uint64_t>(n)) & ~mask;
}
static void init_threads(int) {}
static void run_threaded(void (*func)(thread_data *), uint64_t *work, int) {
  std::atomic<int> next{0};
  std::vector<std::thread> workers;
  workers.reserve(static_cast<size_t>(numthreads));
  for (int t = 0; t < numthreads; ++t) {
    workers.emplace_back([&, t] {
      thread_data data{};
      data.thread = t;
      for (;;) {
        int job = next.fetch_add(1, std::memory_order_relaxed);
        if (job >= total_work) break;
        data.begin = work[job];
        data.end = work[job + 1];
        func(&data);
      }
    });
  }
  for (auto &worker : workers) worker.join();
  timeval stop{};
  gettimeofday(&stop, nullptr);
  long seconds = stop.tv_sec - cur_time.tv_sec;
  long useconds = stop.tv_usec - cur_time.tv_usec;
  if (useconds < 0) { useconds += 1000000; --seconds; }
  std::printf("time taken = %3ld:%02ld.%03ld\n", seconds / 60,
      seconds % 60, useconds / 1000);
  cur_time = stop;
}

FD open_file(const char *name) {
#ifdef _WIN32
  return CreateFileA(name, GENERIC_READ, FILE_SHARE_READ, nullptr,
      OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, nullptr);
#else
  return open(name, O_RDONLY);
#endif
}
void close_file(FD fd) {
#ifdef _WIN32
  CloseHandle(fd);
#else
  close(fd);
#endif
}
size_t file_size(FD fd) {
#ifdef _WIN32
  LARGE_INTEGER size{};
  if (!GetFileSizeEx(fd, &size) || size.QuadPart < 0 ||
      static_cast<uint64_t>(size.QuadPart) > SIZE_MAX) {
    std::fprintf(stderr, "Could not determine input file size.\n");
    std::exit(EXIT_FAILURE);
  }
  return static_cast<size_t>(size.QuadPart);
#else
  struct stat status{};
  if (fstat(fd, &status) != 0 || status.st_size < 0) {
    std::fprintf(stderr, "Could not determine input file size: %s\n",
        std::strerror(errno));
    std::exit(EXIT_FAILURE);
  }
  return static_cast<size_t>(status.st_size);
#endif
}
void *map_file(FD fd, bool shared, map_t *map) {
#ifdef _WIN32
  (void)shared;
  *map = CreateFileMappingA(fd, nullptr, PAGE_READONLY, 0, 0, nullptr);
  if (*map == nullptr) {
    std::fprintf(stderr, "CreateFileMapping() failed (error %lu).\n",
        static_cast<unsigned long>(GetLastError()));
    std::exit(EXIT_FAILURE);
  }
  void *data = MapViewOfFile(*map, FILE_MAP_READ, 0, 0, 0);
  if (data == nullptr) {
    DWORD error = GetLastError();
    CloseHandle(*map);
    std::fprintf(stderr, "MapViewOfFile() failed (error %lu).\n",
        static_cast<unsigned long>(error));
    std::exit(EXIT_FAILURE);
  }
  return data;
#else
  *map = file_size(fd);
  void *data = mmap(nullptr, *map, PROT_READ, shared ? MAP_SHARED : MAP_PRIVATE,
      fd, 0);
  if (data == MAP_FAILED) {
    std::fprintf(stderr, "mmap() failed: %s\n", std::strerror(errno));
    std::exit(EXIT_FAILURE);
  }
#ifdef MADV_RANDOM
  madvise(data, *map, MADV_RANDOM);
#endif
  return data;
#endif
}
void unmap_file(void *data, map_t map) {
  if (!data) return;
#ifdef _WIN32
  UnmapViewOfFile(data);
  CloseHandle(map);
#else
  munmap(data, map);
#endif
}


struct allocation_result {
  void *pointer;
  template <typename T> operator T *() const {
    return static_cast<T *>(pointer);
  }
};
static allocation_result cpp_malloc(size_t size) {
  return {std::malloc(size)};
}
#define malloc(size) cpp_malloc(size)


/*
  Helpers needed by the standalone decompressor without pulling in
  the probing implementation from probe.c.

  This file is distributed under the terms of the GNU GPL, version 2.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


static int subfactor(int k, int n)
{
  int f = n;
  int l = 1;

  for (int i = 1; i < k; i++) {
    f *= n - i;
    l *= i + 1;
  }
  return f / l;
}

uint64_t calc_factors_piece(uint64_t *factor, int num, int order,
    uint8_t *norm, uint8_t enc_type)
{
  static const int pivfac[] = { 31332, 28056, 462 };
  int i = norm[0];
  int n = 64 - norm[0];
  uint64_t f = 1;

  for (int k = 0; i < num || k == order; k++) {
    if (k == order) {
      factor[0] = f;
      f *= pivfac[enc_type];
    } else {
      factor[i] = f;
      f *= subfactor(norm[i], n);
      n -= norm[i];
      i += norm[i];
    }
  }
  return f;
}

void set_norm_piece(struct TBEntry_piece *entry, uint8_t *norm,
    uint8_t *pieces, int order)
{
  (void)order;
  for (int i = 0; i < entry->num; i++)
    norm[i] = 0;

  norm[0] = entry->enc_type == 0 ? 3
          : entry->enc_type == 2 ? 2 : entry->enc_type - 1;
  for (int i = norm[0]; i < entry->num; i += norm[i])
    for (int j = i; j < entry->num && pieces[j] == pieces[i]; j++)
      norm[i]++;
}

uint64_t calc_factors_pawn(uint64_t *factor, int num, int order, int order2,
    uint8_t *norm, int file)
{
  static const uint8_t ptwist[] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    47, 35, 23, 11, 10, 22, 34, 46,
    45, 33, 21, 9, 8, 20, 32, 44,
    43, 31, 19, 7, 6, 18, 30, 42,
    41, 29, 17, 5, 4, 16, 28, 40,
    39, 27, 15, 3, 2, 14, 26, 38,
    37, 25, 13, 1, 0, 12, 24, 36,
    0, 0, 0, 0, 0, 0, 0, 0
  };
  static const uint8_t invflap[] = {
    8, 16, 24, 32, 40, 48,
    9, 17, 25, 33, 41, 49,
    10, 18, 26, 34, 42, 50,
    11, 19, 27, 35, 43, 51
  };
  static uint64_t pfactor[6][4];
  static int initialized;
  int i = norm[0];
  int n;
  uint64_t f = 1;

  if (!initialized) {
    uint64_t binomial[7][64] = {{ 0 }};

    for (int j = 0; j < 64; j++)
      binomial[0][j] = 1;
    for (int k = 1; k < 7; k++)
      for (int j = 1; j < 64; j++)
        binomial[k][j] = binomial[k - 1][j - 1] + binomial[k][j - 1];
    for (int k = 0; k < 6; k++) {
      for (int pawn_file = 0; pawn_file < 4; pawn_file++) {
        uint64_t count = 0;
        for (int j = 6 * pawn_file; j < 6 * (pawn_file + 1); j++)
          count += binomial[k][ptwist[invflap[j]]];
        pfactor[k][pawn_file] = count;
      }
    }
    initialized = 1;
  }

  if (order2 < 0x0f)
    i += norm[i];
  n = 64 - i;
  for (int k = 0; i < num || k == order || k == order2; k++) {
    if (k == order) {
      factor[0] = f;
      f *= pfactor[norm[0] - 1][file];
    } else if (k == order2) {
      factor[norm[0]] = f;
      f *= subfactor(norm[norm[0]], 48 - norm[0]);
    } else {
      factor[i] = f;
      f *= subfactor(norm[i], n);
      n -= norm[i];
      i += norm[i];
    }
  }
  return f;
}

void set_norm_pawn(struct TBEntry_pawn *entry, uint8_t *norm,
    uint8_t *pieces, int order, int order2)
{
  (void)order;
  (void)order2;
  for (int i = 0; i < entry->num; i++)
    norm[i] = 0;

  norm[0] = entry->pawns[0];
  if (entry->pawns[1])
    norm[entry->pawns[0]] = entry->pawns[1];
  for (int i = entry->pawns[0] + entry->pawns[1]; i < entry->num;
      i += norm[i])
    for (int j = i; j < entry->num && pieces[j] == pieces[i]; j++)
      norm[i]++;
}

/*
  Copyright (c) 2011-2013, 2018 Ronald de Man

  This file is distributed under the terms of the GNU GPL, version 2.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


extern int total_work;
extern int numthreads;
extern int numpcs;
extern int numpawns;

static int enc_type;

int pawns0, pawns1;

void decomp_init_piece(int *pcs)
{
  int i, k;

  for (i = 0, k = 0; i < 16; i++)
    if (pcs[i] == 1) k++;
  if (k >= 3) enc_type = 0;
  else if (k == 2) enc_type = 2;
  else { /* only possible for suicide */
    k = 16;
    for (i = 0; i < 16; i++)
      if (pcs[i] < k && pcs[i] > 1) k = pcs[i];
    enc_type = 1 + k;
  }
  pawns0 = pawns1 = 0;
}

void decomp_init_pawn(int *pcs, int *pt)
{
  if (pt[0] == WPAWN) {
    pawns0 = pcs[WPAWN];
    pawns1 = pcs[BPAWN];
  } else {
    pawns0 = pcs[BPAWN];
    pawns1 = pcs[WPAWN];
  }
}

static void calc_symlen(struct PairsData *d, int s, char *tmp)
{
  int s1, s2;

  int w = *(int *)(d->sympat + 3 * s);
  s2 = (w >> 12) & 0x0fff;
  if (s2 == 0x0fff)
    d->symlen[s] = 0;
  else {
    s1 = w & 0x0fff;
    if (!tmp[s1]) calc_symlen(d, s1, tmp);
    if (!tmp[s2]) calc_symlen(d, s2, tmp);
    d->symlen[s] = d->symlen[s1] + d->symlen[s2] + 1;
  }
  tmp[s] = 1;
}

struct PairsData *decomp_setup_pairs(struct tb_handle *H, uint64_t tb_size, uint64_t *size, uint8_t *flags)
{
  struct PairsData *d;
  int i;
  uint8_t data[256];
  FILE *F = H->F;

  fread(data, 1, 2, F);
  *flags = data[0];
  if (*flags & 0x80) {
    d = malloc(sizeof(struct PairsData));
    d->idxbits = 0;
    d->max_len = 0;
    if (H->wdl)
      d->min_len = data[1];
    else
      d->min_len = 0;
    size[0] = size[1] = size[2] = 0;
    return d;
  }
  fread(data + 2, 1, 10, F);
  int blocksize = data[1];
  int idxbits = data[2];
  uint32_t real_num_blocks = data[4] | (data[5] << 8)
                          | (data[6] << 16) | (data[7] << 24);
  uint32_t num_blocks = real_num_blocks + *(uint8_t *)(&data[3]);
  int max_len = data[8];
  int min_len = data[9];
  int h = max_len - min_len + 1;
  fread(data + 12, 1, 2 * h, F);
  int num_syms = data[10 + 2 * h] | (data[11 + 2 * h] << 8);
  d = malloc(sizeof(struct PairsData) + h * sizeof(uint64_t) + num_syms);
  d->blocksize = blocksize;
  d->idxbits = idxbits;
  d->offset = malloc(2 * h);
  memcpy(d->offset, &data[10], 2 * h);
  d->symlen = ((unsigned char *)d) + sizeof(struct PairsData) + h * sizeof(uint64_t);
  d->sympat = malloc(3 * num_syms + (num_syms & 1));
  d->max_len = max_len; // to allow checking max_len with rtbver/rtbverp
  d->min_len = min_len;
  fread(d->sympat, 1, 3 * num_syms + (num_syms & 1), F);

  int num_indices = (tb_size + (1ULL << idxbits) - 1) >> idxbits;
  size[0] = 6ULL * num_indices;
  size[1] = 2ULL * num_blocks;
  size[2] = (1ULL << blocksize) * real_num_blocks;

  char *tmp = (char *)calloc(num_syms, 1);
  for (i = 0; i < num_syms; i++)
    if (!tmp[i])
      calc_symlen(d, i, tmp);
  free(tmp);

  d->base[h - 1] = 0;
  for (i = h - 2; i >= 0; i--)
    d->base[i] = (d->base[i + 1] + d->offset[i] - d->offset[i + 1]) / 2;
  for (i = 0; i < h; i++)
    d->base[i] <<= 64 - (min_len + i);

  d->offset -= d->min_len;

  return d;
}

static void decomp_setup_pieces_piece(struct tb_handle *H, uint64_t *tb_size)
{
  int i;
  int order;
  struct TBEntry_piece *entry = &(H->entry_piece);
  FILE *F = H->F;
  uint8_t data[TBPIECES + 1];

  fread(data, 1, numpcs + 1, F);
  entry->num = numpcs;
  entry->enc_type = enc_type;

  for (i = 0; i < numpcs; i++)
    entry->pieces[0][i] = data[i + 1] & 0x0f;
  order = data[0] & 0x0f;
  entry->order[0] = order;
  set_norm_piece(entry, entry->norm[0], entry->pieces[0], order);
  tb_size[0] = calc_factors_piece(entry->factor[0], entry->num, order, entry->norm[0], entry->enc_type);

  if (H->split) {
    for (i = 0; i < numpcs; i++)
      entry->pieces[1][i] = data[i + 1] >> 4;
    order = data[0] >> 4;
    entry->order[1] = order;
    set_norm_piece(entry, entry->norm[1], entry->pieces[1], order);
    tb_size[1] = calc_factors_piece(entry->factor[1], entry->num, order, entry->norm[1], entry->enc_type);
  } else {
    for (i = 0; i < numpcs; i++) {
      entry->pieces[1][i] = entry->pieces[0][i] ^ 0x08;
      entry->factor[1][i] = entry->factor[0][i];
      entry->norm[1][i] = entry->norm[0][i];
    }
    entry->order[1] = entry->order[0];
  }
}

void decomp_setup_pieces_pawn(struct tb_handle *H, uint64_t *tb_size, int f)
{
  int i, j;
  int order, order2;
  struct TBEntry_pawn *entry = &(H->entry_pawn);
  FILE *F = H->F;
  uint8_t data[TBPIECES + 2];

  entry->num = numpcs;
  entry->pawns[0] = pawns0;
  entry->pawns[1] = pawns1;

  j = 1 + (entry->pawns[1] > 0);
  fread(data, 1, entry->num + j, F);

  order = data[0] & 0x0f;
  order2 = entry->pawns[1] ? (data[1] & 0x0f) : 0x0f;
  for (i = 0; i < entry->num; i++)
    entry->file[f].pieces[0][i] = data[i + j] & 0x0f;
  entry->file[f].order[0] = order;
  entry->file[f].order2[0] = order2;
  set_norm_pawn(entry, entry->file[f].norm[0], entry->file[f].pieces[0], order, order2);
  tb_size[0] = calc_factors_pawn(entry->file[f].factor[0], entry->num, order, order2, entry->file[f].norm[0], f);

  if (H->split) {
    order = data[0] >> 4;
    order2 = entry->pawns[1] ? (data[1] >> 4) : 0x0f0;
    for (i = 0; i < entry->num; i++)
      entry->file[f].pieces[1][i] = data[i + j] >> 4;
    entry->file[f].order[1] = order;
    entry->file[f].order2[1] = order2;
    set_norm_pawn(entry, entry->file[f].norm[1], entry->file[f].pieces[1], order, order2);
    tb_size[1] = calc_factors_pawn(entry->file[f].factor[1], entry->num, order, order2, entry->file[f].norm[1], f);
  } else {
    for (i = 0; i < numpcs; i++) {
      entry->file[f].pieces[1][i] = entry->file[f].pieces[0][i] ^ 0x08;
      entry->file[f].factor[1][i] = entry->file[f].factor[0][i];
      entry->file[f].norm[1][i] = entry->file[f].norm[0][i];
    }
    entry->file[f].order[1] = entry->file[f].order[0];
    entry->file[f].order2[1] = entry->file[f].order2[0];
  }
}

void decomp_init_table(struct tb_handle *H)
{
  uint32_t magic;
  uint8_t byte;
  int f;
  uint64_t size[8 * 3];
  FILE *F = H->F;
  int split, files;
  uint8_t dummy;

  magic = 0;
  fread(&magic, 1, 4, F);
  if (magic != (H->wdl ? WDL_MAGIC : DTZ_MAGIC)) {
    fprintf(stderr, "Corrupted table.\n");
    exit(EXIT_FAILURE);
  }

  fread(&byte, 1, 1, F);
  H->split = split = (H->wdl ? (byte & 0x01) : 0);
  H->num_files = files = (byte & 0x02) ? 4 : 1;
  H->has_pawns = (pawns0 != 0);

  if (pawns0 == 0)
    decomp_setup_pieces_piece(H, H->file[0].size);
  else if (files == 1) {
    fprintf(stderr, "Unsupported pawn table without a-d subtables.\n");
    exit(EXIT_FAILURE);
  } else {
    for (f = 0; f < 4; f++)
      decomp_setup_pieces_pawn(H, H->file[f].size, f);
  }

  if (ftell(F) & 0x01) fgetc(F);

  if (pawns0 == 0) {
    struct TBEntry_piece *entry = &(H->entry_piece);
    entry->precomp[0] = decomp_setup_pairs(H, H->file[0].size[0], &size[0], &(H->dtz_flags[0]));
    if (split)
      entry->precomp[1] = decomp_setup_pairs(H, H->file[0].size[1], &size[3], &dummy);

    if (!H->wdl) {
      if (H->dtz_flags[0] & 2) {
        if (!(H->dtz_flags[0] & 16)) {
          int i;
          uint8_t num;
          H->map[0] = malloc(4 * 256);
          for (i = 0; i < 4; i++) {
            fread(&num, 1, 1, F);
            fread(H->map[0][i], 1, num, F);
          }
        } else {
          int i;
          uint16_t num;
          H->map16[0] = malloc(4 * MAX_VALS * 2);
          for (i = 0; i < 4; i++) {
            fread(&num, 2, 1, F);
            fread(H->map16[0][i], 2, num, F);
          }
        }
      }
      if (ftell(F) & 0x01) fgetc(F);
    }

    entry->precomp[0]->indextable = malloc(size[0]);
    fread(entry->precomp[0]->indextable, 1, size[0], F);
    if (split) {
      entry->precomp[1]->indextable = malloc(size[3]);
      fread(entry->precomp[1]->indextable, 1, size[3], F);
    }

    entry->precomp[0]->sizetable = malloc(size[1]);
    fread(entry->precomp[0]->sizetable, 1, size[1], F);
    if (split) {
      entry->precomp[1]->sizetable = malloc(size[4]);
      fread(entry->precomp[1]->sizetable, 1, size[4], F);
    }

    if (!split)
      entry->precomp[1] = entry->precomp[0];

    uint64_t idx = ftell(F);
    idx = (idx + 0x3f) & ~0x3f;
    H->file[0].idx[0] = idx;
    idx += size[2];
    if (split) {
      idx = (idx + 0x3f) & ~0x3f;
      H->file[0].idx[1] = idx;
      idx += size[5];
    }
  } else {
    struct TBEntry_pawn *entry = &(H->entry_pawn);
    for (f = 0; f < files; f++) {
      entry->file[f].precomp[0] = decomp_setup_pairs(H, H->file[f].size[0], &size[6 * f], &(H->dtz_flags[f]));
      if (split)
        entry->file[f].precomp[1] = decomp_setup_pairs(H, H->file[f].size[1], &size[6 * f + 3], &dummy);
    }

    if (!H->wdl) {
      int i;
      for (f = 0; f < files; f++) {
        if (H->dtz_flags[f] & 2) {
          H->map[f] = malloc(4 * 256);
          uint8_t num;
          for (i = 0; i < 4; i++) {
            fread(&num, 1, 1, F);
            fread(H->map[f][i], 1, num, F);
          }
        }
      }
      if (ftell(F) & 0x01) fgetc(F);
    }

    for (f = 0; f < files; f++) {
      entry->file[f].precomp[0]->indextable = malloc(size[6 * f]);
      fread(entry->file[f].precomp[0]->indextable, 1, size[6 * f], F);
      if (split) {
        entry->file[f].precomp[1]->indextable = malloc(size[6 * f + 3]);
        fread(entry->file[f].precomp[1]->indextable, 1, size[6 * f + 3], F);
      }
    }

    for (f = 0; f < files; f++) {
      entry->file[f].precomp[0]->sizetable = malloc(size[6 * f + 1]);
      fread(entry->file[f].precomp[0]->sizetable, 1, size[6 * f + 1], F);
      if (split) {
        entry->file[f].precomp[1]->sizetable = malloc(size[6 * f + 4]);
        fread(entry->file[f].precomp[1]->sizetable, 1, size[6 * f + 4], F);
      }
    }

    if (!split)
      for (f = 0; f < files; f++)
        entry->file[f].precomp[1] = entry->file[f].precomp[0];

    uint64_t idx = ftell(F);
    for (f = 0; f < files; f++) {
      idx = (idx + 0x3f) & ~0x3f;
      H->file[f].idx[0] = idx;
      idx += size[6 * f + 2];
      if (split) {
        idx = (idx + 0x3f) & ~0x3f;
        H->file[f].idx[1] = idx;
        idx += size[6 * f + 5];
      }
    }
  }
}

uint64_t expand_symbol(uint8_t *dst, int sym, uint64_t idx, uint64_t end, uint8_t *sympat, uint8_t *symlen)
{
  if (idx == end) return idx;
  int w = *(int *)(sympat + 3 * sym);
  if (symlen[sym] == 0) {
    dst[idx++] = w & 0x0fff;
    return idx;
  }
  idx = expand_symbol(dst, w & 0x0fff, idx, end, sympat, symlen);
  idx = expand_symbol(dst, (w >> 12) & 0x0fff, idx, end, sympat, symlen);
  return idx;
}

static uint8_t *table;
static uint64_t table_size = 0;

static struct PairsData *decomp_d;
static uint8_t *decomp_data;
static uint64_t *work_decomp = NULL;

static void decompress_worker(struct thread_data *thread)
{
  uint64_t idx = thread->begin;
  uint64_t end = thread->end;
  struct PairsData *d = decomp_d;
  uint8_t *dst = table;

  if (!d->idxbits) {
    int s = d->min_len;
    for (; idx < end; idx++)
      dst[idx] = s;
    return;
  }

  int l;
  int m = d->min_len;
  uint16_t *offset = d->offset;
  uint64_t *base = d->base - m;
  uint8_t *symlen = d->symlen;
  uint8_t *sympat = d->sympat;
  int sym, bitcnt;

  uint32_t mainidx = idx >> d->idxbits;
  int litidx = (idx & ((1 << d->idxbits) -1)) - (1 << (d->idxbits - 1));
  uint32_t block = *(uint32_t *)(d->indextable + 6 * mainidx);
  litidx += *(uint16_t *)(d->indextable + 6 * mainidx + 4);
  if (litidx < 0) {
    do {
      litidx += d->sizetable[--block] + 1;
    } while (litidx < 0);
  } else {
    mainidx++;
    while (litidx > d->sizetable[block])
      litidx -= d->sizetable[block++] + 1;
  }

  uint64_t idx2 = (1ULL << (d->idxbits - 1)) + (((uint64_t)mainidx) << d->idxbits);
  if (litidx > 0) {
    idx += d->sizetable[block++] + 1 - litidx;
    while (idx >= idx2) {
      idx2 += 1ULL << d->idxbits;
      mainidx++;
    }
  }

  uint8_t *data = decomp_data + (((uint64_t)block) << d->blocksize);
  while (idx < end) {
    int size = d->sizetable[block] + 1;
    while (idx + size > idx2) {
      if (*(uint32_t *)(d->indextable + 6 * mainidx) != block
          || *((uint16_t *)(d->indextable + 6 * mainidx + 4)) != idx2 - idx)
      {
        fprintf(stderr, "ERROR in main index!!\n");
        exit(EXIT_FAILURE);
      }
      idx2 += 1ULL << d->idxbits;
      mainidx++;
    }
    block++;

    uint64_t blockend = idx + size;
    if (blockend > table_size) blockend = table_size;

    uint32_t *ptr = (uint32_t *)data;
    uint64_t code = __builtin_bswap64(*(uint64_t *)ptr);
    ptr += 2;
    bitcnt = 0;
    while (idx < blockend) {
      l = m;
      while (code < base[l]) l++;
      sym = offset[l] + ((code - base[l]) >> (64 - l));
      idx = expand_symbol(dst, sym, idx, blockend, sympat, symlen);
      code <<= l;
      bitcnt += l;
      if (bitcnt >= 32) {
        bitcnt -= 32;
        code |= ((uint64_t)(__builtin_bswap32(*ptr++))) << bitcnt;
      }
    }
    data += 1 << d->blocksize;
  }
}

uint8_t *decompress_table(struct tb_handle *H, int bside, int f)
{
  if (!H->split && bside) return table;

  uint64_t tb_size = H->file[f].size[bside];
  if (tb_size != table_size) {
    if (table) free(table);
    table = malloc(tb_size);
    table_size = tb_size;
    if (!work_decomp)
      work_decomp = alloc_work(total_work);
    fill_work(total_work, tb_size, 0, work_decomp);
  }

  decomp_d = !H->has_pawns ? H->entry_piece.precomp[bside]
                            : H->entry_pawn.file[f].precomp[bside];
  decomp_data = H->data + H->file[f].idx[bside];

  run_threaded(decompress_worker, work_decomp, 1);

  return table;
}

struct tb_handle *open_tb_file(const char *name, int wdl)
{
  struct tb_handle *H = malloc(sizeof(struct tb_handle));

  if (!H) {
    fprintf(stderr, "Could not allocate sufficient memory.\n");
    exit(EXIT_FAILURE);
  }
  if (!(H->F = fopen(name, "rb"))) {
    fprintf(stderr, "Could not open %s for reading.\n", name);
    free(H);
    exit(EXIT_FAILURE);
  }
  FD fd = open_file(name);
  if (fd == FD_ERR) {
    fprintf(stderr, "Could not open %s for memory mapping.\n", name);
    fclose(H->F);
    free(H);
    exit(EXIT_FAILURE);
  }
  H->data_size = file_size(fd);
  H->data = (uint8_t *)map_file(fd, 1, &(H->mmap));
  close_file(fd);
  H->wdl = wdl;

  return H;
}

struct tb_handle *open_tb_handle(char *tablename, int wdl)
{
  size_t len = strlen(tablename) + strlen(wdl ? WDLSUFFIX : DTZSUFFIX) + 1;
  char *name = malloc(len);
  struct tb_handle *H;

  if (!name) {
    fprintf(stderr, "Could not allocate sufficient memory.\n");
    exit(EXIT_FAILURE);
  }
  snprintf(name, len, "%s%s", tablename, wdl ? WDLSUFFIX : DTZSUFFIX);
  H = open_tb_file(name, wdl);
  free(name);
  return H;
}

// incomplete deallocation, but who cares
void close_tb(struct tb_handle *H)
{
  int f;

  fclose(H->F);
  unmap_file((char *)H->data, H->mmap);

  if (!H->has_pawns) {
    struct TBEntry_piece *entry = &(H->entry_piece);
    free(entry->precomp[0]->indextable);
    free(entry->precomp[0]->sizetable);
    free(entry->precomp[0]);
    if (H->split) {
      free(entry->precomp[1]->indextable);
      free(entry->precomp[1]->sizetable);
      free(entry->precomp[1]);
    }
  } else {
    struct TBEntry_pawn *entry = &(H->entry_pawn);
    for (f = 0; f < H->num_files; f++) {
      free(entry->file[f].precomp[0]->indextable);
      free(entry->file[f].precomp[0]->sizetable);
      free(entry->file[f].precomp[0]);
      if (H->split) {
        free(entry->file[f].precomp[1]->indextable);
        free(entry->file[f].precomp[1]->sizetable);
        free(entry->file[f].precomp[1]);
      }
    }
  }
}

void set_perm(struct tb_handle *H, int bside, int f, int *perm, int *pt)
{
  int i, j;
  uint8_t *pieces = !H->has_pawns ? H->entry_piece.pieces[bside]
                                : H->entry_pawn.file[f].pieces[bside];
  int n = !H->has_pawns ? H->entry_piece.num : H->entry_pawn.num;
  int k = 0;

  for (i = 0, j = 0; i < n;) {
    if (pieces[i] != k) {
      k = pieces[i];
      j = 0;
    }
    while (pt[j] != k) j++;
    perm[j++] = i++;
  }
}

struct TBEntry *get_entry(struct tb_handle *H)
{
  return &(H->entry);
}

int get_ply_accurate_win(struct tb_handle *H, int f)
{
  return (H->dtz_flags[f] & (1 << 2)) != 0;
}

int get_ply_accurate_loss(struct tb_handle *H, int f)
{
  return (H->dtz_flags[f] & (1 << 3)) != 0;
}

int get_dtz_side(struct tb_handle *H, int f)
{
  return H->dtz_flags[f] & 0x01;
}

uint8_t (*get_dtz_map(struct tb_handle *H, int f))[256]
{
  if ((H->dtz_flags[f] & 0x02) && !(H->dtz_flags[0] & 16))
    return H->map[f];
  else
    return NULL;
}

uint16_t (*get_dtz_map16(struct tb_handle *H, int f))[MAX_VALS]
{
  if ((H->dtz_flags[f] & 0x02) && (H->dtz_flags[0] & 16))
    return H->map16[f];
  else
    return NULL;
}

/*
  Copyright (c) 2026

  This file is distributed under the terms of the GNU GPL, version 2.
*/

#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>


int numpcs;
int numpawns;

static const char *output_dir = ".";
static int force;

static void usage(FILE *F, const char *program)
{
  fprintf(F,
      "Usage: %s [OPTIONS]\n"
      "Decompress pawnless or pawnful tablebase files into raw symbol streams.\n"
      "Pawnful inputs export their a-, b-, c-, and d-file subtables.\n\n"
      "  -iw, --input-rtbw FILE  decompress the WDL table FILE\n"
      "  -iz, --input-rtbz FILE  decompress the DTZ table FILE\n"
      "  -o, --output-dir DIR  write raw files to DIR (default: .)\n"
      "  -t, --threads N       use N decompression threads (default: 1)\n"
      "  -f, --force           overwrite existing output files\n"
      "  -h, --help            display this help and exit\n\n"
      "Each output filename identifies its WTM, BTM, or shared subtable.\n",
      program);
}

static char *material_from_input(const char *name, const char *suffix)
{
  const char *base = name;
  const char *slash = strrchr(name, '/');
  const char *backslash = strrchr(name, '\\');
  size_t baselen;
  char *material;

  if (slash && (!backslash || slash > backslash))
    base = slash + 1;
  else if (backslash)
    base = backslash + 1;
  baselen = strlen(base);
  if (baselen <= strlen(suffix) ||
      strcmp(base + baselen - strlen(suffix), suffix)) {
    fprintf(stderr, "%s must have the %s suffix.\n", name, suffix);
    exit(EXIT_FAILURE);
  }
  material = malloc(baselen - strlen(suffix) + 1);
  if (!material) {
    fprintf(stderr, "Could not allocate sufficient memory.\n");
    exit(EXIT_FAILURE);
  }
  memcpy(material, base, baselen - strlen(suffix));
  material[baselen - strlen(suffix)] = '\0';
  return material;
}

static char *make_path(const char *dir, const char *name, const char *suffix)
{
  size_t dirlen = strlen(dir);
  int separator = dirlen && dir[dirlen - 1] != '/' && dir[dirlen - 1] != '\\';
  size_t len = dirlen + separator + strlen(name) + strlen(suffix) + 1;
  char *path = malloc(len);

  if (!path) {
    fprintf(stderr, "Could not allocate sufficient memory.\n");
    exit(EXIT_FAILURE);
  }
  snprintf(path, len, "%s%s%s%s", dir, separator ? "/" : "", name, suffix);
  return path;
}

static void parse_material(const char *material, int *pcs)
{
  int color = 0;
  int kings[2] = { 0, 0 };
  int separators = 0;

  memset(pcs, 0, 16 * sizeof(*pcs));
  numpcs = 0;
  numpawns = 0;

  if (!*material)
    goto invalid;

  for (const char *p = material; *p; p++) {
    int piece;

    switch (*p) {
    case 'P': piece = PAWN; numpawns++; break;
    case 'N': piece = KNIGHT; break;
    case 'B': piece = BISHOP; break;
    case 'R': piece = ROOK; break;
    case 'Q': piece = QUEEN; break;
    case 'K': piece = KING; kings[color != 0]++; break;
    case 'v':
      if (separators++ || color || p == material || !p[1])
        goto invalid;
      color = 0x08;
      continue;
    default:
      goto invalid;
    }
    pcs[piece | color]++;
    numpcs++;
  }

  if (separators != 1 || kings[0] != 1 || kings[1] != 1)
    goto invalid;
  if (numpcs < 3) {
    fprintf(stderr, "%s has fewer than three pieces.\n", material);
    exit(EXIT_FAILURE);
  }
  if (numpcs > TBPIECES) {
    fprintf(stderr, "%s has %d pieces; this build supports at most %d.\n",
        material, numpcs, TBPIECES);
    exit(EXIT_FAILURE);
  }
  return;

invalid:
  fprintf(stderr, "Invalid regular tablebase material name: %s\n", material);
  exit(EXIT_FAILURE);
}

static FILE *open_output(const char *final_name, char **temporary_name)
{
  FILE *F;
  size_t len;

  if (!force) {
    F = fopen(final_name, "rb");
    if (F) {
      fclose(F);
      fprintf(stderr, "%s already exists (use --force to overwrite it).\n",
          final_name);
      exit(EXIT_FAILURE);
    }
  }

  len = strlen(final_name) + 5;
  *temporary_name = malloc(len);
  if (!*temporary_name) {
    fprintf(stderr, "Could not allocate sufficient memory.\n");
    exit(EXIT_FAILURE);
  }
  snprintf(*temporary_name, len, "%s.tmp", final_name);
  F = fopen(*temporary_name, "wb");
  if (!F) {
    fprintf(stderr, "Could not open %s for writing: %s\n", *temporary_name,
        strerror(errno));
    exit(EXIT_FAILURE);
  }
  return F;
}

static void write_all(FILE *F, const char *name, const uint8_t *data,
    uint64_t size)
{
  const size_t max_chunk = 64 * 1024 * 1024;

  while (size) {
    size_t chunk = size > (uint64_t)max_chunk ? max_chunk : (size_t)size;
    size_t written = fwrite(data, 1, chunk, F);
    if (!written) {
      fprintf(stderr, "Could not write %s: %s\n", name,
          ferror(F) ? strerror(errno) : "zero-byte write");
      exit(EXIT_FAILURE);
    }
    data += written;
    size -= written;
    if (written != chunk && ferror(F)) {
      fprintf(stderr, "Could not write %s: %s\n", name, strerror(errno));
      exit(EXIT_FAILURE);
    }
  }
}

static void finish_output(FILE *F, const char *temporary_name,
    const char *final_name)
{
  if (fclose(F)) {
    fprintf(stderr, "Could not close %s: %s\n", temporary_name,
        strerror(errno));
    exit(EXIT_FAILURE);
  }
  if (force)
    remove(final_name);
  if (rename(temporary_name, final_name)) {
    fprintf(stderr, "Could not rename %s to %s: %s\n", temporary_name,
        final_name, strerror(errno));
    exit(EXIT_FAILURE);
  }
}

static void decompress_wdl(const char *input, const char *material)
{
  struct tb_handle *H = open_tb_file(input, 1);

  decomp_init_table(H);
  for (int f = 0; f < H->num_files; f++) {
    for (int bside = 0; bside < (H->split ? 2 : 1); bside++) {
      char suffix[32];
      char side = !H->split ? 's' : (bside ? 'b' : 'w');
      char *output;
      char *temporary;
      FILE *F;
      uint64_t size = H->file[f].size[bside];
      uint8_t *data = decompress_table(H, bside, f);

      if (H->has_pawns)
        snprintf(suffix, sizeof(suffix), ".%c.%c.rtbw.raw", 'a' + f, side);
      else
        snprintf(suffix, sizeof(suffix), ".%c.rtbw.raw", side);
      output = make_path(output_dir, material, suffix);
      F = open_output(output, &temporary);
      printf("%s: %s, %" PRIu64 " bytes\n", output,
          bside ? "BTM" : (H->split ? "WTM" : "shared WDL"), size);
      write_all(F, temporary, data, size);
      finish_output(F, temporary, output);
      free(temporary);
      free(output);
    }
  }
  close_tb(H);
}

static void decompress_dtz(const char *input, const char *material)
{
  struct tb_handle *H = open_tb_file(input, 0);

  decomp_init_table(H);
  for (int f = 0; f < H->num_files; f++) {
    char suffix[32];
    int bside = get_dtz_side(H, f);
    char *output;
    char *temporary;
    FILE *F;
    uint64_t size = H->file[f].size[0];
    uint8_t *data = decompress_table(H, 0, f);

    if (H->has_pawns)
      snprintf(suffix, sizeof(suffix), ".%c.%c.rtbz.raw", 'a' + f,
          bside ? 'b' : 'w');
    else
      snprintf(suffix, sizeof(suffix), ".%c.rtbz.raw", bside ? 'b' : 'w');
    output = make_path(output_dir, material, suffix);
    F = open_output(output, &temporary);
    printf("%s: %s DTZ, %" PRIu64 " bytes\n", output,
        bside ? "BTM" : "WTM", size);
    write_all(F, temporary, data, size);
    finish_output(F, temporary, output);
    free(temporary);
    free(output);
  }
  close_tb(H);
}

int main(int argc, char **argv)
{
  int pcs[16];
  int value;
  const char *input_wdl = NULL;
  const char *input_dtz = NULL;
  char *material_wdl = NULL;
  char *material_dtz = NULL;
  const char *material;
  static struct option options[] = {
    { "input-rtbw", required_argument, NULL, 'w' },
    { "input-rtbz", required_argument, NULL, 'z' },
    { "output-dir", required_argument, NULL, 'o' },
    { "threads", required_argument, NULL, 't' },
    { "force", no_argument, NULL, 'f' },
    { "help", no_argument, NULL, 'h' },
    { NULL, 0, NULL, 0 }
  };

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-iw")) argv[i] = const_cast<char *>("-w");
    if (!strcmp(argv[i], "-iz")) argv[i] = const_cast<char *>("-z");
  }
  numthreads = 1;
  while ((value = getopt_long(argc, argv, "w:z:o:t:fh", options, NULL)) != -1) {
    switch (value) {
    case 'w': input_wdl = optarg; break;
    case 'z': input_dtz = optarg; break;
    case 'o': output_dir = optarg; break;
    case 't': {
      char *end;
      long threads = strtol(optarg, &end, 10);
      if (!*optarg || *end || threads < 1 || threads > 1024) {
        fprintf(stderr, "The thread count must be a positive integer.\n");
        return EXIT_FAILURE;
      }
      numthreads = threads;
      break;
    }
    case 'f': force = 1; break;
    case 'h': usage(stdout, argv[0]); return EXIT_SUCCESS;
    default: usage(stderr, argv[0]); return EXIT_FAILURE;
    }
  }
  if (optind != argc || (!input_wdl && !input_dtz)) {
    usage(stderr, argv[0]);
    return EXIT_FAILURE;
  }

  if (input_wdl) material_wdl = material_from_input(input_wdl, ".rtbw");
  if (input_dtz) material_dtz = material_from_input(input_dtz, ".rtbz");
  if (material_wdl && material_dtz && strcmp(material_wdl, material_dtz)) {
    fprintf(stderr, "WDL and DTZ inputs must have the same material name.\n");
    return EXIT_FAILURE;
  }
  material = material_wdl ? material_wdl : material_dtz;
  parse_material(material, pcs);
  if (numpawns) {
    int primary_pawn = pcs[WPAWN] > 0 &&
        (pcs[BPAWN] == 0 || pcs[WPAWN] <= pcs[BPAWN]) ? WPAWN : BPAWN;
    decomp_init_pawn(pcs, &primary_pawn);
  } else {
    decomp_init_piece(pcs);
  }
  total_work = numthreads == 1 ? 1 : 100 + 10 * numthreads;
  init_threads(0);
  gettimeofday(&cur_time, NULL);

  if (input_wdl) decompress_wdl(input_wdl, material);
  if (input_dtz) decompress_dtz(input_dtz, material);
  free(material_wdl);
  free(material_dtz);
  return EXIT_SUCCESS;
}
