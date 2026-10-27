// Tushar sohan Lal 251ADB110
// AI used
// the sources https://eli.thegreenplace.net/2009/03/21/efficient-integer-exponentiation-algorithms
//https://en.wikipedia.org/wiki/Exponentiation_by_squaring
//https://cppreference.com/c/io/fread
//https://blog.jcoglan.com/2017/07/07/precedence-and-associativity-in-recursive-descent/
//https://www.both.org/?p=12023
//https://www.techtarget.com/it-infrastructure/definition/parser
//https://www.geeksforgeeks.org/c/header-files-in-c-cpp-and-its-uses/
//https://chatgpt.com/share/6ae09ad7-e208-83ed-a4a9-5a1824a03036 (my search history)

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// Defined once because both the folder name and the file name use them.
#define NAME "tushar"
#define LAST "sohan"
#define ID "12345"

// One shared pointer p is the position so error offsets are just p - buf + 1
static const char *buf, *p;
static long long errpos = 0;

// Only the first error is kept so later code cannot overwrite it.
static void fail(const char* at)
{
  if (!errpos) errpos = at - buf + 1; }

// '\n' is not skipped because it ends an expression.
static void skip(void) {
  while (*p == ' ' || *p == '\t' || *p == '\r') p++; }

static long long expr(void);
// Each level calls the next-tighter one and tighter operators finish first
// Only integers are accepted and float decision is not needed
static long long primary(void)
{

  skip();
  if (isdigit((unsigned char)*p)) {
    long long v = 0;
    while (isdigit((unsigned char)*p)) v = v * 10 + (*p++ - '0');
    return v;
}
  if (*p == '(') 
{
    p++;
    long long v = expr();
    if (errpos) return 0;
    skip();
    if (*p != ')') 
    {
      fail(p);  // the missing ')' is the mistake not'('
      return 0; }
    p++;
    return v;
  }
  fail(p);  // first character that does not fit the grammar
  return 0;
}

// power() recurses for the exponent so ** is right-associative: 2**(3**2).
//   expressions from text files, evaluates them, and writes each result, or the
//   exact position of the first error, to an output file."
//   I used ai here how recursion makes ** right-associative and how square-and-multiply works
static long long power(void) {
  long long b =primary();
  if (errpos) return 0;
  skip();
  if (p[0] == '*' && p[1] == '*') {
    p+=2;
    skip();
    const char* start = p;  // the exponent is the invalid part
    long long e = power();
    if (errpos) return 0;
    if (e < 0) 
    {
      fail(start);
      return 0;
    }
    
    
    
    
    // Square-and-multiply takes about log2(e) steps instead of e.
    long long r = 1;
    while (e) {
      if (e & 1) r *=b;
      e >>= 1;
      if (e) b *=b;
    }
    return r; }
  return b; }

// The loop makes * and / left-associative 8/2/2 is (8/2)/2
static long long term(void) {
  long long v = power();
  while (!errpos)
   {
    skip();
    char op = *p;
    if (op != '*' && op != '/') break;
    p++;
    skip();
    const char* start =p;  // the divisor is the invalid part
    long long r =power();
    if (errpos) return 0;
    if (op == '*')
      v *= r;
    else if (r==0)
     {
      fail(start);
      return 0;
    } else
      v /= r;  // C truncates toward zero, as the rules require
  }
  return v;
}

// The loop makes + and - from left to right
static long long expr(void) {
  long long v = term();
  while (!errpos) 
  {
    skip();
    char op = *p;
    if (op != '+' && op != '-') break;
    p++;
    long long r = term();
    if (errpos) return 0;
    v = (op == '+') ? v + r : v - r;
  }
  return v;
}

int main(int argc, char** argv) 
{
  const char *in = NULL, *outdir = NULL;
  for (int i = 1; i < argc; i++)
   {
    if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output-dir"))
     {
       if (++i >= argc)            
      {
        fprintf(stderr, "missing output folder\n");
        return 1;
      }
      outdir = argv[i]; 
    }
       else if (argv[i][0] == '-')
     {
      fprintf(stderr, "unknown option %s\n", argv[i]);
      return 1; }
     else
      in = argv[i];
  }
  if (!in) {
    fprintf(stderr, "usage: calc [-o OUTDIR] input.txt\n");
    return 1; }

  // One fread and in-place parsing avoid copying lines because large files can stay fast
  FILE* f = fopen(in, "rb");
  if (!f) {
    fprintf(stderr, "cannot open %s\n", in);
    return 1;
  }
  fseek(f, 0, SEEK_END);
      long size = ftell(f);
        fseek(f, 0, SEEK_SET);
         char* data = malloc(size + 1);
     if (!data) {
       fclose(f);  
    return 1;
  }
    size_t n = fread(data, 1, size, f);
      data[n] = 0;  // the NUL ends a last line that has no '\n'
       fclose(f);
        buf = p = data;

  // Results are buffered because an error must be the only output.
     size_t cnt = 0, cap = 1024;
    long long* res = malloc(cap * sizeof *res);
      while (*p && !errpos)
   {
    skip();
     if (*p == '\n') {
      p++;
        continue;
    }  // blank line
     if (*p == '#') {    // a comment only as the first non-blank character
      while (*p && *p != '\n') p++;
       continue;
    }
    long long v = expr();
     if (!errpos) {
       skip();
        if (*p != '\n' && *p)
             fail(p);  // only '\n' or end of file may follow an expression
    } 
    if (errpos) break;
    if (cnt == cap) {
      cap *= 2;  // doubling makes appending amortized O(1)
      res = realloc(res, cap * sizeof *res);
    }
    res[cnt++] = v;
    if (*p == '\n') p++;
  }

  // Output name base = input file name without folder and ".txt"
  char base[256], dir[512], path[1024];
  const char* slash = strrchr(in, '/');
  snprintf(base, sizeof base, "%s", slash ? slash + 1 : in);
  size_t bl = strlen(base);
  if (bl > 4 && !strcmp(base + bl - 4, ".txt")) base[bl - 4] = 0;
  if (!outdir) {
    snprintf(dir, sizeof dir, "%s_%s_%s", base, NAME, ID);
    outdir = dir;
  }
  // EEXIST is allowed so the program can be rerun on the same folder
  if (mkdir(outdir, 0777) != 0 && errno != EEXIST) {
    fprintf(stderr, "cannot create %s\n", outdir);
    return 1; }
  snprintf(path, sizeof path, "%s/%s_%s_%s_%s.txt", outdir, base, NAME, LAST,ID);
  FILE* out = fopen(path, "w");
  if (!out) 
  {
    fprintf(stderr, "cannot write %s\n", path);
    return 1; }

  if (errpos)
    fprintf(out, "ERROR:%lld\n", errpos);
  else
    for (size_t i =0; i < cnt;i++) fprintf(out, "%lld\n", res[i]);
  fclose(out);
    free(res);
      free(data);
     return 0;  // a parse error is a normal result, not a program failure
}