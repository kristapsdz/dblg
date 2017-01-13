/*	$Id$ */
/*
 * Copyright (c) 2016 Kristaps Dzonsons <kristaps@bsd.lv>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <kcgi.h>
#include <kcgijson.h>
#include <kcgixml.h>
#include <ksql.h>

/*
 * Index values for atom feed XML elements.
 */
enum	xml {
	XML_AUTHOR,
	XML_EMAIL,
	XML_ENTRY,
	XML_FEED,
	XML_ID,
	XML_LINK,
	XML_NAME,
	XML_PUBLISHED,
	XML_TITLE,
	XML_UPDATED,
	XML_URI,
	XML__MAX
};

/*
 * Element names of atom feed XML.
 */
static	const char *xmls[XML__MAX] = {
	"author", /* XML_AUTHOR */
	"email", /* XML_EMAIL */
	"entry", /* XML_ENTRY */
	"feed", /* XML_FEED */
	"id", /* XML_ID */
	"link", /* XML_LINK */
	"name", /* XML_NAME */
	"published", /* XML_PUBLISHED */
	"title", /* XML_TITLE */
	"updated", /* XML_UPDATED */
	"uri", /* XML_URI */
};

struct	meta {
	int64_t	 mtime; /* last modified time */
	char	*title; /* title (or NULL) */
};

struct	cloud {	       
	char		*key; /* API key */
	char		*secret; /* API secret */
	char		*path; /* media directory */
	char		*name; /* name of storage site */
	int		 set; /* whether set or not */
};

struct	user {
	struct cloud	 cloud; /* cloud settings */
	char		*email; /* e-mail (and identifier) */
	char		*link; /* link to user's website (or NULL) */
	char		*name; /* user's public name */
	char		*lang; /* default IETF Language (or NULL) */
	int64_t		 flags;
#define	USER_ADMIN	 0x01 /* administrator */
#define	USER_DISABLED	 0x02 /* disabled */
	int64_t		 id;
};

struct	entry {
	char		*image; /* image URL (or NULL) */
	char		*aside; /* non-binary aside (or NULL) */
	char		*content; /* non-binary content */
	char		*title; /* non-binary title */
	char		*lang; /* IETF language (or NULL) */
	time_t		 ctime; /* created */
	time_t		 mtime; /* last modified */
	int		 coords; /* whether coords set */
	double		 lat; /* decimal degree latitude */
	double		 lng; /* decimal degree longitude */
	int64_t		 flags;
#define	ENTRY_PENDING	 0x01 /* pending (private) */
	int64_t		 id;
};

enum	page {
	PAGE_ADD_USER,
	PAGE_ATOM,
	PAGE_INDEX,
	PAGE_LOGIN,
	PAGE_LOGOUT,
	PAGE_MOD_CLOUD,
	PAGE_MOD_EMAIL,
	PAGE_MOD_ENABLE,
	PAGE_MOD_LANG,
	PAGE_MOD_LINK,
	PAGE_MOD_META_TITLE,
	PAGE_MOD_NAME,
	PAGE_MOD_PASS,
	PAGE_PUBLIC,
	PAGE_REMOVE,
	PAGE_SUBMIT,
	PAGE__MAX
};

enum	key {
	KEY_ADMIN,
	KEY_ASIDE,
	KEY_CLOUDKEY,
	KEY_CLOUDNAME,
	KEY_CLOUDPATH,
	KEY_CLOUDSECRET,
	KEY_EMAIL,
	KEY_ENABLE,
	KEY_ENTRYID,
	KEY_IMAGE,
	KEY_LANG,
	KEY_LATITUDE,
	KEY_LONGITUDE,
	KEY_LIMIT,
	KEY_LINK,
	KEY_MARKDOWN,
	KEY_NAME,
	KEY_ORDER,
	KEY_PASS,
	KEY_SAVE,
	KEY_SESSCOOKIE,
	KEY_SESSID,
	KEY_TITLE,
	KEY_USERID,
	KEY__MAX
};

enum	stmt {
	STMT_ENTRY_DELETE,
	STMT_ENTRY_GET,
	STMT_ENTRY_GET_PUBLIC,
	STMT_ENTRY_LIST_PENDING,
	STMT_ENTRY_LIST_PUBLIC,
	STMT_ENTRY_LIST_PUBLIC_LANG,
	STMT_ENTRY_LIST_PUBLIC_LANG_LIMIT,
	STMT_ENTRY_LIST_PUBLIC_LIMIT,
	STMT_ENTRY_LIST_PUBLIC_MTIME,
	STMT_ENTRY_LIST_PUBLIC_MTIME_LANG,
	STMT_ENTRY_LIST_PUBLIC_MTIME_LANG_LIMIT,
	STMT_ENTRY_LIST_PUBLIC_MTIME_LIMIT,
	STMT_ENTRY_MODIFY,
	STMT_ENTRY_NEW,
	STMT_META_GET,
	STMT_META_MOD_TITLE,
	STMT_META_NEW,
	STMT_META_UPDATE,
	STMT_SESS_DEL,
	STMT_SESS_GET,
	STMT_SESS_NEW,
	STMT_USER_ADD,
	STMT_USER_GET,
	STMT_USER_LIST,
	STMT_USER_LOOKUP,
	STMT_USER_MOD_CLOUD,
	STMT_USER_MOD_DISABLE,
	STMT_USER_MOD_EMAIL,
	STMT_USER_MOD_ENABLE,
	STMT_USER_MOD_HASH,
	STMT_USER_MOD_LANG,
	STMT_USER_MOD_LINK,
	STMT_USER_MOD_NAME,
	STMT__MAX
};

#define	USER	"user.id,user.email,user.name,user.link," \
		"user.cloudkey,user.cloudsecret,user.cloudname," \
		"user.cloudpath,user.flags,user.lang"
#define	ENTRY	"entry.contents,entry.ctime,entry.id,entry.title," \
		"entry.latitude,entry.longitude,entry.mtime," \
		"entry.flags,entry.lang,entry.aside,entry.image"
/* Convenience. */
#define USER_ENTRY \
		"SELECT " USER "," ENTRY " FROM entry " \
		"INNER JOIN user ON user.id=entry.userid " \
		"WHERE entry.flags=0 "
#define META	"meta.mtime,meta.title"

