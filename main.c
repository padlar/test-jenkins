#include <stdio.h>
#include <stdlib.h>
#include "emit.h"
#include "options.h"
#include "common.h"


static void cleanup_flag(struct flag *c)
{
	free(c->flag);
}

static void cleanup_header(struct header *h)
{
	free(h->name);
}

static void cleanup_sources(struct source *s)
{
	free(s->name);
}

static void cleanup_library(struct library *l)
{
	free(l->name);
}

static void cleanup_passthrough(struct passthrough *p)
{
	free(p->name);
}

static void cleanup_module(struct module *m)
{
	int i;

	free(m->name);

	if (m->header_target)
		free(m->header_target);

	if (m->headers) {
		for (i = 0; i < m->headers; i++)
			cleanup_header(&m->header[i]);
		free(m->header);
	}

	if (m->sources) {
		for (i = 0; i < m->sources; i++)
			cleanup_sources(&m->source[i]);
		free(m->source);
	}

	if (m->cflags) {
		for (i = 0; i < m->cflags; i++)
			cleanup_flag(&m->cflag[i]);
		free(m->cflag);
	}

	if (m->cppflags) {
		for (i = 0; i < m->cppflags; i++)
			cleanup_flag(&m->cppflag[i]);
		free(m->cppflag);
	}

	if (m->libraries) {
		for (i = 0; i < m->libraries; i++)
			cleanup_library(&m->library[i]);
		free(m->library);
	}

	if (m->passthroughs) {
		for (i = 0; i < m->passthroughs; i++)
			cleanup_passthrough(&m->passthrough[i]);
		free(m->passthrough);
	}
}

static void cleanup_subdir(struct subdir *s)
{
	free(s->name);
}

static void cleanup(struct project *p)
{
	int i;
	if (p->modules) {
		for (i = 0; i < p->modules; i++)
			cleanup_module(&p->module[i]);
		free(p->module);
	}
	if (p->subdirs) {
		for (i = 0; i < p->subdirs; i++)
			cleanup_subdir(&p->subdir[i]);
		free(p->subdir);
	}
	free(p->name);
	free(p);
}

int main(int argc, char **argv)
{
	struct project *p;
	int err;

	p = options_parse(argc, argv);
	if (p) {
		err = emit_file(p);
		cleanup(p);
	}
	return 0;
}
