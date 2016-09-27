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
		sendQuery('@CGIURI@/remove.json?entryid=' + entry.id, 
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

		if ((sz = res.entries.length) > 2)
			sz = 2;
		for (i = 0; i < sz; i++) {
			cln = sub.cloneNode(true);
			e.appendChild(cln);
			repl(cln, 'blog-date', moment.unix
				(res.entries[i].ctime).calendar());
			repl(cln, 'blog-author', 
				res.entries[i].user.name);
			console.log(res.entries[i].user.link);
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
			attr(cln, 'blog-canonlink', 'href',
				window.location.origin + 
				window.location.pathname + 
				'?entryid=' + res.entries[i].id);

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
				for (j = 0; j < list.length; j++) {
					list[j].onclick = function(root, entry) {
						return function() {
							removebtn(root, entry);
						}
					}(cln, res.entries[i]);
				}
				list = cln.getElementsByClassName('blog-edit');
				for (j = 0; j < list.length; j++)
					list[j].href = '@HTURI@/blogger.html?' +
						'entryid=' + res.entries[i].id;
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

	function blogclient()
	{
		var entryid, query;

		if (null !== (entryid = getQueryVariable('entryid')))
			query = '?entryid=' + entryid;
		else
			query = '';

		return(sendQuery('@CGIURI@/public.json' + query,
			loadSetup, null, loadSuccess));
	}

	root.blogclient = blogclient;
})(this);

window.addEventListener('load', function(){
	blogclient();
});