static	const char *const stmts[STMT__MAX] = {
	/* STMT_ENTRY_DELETE */
	"DELETE FROM entry WHERE id=? AND entry.userid=?",
	/* STMT_ENTRY_GET */
	"SELECT " USER "," ENTRY " FROM entry "
		"INNER JOIN user ON user.id=entry.userid "
		"WHERE entry.id=?",
	/* STMT_ENTRY_GET_PUBLIC */
	"SELECT " USER "," ENTRY " FROM entry "
		"INNER JOIN user ON user.id=entry.userid "
		"WHERE entry.id=? AND entry.flags=0",
	/* STMT_ENTRY_LIST_PENDING */
	"SELECT " ENTRY " FROM entry "
		"WHERE entry.flags=1 AND entry.userid=? "
		"ORDER BY entry.mtime DESC",
	/* STMT_ENTRY_LIST_PUBLIC */
	USER_ENTRY "ORDER BY entry.ctime DESC",
	/* STMT_ENTRY_LIST_PUBLIC_LANG */
	USER_ENTRY "AND entry.lang=? "
		"ORDER BY entry.ctime DESC",
	/* STMT_ENTRY_LIST_PUBLIC_LANG_LIMIT */
	USER_ENTRY "AND entry.lang=? "
		"ORDER BY entry.ctime DESC LIMIT ?",
	/* STMT_ENTRY_LIST_PUBLIC_LIMIT */
	USER_ENTRY "ORDER BY entry.ctime DESC LIMIT ?",
	/* STMT_ENTRY_LIST_PUBLIC_MTIME */
	USER_ENTRY "ORDER BY entry.mtime DESC",
	/* STMT_ENTRY_LIST_PUBLIC_MTIME_LANG */
	USER_ENTRY "AND entry.lang=? "
		"ORDER BY entry.mtime DESC",
	/* STMT_ENTRY_LIST_PUBLIC_MTIME_LANG_LIMIT */
	USER_ENTRY "AND entry.lang=? "
		"ORDER BY entry.mtime DESC LIMIT ?",
	/* STMT_ENTRY_LIST_PUBLIC_MTIME_LIMIT */
	USER_ENTRY "ORDER BY entry.mtime DESC LIMIT ?",
	/* STMT_ENTRY_MODIFY */
	"UPDATE entry SET contents=?,title=?,latitude=?,"
		"longitude=?,mtime=?,flags=?,lang=?,aside=?,"
		"image=? WHERE userid=? AND id=?",
	/* STMT_ENTRY_NEW */
	"INSERT INTO entry (contents,title,userid,latitude,"
		"longitude,flags,lang,aside,image) "
		"VALUES (?,?,?,?,?,?,?,?,?)",
	/* STMT_META_GET */
	"SELECT " META " FROM meta LIMIT 1",
	/* STMT_META_MOD_TITLE */
	"INSERT OR REPLACE INTO meta (mtime,title) VALUES (?,?)",
	/* STMT_META_NEW */
	"INSERT INTO meta (mtime) VALUES (?)",
	/* STMT_META_UPDATE */
	"INSERT OR REPLACE INTO meta (mtime) VALUES (?)",
	/* STMT_SESS_DEL */
	"DELETE FROM sess WHERE id=? AND cookie=?",
	/* STMT_SESS_GET */
	"SELECT " USER " FROM sess "
		"INNER JOIN user ON user.id=sess.userid "
		"WHERE sess.id=? AND sess.cookie=?",
	/* STMT_SESS_NEW */
	"INSERT INTO sess (cookie,userid) VALUES (?,?)",
	/* STMT_USER_ADD */
	"INSERT INTO user (name,email,hash,flags) "
		"VALUES (?,?,?,?)",
	/* STMT_USER_GET */
	"SELECT " USER " FROM user WHERE id=?",
	/* STMT_USER_LIST */
	"SELECT " USER " FROM user",
	/* STMT_USER_LOOKUP */
	"SELECT " USER ",hash FROM user WHERE email=?",
	/* STMT_USER_MOD_CLOUD */
	"UPDATE user SET cloudkey=?,cloudsecret=?,"
	       "cloudname=?,cloudpath=? WHERE id=?",
	/* STMT_USER_MOD_DISABLE */
	"UPDATE user SET flags=flags | ? WHERE id=?",
	/* STMT_USER_MOD_EMAIL */
	"UPDATE user SET email=? WHERE id=?",
	/* STMT_USER_MOD_ENABLE */
	"UPDATE user SET flags=flags & ~? WHERE id=?",
	/* STMT_USER_MOD_HASH */
	"UPDATE user SET hash=? WHERE id=?",
	/* STMT_USER_MOD_LANG */
	"UPDATE user SET lang=? WHERE id=?",
	/* STMT_USER_MOD_LINK */
	"UPDATE user SET link=? WHERE id=?",
	/* STMT_USER_MOD_NAME */
	"UPDATE user SET name=? WHERE id=?",
};

static const struct kvalid keys[KEY__MAX] = {
	{ NULL, "admin" }, /* KEY_ADMIN */
	{ kvalid_string, "aside" }, /* KEY_ASIDE */
	{ kvalid_string, "cloudkey" }, /* KEY_CLOUDKEY */
	{ kvalid_string, "cloudname" }, /* KEY_CLOUDNAME */
	{ kvalid_string, "cloudpath" }, /* KEY_CLOUDPATH */
	{ kvalid_string, "cloudsecret" }, /* KEY_CLOUDSECRET */
	{ kvalid_email, "email" }, /* KEY_EMAIL */
	{ kvalid_uint, "enable" }, /* KEY_ENABLE */
	{ kvalid_int, "entryid" }, /* KEY_ENTRYID */
	{ kvalid_string, "image" }, /* KEY_IMAGE */
	{ kvalid_string, "lang" }, /* KEY_LANG */
	{ kvalid_double, "latitude" }, /* KEY_LATITUDE */
	{ kvalid_double, "longitude" }, /* KEY_LONGITUDE */
	{ kvalid_uint, "limit" }, /* KEY_LIMIT */
	{ kvalid_string, "link" }, /* KEY_LINK */
	{ kvalid_stringne, "markdown" }, /* KEY_MARKDOWN */
	{ kvalid_stringne, "name" }, /* KEY_NAME */
	{ kvalid_stringne, "order" }, /* KEY_ORDER */
	{ kvalid_stringne, "pass" }, /* KEY_PASS */
	{ NULL, "save" }, /* KEY_SAVE */
	{ kvalid_uint, "sesscookie" }, /* KEY_SESSCOOKIE */
	{ kvalid_int, "sessid" }, /* KEY_SESSID */
	{ kvalid_stringne, "title" }, /* KEY_TITLE */
	{ kvalid_int, "userid" }, /* KEY_USERID */
};

static const char *const pages[PAGE__MAX] = {
	"adduser", /* PAGE_ADD_USER */
	"atom", /* PAGE_ATOM */
	"index", /* PAGE_INDEX */
	"login", /* PAGE_LOGIN */
	"logout", /* PAGE_LOGOUT */
	"modcloud", /* PAGE_MOD_CLOUD */
	"modemail", /* PAGE_MOD_EMAIL */
	"modenable", /* PAGE_MOD_ENABLE */
	"modlang", /* PAGE_MOD_LANG */
	"modlink", /* PAGE_MOD_LINK */
	"modmetatitle", /* PAGE_MOD_META_TITLE */
	"modname", /* PAGE_MOD_NAME */
	"modpass", /* PAGE_MOD_PASS */
	"public", /* PAGE_PUBLIC */
	"remove", /* PAGE_REMOVE */
	"submit", /* PAGE_SUBMIT */
};

static void
sendhttphead(struct kreq *r, enum khttp code)
{

	khttp_head(r, kresps[KRESP_STATUS], 
		"%s", khttps[code]);
	khttp_head(r, kresps[KRESP_CONTENT_TYPE], 
		"%s", kmimetypes[r->mime]);
	khttp_head(r, "X-Content-Type-Options", "nosniff");
	khttp_head(r, "X-Frame-Options", "DENY");
	khttp_head(r, "X-XSS-Protection", "1; mode=block");
}

static void
sendhttp(struct kreq *r, enum khttp code)
{

	sendhttphead(r, code);
	khttp_body(r);
}

static void
db_user_unfill(struct user *p)
{

	if (NULL == p)
		return;
	free(p->email);
	free(p->link);
	free(p->lang);
	free(p->name);
	free(p->cloud.key);
	free(p->cloud.secret);
	free(p->cloud.path);
	free(p->cloud.name);
}

static void
db_user_free(struct user *p)
{

	db_user_unfill(p);
	free(p);
}

static void
col_if_not_null(char **p, struct ksqlstmt *stmt, size_t pos)
{

	*p = NULL;
	if ( ! ksql_stmt_isnull(stmt, pos))
		*p = kstrdup(ksql_stmt_str(stmt, pos));
}

static void
bind_if_not_null(struct ksqlstmt *stmt, size_t pos, const char *v)
{

	if ('\0' == v[0])
		ksql_bind_null(stmt, pos);
	else
		ksql_bind_str(stmt, pos, v);
}

