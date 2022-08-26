/* -------------------------------------------------------------------------
 *
 * kwlookup.c
 * 	  lexical token lookup for key words in openGauss
 *
 * NB - this file is also used by ECPG and several frontend programs in
 * src/bin/ including pg_dump and psql
 *
 * Portions Copyright (c) 1996-2012, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 * 	  src/backend/parser/kwlookup.c
 *
 * -------------------------------------------------------------------------
 */

/* use c.h so this can be built as either frontend or backend */
#include "c.h"
#include <ctype.h>
#include "postgres_fe.h"
#include "nodes/pg_list.h"
#include "parser/scanner.h"
#include "parser/scansup.h"
#include "datatypes.h"
#include "nodes/primnodes.h"
#include "nodes/value.h"
#include "catalog/pg_attribute.h"
#include "access/tupdesc.h"
#include "nodes/parsenodes_common.h"
#include "gram.hpp"
#include "parser/keywords.h"

/*
 * ScanKeywordLookup - see if a given word is a keyword
 *
 * Returns a pointer to the ScanKeyword table entry, or NULL if no match.
 *
 * The match is done case-insensitively.  Note that we deliberately use a
 * dumbed-down case conversion that will only translate 'A'-'Z' into 'a'-'z',
 * even if we are in a locale where tolower() would produce more or different
 * translations.  This is to conform to the SQL99 spec, which says that
 * keywords are to be matched in this way even though non-keyword identifiers
 * receive a different case-normalization mapping.
 */
int ScanKeywordLookup(const char *text,
				      const ScanKeywordList *keywords)
{
	int			len,
				i;
	char		word[NAMEDATALEN];
	const char *kw_string;
	const uint16 *kw_offsets;
	const uint16 *low;
	const uint16 *high;

	len = strlen(text);

	if (len > keywords->max_kw_len)
		return -1;				/* too long to be any keyword */

	/* We assume all keywords are shorter than NAMEDATALEN. */
	Assert(len < NAMEDATALEN);

	/*
	 * Apply an ASCII-only downcasing.  We must not use tolower() since it may
	 * produce the wrong translation in some locales (eg, Turkish).
	 */
	for (i = 0; i < len; i++)
	{
		char		ch = text[i];

		if (ch >= 'A' && ch <= 'Z')
			ch += 'a' - 'A';
		word[i] = ch;
	}
	word[len] = '\0';

	/*
	 * Now do a binary search using plain strcmp() comparison.
	 */
	kw_string = keywords->kw_string;
	kw_offsets = keywords->kw_offsets;
	low = kw_offsets;
	high = kw_offsets + (keywords->num_keywords - 1);
	while (low <= high)
	{
		const uint16 *middle;
		int			difference;

		middle = low + (high - low) / 2;
		difference = strcmp(kw_string + *middle, word);
		if (difference == 0)
			return middle - kw_offsets;
		else if (difference < 0)
			low = middle + 1;
		else
			high = middle - 1;
	}

	return -1;
}
