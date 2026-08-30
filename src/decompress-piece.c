/*
  Helpers needed by the standalone decompressor without pulling in
  the probing implementation from probe.c.

  This file is distributed under the terms of the GNU GPL, version 2.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "probe.h"

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