static void
db_user_fill(struct user *p, struct ksqlstmt *stmt, size_t *pos)
{
	size_t	 i = 0;

	if (NULL == pos)
		pos = &i;
	memset(p, 0, sizeof(struct user));
	p->id = ksql_stmt_int(stmt, (*pos)++);
	p->email = kstrdup(ksql_stmt_str(stmt, (*pos)++));
	p->name = kstrdup(ksql_stmt_str(stmt, (*pos)++));
	if ( ! ksql_stmt_isnull(stmt, *pos))
		p->link = kstrdup(ksql_stmt_str(stmt, *pos));
	(*pos)++;
	if ( ! ksql_stmt_isnull(stmt, *pos) &&
	     ! ksql_stmt_isnull(stmt, *pos + 1) &&
	     ! ksql_stmt_isnull(stmt, *pos + 2) &&
	     ! ksql_stmt_isnull(stmt, *pos + 3))
		p->cloud.set = 1;
	col_if_not_null(&p->cloud.key, stmt, (*pos)++);
	col_if_not_null(&p->cloud.secret, stmt, (*pos)++);
	col_if_not_null(&p->cloud.name, stmt, (*pos)++);
	col_if_not_null(&p->cloud.path, stmt, (*pos)++);
	p->flags = ksql_stmt_int(stmt, (*pos)++);
	col_if_not_null(&p->lang, stmt, (*pos)++);
}

static void
db_meta_unfill(struct meta *p)
{

	if (NULL != p)
		free(p->title);
}

static void
db_meta_fill(struct meta *p, struct ksqlstmt *stmt, size_t *pos)
{
	size_t	 i = 0;

	if (NULL == pos)
		pos = &i;
	memset(p, 0, sizeof(struct meta));
	p->mtime = ksql_stmt_int(stmt, (*pos)++);
	col_if_not_null(&p->title, stmt, (*pos)++);
}

static void
db_entry_unfill(struct entry *p)
{

	if (NULL == p)
		return;
	free(p->aside);
	free(p->image);
	free(p->content);
	free(p->title);
	free(p->lang);
}

static void
db_entry_fill(struct entry *p, struct ksqlstmt *stmt, size_t *pos)
{
	size_t	 i = 0;

	if (NULL == pos)
		pos = &i;
	memset(p, 0, sizeof(struct entry));
	p->content = kstrdup(ksql_stmt_str(stmt, (*pos)++));
	p->ctime = ksql_stmt_int(stmt, (*pos)++);
	p->id = ksql_stmt_int(stmt, (*pos)++);
	p->title = kstrdup(ksql_stmt_str(stmt, (*pos)++));
	if ( ! ksql_stmt_isnull(stmt, *pos) &&
	     ! ksql_stmt_isnull(stmt, *pos + 1)) {
		p->coords = 1;
		p->lat = ksql_stmt_double(stmt, *pos);
		p->lng = ksql_stmt_double(stmt, *pos + 1);
	} else 
		p->coords = 0;
	(*pos) += 2;
	p->mtime = ksql_stmt_int(stmt, (*pos)++);
	p->flags = ksql_stmt_int(stmt, (*pos)++);
	col_if_not_null(&p->lang, stmt, (*pos)++);
	col_if_not_null(&p->aside, stmt, (*pos)++);
	col_if_not_null(&p->image, stmt, (*pos)++);
}

/*
 * Get our meta-data (title, mtime, etc.).
 * This will always succeed, and always return valid data even if we had
 * to create it in the process.
 */
static void
db_meta_get(struct kreq *r, struct meta *p)
{
	struct ksqlstmt	*stmt;
	int64_t		 mtime;
	enum ksqlc	 c;

	/* Begin by fetching the global last modifier. */

	ksql_stmt_alloc(r->arg, &stmt, 
		stmts[STMT_META_GET], 
		STMT_META_GET);
	if (KSQL_ROW != ksql_stmt_step(stmt)) {
		ksql_stmt_free(stmt);
		kutil_info(r, NULL, "creating meta row");
		mtime = time(NULL);

		/* 
		 * We might have another creation in the meanwhile, so
		 * make sure we catch the constraint.
		 */

		ksql_stmt_alloc(r->arg, &stmt, 
			stmts[STMT_META_NEW], 
			STMT_META_NEW);
		ksql_bind_int(stmt, 0, mtime);
		ksql_stmt_cstep(stmt);
		ksql_stmt_free(stmt);

		/* Re-fetch. */

		ksql_stmt_alloc(r->arg, &stmt, 
			stmts[STMT_META_GET], 
			STMT_META_GET);
		c = ksql_stmt_step(stmt);
		assert(KSQL_ROW == c);
		db_meta_fill(p, stmt, NULL);
		ksql_stmt_free(stmt);
	} else {
		db_meta_fill(p, stmt, NULL);
		ksql_stmt_free(stmt);
	}
}

static void
db_meta_update(struct kreq *r)
{
	struct ksqlstmt	*stmt;

	ksql_stmt_alloc(r->arg, &stmt, 
		stmts[STMT_META_UPDATE], 
		STMT_META_UPDATE);
	ksql_bind_int(stmt, 0, time(NULL));
	ksql_stmt_step(stmt);
	ksql_stmt_free(stmt);
}

static int64_t
db_entry_modify(struct kreq *r, const struct user *user, 
	const char *title, const char *text, 
	int64_t id, double lat, double lng, int save,
	const char *lang, const char *aside, const char *image)
{
	struct ksqlstmt	*stmt;

	ksql_stmt_alloc(r->arg, &stmt, 
		stmts[STMT_ENTRY_MODIFY], 
		STMT_ENTRY_MODIFY);
	ksql_bind_str(stmt, 0, text);
	ksql_bind_str(stmt, 1, title);
	if ((0.0 == lat || isnormal(lat)) &&
	    (0.0 == lng || isnormal(lng))) {
		ksql_bind_double(stmt, 2, lat);
		ksql_bind_double(stmt, 3, lng);
	} else {
		ksql_bind_null(stmt, 2);
		ksql_bind_null(stmt, 3);
	}
	ksql_bind_int(stmt, 4, time(NULL));
	ksql_bind_int(stmt, 5, save ? 1 : 0);
	bind_if_not_null(stmt, 6, lang);
	bind_if_not_null(stmt, 7, aside);
	bind_if_not_null(stmt, 8, image);
	ksql_bind_int(stmt, 9, user->id);
	ksql_bind_int(stmt, 10, id);
	ksql_stmt_step(stmt);
	ksql_stmt_free(stmt);
	kutil_info(r, user->email, 
		"modified entry: %" PRId64, id);

	db_meta_update(r);
	return(id);
}

static int64_t
db_entry_new(struct kreq *r, const struct user *user, 
	const char *title, const char *text, 
	double lat, double lng, int save, const char *lang,
	const char *aside, const char *img)
{
	struct ksqlstmt	*stmt;
	int64_t		 id;

	ksql_stmt_alloc(r->arg, &stmt, 
		stmts[STMT_ENTRY_NEW], 
		STMT_ENTRY_NEW);
	ksql_bind_str(stmt, 0, text);
	ksql_bind_str(stmt, 1, title);
	ksql_bind_int(stmt, 2, user->id);
	if ((0.0 == lat || isnormal(lat)) &&
	    (0.0 == lng || isnormal(lng))) {
		ksql_bind_double(stmt, 3, lat);
		ksql_bind_double(stmt, 4, lng);
	} else {
		ksql_bind_null(stmt, 3);
		ksql_bind_null(stmt, 4);
	}
	ksql_bind_int(stmt, 5, save ? 1 : 0);
	bind_if_not_null(stmt, 6, lang);
	bind_if_not_null(stmt, 7, aside);
	bind_if_not_null(stmt, 8, img);
	ksql_stmt_step(stmt);
	ksql_stmt_free(stmt);
	ksql_lastid(r->arg, &id);
	kutil_info(r, user->email, 
		"new entry: %" PRId64, id);

	db_meta_update(r);
	return(id);
}

