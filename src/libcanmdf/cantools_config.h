#ifndef _MDF_CONFIG_H
#define _MDF_CONFIG_H

#ifndef CALLOC
#define CALLOC calloc
#endif

#ifndef REALLOC
#define REALLOC realloc
#endif

#ifndef FREE
#define FREE free
#endif

#define PACKED


#define mdf_calloc(n,s)          CALLOC((n),(s))
#define mdf_alloc(s)             CALLOC(1,(s))
#define mdf_malloc(s)            CALLOC(1,(s))
#define mdf_realloc(ptr,s)       REALLOC((ptr),(s))
#define mdf_free                 FREEP

#if defined(MATLAB_MEX_FILE)
  #define mdf_printf(fmt,...)      mexPrintf((fmt),__VA_ARGS__)
  #define mdf_fprintf(dev,fmt,...) mexPrintf((fmt),__VA_ARGS__)
#else
  #define mdf_printf(fmt,...)      printf((fmt),__VA_ARGS__)
  #define mdf_fprintf(dev,fmt,...) fprintf((dev),(fmt),__VA_ARGS__)
#endif

#ifndef LINE_MAX
#define LINE_MAX 80
#endif

char* mdf_strdup   (const char* source);
char* mdf_strndup  (const char* source, int maxlen);
int   mdf_fnmatch  (const char *pattern, const char *string);




#endif /* _MDF_CONFIG_H */
