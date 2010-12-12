#ifndef LOADER_H
#define LOADER_H

#define rtld_local __attribute__ ((visibility("hidden")))
#define EXPORT_VAR(name, export) \
	extern typeof(name) export \
	__attribute__ ((alias (#name), visibility ("hidden")))

#define DEFINE_GLO_VAR(type, name)   \
	extern typeof(type) _##name; \
	EXPORT_VAR(_##name, name);   \
	type _##name

#define DECLARE_GLO_VAR(type, name) \
	extern typeof(type) name    \
	__attribute__ ((visibility("hidden")))

/* USAGE:
 *   DEFINE_GLO_VAR(int, global) = 0;
 */
#endif