static int64_t
db_sess_new(struct ksql *sql, int64_t cookie,
	const struct user *user)
{
	struct ksqlstmt	*stmt;
	int64_t		 id;

	ksql_stmt_alloc(sql, &stmt, 
		stmts[STMT_SESS_NEW], 
		STMT_SESS_NEW);
	ksql_bind_int(stmt, 0, cookie);
	ksql_bind_int(stmt, 1, user->id);
	ksql_stmt_step(stmt);
	ksql_stmt_free(stmt);
	ksql_lastid(sql, &id);
	return(id);
}

struct user *
db_user_find(struct ksql *sql, 
	const char *email, const char *pass)
{
	struct ksqlstmt	*stmt;
	int		 rc;
	size_t		 i;
	const char	*hash;
	struct user	*user;

	ksql_stmt_alloc(sql, &stmt, 
		stmts[STMT_USER_LOOKUP], 
		STMT_USER_LOOKUP);
	ksql_bind_str(stmt, 0, email);
	if (KSQL_ROW != ksql_stmt_step(stmt)) {
		ksql_stmt_free(stmt);
		return(NULL);
	}
	i = 0;
	user = kmalloc(sizeof(struct user));
	db_user_fill(user, stmt, &i);
	hash = ksql_stmt_str(stmt, i);
#ifdef __OpenBSD__
	rc = crypt_checkpass(pass, hash) < 0 ? 0 : 1;
#else
	rc = 0 == strcmp(hash, pass);
#endif
	ksql_stmt_free(stmt);
	if (0 == rc) {
		db_user_free(user);
		user = NULL;
	}
	return(user);
}

static struct user *
db_user_sess_get(struct ksql *sql, int64_t id, int64_t cookie)
{
	struct ksqlstmt	*stmt;
	struct user	*u = NULL;

	if (-1 == id || -1 == cookie)
		return(NULL);

	ksql_stmt_alloc(sql, &stmt, 
		stmts[STMT_SESS_GET], 
		STMT_SESS_GET);
	ksql_bind_int(stmt, 0, id);
	ksql_bind_int(stmt, 1, cookie);
	if (KSQL_ROW == ksql_stmt_step(stmt)) {
		u = kmalloc(sizeof(struct user));
		db_user_fill(u, stmt, NULL);
	}
	ksql_stmt_free(stmt);
	return(u);
}

static void
db_sess_del(struct ksql *sql, int64_t id, int64_t cookie)
{
	struct ksqlstmt	*stmt;

	ksql_stmt_alloc(sql, &stmt, 
		stmts[STMT_SESS_DEL], 
		STMT_SESS_DEL);
	ksql_bind_int(stmt, 0, id);
	ksql_bind_int(stmt, 1, cookie);
	ksql_stmt_step(stmt);
	ksql_stmt_free(stmt);
}

static void
db_user_mod_pass(struct kreq *r, 
	const struct user *u, const char *pass)
{
	struct ksqlstmt	*stmt;
	char		 hash[64];

#ifdef __OpenBSD__
	crypt_newhash(pass, "blowfish,a", hash, sizeof(hash));
#else
	strlcpy(hash, pass, sizeof(hash));
#endif
	ksql_stmt_alloc(r->arg, &stmt,
		stmts[STMT_USER_MOD_HASH],
		STMT_USER_MOD_HASH);
	ksql_bind_str(stmt, 0, hash);
	ksql_bind_int(stmt, 1, u->id);
	ksql_stmt_step(stmt);
	ksql_stmt_free(stmt);
	kutil_info(r, u->email, "changed password");
}

static int
db_user_mod_email(struct kreq *r, 
	const struct user *u, const char *email)
{
	struct ksqlstmt	*stmt;
	enum ksqlc	 c;

	ksql_stmt_alloc(r->arg, &stmt, 
		stmts[STMT_USER_MOD_EMAIL], 
		STMT_USER_MOD_EMAIL);
	ksql_bind_str(stmt, 0, email);
	ksql_bind_int(stmt, 1, u->id);
	c = ksql_stmt_cstep(stmt);
	ksql_stmt_free(stmt);
	if (KSQL_CONSTRAINT != c) {
		kutil_info(r, u->email, 
			"changed email: %s", email);
		/* Update: this is public data. */
		db_meta_update(r);
	}
	return(KSQL_CONSTRAINT != c);
}

static void
db_user_mod_enable(struct kreq *r, 
	const struct user *u, int64_t userid, int enable)
{
	struct ksqlstmt	*stmt;

	if (enable)
		ksql_stmt_alloc(r->arg, &stmt, 
			stmts[STMT_USER_MOD_ENABLE], 
			STMT_USER_MOD_ENABLE);
	else
		ksql_stmt_alloc(r->arg, &stmt, 
			stmts[STMT_USER_MOD_DISABLE], 
			STMT_USER_MOD_DISABLE);
	ksql_bind_int(stmt, 0, USER_DISABLED);
	ksql_bind_int(stmt, 1, userid);
	ksql_stmt_step(stmt);
	ksql_stmt_free(stmt);
	kutil_info(r, u->email, "%s user: %" PRId64, 
		enable ? "enabled" : "disabled", userid);
}

static void
db_mod_meta_title(struct kreq *r, 
	const struct user *u, const char *title)
{
	struct ksqlstmt	*stmt;

	ksql_stmt_alloc(r->arg, &stmt, 
		stmts[STMT_META_MOD_TITLE], 
		STMT_META_MOD_TITLE);
	ksql_bind_int(stmt, 0, time(NULL));
	bind_if_not_null(stmt, 1, title);
	ksql_stmt_step(stmt);
	ksql_stmt_free(stmt);
	kutil_info(r, u->email, "changed meta title");
}

static void
db_user_mod_cloud(struct kreq *r, const struct user *u, 
	const char *key, const char *secret,
	const char *name, const char *path)
{
	struct ksqlstmt	*stmt;

	ksql_stmt_alloc(r->arg, &stmt, 
		stmts[STMT_USER_MOD_CLOUD], 
		STMT_USER_MOD_CLOUD);
	bind_if_not_null(stmt, 0, key);
	bind_if_not_null(stmt, 1, secret);
	bind_if_not_null(stmt, 2, name);
	ksql_bind_str(stmt, 3, path);
	ksql_bind_int(stmt, 4, u->id);
	ksql_stmt_step(stmt);
	ksql_stmt_free(stmt);
	kutil_info(r, u->email, "changed cloud parameters");
}

static void
db_user_mod_lang(struct kreq *r, 
	const struct user *u, const char *lang)
{
	struct ksqlstmt	*stmt;

	ksql_stmt_alloc(r->arg, &stmt, 
		stmts[STMT_USER_MOD_LANG], 
		STMT_USER_MOD_LANG);
	bind_if_not_null(stmt, 0, lang);
	ksql_bind_int(stmt, 1, u->id);
	ksql_stmt_step(stmt);
	ksql_stmt_free(stmt);
	kutil_info(r, u->email, "changed lang");
}

