#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "common.h"

void emit_libraries(struct library *l, int count, enum build_type bt,
                    struct library *filt, int fcount)
{
	int i, j, first;
/*libfilter pass.  this is the king of the kludges, but I haven't
  thought of a reasonable solution... yet?
*/

	for (i = 0; i < count; i++)
		for (j = 0; j < fcount; j++)
			if (strcmp(l[i].name, filt[j].name) == 0)
				l[i].ltype = filt[j].ltype;

	if (bt == BUILD_NDK) {
		first = 1;
		for (i = 0; i < count; i++) {
			if ((l[i].ltype == LIBRARY_NDK) ||
			    (l[i].ltype == LIBRARY_UNSUPPORTED)) {
				if (first) {
					first = 0;
					printf("LOCAL_LDLIBS:=\\\n");
				} else printf(" \\\n");
				printf("\t-l%s", l[i].name);
			}
		}
		if (!first)
			printf("\n");
		first = 1;
		for (i = 0; i < count; i++) {
			if (l[i].ltype == LIBRARY_EXTERNAL) {
				if (first) {
					first = 0;
					printf("LOCAL_SHARED_LIBRARIES:=\\\n");
				} else printf(" \\\n");
				printf("\tlib%s", l[i].name);
			}
		}
		if (!first)
			printf("\n\n");
		first = 1;
		for (i = 0; i < count; i++) {
			if (l[i].ltype == LIBRARY_STATIC) {
				if (first) {
					first = 0;
					printf("LOCAL_STATIC_LIBRARIES:=\\\n");
				} else printf(" \\\n");
				printf("\tlib%s", l[i].name);
			}
		}
		if (!first)
			printf("\n\n");

		first = 1;
		for (i = 0; i < count; i++) {
			if (l[i].ltype == LIBRARY_WHOLE_STATIC) {
				if (first) {
					first = 0;
					printf("LOCAL_WHOLE_STATIC_LIBRARIES:=\\\n");
				} else printf(" \\\n");
				printf("\tlib%s", l[i].name);
			}
		}
		if (!first) {
			printf("\n\n");
		}
	} else { //bt == BUILD_EXTERNAL
		first = 1;
		for (i = 0; i < count; i++) {
			if ((l[i].ltype != LIBRARY_FLAG) &&
			    (l[i].ltype != LIBRARY_STATIC) &&
                            (l[i].ltype != LIBRARY_WHOLE_STATIC)) {
				if (first) {
					first = 0;
					printf("LOCAL_SHARED_LIBRARIES:=\\\n");
				} else printf(" \\\n");
				printf("\tlib%s", l[i].name);
			}
		}
		if (!first)
			printf("\n\n");

		first = 1;
		for (i = 0; i < count; i++) {
			if ((l[i].ltype == LIBRARY_STATIC)) {
				if (first) {
					first = 0;
					printf("LOCAL_STATIC_LIBRARIES:=\\\n");
				} else printf(" \\\n");
				printf("\tlib%s", l[i].name);
			}
		}
		if (!first)
			printf("\n\n");

		first = 1;
		for (i = 0; i < count; i++) {
			if ((l[i].ltype == LIBRARY_WHOLE_STATIC)) {
				if (first) {
					first = 0;
					printf("LOCAL_WHOLE_STATIC_LIBRARIES:=\\\n");
				} else printf(" \\\n");
				printf("\tlib%s", l[i].name);
			}
		}
		if (!first)
			printf("\n\n");
	}

	first = 1;
	for (i = 0; i < count; i++) {
		if (l[i].ltype == LIBRARY_FLAG) {
			if (first) {
				first = 0;
				printf("LOCAL_LDFLAGS:=\\\n");
			} else printf("\\\n");
			printf("\t%s", l[i].name);
		}
	}
	if (!first)
		printf("\n\n");
}

int emit_file(struct project *p)
{
	int i, j;

	printf("LOCAL_PATH:=$(call my-dir)\n");

	if (p->stype == SCRIPT_TOP)
		printf("%s_TOP := $(LOCAL_PATH)\n", p->name);

	for (i = 0; i < p->modules; i++) {
		struct module *m = &p->module[i];
		printf("include $(CLEAR_VARS)\n\n");

		if (m->passthrough) {
			for (j = 0; j < m->passthroughs; j++)
				printf("%s\n", m->passthrough[j].name);
			printf("\n");
		}
		printf("LOCAL_MODULE:=%s\n\n", m->name);
//no tags == no build for the external dir...
		if (m->tags) {
			printf("LOCAL_MODULE_TAGS:=");
			if (m->tags & TAG_USER)
				printf("user ");
			if (m->tags & TAG_ENG)
				printf("eng ");
			if (m->tags & TAG_TESTS)
				printf("tests ");
			if (m->tags & TAG_OPTIONAL)
				printf("optional ");
			if (m->tags & TAG_DEBUG)
				printf("debug ");
			printf("\n\n");
		}

		if (m->sources) {
//should do two passes?  one for LOCAL_SRC_FILES, one for generated
			printf("LOCAL_SRC_FILES := \\\n");
			for (j = 0; j < m->sources - 1; j++)
				printf("\t%s \\\n", m->source[j].name);
			printf("\t%s\n\n", m->source[j].name);
		}

		emit_libraries(m->library,
		               m->libraries,
		               p->btype,
		               m->libfilter,
		               m->libfilters);

		if (m->cflags) {
			printf("LOCAL_CFLAGS := \\\n");
			for (j = 0; j < m->cflags - 1; j++)
				printf("\t%s \\\n",  m->cflag[j].flag);
			printf("\t%s\n\n", m->cflag[j].flag);
		}

/* OOPS!  Android actually ignores LOCAL_CPPFLAGS */
		if (m->cppflags) {
			printf("LOCAL_CFLAGS += \\\n");
			for (j = 0; j < m->cppflags - 1; j++)
				printf("\t%s \\\n",  m->cppflag[j].flag);
			printf("\t%s\n\n", m->cppflag[j].flag);
		}

		printf("LOCAL_PRELINK_MODULE := false\n");

		if (m->header_target) {
			printf("LOCAL_COPY_HEADERS_TO := %s\n", m->header_target);
		}

		if (m->headers) {
			printf("LOCAL_COPY_HEADERS := \\\n");
			for (j = 0; j < m->headers - 1; j++)
				printf("\t%s \\\n",  m->header[j].name);
			printf("\t%s\n\n", m->header[j].name);
		}

		switch (p->module[i].mtype) {
		case MODULE_SHARED_LIBRARY:
			printf("include $(BUILD_SHARED_LIBRARY)\n");
			break;
		case MODULE_STATIC_LIBRARY:
			printf("include $(BUILD_STATIC_LIBRARY)\n");
			break;
		case MODULE_EXECUTABLE:
			printf("include $(BUILD_EXECUTABLE)\n");
			break;
		default:
			assert(!!!"OH NOES!!!");
		}
	}

	for (i = 0; i < p->subdirs; i++)
		printf("-include $(%s_TOP)/%s/Android.mk\n", p->name, p->subdir[i].name);

	return 0;
}
