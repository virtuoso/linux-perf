/*
 * Extended error reporting
 */
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/exterr.h>
#include <linux/uaccess.h>

static struct ext_err_domain_desc ** domains;
static unsigned int nr_domains;

int exterr_last = EXT_ERRNO;

static struct ext_err_domain_desc *ext_err_domain_find(int code)
{
	unsigned int i = nr_domains / 2;

	if (code < EXT_ERRNO || code > domains[nr_domains - 1]->last)
		return NULL;

	do {
		if (code > domains[i]->last)
			i -= i / 2;
		else if (code < domains[i]->first)
			i += i / 2;
		else
			return domains[i];
	} while (i < nr_domains);

	return NULL;
}

static struct ext_err_site *ext_err_site_find(int code)
{
	struct ext_err_domain_desc *dom = ext_err_domain_find(code);
	struct ext_err_site *site;
	unsigned long off;

	if (!code)
		return NULL;

	/* shouldn't happen */
	if (WARN_ON_ONCE(!dom))
		return NULL;

	code -= dom->first;
	off = (unsigned long)dom->start + dom->err_site_size * code;
	site = (struct ext_err_site *)off;
	
	return site;
}

int ext_err_copy_to_user(void __user *buf, size_t size)
{
	struct ext_err_domain_desc *dom = ext_err_domain_find(current->ext_err_code);
	struct ext_err_site *site = ext_err_site_find(current->ext_err_code);
	char *output, *dom_fmt = NULL;
	unsigned long len;
	int ret;

	if (!site)
		return 0;

	if (dom->format)
		dom_fmt = dom->format(site);

        output = kasprintf(GFP_KERNEL,
                           "{\n"
                           "\t\"file\": \"%s\",\n"
                           "\t\"line\": %d,\n"
                           "\t\"code\": %d,\n"
                           "\t\"module\": \"%s\",\n"
                           "\t\"message\": \"%s\"\n%s"
                           "}\n",
                           site->file, site->line,
                           site->code, site->owner, site->message,
			   dom_fmt ? dom_fmt : ""
                           );

	kfree(dom_fmt);

        if (!output)
                return -ENOMEM;

        /* trim the buffer to the supplied boundary */
        len = strlen(output);
        if (len >= size) {
                len = size - 1;
                output[len] = 0;
        }

        ret = copy_to_user(buf, output, len + 1);

        kfree(output);

	if (!ret)
		current->ext_err_code = 0;

	return ret;
}

int ext_err_errno(int code)
{
	struct ext_err_site *site;

	if (code > -EXT_ERRNO)
		return code;

	site = ext_err_site_find(-code);
	if (!site)
		return code;

	current->ext_err_code = -code;

	return site->code;
}

static int ext_err_init(void)
{
	struct ext_err_domain_desc *dom;
	size_t size;
	int i;

	if (!__start___ext_err_domain_desc)
		return 0;

	nr_domains = __stop___ext_err_domain_desc - __start___ext_err_domain_desc;
	domains = kcalloc(nr_domains, sizeof(void *), GFP_KERNEL);
	if (!domains)
		return -ENOMEM;

	/* allocate error code to the domains */
	printk("Extended error reporting domains:\n");
	for (dom = __start___ext_err_domain_desc, i = 0;
	     dom < __stop___ext_err_domain_desc;
	     dom++, i++) {
		size = dom->end - dom->start;
		size /= dom->err_site_size;

		if (WARN_ON_ONCE(exterr_last + size >= MAX_ERRNO))
			break;

		domains[i]         = dom;
		domains[i]->first  = exterr_last;
		domains[i]->last   = exterr_last + size - 1;

		printk("  \"%s\": %d..%d\n", dom->name,
		       -domains[i]->first, -domains[i]->last);
		
		exterr_last += size;
	}

	return 0;
}

core_initcall(ext_err_init);
