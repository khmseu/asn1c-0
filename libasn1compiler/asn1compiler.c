#include "asn1c_internal.h"
#include "asn1c_lang.h"
#include "asn1c_out.h"
#include "asn1c_save.h"

static void default_logger_cb(int, const char *fmt, ...) __attribute__((format(printf,2, 3)));
static int asn1c_compile_expr(arg_t *arg);
static int asn1c_attach_streams(asn1p_expr_t *expr);

int
asn1_compile(asn1p_t *asn, const char *datadir, enum asn1c_flags flags, const char *prefix,
		int argc, int optc, char **argv) {
	arg_t arg_s;
	arg_t *arg = &arg_s;
	asn1p_module_t *mod;
	int ret;

	/*
	 * Initialize target language.
	 */
	ret = asn1c_with_language(ASN1C_LANGUAGE_C);
	assert(ret == 0);

	memset(arg, 0, sizeof(*arg));
	arg->default_cb = asn1c_compile_expr;
	arg->logger_cb = default_logger_cb;
	arg->flags = flags;
	arg->prefix = prefix;
	arg->asn = asn;

	/*
	 * Compile each individual top level structure.
	 */
	TQ_FOR(mod, &(asn->modules), mod_next) {
		TQ_FOR(arg->expr, &(mod->members), next) {
			compiler_streams_t *cs = NULL;

			if(asn1c_attach_streams(arg->expr))
				return -1;

			cs = arg->expr->data;
			cs->target = OT_TYPE_DECLS;
			arg->target = cs;

			ret = asn1c_compile_expr(arg);
			if(ret) {
				FATAL("Cannot compile \"%s\" (%x:%x) at line %d",
					arg->expr->Identifier,
					arg->expr->expr_type,
					arg->expr->meta_type,
					arg->expr->_lineno);
				return ret;
			}
		}
	}

	DEBUG("Saving compiled data");

	/*
	 * Save or print out the compiled result.
	 */
	if(asn1c_save_compiled_output(arg, datadir, argc, optc, argv))
		return -1;

	return 0;
}

static int
asn1c_compile_expr(arg_t *arg) {
	asn1p_expr_t *expr = arg->expr;
	int (*type_cb)(arg_t *);
	int ret;

	assert((int)expr->meta_type >= AMT_INVALID);
	assert(expr->meta_type < AMT_EXPR_META_MAX);
	assert((int)expr->expr_type >= A1TC_INVALID);
	assert(expr->expr_type < ASN_EXPR_TYPE_MAX);

	type_cb = asn1_lang_map[expr->meta_type][expr->expr_type].type_cb;
	if(type_cb) {

		DEBUG("Compiling %s at line %d",
			expr->Identifier,
			expr->_lineno);

		if(expr->lhs_params && expr->spec_index == -1) {
			int i;
			ret = 0;
			DEBUG("Parameterized type %s at line %d: %s (%d)",
				expr->Identifier, expr->_lineno,
				expr->specializations.pspecs_count
				? "compiling" : "unused, skipping",
				expr->specializations.pspecs_count); /* was this what was meant? */
			for(i = 0; i<expr->specializations.pspecs_count; i++) {
				arg->expr = expr->specializations
						.pspec[i].my_clone;
				ret = asn1c_compile_expr(arg);
				if(ret) break;
			}
			arg->expr = expr;	/* Restore */
		} else {
			ret = type_cb(arg);
			if(arg->target->destination[OT_TYPE_DECLS]
					.indent_level == 0)
				OUT(";\n");
		}
	} else {
		ret = -1;
		/*
		 * Even if the target language compiler does not know
		 * how to compile the given expression, we know that
		 * certain expressions need not to be compiled at all.
		 */
		switch(expr->meta_type) {
		case AMT_OBJECT:
		case AMT_OBJECTCLASS:
		case AMT_OBJECTFIELD:
		case AMT_VALUE:
		case AMT_VALUESET:
			ret = 0;
			break;
		default:
			break;
		}
	}

	if(ret == -1) {
		FATAL("Cannot compile \"%s\" (%x:%x) at line %d",
			arg->expr->Identifier,
			arg->expr->expr_type,
			arg->expr->meta_type,
			arg->expr->_lineno);
		OUT("#error Cannot compile \"%s\" (%x/%x) at line %d\n",
			arg->expr->Identifier,
			arg->expr->meta_type,
			arg->expr->expr_type,
			arg->expr->_lineno
		);
	}

	return ret;
}

static int
asn1c_attach_streams(asn1p_expr_t *expr) {
	compiler_streams_t *cs;
	int i;

	if(expr->data)
		return 0;	/* Already attached? */

	expr->data = calloc(1, sizeof(compiler_streams_t));
	if(expr->data == NULL)
		return -1;

	cs = expr->data;
	for(i = 0; i < OT_MAX; i++) {
		TQ_INIT(&(cs->destination[i].chunks));
	}

	return 0;
}

static void
default_logger_cb(int _severity, const char *fmt, ...) {
	va_list ap;
	char *pfx = "";

	switch(_severity) {
	case -1: pfx = "DEBUG: "; break;
	case 0: pfx = "WARNING: "; break;
	case 1: pfx = "FATAL: "; break;
	}

	fprintf(stderr, "%s", pfx);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