static void
db_user_mod_link(struct kreq *r, 
	const struct user *u, const char *link)
{
	struct ksqlstmt	*stmt;

	ksql_stmt_alloc(r->arg, &stmt, 
		stmts[STMT_USER_MOD_LINK], 
		STMT_USER_MOD_LINK);
	bind_if_not_null(stmt, 0, link);
	ksql_bind_int(stmt, 1, u->id);
	ksql_stmt_step(stmt);
	ksql_stmt_free(stmt);
	kutil_info(r, u->email, "changed link");

	/* Update: this is public data. */
	db_meta_update(r);
}

static int
db_user_add(struct kreq *r, const struct user *u,
	const char *email, const char *pass, int admin)
{
	struct ksqlstmt	*stmt;
	char		 hash[64];
	enum ksqlc	 c;

#ifdef __OpenBSD__
	crypt_newhash(pass, "blowfish,a", hash, sizeof(hash));
#else
	strlcpy(hash, pass, sizeof(hash));
#endif
	ksql_stmt_alloc(r->arg, &stmt, 
		stmts[STMT_USER_ADD], 
		STMT_USER_ADD);
	ksql_bind_str(stmt, 0, "Anonymous user");
	ksql_bind_str(stmt, 1, email);
	ksql_bind_str(stmt, 2, hash);
	ksql_bind_int(stmt, 3, admin ? 1 : 0);
	c = ksql_stmt_cstep(stmt);
	ksql_stmt_free(stmt);
	kutil_info(r, u->email, "added user: %s", email);
	return(KSQL_CONSTRAINT != c);
}

static void
db_user_mod_name(struct kreq *r, 
	const struct user *u, const char *name)
{
	struct ksqlstmt	*stmt;

	ksql_stmt_alloc(r->arg, &stmt, 
		stmts[STMT_USER_MOD_NAME], 
		STMT_USER_MOD_NAME);
	ksql_bind_str(stmt, 0, name);
	ksql_bind_int(stmt, 1, u->id);
	ksql_stmt_step(stmt);
	ksql_stmt_free(stmt);
	kutil_info(r, u->email, "changed name");

	/* Update: this is public data. */
	db_meta_update(r);
}

static void
db_entry_delete(struct kreq *r, 
	const struct user *u, int64_t id)
{
	struct ksqlstmt	*stmt;

	ksql_stmt_alloc(r->arg, &stmt, 
		stmts[STMT_ENTRY_DELETE], 
		STMT_ENTRY_DELETE);
	ksql_bind_int(stmt, 0, id);
	ksql_bind_int(stmt, 1, u->id);
	ksql_stmt_step(stmt);
	ksql_stmt_free(stmt);
	kutil_info(r, u->email, "deleted entry: %" PRId64, id);
	db_meta_update(r);
}

static void
json_if_not_null(struct kjsonreq *req, const char *name, const char *val)
{

	kjson_putstringp(req, name, NULL == val ? "" : val);
}

static void
json_putuserdata(struct kjsonreq *req, 
	const struct user *u, int public)
{

	kjson_putstringp(req, "name", u->name);
	kjson_putstringp(req, "email", u->email);
	json_if_not_null(req, "link", u->link);
	kjson_putintp(req, "id", u->id);
	if ( ! public) {
		kjson_objp_open(req, "cloud");
		json_if_not_null(req, "key", u->cloud.key);
		json_if_not_null(req, "secret", u->cloud.secret);
		json_if_not_null(req, "name", u->cloud.name);
		json_if_not_null(req, "path", u->cloud.path);
		kjson_putintp(req, "set", u->cloud.set);
		kjson_obj_close(req);
	} else
		kjson_putnullp(req, "cloud");

	kjson_objp_open(req, "attrs");
	kjson_putboolp(req, "admin", 
		USER_ADMIN & u->flags);
	kjson_putboolp(req, "disabled", 
		USER_DISABLED & u->flags);
	kjson_obj_close(req);
	json_if_not_null(req, "lang", u->lang);
}

static void
json_putuser(struct kjsonreq *req, 
	const struct user *u, int public)
{

	if (NULL != u) {
		kjson_objp_open(req, "user");
		json_putuserdata(req, u, public);
		kjson_obj_close(req);
	} else
		kjson_putnullp(req, "user");
}

static void
json_putentry(struct kjsonreq *req, const struct user *u,
	const struct entry *entry, const char *name)
{

	if (NULL != name)
		kjson_objp_open(req, name);
	else
		kjson_obj_open(req);
	json_putuser(req, u, 0);
	kjson_putintp(req, "ctime", entry->ctime);
	kjson_putintp(req, "mtime", entry->mtime);
	kjson_putintp(req, "id", entry->id);
	kjson_putstringp(req, "content", entry->content);
	json_if_not_null(req, "aside", entry->aside);
	json_if_not_null(req, "image", entry->image);
	kjson_putstringp(req, "title", entry->title);
	if (entry->coords) {
		kjson_objp_open(req, "coords");
		kjson_putdoublep(req, "lat", entry->lat);
		kjson_putdoublep(req, "lng", entry->lng);
		kjson_obj_close(req);
	} else
		kjson_putnullp(req, "coords");
	kjson_objp_open(req, "attrs");
	kjson_putboolp(req, "pending", 
		ENTRY_PENDING & entry->flags);
	kjson_obj_close(req);
	json_if_not_null(req, "lang", entry->lang);
	kjson_obj_close(req);
}

static void
sendremove(struct kreq *r, const struct user *u)
{
	struct kpair	*kpn;

	if (NULL == (kpn = r->fieldmap[KEY_ENTRYID])) {
		sendhttp(r, KHTTP_400);
		return;
	}

	assert(NULL != u);
	db_entry_delete(r, u, kpn->parsed.i);
	sendhttp(r, KHTTP_200);
}

static void
sendsubmit(struct kreq *r, const struct user *u)
{
	struct kpair	*kpm, *kpt, *kpi, *kplat, *kplng, *kpl,
			*kpa, *kpimg;
	int64_t		 id;
	struct kjsonreq	 req;
	double		 lat, lng;
	int		 save;

	/* Are we here to save or to publish? */

	save = NULL != r->fieldmap[KEY_SAVE];
	assert(NULL != u);

	kpa = r->fieldmap[KEY_ASIDE];
	kpimg = r->fieldmap[KEY_IMAGE];
	kpm = r->fieldmap[KEY_MARKDOWN];
	kpt = r->fieldmap[KEY_TITLE];
    	kplat = r->fieldmap[KEY_LATITUDE];
    	kplng = r->fieldmap[KEY_LONGITUDE];
	kpl = r->fieldmap[KEY_LANG];

	/* Require fields only if we're publishing. */

	if ( ! save && (NULL == kpt || NULL == kpm)) {
		sendhttp(r, KHTTP_400);
		return;
	}

	/* We need both coordinates for orientation. */

	if (NULL != kplat && NULL != kplng) {
		lat = r->fieldmap[KEY_LATITUDE]->parsed.d;
		lng = r->fieldmap[KEY_LONGITUDE]->parsed.d;
	} else 
		lat = lng = 1.0 / 0.0;

	if (NULL != (kpi = r->fieldmap[KEY_ENTRYID]) &&
	    kpi->parsed.i > 0)  
		id = db_entry_modify(r, u, 
			NULL == kpt ? "" : kpt->parsed.s, 
			NULL == kpm ? "" : kpm->parsed.s, 
			kpi->parsed.i, lat, lng, save,
			NULL != kpl ? kpl->parsed.s : "",
			NULL != kpa ? kpa->parsed.s : "",
			NULL != kpimg ? kpimg->parsed.s : "");
	else
		id = db_entry_new(r, u, 
			NULL == kpt ? "" : kpt->parsed.s, 
			NULL == kpm ? "" : kpm->parsed.s, 
			lat, lng, save,
			NULL != kpl ? kpl->parsed.s : "",
			NULL != kpa ? kpa->parsed.s : "",
			NULL != kpimg ? kpimg->parsed.s : "");

	sendhttp(r, KHTTP_200);
	kjson_open(&req, r);
	kjson_obj_open(&req);
	kjson_putintp(&req, "id", id);
	kjson_obj_close(&req);
	kjson_close(&req);
}

