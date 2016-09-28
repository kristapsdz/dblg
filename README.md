## Synopsis

This is a read-only repository mirror for dblg.
There's nothing to say here yet---stay tuned!
But in short, the project consists of a backend server that serves JSON
to two front-ends.
The first is for writing blogs.
The second is for viewing them.
The writing tool would probably be installed as-is, but I'd expect the
latter to be integrated into one or more other pages.

## Installation

There are three components to this blogger: the back-end, the editor,
and the viewer.

For the former two, you'll need to download and configure the software.
This is fairly straightforward.  First, download.  Then, open the
[GNUmakefile](GNUmakefile).  Override the variables you'd like in a
GNUmakefile.local file, which won't be touched.  The
[GNUmakefile](GNUmakefile) documents each variable.

You'll need [kcgi](https://kristaps.bsd.lv/kcgi) and
[ksql](https://kristaps.bsd.lv/ksql) for the back-end server.

### Viewer

To install the viewer, you don't need to download this software.  Simply
use the existing [blog.xml](blog.xml), [blog.css](blog.css), and
[blog.js](blog.js) files in your application.  You can edit these as you
wish, or just use the [blog.xml](blog.xml) file and link to the CSS and
JavaScript through the GitHub CDN.

You'll need to invoke the viewer JavaScript after loading all media.
See the `script` bits of [blog.xml](blog.xml) for an example.

### Editor

To use the editor, you'll need to download and configure the dblg
software package first.  Once you've customised the
[GNUmakefile](GNUmakefile) with your parameters, simply run `make
installserver`.

### Back-end

To use the backend, you'll need to download and configure the dblg
software package first.  Once you've customised the
[GNUmakefile](GNUmakefile) with your parameters, simply run `make
installcgi` (or `make updatecgi` for existing installations).

Then, log in to the editor with your default username and password (as
configured) and add the users you need.

## License

All sources use the ISC (like OpenBSD) license.
See the [LICENSE.md](LICENSE.md) file for details.
