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
(function(root) {
	'use strict';

	var options = null;
	var cgiuri = null;

	function sendQuery(url, setup, error, success) 
	{
		var xmh = new XMLHttpRequest();

		if (null !== setup)
			setup();

		xmh.onreadystatechange=function() {
			if (xmh.readyState === 4 && 
		 	    xmh.status === 200) {
				console.log(url + ': success!');
				if (null !== success)
					success(xmh.responseText);
			} else if (xmh.readyState === 4) {
				console.log(url + 
					': failure: ' + xmh.status);
				if (null !== error)
					error(xmh.status);
			}
		};

		console.log(url + ': sending request');
		xmh.open('GET', url, true);
		xmh.send(null);
	}

	function getQueryVariable(variable)
	{
		var query, vars, i, pair;

		query = window.location.search.substring(1);
		vars = query.split("&");

		for (i = 0; i < vars.length; i++) {
			pair = vars[i].split("=");
			if(pair[0] == variable)
				return(pair[1]);
		}
		return(null);
	}

	function find(name)
	{
		var e;

		if (null === (e = document.getElementById(name)))
			console.log('cannot find node: ' + name);

		return(e);
	}

	function hide(key)
	{
		var e;

		e = (typeof key === 'string' || 
		     key instanceof String) ? find(key) : key;

		if (null !== e && ! e.classList.contains('hide'))
			e.classList.add('hide');

		return(e);
	}

	function show(key)
	{
		var e;

		e = (typeof key === 'string' || 
		     key instanceof String) ? find(key) : key;

		if (null !== e)
			e.classList.remove('hide');

		return(e);
	}

	function showc(node, cls)
	{
		var l, i;

		l = node.getElementsByClassName(cls);
		for (i = 0; i < l.length; i++)
			show(l[i]);
	}

	function hidec(node, cls)
	{
		var l, i;

		l = node.getElementsByClassName(cls);
		for (i = 0; i < l.length; i++)
			hide(l[i]);
	}

	function rattr(node, cls, attr)
	{
		var l, i;

		l = node.getElementsByClassName(cls);
		for (i = 0; i < l.length; i++)
			l[i].removeAttribute(attr);
	}

	function attr(node, cls, attr, txt)
	{
		var l, i;

		l = node.getElementsByClassName(cls);
		for (i = 0; i < l.length; i++)
			l[i].setAttribute(attr, txt);
	}

	function replhtml(node, cls, txt)
	{
		var l, i;

		l = node.getElementsByClassName(cls);
		for (i = 0; i < l.length; i++)
			l[i].innerHTML = txt;
	}

	function repl(node, cls, txt)
	{
		var l, i;

		l = node.getElementsByClassName(cls);
		for (i = 0; i < l.length; i++) {
			while (l[i].firstChild)
				l[i].removeChild(l[i].firstChild);
			l[i].appendChild(document.createTextNode(txt));
		}
	}

	function parser(resp)
	{
		var res;

		try  { 
			res = JSON.parse(resp);
		} catch (error) {
			console.log('JSON parse fail: ' + resp);
			return(null);
		}

		return(res);
	}

	function removebtnSetup(e)
	{
		showc(e, 'blog-delete-pending');
		hidec(e, 'blog-delete-submit');
	}

	function removebtnError(e)
	{
		hidec(e, 'blog-delete-pending');
		showc(e, 'blog-delete-submit');
		showc(e, 'blog-delete-check');
		hidec(e, 'blog-delete-commit');
	}

	function removebtnuncheck(e, entry)
	{
		showc(e, 'blog-delete-check');
		hidec(e, 'blog-delete-commit');
	}

	function removebtncheck(e, entry)
	{
		hidec(e, 'blog-delete-check');
		showc(e, 'blog-delete-commit');
	}

	function removebtn(e, entry)
	{
		sendQuery(cgiuri + '/remove.json?entryid=' + entry.id, 
			function() { removebtnSetup(e); }, 
			function() { removebtnError(e); },
			function() { location.href='index.html'; });
	}

	function loadSuccess(resp)
	{
		var res = parser(resp);
		var e, sub, cln, list, i, j, sz, conv;

		if (null === res)
			return;

		if (typeof showdown !== 'undefined')
			conv = new showdown.Converter();
		else
			conv = null;
		e = find('blog');
		sub = e.children[0];
		e.removeChild(sub);

		sz = res.entries.length;
		if (null !== options.limit && sz > options.limit)
			sz = options.limit;

		for (i = 0; i < sz; i++) {
			cln = sub.cloneNode(true);
			e.appendChild(cln);
			repl(cln, 'blog-ctime', moment.unix
				(res.entries[i].ctime).calendar());
			repl(cln, 'blog-mtime', moment.unix
				(res.entries[i].mtime).calendar());
			if (res.entries[i].mtime !== 
			    res.entries[i].ctime) {
				hidec(cln, 'blog-ctime-box');
				showc(cln, 'blog-mtime-box');
			} else {
				showc(cln, 'blog-ctime-box');
				hidec(cln, 'blog-mtime-box');
			}

			repl(cln, 'blog-author', 
				res.entries[i].user.name);
			if (null !== res.entries[i].user.link)
				attr(cln, 'blog-author', 'href', 
					res.entries[i].user.link);
			else
				rattr(cln, 'blog-author', 'href');
			repl(cln, 'blog-title', 
				res.entries[i].title);
			attr(cln, 'blog-title', 'href', 
				'blog.html?entryid=' + 
				res.entries[i].id);

			if (null !== options.blog) {
				showc(cln, 'blog-canonlink');
				attr(cln, 'blog-canonlink', 'href',
					options.blog + '?entryid=' + 
					res.entries[i].id);
			} else
				hidec(cln, 'blog-canonlink');

			res.entries[i].html = null !== conv ?
				conv.makeHtml(res.entries[i].content) :
				null;

			if (null !== res.entries[i].html)
				replhtml(cln, 'blog-content', 
					res.entries[i].html);

			if (null !== res.entries[i].coords) {
				showc(cln, 'blog-coords');
				attr(cln, 'blog-coords', 'href',
					'https://maps.google.com/?t=k&q=' + 
					res.entries[i].coords.lat + ',' + 
					res.entries[i].coords.lng + 
					'&zoom=13&sensor=false');
			} else
				hidec(cln, 'blog-coords');

			if (null !== res.user &&
			    res.user.id === res.entries[i].user.id) {
				showc(cln, 'blog-control');
				hidec(cln, 'blog-delete-pending');
				showc(cln, 'blog-delete-submit');
				hidec(cln, 'blog-delete-commit');
				showc(cln, 'blog-delete-check');
				list = cln.getElementsByClassName('blog-nodelete');
				for (j = 0; j < list.length; j++) {
					list[j].onclick = function(root, entry) {
						return function() {
							removebtnuncheck(root, entry);
						}
					}(cln, res.entries[i]);
				}
				list = cln.getElementsByClassName('blog-delete-check');
				for (j = 0; j < list.length; j++) {
					list[j].onclick = function(root, entry) {
						return function() {
							removebtncheck(root, entry);
						}
					}(cln, res.entries[i]);
				}
				list = cln.getElementsByClassName('blog-delete');
				for (j = 0; j < list.length; j++)
					list[j].onclick = function(root, entry) {
						return function() {
							removebtn(root, entry);
						}
					}(cln, res.entries[i]);

				list = cln.getElementsByClassName('blog-edit');
				for (j = 0; j < list.length; j++)
					if (null != options.editor) {
						show(list[j]);
						list[j].href = options.editor + 
							'?entryid=' + res.entries[i].id;
					} else
						hide(list[j]);
			
			} else
				hidec(cln, 'blog-control');
		}
		show(e);
		show('blog');
	}

	function loadSetup()
	{
		hide('blog');
	}

	function blogclient(uri, opts)
	{
		var entryid, query;

		options = {};
		cgiuri = uri;

		if (null !== opts && typeof opts !== 'undefined') {
			options.editor = 
				(typeof opts.editor === 'string') ? 
				opts.editor : null;
			options.limit = 
				(typeof opts.limit === 'number') ? 
				opts.limit : null;
			options.blog = 
				(typeof opts.blog === 'string') ? 
				opts.blog : null;
			options.lang = 
				(typeof opts.lang === 'string') ? 
				opts.lang : null;
		} else {
			options.editor = null;
			options.limit = null;
			options.blog = null;
			options.lang = null;
		}

		query = '';
		if (null !== (entryid = getQueryVariable('entryid')))
			query = '?entryid=' + entryid;
		return(sendQuery(cgiuri + '/public.json' + query,
			loadSetup, null, loadSuccess));
	}

	root.blogclient = blogclient;
})(this);
