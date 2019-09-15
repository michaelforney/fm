#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "fm.h"

#define PTR(a, s) (Port){PORT_PTR0 + s, a}
#define NUM(n)    (Port){PORT_NUMB, n}
#define ERA       (Port){PORT_ERAS}

static unsigned
powi(unsigned fst, unsigned snd)
{
	unsigned res;

	for (res = 1; snd; snd >>= 1, fst *= fst) {
		if (snd & 1)
			res *= fst;
	}
	return res;
}

static void
grow(Net *net)
{
	net->node_cap = net->node_cap ? net->node_cap * 4 : 0x1000;
	net->node = reallocarray(net->node, net->node_cap, sizeof(net->node[0]));
	if (!net->node)
		abort();
}

static unsigned
new(Net *net, int kind, int op)
{
	Node *n;
	unsigned i;

	if (net->free) {
		i = net->free - 1;
		net->free = net->node[i].port[0];
	} else {
		assert(net->node_len < net->node_cap);
		i = net->node_len++;
	}
	n = &net->node[i];
	n->kind = kind;
	n->op = op;

	return i;
}

static void
del(Net *net, unsigned i)
{
	net->node[i].port[0] = net->free;
	net->free = i + 1;
}

static void
queue(Net *net, unsigned i)
{
	if (net->redex_len == net->redex_cap) {
		net->redex_cap = net->redex_cap ? net->redex_cap * 2 : 0x400;
		net->redex = reallocarray(net->redex, net->redex_cap, sizeof(net->redex[0]));
		if (!net->redex)
			abort();
	}
	net->redex[net->redex_len++] = i;
}

static inline void
set(Net *net, Port a, Port b)
{
	int slot = a.kind - PORT_PTR0;
	Node *n;

	assert(a.data < net->node_len);
	n = &net->node[a.data];
	n->port[slot] = b.data;
	n->type = (n->type & ~(7 << slot * 3)) | b.kind << slot * 3;
}

static inline void
link(Net *net, Port a, Port b)
{
	if (a.kind >= PORT_PTR0)
		set(net, a, b);
	if (b.kind >= PORT_PTR0)
		set(net, b, a);
	if (a.kind == PORT_PTR0 && b.kind <= PORT_PTR0)
		queue(net, a.data);
	else if (b.kind == PORT_PTR0 && a.kind < PORT_PTR0)
		queue(net, b.data);
}

static inline Port
enter(Node *node, int slot)
{
	return (Port){
		.kind = node->type >> slot * 3 & 7,
		.data = node->port[slot],
	};
}

