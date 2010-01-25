#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <Python.h>
#include <stdint.h>

typedef struct {
  int ai, bj, size;
} matchingblock;

typedef struct list {
  union {int i; matchingblock *block;} data;
  struct list* next;
} list;

static matchingblock *find_longest_match(const uint8_t *a, int alo, int ahi,
					 const uint8_t *b, int blo, int bhi,
					 list *b2j[])
{
  int n = bhi-blo+1;
  matchingblock *best = malloc(sizeof(matchingblock));
  int besti = alo, bestj = blo, bestsize = 0;
  int *j2len = calloc(n, sizeof(int));
  int i;

  for (i = alo; i < ahi; ++i) {
    int *newj2len = calloc(n, sizeof(int));
    list *l;
    uint8_t elt = a[i];
    for (l = b2j[elt]; l; l = l->next) {
      int k;
      int j = l->data.i;

      if (j < blo)
	continue;
      if (j >= bhi)
	break;

      j -= blo;

      if (j > 0) {
	k = newj2len[j] = j2len[j -1] + 1;
      } else
	k = newj2len[j] = 1;

      if (k > bestsize) {
	besti = i - k + 1;
	bestj = j - k + 1;
	bestsize = k;
      }
    }
    //memcpy (j2len, newj2len, s);
    free(j2len);
    j2len = newj2len;
  }

  bestj += blo;

  if (bestsize && (besti + bestsize > ahi || bestj + bestsize > bhi))
    printf ("longest : %d-%d  %d-%d found %d-%d %d-%d\n", alo, ahi, blo, bhi, besti, besti+bestsize, bestj, bestj+bestsize);

  /*while (besti > alo && bestj > blo && a[besti - 1] == b[bestj -1]) {
    --besti;
    --bestj;
    ++bestsize;
  }

  while (besti + bestsize < ahi && bestj + bestsize < bhi
	 && a[besti + bestsize] == b[besti + bestsize]) {
    ++bestsize;
  }*/

  free(j2len);

  best->ai = besti;
  best->bj = bestj;
  best->size = bestsize;
  return best;
}

static list *recurse (const uint8_t *a, int alo, int ahi,
		      const uint8_t *b, int blo, int bhi,
		      list *b2j[])
{
  list *blocks = NULL, *tail = NULL;
  matchingblock *block = find_longest_match(a, alo, ahi, b, blo, bhi, b2j);
  /* No need to keep searching for small matches, they won't benefit us */
  if (block->size > 2) {
    list *item = malloc(sizeof(list));
    item->data.block = block;
    item->next = NULL;
    blocks = tail = item;

    if (alo < block->ai && blo < block->bj) {
      tail->next = recurse (a, alo, block->ai, b, blo, block->bj, b2j);
      while (tail->next)
	tail = tail->next;
    }
    if (block->ai + block->size < ahi && block->bj + block-> size < bhi) {
      tail->next = recurse (a, block->ai + block->size, ahi,
			    b, block->bj + block-> size, bhi,
			    b2j);
      while (tail->next)
	tail = tail->next;
    }
  }
  return blocks;
}
void freeilist(list * l) {
  list *t;
  while (l) {
    t = l;
    l = l->next;
    free(t);
  }
}

void freeblist(list* l) {
  list *t;
  while (l) {
    t = l;
    l = l->next;
    free (t->data.block);
    free (t);
  }
}

static list *get_matching_blocks(const uint8_t *a, int la, const uint8_t *b, int lb)
{
  /*static uint8_t *bc = NULL;
  static int lbc = 0;
  static*/ list *b2j[256] = {0, };

  /*if (lb != lbc || memcmp(b, bc, lb) != 0) {*/
    list *tails[256];
    int i;

    /*free(bc);
    bc = malloc(lb);
    memcpy(bc, b, lb);
    lbc = lb;

    for (i = 0; i < 256; ++i)
      freeilist(b2j[i]);

      memset(b2j, 0, 256);*/

    for (i = 0; i < lb; ++i) {
      uint8_t elt = b[i];

      list *newi = alloca (sizeof(list));
      newi->data.i = i;
      newi->next = NULL;

      if (b2j[elt])
	tails[elt]->next = newi;
      else
	b2j[elt] = newi;

      tails[elt] = newi;
    }
    //}
  return recurse (a, 0, la, b, 0, lb, b2j);
}

#if 0
int main (int argc, char *argv[]) {
  if (argc < 4)
    return 1;

  list *blocks = get_matching_blocks(argv[1], strlen(argv[1]), argv[2], strlen(argv[2]));
  list *l;
  for (l = blocks; l; l = l->next) {
    matchingblock *b = l->data.block;
    printf ("%d %d %d\n", b->ai, b->bj, b->size);
  }

  blocks = get_matching_blocks(argv[1], strlen(argv[1]), argv[3], strlen(argv[3]));
  for (l = blocks; l; l = l->next) {
    matchingblock *b = l->data.block;
    printf ("%d %d %d\n", b->ai, b->bj, b->size);
  }
  return 0;
}
#endif

PyObject *py_blocks(PyObject *self, PyObject *args) {
  uint8_t *a, *b;
  int la, lb;
  list *blocks, *l;
  int i = 0;
  PyObject *ret;
  if(!PyArg_ParseTuple(args, "s#s#", &a, &la, &b, &lb))
    return NULL;

  blocks = get_matching_blocks(a, la, b, lb);
  for (l = blocks; l; l = l->next)
    ++i;
  ret = PyList_New(i);

  for (i = 0, l = blocks; l; ++i, l = l->next) {
    matchingblock *block = l->data.block;
    PyObject *item = Py_BuildValue("(iii)", block->ai, block->bj, block->size);
    PyList_SET_ITEM(ret, i, item);
  }

  PyList_Sort(ret);
  PyList_Append(ret, Py_BuildValue("(iii)", la, lb, 0));

  freeblist(blocks);

  return ret;
}

static PyMethodDef py_diff_methods[] = {
	{ "get_matching_blocks", (PyCFunction)py_blocks, METH_VARARGS, NULL },
	{ NULL, NULL, 0, NULL }
};

void initdiff(void)
{
	Py_InitModule3("diff", py_diff_methods, NULL);
}