static void
sendmodlang(struct kreq *r, const struct user *u)
{
	struct kpair	*kp;

	kp = r->fieldmap[KEY_LANG];
	db_user_mod_lang(r, u, NULL != kp ? kp->parsed.s : "");
	sendhttp(r, KHTTP_200);
}

static void
sendmodlink(struct kreq *r, const struct user *u)
{
	struct kpair	*kp;

	kp = r->fieldmap[KEY_LINK];
	db_user_mod_link(r, u, NULL != kp ? kp->parsed.s : "");
	sendhttp(r, KHTTP_200);
}

static void
sendmodmetatitle(struct kreq *r, const struct user *u)
{
	struct kpair	*kp;

	kp = r->fieldmap[KEY_TITLE];
	db_mod_meta_title(r, u, NULL == kp ? "" : kp->parsed.s);
	sendhttp(r, KHTTP_200);
}

static void
sendmodcloud(struct kreq *r, const struct user *u)
{
	struct kpair	*kpn, *kpk, *kps, *kpp;

	if (NULL != (kpk = r->fieldmap[KEY_CLOUDKEY]) &&
	    NULL != (kps = r->fieldmap[KEY_CLOUDSECRET]) &&
	    NULL != (kpn = r->fieldmap[KEY_CLOUDNAME]) &&
	    NULL != (kpp = r->fieldmap[KEY_CLOUDPATH])) {
		db_user_mod_cloud(r, u, 
			kpk->parsed.s, kps->parsed.s,
			kpn->parsed.s, kpp->parsed.s);
		sendhttp(r, KHTTP_200);
	} else
		sendhttp(r, KHTTP_400);
}

static void
sendmodenable(struct kreq *r, const struct user *u)
{
	struct kpair	*kpe, *kpn;

	if (NULL != (kpe = r->fieldmap[KEY_USERID]) &&
	    NULL != (kpn = r->fieldmap[KEY_ENABLE]) &&
	    u->id != kpe->parsed.i) {
		db_user_mod_enable(r, u,
			kpe->parsed.i, 
			kpn->parsed.i ? 1 : 0);
		sendhttp(r, KHTTP_200);
	} else
		sendhttp(r, KHTTP_400);
}

static void
sendmodemail(struct kreq *r, const struct user *u)
{
	struct kpair	*kp;
	int		 rc;

	if (NULL != (kp = r->fieldmap[KEY_EMAIL])) {
		rc = db_user_mod_email(r, u, kp->parsed.s);
		sendhttp(r, rc ? KHTTP_200 : KHTTP_400);
	} else
		sendhttp(r, KHTTP_400);
}

static void
sendmodpass(struct kreq *r, const struct user *u)
{
	struct kpair	*kp;

	if (NULL != (kp = r->fieldmap[KEY_PASS])) {
		sendhttp(r, KHTTP_200);
		db_user_mod_pass(r, u, kp->parsed.s);
	} else
		sendhttp(r, KHTTP_400);
}

static void
sendmodname(struct kreq *r, const struct user *u)
{
	struct kpair	*kp;

	if (NULL != (kp = r->fieldmap[KEY_NAME])) {
		sendhttp(r, KHTTP_200);
		db_user_mod_name(r, u, kp->parsed.s);
	} else 
		sendhttp(r, KHTTP_400);
}

static void
sendadduser(struct kreq *r, const struct user *u)
{
	struct kpair	*kpe, *kpp, *kpa;
	int		 rc;

	assert(NULL != u && USER_ADMIN & u->flags);

	kpa = r->fieldmap[KEY_ADMIN];

	rc = 0;
	if (NULL != (kpe = r->fieldmap[KEY_EMAIL]) &&
	    NULL != (kpp = r->fieldmap[KEY_PASS]))
		rc = db_user_add(r, u, kpe->parsed.s, 
			kpp->parsed.s, NULL != kpa);
	sendhttp(r, rc ? KHTTP_200 : KHTTP_400);
}

static void
sendindex(struct kreq *r, const struct user *u)
{
	struct kjsonreq	 req;
	struct ksqlstmt	*stmt;
	struct kpair	*kpi;
	size_t		 i;
	struct user	 user;
	struct entry	 entry;
	struct meta	 meta;

	assert(NULL != u);
	sendhttp(r, KHTTP_200);
	kjson_open(&req, r);
	kjson_obj_open(&req);
	json_putuser(&req, u, 0);

	if (USER_ADMIN & u->flags) {
		db_meta_get(r, &meta);
		kjson_objp_open(&req, "meta");
		kjson_putstringp(&req, "title",
			NULL == meta.title ? "" : meta.title);
		kjson_obj_close(&req);
		db_meta_unfill(&meta);
	} else
		kjson_putnullp(&req, "meta");

	kjson_arrayp_open(&req, "users");
	if (USER_ADMIN & u->flags) {
		ksql_stmt_alloc(r->arg, &stmt, 
			stmts[STMT_USER_LIST], 
			STMT_USER_LIST);
		while (KSQL_ROW == ksql_stmt_step(stmt)) {
			db_user_fill(&user, stmt, NULL);
			kjson_obj_open(&req);
			json_putuserdata(&req, &user, 0);
			kjson_obj_close(&req);
		}
		ksql_stmt_free(stmt);
	}
	kjson_array_close(&req);

	ksql_stmt_alloc(r->arg, &stmt, 
		stmts[STMT_ENTRY_LIST_PENDING], 
		STMT_ENTRY_LIST_PENDING);
	ksql_bind_int(stmt, 0, u->id);
	kjson_arrayp_open(&req, "pending");
	while (KSQL_ROW == ksql_stmt_step(stmt)) {
		db_entry_fill(&entry, stmt, NULL);
		json_putentry(&req, NULL, &entry, NULL);
		db_entry_unfill(&entry);
	} 
	ksql_stmt_free(stmt);
	kjson_array_close(&req);

	if (NULL != (kpi = r->fieldmap[KEY_ENTRYID])) {
		ksql_stmt_alloc(r->arg, &stmt, 
			stmts[STMT_ENTRY_GET], 
			STMT_ENTRY_GET);
		ksql_bind_int(stmt, 0, kpi->parsed.i);
		if (KSQL_ROW == ksql_stmt_step(stmt)) {
			i = 0;
			db_user_fill(&user, stmt, &i);
			db_entry_fill(&entry, stmt, &i);
			json_putentry(&req, &user, &entry, "entry");
			db_user_unfill(&user);
			db_entry_unfill(&entry);
		} else
			kjson_putnullp(&req, "entry");
		ksql_stmt_free(stmt);
	} else
		kjson_putnullp(&req, "entry");

	kjson_obj_close(&req);
	kjson_close(&req);
}

