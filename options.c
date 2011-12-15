#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include "common.h"
#include "library.h"

#define OPTION_ENTRY(x) MODE_##x,
enum mode {
#include "option_entries.h"
};
#undef OPTION_ENTRY

struct opstrings {
	char *str;
	enum mode mode;
};

#define OPTION_ENTRY(x) {"-:"#x, MODE_##x},
static struct opstrings opstrings[]={
#include "option_entries.h"
	{NULL, 0}
};
#undef OPTION_ENTRY

/* need to maintain state for parsing -I<space>path */
static int include_space;

void die(char *error)
{
	fprintf(stderr, "Error in command line: %s\n", error);
	exit(1);
}

static char *add_slashes(char *in)
{
	int incmode = 0;
	int newlen = 0;
	char *ptr, *out, *outptr;

	if (strncasecmp(in, "-i", 2) == 0) {
		incmode = 1;
	}

	ptr = in;
	while (*ptr) {
		if (*ptr == '"' || *ptr == ' ')
			newlen++;
		if (!incmode && (*ptr == '(' || *ptr == ')'))
			newlen++;

		newlen++; ptr++;
	}
	out = malloc(sizeof(char) * newlen + 1);
	ptr = in;
	outptr = out;
	while (*ptr) {
		if (*ptr == '"' || *ptr == ' ')
			*(outptr++) = '\\';

		if (!incmode && (*ptr == '(' || *ptr ==')'))
			*(outptr++) = '\\';

		if (!incmode && (*ptr == '<' || *ptr =='>'))
			*(outptr++) = '\\';

		*(outptr++) = *(ptr++);
	}
	*outptr = 0;
	return out;
}

static void set_abs_top(struct project *p, char *str)
{
	if (p->abs_top)
		free(p->abs_top);
	p->abs_top = str;
}

static void set_rel_top(struct project *p, char *str)
{
	if (p->rel_top)
		free(p->rel_top);
	p->rel_top = str;
}

static struct project *new_project(char *name, enum script_type stype, enum build_type btype)
{
	struct project *out = calloc(1, sizeof(struct project));
	out->name = name;
	out->stype = stype;
	out->btype = btype;
	return out;
}

static struct module *new_module(char *name, enum module_type mtype)
{
	struct module *out = calloc(1, sizeof(struct module));
	out->name = name;
	out->mtype = mtype;
	return out;
}

void add_tag(struct module *m, char *name)
{
	enum tags tag = TAG_NONE;

	if (strcmp("user", name) == 0)
		tag = TAG_USER;

	if (strcmp("eng", name) == 0)
		tag = TAG_ENG;

	if (strcmp("tests", name) == 0)
		tag = TAG_TESTS;

	if (strcmp("optional", name) == 0)
		tag = TAG_OPTIONAL;

        if (strcmp("debug", name) == 0)
                tag = TAG_DEBUG;

	m->tags |= tag;
	free(name);
}

static char *path_subst(struct project *p, char *in)
{
	char *ptr;
	if (include_space) {
		include_space = 0;
		ptr = malloc(strlen(in) + 2);
		ptr[0] = '-';
		ptr[1] = 'I';
		strcpy(ptr + 2, in);
		free(in);
	} else {
		ptr = in;
	}

	if (p->abs_top && p->rel_top &&
	    (strlen(ptr) > 2) && (ptr[0] == '-') && (ptr[1] == 'I')) {
		char *newstr = malloc(PATH_MAX);
		char *pstr = newstr;
		*pstr++ = '-';
		*pstr++ = 'I';
		if (realpath(in + 2, pstr)) {
		    char *atop = getenv("ANDROID_BUILD_TOP");
		    if (atop && strstr(pstr, atop) == pstr) {
			int len = strlen(atop);
			if (pstr[len] == '/')
			    len++;
			memmove(pstr, pstr + len, strlen(pstr + len) + 1);
		    }
		    free(ptr);
		    return newstr;
		}
		free(newstr);
	}
	return ptr;
}

static void add_cflag(struct project *p, struct module *m, char *flag)
{
	int i;
	if (strcmp("-I", flag) == 0) {
		free(flag);
		include_space = 1;
		return;
	}
	if (strcmp("-Werror", flag) == 0) {
		free(flag);
		return;
	}

	if (strcmp("-pthread", flag) == 0) {
		free(flag);
		return;
	}

	flag = path_subst(p, flag);

	for (i = 0; i < m->cflags; i++) {
		if (strcmp(flag, m->cflag[i].flag) == 0) {
		    free(flag);
		    return;
		}
	}

	m->cflags++;
	m->cflag = realloc(m->cflag, m->cflags * sizeof(struct flag));
	m->cflag[m->cflags - 1].flag = flag;
}

