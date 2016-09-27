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
	-- Latitude (decimal degrees) of post.
	-- Both latitude and longitude are required for location.
	latitude REAL,
	-- Longitude (decimal degrees) of post.
	-- Both latitude and longitude are required for location.
	longitude REAL,
	-- Creation time.
	ctime INTEGER NOT NULL DEFAULT(strftime('%s', 'now')),
	-- Last updated.
	mtime INTEGER NOT NULL DEFAULT(strftime('%s', 'now')),
	-- Not currently used.
	flags INTEGER NOT NULL DEFAULT(0),
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
	-- If set to 1, the user can administer other users.
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
