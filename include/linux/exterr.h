/*
 * Extended syscall error reporting
 */
#ifndef _LINUX_EXTERR_H
#define _LINUX_EXTERR_H

#include <linux/compiler.h>

/*
 * Extended error reporting: annotate an error code with a string
 * and a module name to help users diagnase problems with their
 * attributes and other syscall parameters.
 */

/*
 * This is the basic error descriptor structure that is statically
 * allocated for every annotated error (error site).
 *
 * Subsystems that wish to extend this structure should embed it
 * and provide a callback for formatting the additional fields.
 */
struct ext_err_site {
	const char		*message;
	const char		*owner;
	const char		*file;
	const int		line;
	const int		code;
};

/*
 * Error domain descriptor (compile/link time)
 */
struct ext_err_domain_desc {
	const char	*name;
	const size_t	err_site_size;
	const void	*start, *end;
	char		*(*format)(void *site);
	int		first;
	int		last;
};

extern struct ext_err_domain_desc __attribute__((weak)) __start___ext_err_domain_desc[];
extern struct ext_err_domain_desc __attribute__((weak)) __stop___ext_err_domain_desc[];

#define DECLARE_EXTERR_DOMAIN(__name, __format)				\
	extern const struct __name ## _ext_err_site __attribute__((weak)) __start_ ## __name ## _ext_err[]; \
	extern const struct __name ## _ext_err_site __attribute__((weak)) __stop_ ## __name ## _ext_err[]; \
	const struct ext_err_domain_desc __used				\
	__attribute__ ((__section__("__ext_err_domain_desc")))		\
	__name ## _ext_err_domain_desc = {				\
		.name		= __stringify(__name),			\
		.err_site_size	= sizeof(struct __name ## _ext_err_site), \
		.start		= __start_ ## __name ## _ext_err,	\
		.end		= __stop_ ## __name ## _ext_err,	\
		.format		= __format,				\
	};								\

#define __ext_err(__domain, __c, __m, __domain__fields ...) ({		\
	static struct __domain ## _ext_err_site				\
	__attribute__ ((__section__(__stringify(__domain) "_ext_err"))) \
		__err_site = {						\
		.site = {						\
			.message	= (__m),			\
			.owner		= EXTERR_MODNAME,		\
			.file		= __FILE__,			\
			.line		= __LINE__,			\
			.code		= __builtin_constant_p((__c)) ?	\
			(__c) : -EINVAL,				\
		},							\
		## __domain__fields,					\
	};								\
	&__err_site;							\
})

#define ext_err(__domain, __c, __m, __domain__fields ...)		\
	({								\
		extern const struct __domain ## _ext_err_site __start_ ## __domain ## _ext_err[]; \
		extern struct ext_err_domain_desc __domain ## _ext_err_domain_desc; \
		-(__domain ## _ext_err_domain_desc.first +		\
		  (__ext_err(__domain, __c, __m, __domain__fields) -	\
		   __start_ ## __domain ## _ext_err));			\
	})

int ext_err_copy_to_user(void __user *buf, size_t size);

int ext_err_errno(int code);

/*
 * Use part of the [-1, -MAX_ERRNO] errno range for the extended error
 * reporting. Anything within [-EXT_ERRNO, -MAX_ERRNO] is an index of a
 * ext_err_site structure within __ext_err section. 3k should be enough
 * for everybody, but let's add a boot-time warning just in case it overflows
 * one day.
 */
#define EXT_ERRNO 1024

#define EXT_ERR_PTR(__e, __m)	(ERR_PTR(ext_err(__e, __m)))

#endif /* _LINUX_EXTERR_H */
