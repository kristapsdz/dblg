-- "Meta" information relating to the entire blog as an object.
-- This will be created on-demand, if not found, with meaningful
-- defaults.
-- It will only have one row.

CREATE TABLE meta (
	-- The last modification time to any article or any user data
	-- that is public-facing.
	mtime INTEGER NOT NULL DEFAULT(0),
	-- The title of the blog.
	-- This is used in the Atom feed for reporting the title.
	-- If unspecified, it will be set to the hostname.
	title TEXT,
	-- Template file for providing static web pages.
	-- If this is not NULL, requests to the "public" resources using
	-- the text/html MIME type will consider this a template file
	-- relative to the file-system root.
	-- If unspecifie,d requests to the text/html MIME type will
	-- return an HTTP error 404.
	template TEXT,
	-- Used to make sure there's only one row.
	-- DO NOT set this value to non-zero.
	lock INTEGER NOT NULL DEFAULT(0),
	id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
	unique(lock)
);

-- A blog entry.
-- This can be associated with a location (latitude and longitude), and
-- may be edited over time.

CREATE TABLE entry (
	-- Contents (markdown).
	contents TEXT NOT NULL,
	-- Title (text).
	title TEXT NOT NULL,
	-- Posting user.
	userid INTEGER NOT NULL,
	-- IETF language tag (as in RFC 2616, 3.10).
	lang TEXT,
	-- Latitude (decimal degrees) of post.
	-- Both latitude and longitude are required for location.
	latitude REAL,
	-- Longitude (decimal degrees) of post.
	-- Both latitude and longitude are required for location.
	longitude REAL,
	-- URL of an image associated with the entry.
	-- This is useful for showing a picture next to, say, the aside.
	image TEXT,
	-- Creation time.
	ctime INTEGER NOT NULL DEFAULT(strftime('%s', 'now')),
	-- Last updated.
	mtime INTEGER NOT NULL DEFAULT(strftime('%s', 'now')),
	-- 1 if currently being edited (and thus not to be shown).
	flags INTEGER NOT NULL DEFAULT(0),
	-- An aside is a short bit of markdown describing the entry.
	-- It can be NULL.
	aside TEXT,
	-- Unique identifier.
	id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
	FOREIGN KEY(userid) REFERENCES user(id)
);

-- A blogger.
-- This is just an e-mail identified individual.
-- Nothing complicated.

CREATE TABLE user (
	-- Free-form username.
	name TEXT NOT NULL,
	-- E-mail address (and login identifier). 
	email TEXT NOT NULL,
	-- Default IETF language tag (as in RFC 2616, 3.10).
	lang TEXT,
	-- A link to their homepage, if applicable.
	link TEXT,
	-- Hash representation of password.
	hash TEXT NOT NULL,
	-- If applicable, a public key for cloud (media) updates.
	cloudkey TEXT,
	-- If applicable, a secret key for cloud (media) content.
	cloudsecret TEXT,
	-- If applicable, the path on the cloud server for media storage.
	cloudpath TEXT NOT NULL DEFAULT(''),
	-- The name of the cloud account for media storage, if applicable.
	cloudname TEXT,
	-- Bitmask of flags.
	-- If containing 0x01, the user can administer other users.
	-- If containing 0x02, the user is disabled and can't login.
	flags INTEGER NOT NULL DEFAULT(0),
	-- Unique identifier.
	id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
	unique (email)
);

-- Browser session.
-- This is the usual setup.

CREATE TABLE sess (
	-- User attached to session.
	userid INTEGER NOT NULL,
	-- Unique cookie generated for session.
	cookie INTEGER NOT NULL,
	-- Creation time in database.
	ctime INTEGER NOT NULL DEFAULT(strftime('%s', 'now')),
	-- Unique identifier.
	id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
	FOREIGN KEY(userid) REFERENCES user(id)
);

-- A default "root" user.

INSERT INTO user (name, email, hash, flags) VALUES ('Charlie Root', '@AEMAIL@', '@AHASH@', 1);
