/*
  Helpers needed by the standalone pawnless decompressor without pulling in
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
  (void)factor; (void)num; (void)order; (void)order2; (void)norm; (void)file;
  fprintf(stderr, "Pawnful table decompression is not supported yet.\n");
  exit(EXIT_FAILURE);
}

void set_norm_pawn(struct TBEntry_pawn *entry, uint8_t *norm,
    uint8_t *pieces, int order, int order2)
{
  (void)entry; (void)norm; (void)pieces; (void)order; (void)order2;
  fprintf(stderr, "Pawnful table decompression is not supported yet.\n");
  exit(EXIT_FAILURE);
}
