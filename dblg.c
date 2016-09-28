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
#include <ksql.h>

struct	cloud {	       
	char		*key;
	char		*secret;
	char		*path;
	char		*name;
	int		 set;
};

struct	user {
	struct cloud	 cloud;
	char		*email;
	char		*link;
	char		*name;
	int64_t		 flags;
#define	USER_ADMIN	 0x01
	int64_t		 id;
};

struct	entry {
	char		*content;
	char		*title;
	time_t		 ctime;
	time_t		 mtime;
	int		 coords;
	double		 lat;
	double		 lng;
	int64_t		 flags;
	int64_t		 id;
};

enum	page {
	PAGE_ADD_USER,
	PAGE_INDEX,
	PAGE_LOGIN,
	PAGE_LOGOUT,
	PAGE_MOD_CLOUD,
	PAGE_MOD_EMAIL,
	PAGE_MOD_LINK,
	PAGE_MOD_NAME,
	PAGE_MOD_PASS,
	PAGE_PUBLIC,
	PAGE_REMOVE,
	PAGE_SUBMIT,
	PAGE__MAX
};

enum	key {
	KEY_CLOUDKEY,
	KEY_CLOUDNAME,
	KEY_CLOUDPATH,
	KEY_CLOUDSECRET,
	KEY_EMAIL,
	KEY_ENTRYID,
	KEY_LATITUDE,
	KEY_LONGITUDE,
	KEY_LINK,
	KEY_MARKDOWN,
	KEY_NAME,
	KEY_PASS,
	KEY_SESSCOOKIE,
	KEY_SESSID,
	KEY_TITLE,
	KEY__MAX
};

enum	stmt {
	STMT_ENTRY_DELETE,
	STMT_ENTRY_GET,
	STMT_ENTRY_LIST,
	STMT_ENTRY_MODIFY,
	STMT_ENTRY_NEW,
	STMT_SESS_DEL,
	STMT_SESS_GET,
	STMT_SESS_NEW,
	STMT_USER_ADD,
	STMT_USER_GET,
	STMT_USER_LIST,
	STMT_USER_LOOKUP,
	STMT_USER_MOD_CLOUD,
	STMT_USER_MOD_EMAIL,
	STMT_USER_MOD_HASH,
	STMT_USER_MOD_LINK,
	STMT_USER_MOD_NAME,
	STMT__MAX
};

#define	USER	"user.id,user.email,user.name,user.link," \
		"user.cloudkey,user.cloudsecret,user.cloudname," \
		"user.cloudpath,user.flags"
#define	ENTRY	"entry.contents,entry.ctime,entry.id,entry.title," \
		"entry.latitude,entry.longitude,entry.mtime," \
		"entry.flags"

static	const char *const stmts[STMT__MAX] = {
	/* STMT_ENTRY_DELETE */
	"DELETE FROM entry WHERE id=? AND entry.userid=?",
	/* STMT_ENTRY_GET */
	"SELECT " USER "," ENTRY " FROM entry "
		"INNER JOIN user ON user.id=entry.userid "
		"WHERE entry.id=?",
	/* STMT_ENTRY_LIST */
	"SELECT " USER "," ENTRY " FROM entry "
		"INNER JOIN user ON user.id=entry.userid "
		"ORDER BY entry.mtime DESC",
	/* STMT_ENTRY_MODIFY */
	"UPDATE entry SET contents=?,title=?,latitude=?,"
		"longitude=?,mtime=? WHERE userid=? AND id=?",
	/* STMT_ENTRY_NEW */
	"INSERT INTO entry (contents,title,userid,latitude,"
		"longitude) VALUES (?,?,?,?,?)",
	/* STMT_SESS_DEL */
	"DELETE FROM sess WHERE id=? AND cookie=?",
	/* STMT_SESS_GET */
	"SELECT " USER " FROM sess "
		"INNER JOIN user ON user.id=sess.userid "
		"WHERE sess.id=? AND sess.cookie=?",
	/* STMT_SESS_NEW */
	"INSERT INTO sess (cookie,userid) VALUES (?,?)",
	/* STMT_USER_ADD */
	"INSERT INTO user (name,email,hash) VALUES (?,?,?)",
	/* STMT_USER_GET */
	"SELECT " USER " FROM user WHERE id=?",
	/* STMT_USER_LIST */
	"SELECT " USER " FROM user",
	/* STMT_USER_LOOKUP */
	"SELECT " USER ",hash FROM user WHERE email=?",
	/* STMT_USER_MOD_CLOUD */
	"UPDATE user SET cloudkey=?,cloudsecret=?,"
	       "cloudname=?,cloudpath=? WHERE id=?",
	/* STMT_USER_MOD_EMAIL */
	"UPDATE user SET email=? WHERE id=?",
	/* STMT_USER_MOD_HASH */
	"UPDATE user SET hash=? WHERE id=?",
	/* STMT_USER_MOD_LINK */
	"UPDATE user SET link=? WHERE id=?",
	/* STMT_USER_MOD_NAME */
	"UPDATE user SET name=? WHERE id=?",
};