static void
sendatom(struct kreq *r)
{
	struct kxmlreq	 req;
	struct ksqlstmt	*stmt;
	struct entry	 entry;
	struct user	 user;
	size_t		 i;
	char		 buf[256];
	struct meta	 meta;
	struct tm	 tm;

	db_meta_get(r, &meta);

	sendhttp(r, KHTTP_200);

	ksql_stmt_alloc(r->arg, &stmt, 
		stmts[STMT_ENTRY_LIST_PUBLIC], 
		STMT_ENTRY_LIST_PUBLIC);
	kxml_open(&req, r, xmls, XML__MAX);

	kxml_pushattrs(&req, XML_FEED, "xmlns", 
		"http://www.w3.org/2005/Atom", NULL);

	kutil_epoch2utcstr(meta.mtime, buf, sizeof(buf));
	kxml_push(&req, XML_UPDATED);
	kxml_puts(&req, buf);
	kxml_pop(&req);

	snprintf(buf, sizeof(buf), "%s://%s/",
		kschemes[r->scheme], r->host);
	kxml_push(&req, XML_ID);
	kxml_puts(&req, buf);
	kxml_pop(&req);

	snprintf(buf, sizeof(buf), "%s%s", r->pname, r->fullpath);
	kxml_pushnullattrs(&req, XML_LINK, 
		"rel", "self", "href", buf, NULL);

	kxml_push(&req, XML_TITLE);
	if (NULL == meta.title || '\0' == meta.title[0])
		kxml_puts(&req, r->host);
	else
		kxml_puts(&req, meta.title);
	kxml_pop(&req);

	while (KSQL_ROW == ksql_stmt_step(stmt)) {
		kxml_push(&req, XML_ENTRY);
		i = 0;
		db_user_fill(&user, stmt, &i);
		db_entry_fill(&entry, stmt, &i);
		kxml_push(&req, XML_TITLE);
		kxml_puts(&req, entry.title);
		kxml_pop(&req);

		KUTIL_EPOCH2TM(entry.ctime, &tm);

		snprintf(buf, sizeof(buf),
			"tag:%s,%.4d-%.2d-%.2d:%" PRId64, r->host, 
			tm.tm_year + 1900, tm.tm_mon, tm.tm_mday, entry.id);
		kxml_push(&req, XML_ID);
		kxml_puts(&req, buf);
		kxml_pop(&req);

		kutil_epoch2utcstr(entry.ctime, buf, sizeof(buf));
		kxml_push(&req, XML_PUBLISHED);
		kxml_puts(&req, buf);
		kxml_pop(&req);

		kutil_epoch2utcstr(entry.mtime, buf, sizeof(buf));
		kxml_push(&req, XML_UPDATED);
		kxml_puts(&req, buf);
		kxml_pop(&req);

		kxml_push(&req, XML_AUTHOR);
		kxml_push(&req, XML_NAME);
		kxml_puts(&req, user.name);
		kxml_pop(&req);
		kxml_push(&req, XML_EMAIL);
		kxml_puts(&req, user.email);
		kxml_pop(&req);
		if (NULL != user.link) {
			kxml_push(&req, XML_URI);
			kxml_puts(&req, user.link);
			kxml_pop(&req);
		}
		kxml_pop(&req);

		db_user_unfill(&user);
		db_entry_unfill(&entry);

		kxml_pop(&req);
	}

	kxml_close(&req);
	ksql_stmt_free(stmt);
	db_meta_unfill(&meta);
}

static void
sendpublic(struct kreq *r, const struct user *u)
{
	struct khead	*kr;
	struct kpair	*kpi, *kplim, *kpo;
	struct kjsonreq	 req;
	struct ksqlstmt	*stmt;
	struct entry	 entry;
	struct user	 user;
	size_t		 first, i;
	enum stmt	 estmt;
	char		 buf[64];
	int		 omtime = 0;
	const char	*lang;
	struct meta	 meta;

	db_meta_get(r, &meta);

	/* Should we order by mtime instead of the default? */

	if (NULL != (kpo = r->fieldmap[KEY_ORDER]) &&
	    0 == strcmp(kpo->parsed.s, "mtime"))
		omtime = 1;

	kr = r->reqmap[KREQU_IF_NONE_MATCH];
	kplim = r->fieldmap[KEY_LIMIT];

	/* Can be NULL or empty: either way, disregard. */

	lang = NULL == r->fieldmap[KEY_LANG] ? 
		NULL : '\0' == *r->fieldmap[KEY_LANG]->parsed.s ?
		NULL : r->fieldmap[KEY_LANG]->parsed.s;

	if (NULL != (kpi = r->fieldmap[KEY_ENTRYID])) {
		/* 
		 * If we're looking for a specific entry, we ignore all
		 * of the other filters.
		 */
		ksql_stmt_alloc(r->arg, &stmt, 
			stmts[STMT_ENTRY_GET_PUBLIC], 
			STMT_ENTRY_GET_PUBLIC);
		ksql_bind_int(stmt, 0, kpi->parsed.i);
	} else if (NULL != kplim && NULL == lang) {
		/* Filter by establishing a limit. */
		estmt = omtime ?
			STMT_ENTRY_LIST_PUBLIC_MTIME_LIMIT :
			STMT_ENTRY_LIST_PUBLIC_LIMIT;
		ksql_stmt_alloc(r->arg, &stmt, 
			stmts[estmt], estmt);
		ksql_bind_int(stmt, 0, kplim->parsed.i);
	} else if (NULL != kplim && NULL != lang) {
		/* Filter by establishing a limit and language. */
		estmt = omtime ?
			STMT_ENTRY_LIST_PUBLIC_MTIME_LANG_LIMIT :
			STMT_ENTRY_LIST_PUBLIC_LANG_LIMIT;
		ksql_stmt_alloc(r->arg, &stmt, 
			stmts[estmt], estmt);
		ksql_bind_str(stmt, 0, lang);
		ksql_bind_int(stmt, 1, kplim->parsed.i);
	} else if (NULL == kplim && NULL != lang) {
		/* Filter by language. */
		estmt = omtime ?
			STMT_ENTRY_LIST_PUBLIC_MTIME_LANG:
			STMT_ENTRY_LIST_PUBLIC_LANG;
		ksql_stmt_alloc(r->arg, &stmt, 
			stmts[estmt], estmt);
		ksql_bind_str(stmt, 0, lang);
	} else {
		/* Do not filter at all: grab all. */
		estmt = omtime ?
			STMT_ENTRY_LIST_PUBLIC_MTIME:
			STMT_ENTRY_LIST_PUBLIC;
		ksql_stmt_alloc(r->arg, &stmt, 
			stmts[estmt], estmt);
	}

	first = 1;
	while (KSQL_ROW == ksql_stmt_step(stmt)) {
		i = 0;
		db_user_fill(&user, stmt, &i);
		db_entry_fill(&entry, stmt, &i);
		if (first) {
			first = 0;
			snprintf(buf, sizeof(buf), 
				"\"%" PRId64 "\"", meta.mtime);
			if (NULL != kr && 
			    0 == strcmp(buf, kr->val)) {
				sendhttp(r, KHTTP_304);
				ksql_stmt_free(stmt);
				db_user_unfill(&user);
				db_entry_unfill(&entry);
				return;
			} 
			sendhttphead(r, KHTTP_200);
			khttp_head(r, kresps[KRESP_ETAG], "%s", buf);
			khttp_body(r);
			kjson_open(&req, r);
			kjson_obj_open(&req);
			json_putuser(&req, u, 1);
			kjson_arrayp_open(&req, "entries");
		}
		json_putentry(&req, &user, &entry, NULL);
		db_user_unfill(&user);
		db_entry_unfill(&entry);
	}

	if (first) {
		sendhttp(r, KHTTP_200);
		kjson_open(&req, r);
		kjson_obj_open(&req);
		json_putuser(&req, u, 1);
		kjson_arrayp_open(&req, "entries");
	}

	ksql_stmt_free(stmt);
	kjson_array_close(&req);
	kjson_obj_close(&req);
	kjson_close(&req);
	db_meta_unfill(&meta);
}