static void add_cppflag(struct project *p, struct module *m, char *flag)
{
	if (strcmp("-Werror", flag) == 0) {
		free(flag);
		return;
	}

	flag = path_subst(p, flag);

	m->cppflags++;
	m->cppflag = realloc(m->cppflag, m->cppflags * sizeof(struct flag));
	m->cppflag[m->cppflags - 1].flag = flag;
}

static int sources_filter(char *name)
{
	int len = strlen(name);
	if (len > 2) {
		if ((strcmp(".h", name + len - 2) == 0)
		 || (strcmp(".d", name + len - 2) == 0)) {
			return 1;
		}
	}
	if (len > 4) {
		if ((strcmp(".asn", name + len - 4) == 0)
		 || (strcmp(".map", name + len - 4) == 0)) {
			return 1;
		}
	}
	if (len > 5) {
		if (strcmp(".list", name + len - 5) == 0)
			return 1;
	}
	return 0;
}

static void add_source(struct module *m, char *name, struct generator *g)
{
	if (sources_filter(name)) {
		free(name);
		return;
	}
	m->sources++;
	m->source = realloc(m->source, m->sources * sizeof(struct source));
	m->source[m->sources - 1].name = name;
	m->source[m->sources - 1].gen = g;
}

static void add_header(struct module *m, char *name)
{
	m->headers++;
	m->header = realloc(m->header, m->headers * sizeof(struct header));
	m->header[m->headers - 1].name = name;
}

static void add_passthrough(struct module *m, char *name)
{
	m->passthroughs++;
	m->passthrough = realloc(m->passthrough, m->passthroughs * sizeof(struct passthrough));
	m->passthrough[m->passthroughs - 1].name = name;
}

static void add_libfilter(struct module *m, char *name, enum library_type ltype)
{
	m->libfilters++;
	m->libfilter = realloc(m->libfilter, m->libfilters * sizeof(struct library));
	m->libfilter[m->libfilters - 1].name = name;
	m->libfilter[m->libfilters - 1].ltype = ltype;
}

static void add_library(struct module *m, char *name, enum library_type ltype)
{
	m->libraries++;
	m->library = realloc(m->library, m->libraries * sizeof(struct library));
	m->library[m->libraries - 1].name = name;
	m->library[m->libraries - 1].ltype = ltype;
}

static int add_ldflag(struct module *m, char *flag, enum build_type btype)
{
	enum library_type ltype;
	int len = strlen(flag);

	if (len < 2)  {//this is probably a WTF condition...
		free(flag);
		return 0;
	}

	if (flag[0] == '-') {
		if (flag[1] == 'L') {
			free(flag);
			return 0;
		}
		if (flag[1] == 'R') {
			free(flag);
			return 0;
		}
		if ((strcmp(flag, "-pthread") == 0) ||
		    (strcmp(flag, "-lpthread") == 0)) {
			free(flag);
			return 0;
		}
		if ((strcmp(flag, "-dlopen") == 0)) {
			free(flag);
			return 1;
		}
		if (flag[1] == 'l') {// actually figure out what libtype...
			ltype = library_scope(flag + 2);
			add_library(m, strdup(flag+2), ltype);
			free(flag);
			return 0;
		}
		add_library(m, flag, LIBRARY_FLAG);
	} else {
		char *dot = rindex(flag, '.');

		if (dot && (strcmp(dot, ".lo") == 0)) {
			free(flag);
			return 0;
		}

		if (dot && (strcmp(dot, ".la") == 0)) {
			char *slash = rindex(flag, '/');
			char *temp, *lname;

			if (slash)
				lname = strstr(slash, "lib");
			else
				lname = strstr(flag, "lib");
			*dot = 0;
			if (lname) {
				temp = flag;
				flag = strdup(lname + 3);
				free(temp);
				add_library(m, flag, LIBRARY_EXTERNAL);
				return 0;
			}
			free(flag);
			return 0;
		}
		free(flag);
//		add_library(m, flag, LIBRARY_FLAG);
	}

	return 0;
}

static void add_module(struct project *p, struct module *m)
{
	p->modules++;
	p->module = realloc(p->module, p->modules * sizeof(struct module));
	p->module[p->modules - 1] = *m;
	free(m);
}

static void add_subdir(struct project *p, char *name)
{
	p->subdirs++;
	p->subdir = realloc(p->subdir, p->subdirs * sizeof(struct subdir));
	p->subdir[p->subdirs - 1].name = name;
}