static const struct kvalid keys[KEY__MAX] = {
	{ kvalid_string, "cloudkey" }, /* KEY_CLOUDKEY */
	{ kvalid_string, "cloudname" }, /* KEY_CLOUDNAME */
	{ kvalid_string, "cloudpath" }, /* KEY_CLOUDPATH */
	{ kvalid_string, "cloudsecret" }, /* KEY_CLOUDSECRET */
	{ kvalid_email, "email" }, /* KEY_EMAIL */
	{ kvalid_int, "entryid" }, /* KEY_ENTRYID */
	{ kvalid_double, "latitude" }, /* KEY_LATITUDE */
	{ kvalid_double, "longitude" }, /* KEY_LONGITUDE */
	{ kvalid_string, "link" }, /* KEY_LINK */
	{ kvalid_stringne, "markdown" }, /* KEY_MARKDOWN */
	{ kvalid_stringne, "name" }, /* KEY_NAME */
	{ kvalid_stringne, "pass" }, /* KEY_PASS */
	{ kvalid_uint, "sesscookie" }, /* KEY_SESSCOOKIE */
	{ kvalid_int, "sessid" }, /* KEY_SESSID */
	{ kvalid_stringne, "title" }, /* KEY_TITLE */
};

static const char *const pages[PAGE__MAX] = {
	"adduser", /* PAGE_ADD_USER */
	"index", /* PAGE_INDEX */
	"login", /* PAGE_LOGIN */
	"logout", /* PAGE_LOGOUT */
	"modcloud", /* PAGE_MOD_CLOUD */
	"modemail", /* PAGE_MOD_EMAIL */
	"modlink", /* PAGE_MOD_LINK */
	"modname", /* PAGE_MOD_NAME */
	"modpass", /* PAGE_MOD_PASS */
	"public", /* PAGE_PUBLIC */
	"remove", /* PAGE_REMOVE */
	"submit", /* PAGE_SUBMIT */
};

/* Forward declaration for attributes. */

static void lwarnx(const char *, ...) 
	__attribute__((format(printf, 1, 2)));
static void linfo(const char *, ...) 
	__attribute__((format(printf, 1, 2)));

static void
linfo(const char *fmt, ...)
{
	va_list	 ap;
	time_t	 t;
	char	 buf[32];
	size_t	 sz;

	t = time(NULL);
	ctime_r(&t, buf);
	sz = strlen(buf);
	buf[sz - 1] = '\0';
	fprintf(stderr, "[%s] ", buf);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
}

static void
lwarnx(const char *fmt, ...)
{
	va_list	 ap;
	time_t	 t;
	char	 buf[32];
	size_t	 sz;

	t = time(NULL);
	ctime_r(&t, buf);
	sz = strlen(buf);
	buf[sz - 1] = '\0';
	fprintf(stderr, "[%s] WARNING: ", buf);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
}

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
}

static void
db_entry_unfill(struct entry *p)
{

	if (NULL == p)
		return;
	free(p->content);
	free(p->title);
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
}

