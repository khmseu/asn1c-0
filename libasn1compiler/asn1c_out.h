#ifndef	_ASN1_COMPILED_OUTPUT_H_
#define	_ASN1_COMPILED_OUTPUT_H_

/*
 * An elementary chunk of target language text.
 */
typedef struct out_chunk {
	char *buf;
	int len;

	TQ_ENTRY(struct out_chunk) next;
} out_chunk_t;

typedef struct compiler_streams {
	enum {
		OT_IGNORE,	/* Ignore this output */
		OT_INCLUDES,	/* #include files */
		OT_DEPS,	/* Dependencies (other than #includes) */
		OT_FWD_DECLS,	/* Forward declarations */
		OT_TYPE_DECLS,	/* Type declarations */
		OT_FUNC_DECLS,	/* Function declarations */
		OT_POST_INCLUDE,/* #include after type definition */
		OT_CTABLES,	/* Constraint tables */
		OT_CODE,	/* Some code */
		OT_CTDEFS,	/* Constraint definitions */
		OT_STAT_DEFS,	/* Static definitions */
		OT_MAX
	} target;

	struct compiler_stream_destination_s {
		TQ_HEAD(out_chunk_t) chunks;
		int indent_level;
		int indented;
	} destination[OT_MAX];
} compiler_streams_t;

static char *_compiler_stream2str[] __attribute__ ((unused))
    = { "IGNORE", "INCLUDES", "DEPS", "FWD-DECLS", "TYPE-DECLS", "FUNC-DECLS", "POST-INCLUDE", "CTABLES", "CODE", "CTDEFS", "STAT-DEFS" };

int asn1c_compiled_output(arg_t *arg, const char *fmt, ...) __attribute__((format(printf,2, 3)));


/*****************************************************************
 * Useful macros for invoking asn1c_compiled_output() and friends.
 */

/* Redirect output to a different stream. */
#define	REDIR(foo)	do { arg->target->target = foo; } while(0)
#define INDENT_LEVEL	\
		arg->target->destination[arg->target->target].indent_level
#define	INDENT(val)	INDENT_LEVEL += (val)
#define	INDENTED(code)	do {					\
		INDENT(+1);					\
		do { code; } while(0);				\
		INDENT(-1);					\
	} while(0)

#define	EMBED(ev)	do {					\
		int saved_target = arg->target->target;		\
		REDIR(OT_TYPE_DECLS);				\
		arg->embed++;					\
		INDENTED(arg_t _tmp = *arg;			\
			_tmp.expr = ev;				\
			_tmp.default_cb(&_tmp);			\
		);						\
		arg->embed--;					\
		if(ev->expr_type != A1TC_EXTENSIBLE)		\
			OUT(";\n");				\
		assert(arg->target->target == OT_TYPE_DECLS);	\
		REDIR(saved_target);				\
	} while(0)

/* Output a piece of text into a default stream */
#define	OUT(fmt, args...)	asn1c_compiled_output(arg, fmt, ##args)
#define	OUT_NOINDENT(fmt, args...)	do {			\
	int _saved_indent = INDENT_LEVEL;			\
	INDENT_LEVEL = 0;					\
	OUT(fmt, ##args);					\
	INDENT_LEVEL = _saved_indent;				\
} while(0)
#define	OUT_DEBUG(fmt, args...) do {				\
		if(arg->flags & A1C_DEBUG) OUT(fmt, ##args);	\
	} while(0)

/* Generate #include line */
#define	GEN_INCLUDE_STD(typename)	do {			\
	if((arg->flags & A1C_INCLUDES_QUOTED)) {			\
		GEN_INCLUDE("\"" typename ".h\"");		\
	} else {						\
		GEN_INCLUDE("<" typename ".h>");		\
	} } while(0)
#define	GEN_INCLUDE(filename)	do {				\
	int saved_target = arg->target->target;			\
	if(!filename) break;					\
	REDIR(OT_INCLUDES);					\
	OUT_NOINDENT("#include %s\n", filename);		\
	REDIR(saved_target);					\
} while(0)
#define	GEN_POSTINCLUDE(filename)	do {			\
	int saved_target = arg->target->target;			\
	if(!filename) break;					\
	REDIR(OT_POST_INCLUDE);					\
	OUT_NOINDENT("#include %s\n", filename);		\
	REDIR(saved_target);					\
} while(0)

/* Generate ASN.1 type declaration */
#define	GEN_DECLARE(expr)	do {				\
	int saved_target = arg->target->target;			\
	REDIR(OT_FUNC_DECLS);					\
	OUT_NOINDENT("extern asn_TYPE_descriptor_t "		\
			"asn_DEF_%s;\n", MKID(expr));		\
	REDIR(saved_target);					\
} while(0)

/*
 * Format LONG_MIN according to C90 rules.
 */
#define OINT(iv)	do {					\
	if(iv == (-2147483647L - 1))				\
		OUT("(-2147483647L - 1)");			\
	else							\
		OUT("%" PRIdASN, iv);				\
} while(0)

#define OINTS(iv)	do {					\
	if(iv == (-2147483647L - 1))				\
		OUT("(-2147483647L - 1)");			\
	else							\
		OUT("% " PRIdASN, iv);				\
} while(0)

#endif	/* _ASN1_COMPILED_OUTPUT_H_ */
