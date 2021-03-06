#include <string.h>
#include <sys/stat.h>

#ifndef _MSC_VER
#include <unistd.h>
#include <dirent.h>
#else
#include <io.h>
#endif

#include "../simdebug.h"
#include "../simmem.h"
#include "searchfolder.h"

#ifdef _MSC_VER
#define STRICMP stricmp
#else
#define STRICMP strcasecmp
#endif

 /*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Endet filepath mit einem slash so werden alle Files im diesem
 *	Verzeichnis mit der angegebene Extension gesucht.
 *      Endet filepath nicht mit einem slash und enth�lt keinen Punkt
 *	nach dem letzten Slash. So wird er um die Extension erweitert
 *	und gesucht.
 *	Ansonsten wird direkt nach filepath gesucht.
 *
 *	Keine Wildcards, bitte!
 *
 *  Return type:
 *      int	    Anzahl gefundener Dateien.
 */
int searchfolder_t::search(const char *filepath, const char *extension)
{
    cstring_t path(filepath);
    cstring_t name;
    cstring_t lookfor;
    cstring_t ext;

	for (vector_tpl<char*>::const_iterator i = files.begin(), end = files.end(); i != end; ++i) {
		guarded_free(*i);
	}
	files.clear();

	if(path.right(1) == "/") {
		// Look for a directory
		name = "*";
		ext = cstring_t(".") + extension;
	}
	else {
		int slash = path.find_back('/');
		int dot = path.find_back('.');

		if(dot == -1 || dot < slash) {
			// Look for a file with default extension
			name = path.mid(slash + 1);
			path = path.left(slash + 1);
			ext = cstring_t(".") + extension;
		}
		else {
			// Look for a file with own extension
			ext = path.mid(dot);
			name = path.mid(slash + 1, dot - slash - 1);
			path = path.left(slash + 1);
		}
	}
#ifdef _MSC_VER
	lookfor = path + name + ext;
	struct _finddata_t entry;
	intptr_t hfind = _findfirst(lookfor, &entry);

	if(hfind != -1) {
		lookfor = ext;
		do {
			size_t entry_len = strlen(entry.name);
			if(stricmp(entry.name + entry_len - lookfor.len(), lookfor) == 0) {
				char* const c = MALLOCN(char, path.len() + entry_len + 1);
				sprintf(c,"%s%s",(const char*)path,entry.name);
				files.append(c);
			}
		} while(_findnext(hfind, &entry) == 0 );
	}
#else
	lookfor = path + ".";

	DIR* dir = opendir(lookfor);
	struct  dirent  *entry;

	if(dir != NULL) {
		lookfor = (name == "*") ? ext : name + ext;

		while((entry = readdir(dir)) != NULL) {
			if(entry->d_name[0]!='.' || (entry->d_name[1]!='.' && entry->d_name[1]!=0)) {
				int entry_len = strlen(entry->d_name);
				if (strcasecmp(entry->d_name + entry_len - lookfor.len(), lookfor) == 0) {
					char* const c = MALLOCN(char, path.len() + entry_len + 1);
					sprintf(c,"%s%s", (const char*)path, entry->d_name);
					files.append(c);
				}
			}
		}
		closedir(dir);
	}
#endif
	return files.get_count();
}

cstring_t searchfolder_t::complete(const char *filepath, const char *extension)
{
	cstring_t path = filepath;

	if(path.right(1) != "/") {
		int slash = path.find_back('/');
		int dot = path.find_back('.');

		if(dot == -1 || dot < slash) {
			return path + "." + extension;
		}
	}
	return path;
}


/*
 * since we explicitly alloc the char *'s we must free them here
 */
searchfolder_t::~searchfolder_t()
{
	for (vector_tpl<char*>::const_iterator i = files.begin(), end = files.end(); i != end; ++i) {
		guarded_free(*i);
	}
}
