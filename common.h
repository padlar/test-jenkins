#ifndef __COMMON_H__
#define __COMMON_H__

enum module_type {
	MODULE_SHARED_LIBRARY,
	MODULE_STATIC_LIBRARY,
	MODULE_EXECUTABLE,
};

enum library_type {
	LIBRARY_NDK,
	LIBRARY_UNSUPPORTED,
	LIBRARY_EXTERNAL,
	LIBRARY_STATIC,
	LIBRARY_WHOLE_STATIC,
	LIBRARY_FLAG
};

enum script_type {
	SCRIPT_SUBDIRECTORY,
	SCRIPT_TOP,
};

enum build_type {
	BUILD_NDK,
	BUILD_EXTERNAL
};

enum tags {
	TAG_NONE = 0,
	TAG_USER = 1,
	TAG_ENG = 2,
	TAG_TESTS = 4,
	TAG_OPTIONAL = 8,
        TAG_DEBUG = 16
};

struct generator {

};

struct passthrough {
	char *name;
};

struct source {
	char *name;
	struct generator *gen;
};

struct flag {
	char *flag;
};

struct library {
	char *name;
	enum library_type ltype;
};

struct subdir {
	char *name;
};

struct header {
	char *name;
};

struct module {
	char *name; //local_module;
	enum module_type mtype;
	char *header_target;
	struct header *header;
	int headers;
	struct source *source;
	int sources;
	struct flag *cflag;
	int cflags;
	struct flag *cppflag;
	int cppflags;
	struct library *library;
	int libraries;
	struct library *libfilter;
	int libfilters;
	struct passthrough *passthrough;
	int passthroughs;
	int tags;
};

struct project {
	char *name;
	struct module *module;
	int modules;
	struct subdir *subdir;
	int subdirs;
	enum build_type btype;
	enum script_type stype;
	char *abs_top;
	char *rel_top;
};

#endif /*__COMMON_H__*/
