#include <assert.h>

#include "postgres.h"
#include "fmgr.h"
#include "libpq/pqformat.h"


/*
 * Compile w/

gcc -fPIC -shared \
  -o xenoid.so xid.c \
  -I/usr/include/postgresql/server \
  -std=c99 -D_POSIX_SOURCE \
  -Wall -Wextra

 */


/*
 * Add to PostgreSQL with something like,

CREATE TYPE xenoid;
CREATE FUNCTION xenoid_in(cstring) RETURNS xenoid
  AS '<sofile>' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION xenoid_out(xenoid) RETURNS cstring
  AS '<sofile>' LANGUAGE C IMMUTABLE STRICT;
CREATE TYPE xenoid (
  internallength = 16,
  input = xenoid_in,
  output = xenoid_out,
  alignment = char
);

CREATE FUNCTION xenoid_lt(xenoid, xenoid) RETURNS bool
  AS '<sofile>' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR < (
  leftarg = xenoid, rightarg = xenoid,
  procedure = xenoid_lt,
  commutator = > ,
  negator = >= ,
  restrict = scalarltsel, join = scalarltjoinsel
);

CREATE FUNCTION xenoid_le(xenoid, xenoid) RETURNS bool
  AS '<sofile>' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR <= (
  leftarg = xenoid, rightarg = xenoid,
  procedure = xenoid_le,
  commutator = >= ,
  negator = >
);

CREATE FUNCTION xenoid_eq(xenoid, xenoid) RETURNS bool
  AS '<sofile>' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR = (
  leftarg = xenoid, rightarg = xenoid,
  procedure = xenoid_eq,
  commutator = = ,
  negator = !=
);

CREATE FUNCTION xenoid_gt(xenoid, xenoid) RETURNS bool
  AS '<sofile>' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR > (
  leftarg = xenoid, rightarg = xenoid,
  procedure = xenoid_gt,
  commutator = < ,
  negator = <= ,
  restrict = scalargtsel, join = scalargtjoinsel
);

CREATE FUNCTION xenoid_ge(xenoid, xenoid) RETURNS bool
  AS '<sofile>' LANGUAGE C IMMUTABLE STRICT;
CREATE OPERATOR >= (
  leftarg = xenoid, rightarg = xenoid,
  procedure = xenoid_ge,
  commutator = <= ,
  negator = <
);

CREATE FUNCTION xenoid_cmp(xenoid, xenoid) RETURNS integer
  AS '<sofile>' LANGUAGE C IMMUTABLE STRICT;


CREATE OPERATOR CLASS xenoid_ops
  DEFAULT FOR TYPE xenoid USING btree AS
  OPERATOR 1 < ,
  OPERATOR 2 <= ,
  OPERATOR 3 = ,
  OPERATOR 4 >= ,
  OPERATOR 5 > ,
  FUNCTION 1 xenoid_cmp(xenoid, xenoid)
;

...

-- When you don't want it anymore:
-- WARNING: THE CASCADE **WILL** CASCADE TO TABLES.
DROP TYPE xenoid CASCADE;
*/


PG_MODULE_MAGIC;


typedef struct Xid {
	uint8_t repr[16];
} Xid;


char is_hex(char c) {
	return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f');
}


uint8_t to_nibble(char c) {
	if('0' <= c && c <= '9') {
		return c - '0';
	} else if('a' <= c && c <= 'f') {
		return c - 'a' + 0xa;
	} else {
		assert(0);
	}
}


PG_FUNCTION_INFO_V1(xenoid_in);

Datum xenoid_in(PG_FUNCTION_ARGS) {
	char *str = PG_GETARG_CSTRING(0);

	// Validate the XID is exactly 32 lowercase hex digits.
	size_t len = 0;
	for(char *c = str; *c; ++c, ++len) {
		if(!is_hex(*c)) {
			ereport(
				ERROR,
				(
					errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					errmsg("invalid input syntax for xenoid: \"%s\"", str)
				)
			);
		}
	}
	if(len != 32) {
		ereport(
			ERROR,
			(
				errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				errmsg("invalid input syntax for xenoid: \"%s\"", str)
			)
		);
	}

	// Parse the XID.
	Xid *result = (Xid *) palloc(sizeof(Xid));
	size_t idx = 0;
	for(char *c = str; *c; c += 2, ++idx) {
		result->repr[idx] = (to_nibble(c[0]) << 4) | to_nibble(c[1]);
	}
	PG_RETURN_POINTER(result);
}


PG_FUNCTION_INFO_V1(xenoid_out);

Datum xenoid_out(PG_FUNCTION_ARGS) {
	Xid *xid = (Xid *) PG_GETARG_POINTER(0);
	char *result = palloc(33);
	const char *const hex_lookup = "0123456789abcdef";

	for(size_t idx = 0; idx < 16; ++idx) {
		uint8_t repr_byte = xid->repr[idx];
		result[idx * 2] = hex_lookup[repr_byte >> 4];
		result[idx * 2 + 1] = hex_lookup[repr_byte & 0xf];
	}
	result[32] = 0;
	PG_RETURN_CSTRING(result);
}


PG_FUNCTION_INFO_V1(xenoid_lt);

Datum xenoid_lt(PG_FUNCTION_ARGS) {
	Xid *a = (Xid *) PG_GETARG_POINTER(0);
	Xid *b = (Xid *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(memcmp(a->repr, b->repr, sizeof(Xid)) < 0);
}


PG_FUNCTION_INFO_V1(xenoid_le);

Datum xenoid_le(PG_FUNCTION_ARGS) {
	Xid *a = (Xid *) PG_GETARG_POINTER(0);
	Xid *b = (Xid *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(memcmp(a->repr, b->repr, sizeof(Xid)) <= 0);
}


PG_FUNCTION_INFO_V1(xenoid_eq);

Datum xenoid_eq(PG_FUNCTION_ARGS) {
	Xid *a = (Xid *) PG_GETARG_POINTER(0);
	Xid *b = (Xid *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(memcmp(a->repr, b->repr, sizeof(Xid)) == 0);
}


PG_FUNCTION_INFO_V1(xenoid_ge);

Datum xenoid_ge(PG_FUNCTION_ARGS) {
	Xid *a = (Xid *) PG_GETARG_POINTER(0);
	Xid *b = (Xid *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(memcmp(a->repr, b->repr, sizeof(Xid)) >= 0);
}


PG_FUNCTION_INFO_V1(xenoid_gt);

Datum xenoid_gt(PG_FUNCTION_ARGS) {
	Xid *a = (Xid *) PG_GETARG_POINTER(0);
	Xid *b = (Xid *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(memcmp(a->repr, b->repr, sizeof(Xid)) > 0);
}


PG_FUNCTION_INFO_V1(xenoid_cmp);

Datum xenoid_cmp(PG_FUNCTION_ARGS) {
	Xid *a = (Xid *) PG_GETARG_POINTER(0);
	Xid *b = (Xid *) PG_GETARG_POINTER(1);

	PG_RETURN_INT32(memcmp(a->repr, b->repr, sizeof(Xid)));
}
