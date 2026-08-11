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

#include "decompress.h"
#include "defs.h"
#include "threads.h"

int numpcs;
int numpawns;

static const char *input_dir = ".";
static const char *output_dir = ".";
static int force;

static void usage(FILE *F, const char *program)
{
  fprintf(F,
      "Usage: %s [OPTIONS] MATERIAL\n"
      "Decompress MATERIAL.rtbw and MATERIAL.rtbz into raw symbol streams.\n\n"
      "  -i, --input-dir DIR   read tablebases from DIR (default: .)\n"
      "  -o, --output-dir DIR  write raw files to DIR (default: .)\n"
      "  -t, --threads N       use N decompression threads (default: 1)\n"
      "  -f, --force           overwrite existing output files\n"
      "  -h, --help            display this help and exit\n\n"
      "The output files are MATERIAL.rtbw.raw and MATERIAL.rtbz.raw.\n"
      "For split WDL tables, the WTM stream is followed by the BTM stream.\n",
      program);
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
  if (numpawns) {
    fprintf(stderr,
        "Pawnful material is not supported yet; the initial implementation "
        "supports pawnless tables.\n");
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
  while (size) {
    size_t chunk = size > (uint64_t)SIZE_MAX ? SIZE_MAX : (size_t)size;
    size_t written = fwrite(data, 1, chunk, F);
    if (written != chunk) {
      fprintf(stderr, "Could not write %s: %s\n", name,
          ferror(F) ? strerror(errno) : "short write");
      exit(EXIT_FAILURE);
    }
    data += written;
    size -= written;
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

static void decompress_wdl(const char *base, const char *material)
{
  char *output = make_path(output_dir, material, ".rtbw.raw");
  char *temporary;
  FILE *F = open_output(output, &temporary);
  struct tb_handle *H = open_tb_handle((char *)base, 1);
  uint64_t offset = 0;

  decomp_init_table(H);
  for (int bside = 0; bside < (H->split ? 2 : 1); bside++) {
    uint64_t size = H->file[0].size[bside];
    uint8_t *data = decompress_table(H, bside, 0);
    printf("%s: offset %" PRIu64 ", %s, %" PRIu64 " bytes\n", output,
        offset, bside ? "BTM" : (H->split ? "WTM" : "shared WDL"), size);
    write_all(F, temporary, data, size);
    offset += size;
  }
  close_tb(H);
  finish_output(F, temporary, output);
  free(temporary);
  free(output);
}

static void decompress_dtz(const char *base, const char *material)
{
  char *output = make_path(output_dir, material, ".rtbz.raw");
  char *temporary;
  FILE *F = open_output(output, &temporary);
  struct tb_handle *H = open_tb_handle((char *)base, 0);

  decomp_init_table(H);
  uint64_t size = H->file[0].size[0];
  uint8_t *data = decompress_table(H, 0, 0);
  printf("%s: offset 0, %s DTZ, %" PRIu64 " bytes\n", output,
      get_dtz_side(H, 0) ? "BTM" : "WTM", size);
  write_all(F, temporary, data, size);
  close_tb(H);
  finish_output(F, temporary, output);
  free(temporary);
  free(output);
}

int main(int argc, char **argv)
{
  int pcs[16];
  int value;
  char *base;
  static struct option options[] = {
    { "input-dir", required_argument, NULL, 'i' },
    { "output-dir", required_argument, NULL, 'o' },
    { "threads", required_argument, NULL, 't' },
    { "force", no_argument, NULL, 'f' },
    { "help", no_argument, NULL, 'h' },
    { NULL, 0, NULL, 0 }
  };

  numthreads = 1;
  while ((value = getopt_long(argc, argv, "i:o:t:fh", options, NULL)) != -1) {
    switch (value) {
    case 'i': input_dir = optarg; break;
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
  if (optind + 1 != argc) {
    usage(stderr, argv[0]);
    return EXIT_FAILURE;
  }

  parse_material(argv[optind], pcs);
  decomp_init_piece(pcs);
  total_work = numthreads == 1 ? 1 : 100 + 10 * numthreads;
  init_threads(0);
  gettimeofday(&cur_time, NULL);

  base = make_path(input_dir, argv[optind], "");
  decompress_wdl(base, argv[optind]);
  decompress_dtz(base, argv[optind]);
  free(base);
  return EXIT_SUCCESS;
}