static enum mode get_mode(char *arg)
{
	int i;
	int outval = MODE_UNDEFINED;
	int len = strlen(arg);

	if (len < 3)
		return MODE_UNDEFINED;
	if ((arg[0] != '-') || (arg[1] != ':'))
		return MODE_UNDEFINED;

	for  (i = 0; opstrings[i].str != NULL; i++)
		if (strcmp(opstrings[i].str, arg) == 0)
			outval = opstrings[i].mode;

	return outval;
}

struct project *options_parse(int argc, char **args)
{
	enum mode mode = MODE_UNDEFINED;
	char *arg;
	int i, skip = 0;
	enum build_type bt;
	enum module_type mt;
	struct project *p = NULL;
	struct module *m = NULL;

	if (getenv("ANDROGENIZER_NDK"))
		bt = BUILD_NDK;
	else bt = BUILD_EXTERNAL;

	if (argc < 2) {
//print help!
		return NULL;
	}
	for (i = 1; i < argc; i++) {
		enum mode nm;
		nm = get_mode(args[i]);
		if (mode != MODE_PASSTHROUGH)
			arg = add_slashes(args[i]);
		else
			arg = strdup(args[i]);

		if (nm != MODE_UNDEFINED)
			free(arg);

		if (nm == MODE_UNDEFINED) {
			switch (mode) {
			case MODE_UNDEFINED:
				assert(!!!"OH NOES!!!");
				break;
			case MODE_PROJECT:
				p = new_project(arg, SCRIPT_SUBDIRECTORY, bt);
				break;
			case MODE_SUBDIR:
				if (!p)
					die("-:PROJECT must come before -:SUBDIR");
				add_subdir(p, arg);
				break;
			case MODE_SHARED:
			case MODE_STATIC:
			case MODE_EXECUTABLE:
				if (!p)
					die("-:PROJECT must come before a module type");
				if (m)
					add_module(p, m);
				if (mode == MODE_SHARED)
					mt = MODULE_SHARED_LIBRARY;
				if (mode == MODE_STATIC)
					mt = MODULE_STATIC_LIBRARY;
				if (mode == MODE_EXECUTABLE)
					mt = MODULE_EXECUTABLE;
				m = new_module(arg, mt);
				break;
			case MODE_SOURCES:
				if (!m)
					die("a module type must be declared before adding -:SOURCES");
				add_source(m, arg, NULL);
				break;
			case MODE_LDFLAGS:
				if (!m)
					die("a module type must be declared before adding -:LDFLAGS");
				if (!skip) {
					skip = add_ldflag(m, arg, p->btype);
				} else {
					/* We were asked to skip this argument in the previous step */
					skip = 0;
				}
				break;
			case MODE_CFLAGS:
				if (!p || !m)
					die("a module type must be declared before adding -:CFLAGS");
				add_cflag(p, m, arg);
				break;
			case MODE_CPPFLAGS:
				if (!p || !m)
					die("a module type must be declared before adding -:CPPFLAGS");
				add_cppflag(p, m, arg);
				break;
			case MODE_TAGS:
				if (!m)
					die("a module type must be declared before setting -:TAGS");
				add_tag(m, arg);
				break;
			case MODE_HEADER_TARGET:
				if (!m)
					die("a module type must be declared before setting a -:HEADER_TARGET");
				if (m->header_target)
					free(m->header_target);
				m->header_target = arg;
				break;
			case MODE_HEADERS:
				if (!m)
					die("a module type must be declared before adding -:HEADERS");
				add_header(m, arg);
				break;
			case MODE_PASSTHROUGH:
				if (!m)
					die("a module type must be declared before a -:PASSTHROUGH");
				add_passthrough(m, arg);
				break;
			case MODE_REL_TOP:
				if (!p)
					die("a -:PROJECT must be declared before -:REL_TOP");
				set_rel_top(p, arg);
				break;
			case MODE_ABS_TOP:
				if (!p)
					die("a -:PROJECT must be declared before -:ABS_TOP");
				set_abs_top(p, arg);
				break;
			case MODE_LIBFILTER_STATIC:
				if (!m)
					die("a module type must be declared before adding libfilters");
				add_libfilter(m, arg, LIBRARY_STATIC);
				break;
			case MODE_LIBFILTER_WHOLE:
				if (!m)
					die("a module type must be declared before adding libfilters");
				add_libfilter(m, arg, LIBRARY_WHOLE_STATIC);
				break;
			case MODE_END:
				break;
			}
		} else mode = nm;
	}
	if (p && m)
		add_module(p, m);
	return p;
}