static int64_t
db_entry_modify(struct ksql *sql, const struct user *user, 
	const char *title, const char *text, 
	int64_t id, double lat, double lng)
{
	struct ksqlstmt	*stmt;

	ksql_stmt_alloc(sql, &stmt, 
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
	ksql_bind_int(stmt, 5, user->id);
	ksql_bind_int(stmt, 6, id);
	ksql_stmt_step(stmt);
	ksql_stmt_free(stmt);
	return(id);
}

static int64_t
db_entry_new(struct ksql *sql, const struct user *user, 
	const char *title, const char *text, double lat, double lng)
{
	struct ksqlstmt	*stmt;
	int64_t		 id;

	ksql_stmt_alloc(sql, &stmt, 
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
	ksql_stmt_step(stmt);
	ksql_stmt_free(stmt);
	ksql_lastid(sql, &id);
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
db_user_mod_pass(struct ksql *sql, 
	const struct user *u, const char *pass)
{
	struct ksqlstmt	*stmt;
	char		 hash[64];

#ifdef __OpenBSD__
	crypt_newhash(pass, "blowfish,a", hash, sizeof(hash));
#else
	strlcpy(hash, pass, sizeof(hash));
#endif
	ksql_stmt_alloc(sql, &stmt,
		stmts[STMT_USER_MOD_HASH],
		STMT_USER_MOD_HASH);
	ksql_bind_str(stmt, 0, hash);
	ksql_bind_int(stmt, 1, u->id);
	ksql_stmt_step(stmt);
	ksql_stmt_free(stmt);
}

static int
db_user_mod_email(struct ksql *sql, 
	const struct user *u, const char *email)
{
	struct ksqlstmt	*stmt;
	enum ksqlc	 c;

	ksql_stmt_alloc(sql, &stmt, 
		stmts[STMT_USER_MOD_EMAIL], 
		STMT_USER_MOD_EMAIL);
	ksql_bind_str(stmt, 0, email);
	ksql_bind_int(stmt, 1, u->id);
	c = ksql_stmt_cstep(stmt);
	ksql_stmt_free(stmt);
	return(KSQL_CONSTRAINT != c);
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
db_user_mod_cloud(struct ksql *sql, const struct user *u, 
	const char *key, const char *secret,
	const char *name, const char *path)
{
	struct ksqlstmt	*stmt;

	ksql_stmt_alloc(sql, &stmt, 
		stmts[STMT_USER_MOD_CLOUD], 
		STMT_USER_MOD_CLOUD);
	bind_if_not_null(stmt, 0, key);
	bind_if_not_null(stmt, 1, secret);
	bind_if_not_null(stmt, 2, name);
	bind_if_not_null(stmt, 3, path);
	ksql_bind_int(stmt, 4, u->id);
	ksql_stmt_step(stmt);
	ksql_stmt_free(stmt);
}

static void
db_user_mod_link(struct ksql *sql, 
	const struct user *u, const char *link)
{
	struct ksqlstmt	*stmt;

	ksql_stmt_alloc(sql, &stmt, 
		stmts[STMT_USER_MOD_LINK], 
		STMT_USER_MOD_LINK);
	if (NULL != link)
		ksql_bind_str(stmt, 0, link);
	else
		ksql_bind_null(stmt, 0);
	ksql_bind_int(stmt, 1, u->id);
	ksql_stmt_step(stmt);
	ksql_stmt_free(stmt);
}

static int
db_user_add(struct ksql *sql, 
	const char *email, const char *pass)
{
	struct ksqlstmt	*stmt;
	char		 hash[64];
	enum ksqlc	 c;

#ifdef __OpenBSD__
	crypt_newhash(pass, "blowfish,a", hash, sizeof(hash));
#else
	strlcpy(hash, pass, sizeof(hash));
#endif
	ksql_stmt_alloc(sql, &stmt, 
		stmts[STMT_USER_ADD], 
		STMT_USER_ADD);
	ksql_bind_str(stmt, 0, "Anonymous user");
	ksql_bind_str(stmt, 1, email);
	ksql_bind_str(stmt, 2, hash);
	c = ksql_stmt_cstep(stmt);
	ksql_stmt_free(stmt);
	return(KSQL_CONSTRAINT != c);
}

static void
db_user_mod_name(struct ksql *sql, 
	const struct user *u, const char *name)
{
	struct ksqlstmt	*stmt;

	ksql_stmt_alloc(sql, &stmt, 
		stmts[STMT_USER_MOD_NAME], 
		STMT_USER_MOD_NAME);
	ksql_bind_str(stmt, 0, name);
	ksql_bind_int(stmt, 1, u->id);
	ksql_stmt_step(stmt);
	ksql_stmt_free(stmt);
}

static void
db_entry_delete(struct ksql *sql, 
	const struct user *u, int64_t id)
{
	struct ksqlstmt	*stmt;

	ksql_stmt_alloc(sql, &stmt, 
		stmts[STMT_ENTRY_DELETE], 
		STMT_ENTRY_DELETE);
	ksql_bind_int(stmt, 0, id);
	ksql_bind_int(stmt, 1, u->id);
	ksql_stmt_step(stmt);
	ksql_stmt_free(stmt);
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
	if (NULL != u->link)
		kjson_putstringp(req, "link", u->link);
	else
		kjson_putnullp(req, "link");
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
	kjson_putboolp(req, "admin", USER_ADMIN & u->flags);
	kjson_obj_close(req);
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
	kjson_putstringp(req, "title", entry->title);
	if (entry->coords) {
		kjson_objp_open(req, "coords");
		kjson_putdoublep(req, "lat", entry->lat);
		kjson_putdoublep(req, "lng", entry->lng);
		kjson_obj_close(req);
	} else
		kjson_putnullp(req, "coords");
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
	db_entry_delete(r->arg, u,
		r->fieldmap[KEY_ENTRYID]->parsed.i);
	sendhttp(r, KHTTP_200);
}

static void
sendsubmit(struct kreq *r, const struct user *u)
{
	struct kpair	*kpm, *kpt, *kpi, *kplat, *kplng;
	int64_t		 id;
	struct kjsonreq	 req;
	double		 lat, lng;

	if (NULL == (kpt = r->fieldmap[KEY_TITLE]) ||
	    NULL == (kpm = r->fieldmap[KEY_MARKDOWN])) {
		sendhttp(r, KHTTP_400);
		return;
	}

	assert(NULL != u);

    	kplat = r->fieldmap[KEY_LATITUDE];
    	kplng = r->fieldmap[KEY_LONGITUDE];

	if (NULL != kplat && NULL != kplng) {
		lat = r->fieldmap[KEY_LATITUDE]->parsed.d;
		lng = r->fieldmap[KEY_LONGITUDE]->parsed.d;
	} else 
		lat = lng = 1.0 / 0.0;

	if (NULL != (kpi = r->fieldmap[KEY_ENTRYID]) &&
	    kpi->parsed.i > 0)  
		id = db_entry_modify(r->arg, u, 
			kpt->parsed.s, kpm->parsed.s, 
			kpi->parsed.i, lat, lng);
	else
		id = db_entry_new(r->arg, u, 
			kpt->parsed.s, kpm->parsed.s, 
			lat, lng);

	sendhttp(r, KHTTP_200);
	kjson_open(&req, r);
	kjson_obj_open(&req);
	kjson_putintp(&req, "id", id);
	kjson_obj_close(&req);
	kjson_close(&req);
}

static void
sendmodlink(struct kreq *r, const struct user *u)
{
	struct kpair	*kp;

	kp = r->fieldmap[KEY_LINK];
	db_user_mod_link(r->arg, u, 
		NULL != kp ? kp->parsed.s : NULL);
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
		db_user_mod_cloud(r->arg, u, 
			kpk->parsed.s, kps->parsed.s,
			kpn->parsed.s, kpp->parsed.s);
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
		rc = db_user_mod_email(r->arg, u, kp->parsed.s);
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
		db_user_mod_pass(r->arg, u, kp->parsed.s);
	} else
		sendhttp(r, KHTTP_400);
}

static void
sendmodname(struct kreq *r, const struct user *u)
{
	struct kpair	*kp;

	if (NULL != (kp = r->fieldmap[KEY_NAME])) {
		sendhttp(r, KHTTP_200);
		db_user_mod_name(r->arg, u, kp->parsed.s);
	} else 
		sendhttp(r, KHTTP_400);
}

static void
sendadduser(struct kreq *r, const struct user *u)
{
	struct kpair	*kpe, *kpp;
	int		 rc;

	assert(NULL != u && USER_ADMIN & u->flags);

	rc = 0;
	if (NULL != (kpe = r->fieldmap[KEY_EMAIL]) &&
	    NULL != (kpp = r->fieldmap[KEY_PASS]))
		rc = db_user_add(r->arg, 
			kpe->parsed.s, kpp->parsed.s);
	sendhttp(r, rc ? KHTTP_200 : KHTTP_400);
	if (rc) 
		linfo("%s: added user", kpe->parsed.s);
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

	assert(NULL != u);
	sendhttp(r, KHTTP_200);
	kjson_open(&req, r);
	kjson_obj_open(&req);
	json_putuser(&req, u, 0);

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
sendpublic(struct kreq *r, const struct user *u)
{
	struct khead	*kr;
	struct kpair	*kpi;
	struct kjsonreq	 req;
	struct ksqlstmt	*stmt;
	struct entry	 entry;
	struct user	 user;
	size_t		 first, i;
	char		 buf[64];

	kr = r->reqmap[KREQU_IF_NONE_MATCH];

	if (NULL != (kpi = r->fieldmap[KEY_ENTRYID])) {
		ksql_stmt_alloc(r->arg, &stmt, 
			stmts[STMT_ENTRY_GET], 
			STMT_ENTRY_GET);
		ksql_bind_int(stmt, 0, kpi->parsed.i);
	} else
		ksql_stmt_alloc(r->arg, &stmt, 
			stmts[STMT_ENTRY_LIST], 
			STMT_ENTRY_LIST);

	first = 1;
	while (KSQL_ROW == ksql_stmt_step(stmt)) {
		i = 0;
		db_user_fill(&user, stmt, &i);
		db_entry_fill(&entry, stmt, &i);
		if (first) {
			first = 0;
			snprintf(buf, sizeof(buf), 
				"\"%" PRId64 "-%lld\"", entry.id, 
				(long long)entry.mtime);
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
}

static void
sendlogin(struct kreq *r)
{
	int64_t		 sid, cookie;
	struct kpair	*kpi, *kpp;
	time_t		 t;
	struct tm	*tm;
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
	} 

	cookie = arc4random();
	sid = db_sess_new(r->arg, cookie, u);
	t = time(NULL) + 60 * 60 * 24 * 365;
	tm = gmtime(&t);
	strftime(buf, sizeof(buf), "%a, %d %b %Y %T GMT", tm);
#ifdef SECURE
	secure = " secure;";
#else
	secure = "";
#endif
	khttp_head(r, kresps[KRESP_STATUS], 
		"%s", khttps[KHTTP_200]);
	khttp_head(r, kresps[KRESP_SET_COOKIE],
		"%s=%" PRId64 ";%s path=/; expires=%s", 
		keys[KEY_SESSCOOKIE].name, cookie, secure, buf);
	khttp_head(r, kresps[KRESP_SET_COOKIE],
		"%s=%" PRId64 ";%s path=/; expires=%s", 
		keys[KEY_SESSID].name, sid, secure, buf);
	khttp_body(r);

	db_user_free(u);
}

static void
sendlogout(struct kreq *r)
{
	const char	*secure;

#ifdef SECURE
	secure = " secure;";
#else
	secure = "";
#endif

	sendhttphead(r, KHTTP_200);
	khttp_head(r, kresps[KRESP_SET_COOKIE],
		"%s=; path=/;%s expires=", 
		keys[KEY_SESSCOOKIE].name, secure);
	khttp_head(r, kresps[KRESP_SET_COOKIE],
		"%s=; path=/;%s expires=", 
		keys[KEY_SESSID].name, secure);
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

	freopen(LOGFILE, "a", stderr);
	setlinebuf(stderr);

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
		lwarnx("HTTP parse error: %d", er);
		khttp_free(&r);
		return(EXIT_FAILURE);
	}

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
	           KMIME_APP_JSON != r.mime) {
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
	    PAGE_PUBLIC != r.page && NULL == u) {
		sendhttp(&r, KHTTP_403);
		khttp_free(&r);
		ksql_free(sql);
		return(EXIT_SUCCESS);
	}

	if (PAGE_ADD_USER == r.page &&
	    (NULL == u || ! (USER_ADMIN & u->flags))) {
		sendhttp(&r, KHTTP_404);
		khttp_free(&r);
		ksql_free(sql);
		return(EXIT_SUCCESS);
	}

	switch (r.page) {
	case (PAGE_ADD_USER):
		sendadduser(&r, u);
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
	case (PAGE_MOD_LINK):
		sendmodlink(&r, u);
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
