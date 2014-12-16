/*
	module.c: modular code loader

	copyright 1995-2011 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Nicholas J Humfrey
*/

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <ltdl.h>

#include "mpg123app.h"
#include "debug.h"

#ifndef HAVE_LTDL
#error Cannot build without LTDL library support
#endif

#define MODULE_SYMBOL_PREFIX 	"mpg123_"
#define MODULE_SYMBOL_SUFFIX 	"_module_info"

/* It's nasty to hardcode that here...
   also it does need hacking around libtool's hardcoded .la paths:
   When the .la file is in the same dir as .so file, you need libdir='.' in there. */
static const char* modulesearch[] =
{
	 "../lib/mpg123"
	,"plugins"
};

static char *get_the_cwd(); /* further down... */
static char *get_module_dir()
{
	/* Either PKGLIBDIR is accessible right away or we search for some possible plugin dirs relative to binary path. */
	DIR* dir = NULL;
	char *moddir = NULL;
	const char *defaultdir;
	/* Compiled-in default module dir or environment variable MPG123_MODDIR. */
	defaultdir = getenv("MPG123_MODDIR");
	if(defaultdir == NULL)
	defaultdir=PKGLIBDIR;

	dir = opendir(defaultdir);
	if(dir != NULL)
	{
		size_t l = strlen(defaultdir);

		if(param.verbose > 1) fprintf(stderr, "Using default module dir: %s\n", defaultdir);
		moddir = malloc(l+1);
		if(moddir != NULL)
		{
			strcpy(moddir, defaultdir);
			moddir[l] = 0;
		}
		closedir(dir);
	}
	else /* Search relative to binary. */
	{
		size_t i;
		for(i=0; i<sizeof(modulesearch)/sizeof(char*); ++i)
		{
			const char *testpath = modulesearch[i];
			size_t l;
			if(binpath != NULL) l = strlen(binpath) + strlen(testpath) + 1;
			else l = strlen(testpath);

			moddir = malloc(l+1);
			if(moddir != NULL)
			{
				if(binpath==NULL) /* a copy of testpath, when there is no prefix */
				snprintf(moddir, l+1, "%s", testpath);
				else
				snprintf(moddir, l+1, "%s/%s", binpath, testpath);

				moddir[l] = 0;
				if(param.verbose > 1) fprintf(stderr, "Looking for module dir: %s\n", moddir);

				dir = opendir(moddir);
				if(dir != NULL)
				{
					closedir(dir);
					break; /* found it! */
				}
				else{ free(moddir); moddir=NULL; }
			}
		}
	}
	if(param.verbose > 1) fprintf(stderr, "Module dir: %s\n", moddir != NULL ? moddir : "<nil>");
	return moddir;
}

/* Open a module in current directory. */
mpg123_module_t* open_module_here(const char* type, const char* name)
{
	lt_dlhandle handle = NULL;
	mpg123_module_t *module = NULL;
	char* module_path = NULL;
	size_t module_path_len = 0;
	char* module_symbol = NULL;
	size_t module_symbol_len = 0;

	/* Initialize libltdl */
	if(lt_dlinit())
	{
		error("Failed to initialise libltdl");
		return NULL;
	}

	/* Work out the path of the module to open */
	/* Note that we need to open ./file, not just file! */
	module_path_len = 2 + strlen(type) + 1 + strlen(name) + strlen(MODULE_FILE_SUFFIX) + 1;
	module_path = malloc( module_path_len );
	if (module_path == NULL) {
		error1( "Failed to allocate memory for module name: %s", strerror(errno) );
		return NULL;
	}
	snprintf( module_path, module_path_len, "./%s_%s%s", type, name, MODULE_FILE_SUFFIX );
	/* Display the path of the module created */
	if(param.verbose > 1) fprintf(stderr, "Module path: %s\n", module_path );

	/* Open the module */
	handle = lt_dlopen( module_path );
	free( module_path );
	if (handle==NULL) {
		error2( "Failed to open module %s: %s", name, lt_dlerror() );
		if(param.verbose > 1)
		fprintf(stderr, "Note: This could be because of braindead path in the .la file...\n");

		return NULL;
	}
	
	/* Work out the symbol name */
	module_symbol_len = strlen( MODULE_SYMBOL_PREFIX ) +
						strlen( type )  +
						strlen( MODULE_SYMBOL_SUFFIX ) + 1;
	module_symbol = malloc(module_symbol_len);
	if (module_symbol == NULL) {
		error1( "Failed to allocate memory for module symbol: %s", strerror(errno) );
		return NULL;
	}
	snprintf( module_symbol, module_symbol_len, "%s%s%s", MODULE_SYMBOL_PREFIX, type, MODULE_SYMBOL_SUFFIX );
	debug1( "Module symbol: %s", module_symbol );
	
	/* Get the information structure from the module */
	module = (mpg123_module_t*)lt_dlsym(handle, module_symbol );
	free( module_symbol );
	if (module==NULL) {
		error1( "Failed to get module symbol: %s", lt_dlerror() );
		return NULL;
	}
	
	/* Check the API version */
	if (MPG123_MODULE_API_VERSION != module->api_version)
	{
		error2( "API version of module does not match (got %i, expected %i).", module->api_version, MPG123_MODULE_API_VERSION);
		lt_dlclose(handle);
		return NULL;
	}

	/* Store handle in the data structure */
	module->handle = handle;
	return module;
}


