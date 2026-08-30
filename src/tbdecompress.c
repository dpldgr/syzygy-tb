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
    if (!strcmp(argv[i], "-iw")) argv[i] = "-w";
    if (!strcmp(argv[i], "-iz")) argv[i] = "-z";
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
