#define LEN(a) (sizeof(a) / sizeof((a)[0]))
#define ALIGN(x, n) (((x) + (n) - 1) & -(n))

typedef struct Net Net;
typedef struct Node Node;
typedef struct Port Port;

/* numeric operator */
enum {
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_MOD,
	OP_POW,
	OP_AND,
	OP_BOR,
	OP_XOR,
	OP_NOT,
	OP_SHR,
	OP_SHL,
	OP_GTR,
	OP_LES,
	OP_EQL,
	OP_FADD,
	OP_FSUB,
	OP_FMUL,
	OP_FDIV,
	OP_FMOD,
	OP_FPOW,
	OP_UTOF,
	OP_FTOU,
};

struct Port {
	enum {
		PORT_NUMB,
		PORT_ERAS,
		PORT_PTR0,
		PORT_PTR1,
		PORT_PTR2,
	} kind;
	unsigned data;
};

struct Node {
	enum {
		NODE_CON,
		NODE_OP1,
		NODE_OP2,
		NODE_ITE,
	} kind : 2;
	unsigned op : 21;
	unsigned type : 9;  /* NUM, ERA, PTR0, PTR1, PTR2 (0:2 - port[0], 3:5 - port[1], 6:8 - port[2]) */
	unsigned port[3];
};

struct Net {
	Node *node;
	unsigned node_len, node_cap;
	int *redex;
	unsigned redex_len, redex_cap;
	unsigned free;
};

/* util.c */
void *reallocarray(void *, size_t, size_t);

/* net.c */
void net_reduce(Net *);
void net_find_redex(Net *);