static void
rewrite(Net *net, unsigned i)
{
	Node *a, *b;
	Port port;
	union {unsigned i; float f;} res, fst, snd;
	unsigned j, k, l;

	/* make sure we have enough space */
	if (net->node_cap - net->node_len < 2)
		grow(net);

	a = &net->node[i];
	port = enter(a, 0);
	switch (port.kind) {
	case PORT_NUMB:
		fst.i = port.data;
		switch (a->kind) {
		case NODE_OP1:
			snd.i = a->port[1];
			switch (a->op) {
			case OP_ADD:  res.i = fst.i + snd.i;       break;
			case OP_SUB:  res.i = fst.i - snd.i;       break;
			case OP_MUL:  res.i = fst.i * snd.i;       break;
			case OP_DIV:  res.i = fst.i / snd.i;       break;
			case OP_MOD:  res.i = fst.i % snd.i;       break;
			case OP_POW:  res.i = powi(fst.i, snd.i);  break;
			case OP_AND:  res.i = fst.i & snd.i;       break;
			case OP_BOR:  res.i = fst.i | snd.i;       break;
			case OP_XOR:  res.i = fst.i ^ snd.i;       break;
			case OP_NOT:  res.i = ~snd.i;              break;
			case OP_SHR:  res.i = fst.i >> snd.i;      break;
			case OP_SHL:  res.i = fst.i << snd.i;      break;
			case OP_GTR:  res.i = fst.i > snd.i;       break;
			case OP_LES:  res.i = fst.i < snd.i;       break;
			case OP_EQL:  res.i = fst.i == snd.i;      break;
			case OP_FADD: res.f = fst.f + snd.f;       break;
			case OP_FSUB: res.f = fst.f - snd.f;       break;
			case OP_FMUL: res.f = fst.f * snd.f;       break;
			case OP_FDIV: res.f = fst.f / snd.f;       break;
			case OP_FMOD: res.f = fmodf(fst.f, snd.f); break;
			case OP_FPOW: res.f = powf(fst.f, snd.f);  break;
			case OP_UTOF: res.f = snd.i;               break;
			case OP_FTOU: res.i = snd.f;               break;
			/* unreachable */
			default:      res.i = 0;                   break;
			}
			link(net, enter(a, 2), NUM(res.i));
			del(net, i);
			break;
		case NODE_OP2:
			a->kind = NODE_OP1;
			link(net, PTR(i, 0), enter(a, 1));
			link(net, PTR(i, 1), NUM(fst.i));
			break;
		case NODE_CON:
			link(net, enter(a, 1), NUM(fst.i));
			link(net, enter(a, 2), NUM(fst.i));
			del(net, i);
			break;
		case NODE_ITE:
			a->kind = NODE_CON;
			link(net, PTR(i, 0), enter(a, 1));
			if (fst.i) {
				link(net, PTR(i, 1), enter(a, 2));
				link(net, PTR(i, 2), ERA);
			} else {
				link(net, PTR(i, 1), ERA);
			}
		}
		break;
	case PORT_ERAS:
		link(net, enter(a, 1), ERA);
		link(net, enter(a, 2), ERA);
		del(net, i);
		break;
	case PORT_PTR0:
		j = port.data;
		assert(j < net->node_len);
		b = &net->node[j];
		if (a->kind == b->kind && (a->kind != NODE_CON || a->op == b->op)) {
			/* annihilation */
			link(net, enter(a, 1), enter(b, 1));
			link(net, enter(a, 2), enter(b, 2));
			del(net, i);
			assert(i != j);
			del(net, j);
		} else if (a->kind == NODE_CON && b->kind != NODE_OP1) {
			/* node/binary duplication */
			k = new(net, a->kind, a->op);
			l = new(net, b->kind, b->op);
			link(net, PTR(i, 0), enter(b, 1));
			link(net, PTR(j, 0), enter(a, 2));
			link(net, PTR(k, 0), enter(b, 2));
			link(net, PTR(l, 0), enter(a, 1));
			link(net, PTR(i, 2), PTR(j, 1));
			link(net, PTR(k, 1), PTR(l, 2));
			link(net, PTR(i, 1), PTR(l, 1));
			link(net, PTR(j, 2), PTR(k, 2));
		} else if (a->kind != NODE_OP2 && b->kind == NODE_OP1) {
			/* unary duplication */
			k = new(net, b->kind, b->op);
			link(net, PTR(i, 0), enter(b, 2));
			link(net, PTR(j, 0), enter(a, 1));
			link(net, PTR(k, 0), enter(a, 2));
			link(net, PTR(i, 1), PTR(j, 2));
			link(net, PTR(i, 2), PTR(k, 2));
			link(net, PTR(k, 1), NUM(b->port[1]));
		} else if (b->kind == NODE_CON) {
			/* permutations */
			rewrite(net, j);
		}
		break;
	case PORT_PTR1:
	case PORT_PTR2:
		/* unreachable */
		break;
	}
}

void
net_find_redex(Net *net)
{
	unsigned i;
	Port port;

	for (i = 0; i < net->node_len; ++i) {
		port = enter(&net->node[i], 0);
		if (port.kind == PORT_NUMB || (port.kind == PORT_PTR0 && port.data >= i))
			queue(net, i);
	}
}

void
net_reduce(Net *net)
{
	unsigned rewrites;

	for (rewrites = 0; net->redex_len > 0; ++rewrites)
		rewrite(net, net->redex[--net->redex_len]);

	printf("rewrites: %u\n", rewrites);
	printf("max nodes: %u\n", net->node_len);
}
