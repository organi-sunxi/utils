
#ifdef __cplusplus
extern "C" {
#endif

#ifdef INITROOT_STARTUP

typedef int(*startup_child_func)(void);
int startup_begin(void);
int startup_end(startup_child_func pf);

#else
#define startup_begin(c,v)
#define startup_end(pf)	(*pf)()

#endif

#ifdef __cplusplus
}
#endif