static void
sendlogin(struct kreq *r)
{
	int64_t		 sid, cookie;
	struct kpair	*kpi, *kpp;
	char		 buf[64];
	struct user	*u;
	const char	*secure;

	if (NULL == (kpi = r->fieldmap[KEY_EMAIL]) ||
	    NULL == (kpp = r->fieldmap[KEY_PASS])) {
		sendhttp(r, KHTTP_400);
		return;
	}

	u = db_user_find(r->arg, 
		kpi->parsed.s, kpp->parsed.s);

	if (NULL == u) {
		sendhttp(r, KHTTP_400);
		return;
	} else if (USER_DISABLED & u->flags) {
		kutil_info(r, u->email, 
			"logging in when disabled");
		sendhttp(r, KHTTP_400);
		return;
	}

	cookie = arc4random();
	sid = db_sess_new(r->arg, cookie, u);
	kutil_epoch2str
		(time(NULL) + 60 * 60 * 24 * 365,
		 buf, sizeof(buf));
#ifdef SECURE
	secure = " secure;";
#else
	secure = "";
#endif
	khttp_head(r, kresps[KRESP_STATUS], 
		"%s", khttps[KHTTP_200]);
	khttp_head(r, kresps[KRESP_SET_COOKIE],
		"%s=%" PRId64 ";%s HttpOnly; path=/; expires=%s", 
		keys[KEY_SESSCOOKIE].name, cookie, secure, buf);
	khttp_head(r, kresps[KRESP_SET_COOKIE],
		"%s=%" PRId64 ";%s HttpOnly; path=/; expires=%s", 
		keys[KEY_SESSID].name, sid, secure, buf);
	khttp_body(r);

	db_user_free(u);
}

static void
sendlogout(struct kreq *r)
{
	const char	*secure;
	char		 buf[32];

	kutil_epoch2str(0, buf, sizeof(buf));
#ifdef SECURE
	secure = " secure;";
#else
	secure = "";
#endif

	sendhttphead(r, KHTTP_200);
	khttp_head(r, kresps[KRESP_SET_COOKIE],
		"%s=; path=/;%s HttpOnly; expires=%s", 
		keys[KEY_SESSCOOKIE].name, secure, buf);
	khttp_head(r, kresps[KRESP_SET_COOKIE],
		"%s=; path=/;%s HttpOnly; expires=%s", 
		keys[KEY_SESSID].name, secure, buf);
	khttp_body(r);
	db_sess_del(r->arg, 
		r->cookiemap[KEY_SESSID]->parsed.i, 
		r->cookiemap[KEY_SESSCOOKIE]->parsed.i);
}

int
main(void)
{
	struct kreq	 r;
	enum kcgi_err	 er;
	struct ksql	*sql;
	struct ksqlcfg	 cfg;
	struct user	*u;

	/* Log into a separate logfile (not system log). */

	kutil_openlog(LOGFILE);

	/* Configure normal database except with foreign keys. */

	memset(&cfg, 0, sizeof(struct ksqlcfg));
	cfg.flags = KSQL_EXIT_ON_ERR |
		    KSQL_FOREIGN_KEYS |
		    KSQL_SAFE_EXIT;
	cfg.err = ksqlitemsg;
	cfg.dberr = ksqlitedbmsg;

	/* Actually parse HTTP document. */

	er = khttp_parse(&r, keys, KEY__MAX, 
		pages, PAGE__MAX, PAGE_INDEX);

	if (KCGI_OK != er) {
		fprintf(stderr, "HTTP parse error: %d\n", er);
		return(EXIT_FAILURE);
	}

#ifdef	__OpenBSD__
	if (-1 == pledge("stdio rpath cpath wpath flock fattr", NULL)) {
		kutil_warn(&r, NULL, "pledge");
		khttp_free(&r);
		return(EXIT_FAILURE);
	}
#endif

	/*
	 * Front line of defence: make sure we're a proper method, make
	 * sure we're a page, make sure we're a JSON file.
	 */

	if (KMETHOD_GET != r.method && 
	    KMETHOD_POST != r.method) {
		sendhttp(&r, KHTTP_405);
		khttp_free(&r);
		return(EXIT_SUCCESS);
	} else if (PAGE__MAX == r.page || 
		   (KMIME_APP_JSON != r.mime && 
		    PAGE_ATOM != r.page) ||
		   (KMIME_TEXT_XML != r.mime &&
		    PAGE_ATOM == r.page)) {
		sendhttp(&r, KHTTP_404);
		khttp_puts(&r, "Page not found.");
		khttp_free(&r);
		return(EXIT_SUCCESS);
	}

	/* Allocate database. */

	if (NULL == (sql = ksql_alloc(&cfg))) {
		sendhttp(&r, KHTTP_500);
		khttp_free(&r);
		return(EXIT_SUCCESS);
	} 

	ksql_open(sql, DATADIR "/dblg.db");
	r.arg = sql;

	/* 
	 * Assume we're logging in with a session and grab the session
	 * from the database.
	 * This is our first database access.
	 */

	u = db_user_sess_get(r.arg,
		NULL != r.cookiemap[KEY_SESSID] ?
		r.cookiemap[KEY_SESSID]->parsed.i : -1,
		NULL != r.cookiemap[KEY_SESSCOOKIE] ?
		r.cookiemap[KEY_SESSCOOKIE]->parsed.i : -1);

	/* User authorisation. */

	if (PAGE_LOGIN != r.page &&
	    PAGE_PUBLIC != r.page && 
	    PAGE_ATOM != r.page && 
	    NULL == u) {
		sendhttp(&r, KHTTP_403);
		khttp_free(&r);
		ksql_free(sql);
		return(EXIT_SUCCESS);
	}

	if ((PAGE_ADD_USER == r.page ||
	     PAGE_MOD_META_TITLE == r.page ||
	     PAGE_MOD_ENABLE == r.page) &&
	    (NULL == u || ! (USER_ADMIN & u->flags))) {
		sendhttp(&r, KHTTP_404);
		khttp_free(&r);
		ksql_free(sql);
		return(EXIT_SUCCESS);
	}

	if (PAGE_LOGIN != r.page && NULL != u && 
	    USER_DISABLED & u->flags) {
		kutil_info(&r, u->email,  
			"using site when disabled");
		sendhttp(&r, KHTTP_404);
		khttp_free(&r);
		ksql_free(sql);
		return(EXIT_SUCCESS);
	}

	switch (r.page) {
	case (PAGE_ADD_USER):
		sendadduser(&r, u);
		break;
	case (PAGE_ATOM):
		sendatom(&r);
		break;
	case (PAGE_INDEX):
		sendindex(&r, u);
		break;
	case (PAGE_LOGIN):
		sendlogin(&r);
		break;
	case (PAGE_LOGOUT):
		sendlogout(&r);
		break;
	case (PAGE_MOD_CLOUD):
		sendmodcloud(&r, u);
		break;
	case (PAGE_MOD_EMAIL):
		sendmodemail(&r, u);
		break;
	case (PAGE_MOD_ENABLE):
		sendmodenable(&r, u);
		break;
	case (PAGE_MOD_LANG):
		sendmodlang(&r, u);
		break;
	case (PAGE_MOD_LINK):
		sendmodlink(&r, u);
		break;
	case (PAGE_MOD_META_TITLE):
		sendmodmetatitle(&r, u);
		break;
	case (PAGE_MOD_NAME):
		sendmodname(&r, u);
		break;
	case (PAGE_MOD_PASS):
		sendmodpass(&r, u);
		break;
	case (PAGE_PUBLIC):
		sendpublic(&r, u);
		break;
	case (PAGE_REMOVE):
		sendremove(&r, u);
		break;
	case (PAGE_SUBMIT):
		sendsubmit(&r, u);
		break;
	default:
		abort();
	}

	khttp_free(&r);
	ksql_free(sql);
	return(EXIT_SUCCESS);
}