/* Open a module, including directory search. */
mpg123_module_t* open_module(const char* type, const char* name)
{
	mpg123_module_t *module = NULL;
	char *workdir = NULL;
	char *moddir  = NULL;

	workdir = get_the_cwd();
	moddir  = get_module_dir();
	if(workdir == NULL || moddir == NULL)
	{
		error("Failure getting workdir or moddir! (Perhaps set MPG123_MODDIR?)");
		if(workdir == NULL) fprintf(stderr, "Hint: I need to know the current working directory to be able to come back after hunting modules. I will not leave because I do not know where I am.\n");

		if(workdir != NULL) free(workdir);
		if(moddir  != NULL) free(moddir);
		return NULL;
	}

	if(chdir(moddir) == 0) module = open_module_here(type, name);
	else error2("Failed to enter module directory %s: %s", moddir, strerror(errno));

	chdir(workdir);
	free(moddir);
	free(workdir);
	return module;
}

void close_module( mpg123_module_t* module )
{
	lt_dlhandle handle = module->handle;
	int err = lt_dlclose( handle );
	
	if (err) error1("Failed to close module: %s", lt_dlerror() );

}

#define PATH_STEP 50
static char *get_the_cwd()
{
	size_t bs = PATH_STEP;
	char *buf = malloc(bs);
	errno = 0;
	while((buf != NULL) && getcwd(buf, bs) == NULL)
	{
		char *buf2;
		if(errno != ERANGE)
		{
			error1("getcwd returned unexpected error: %s", strerror(errno));
			free(buf);
			return NULL;
		}
		buf2 = realloc(buf, bs+=PATH_STEP);
		if(buf2 == NULL){ free(buf); buf = NULL; }
		else debug1("pwd: increased buffer to %lu", (unsigned long)bs);

		buf = buf2;
	}
	return buf;
}

void list_modules()
{
	DIR* dir = NULL;
	struct dirent *dp = NULL;
	char *moddir  = NULL;

	moddir = get_module_dir();
	if(moddir == NULL)
	{
		error("Failure getting module directory! (Perhaps set MPG123_MODDIR?)");
		exit(-1); /* TODO: change this to return a value instead of exit()! */
	}
	/* Open the module directory */
	dir = opendir(moddir);
	if (dir==NULL) {
		error2("Failed to open the module directory (%s): %s\n", PKGLIBDIR, strerror(errno));
		free(moddir);
		exit(-1);
	}
	
	if(chdir(moddir) != 0)
	{
		error2("Failed to enter module directory (%s): %s\n", PKGLIBDIR, strerror(errno));
		closedir( dir );
		free(moddir);
		exit(-1);
	}
	/* Display the program title */
	/* print_title(stderr); */

	/* List the output modules */
	printf("\n");
	printf("Available modules\n");
	printf("-----------------\n");
	
	while( (dp = readdir(dir)) != NULL ) {
		struct stat fst;
		if(stat(dp->d_name, &fst) != 0) continue;
		if(S_ISREG(fst.st_mode)) /* Allow links? */
		{
			char* ext = dp->d_name + strlen( dp->d_name ) - strlen( MODULE_FILE_SUFFIX );
			if (strcmp(ext, MODULE_FILE_SUFFIX) == 0)
			{
				char *module_name = NULL;
				char *module_type = NULL;
				char *uscore_pos = NULL;
				mpg123_module_t *module = NULL;
				
				/* Extract the module type */
				module_type = strdup( dp->d_name );
				uscore_pos = strchr( module_type, '_' );
				if (uscore_pos==NULL || (uscore_pos>=module_type+strlen(module_type)+1) )
				{
					free(module_type);
					continue;
				}
				
				*uscore_pos = '\0';
				
				/* Extract the short name of the module */
				module_name = strdup( dp->d_name + strlen( module_type ) + 1 );
				module_name[ strlen( module_name ) - strlen( MODULE_FILE_SUFFIX ) ] = '\0';
				/* Open the module */
				module = open_module_here(module_type, module_name);
				if (module) {
					printf("%-15s%s  %s\n", module->name, module_type, module->description );
				
					/* Close the module again */
					close_module( module );
				}
				free( module_name );
				free( module_type );
			}
		}
	}

	closedir( dir );
	free(moddir);
	exit(0);
}


