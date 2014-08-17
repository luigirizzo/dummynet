/*
 * Copyright (C) 2009 Luigi Rizzo, Marta Carbone, Universita` di Pisa
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * $Id: glue.c 12264 2013-04-27 20:21:06Z luigi $
 *
 * Userland functions missing in linux/Windows
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <netdb.h>
#include <windows.h>
#endif /* _WIN32 */

#ifndef HAVE_NAT
/* dummy nat functions */
void
ipfw_show_nat(int ac, char **av)
{
	fprintf(stderr, "%s unsupported\n", __FUNCTION__);
}

void
ipfw_config_nat(int ac, char **av)
{
	fprintf(stderr, "%s unsupported\n", __FUNCTION__);
}
#endif

#ifdef __linux__
int optreset;	/* missing in linux */
#endif

/*
 * not implemented in linux.
 * taken from /usr/src/lib/libc/string/strlcpy.c
 */
size_t
strlcpy(char *dst, const char *src, size_t siz)
{
        char *d = dst;
        const char *s = src;
        size_t n = siz;

        /* Copy as many bytes as will fit */
        if (n != 0 && --n != 0) {
                do {
                        if ((*d++ = *s++) == 0)
                                break;
                } while (--n != 0);
        }

        /* Not enough room in dst, add NUL and traverse rest of src */
        if (n == 0) {
                if (siz != 0)
                        *d = '\0';              /* NUL-terminate dst */
                while (*s++)
                        ;
        }

        return(s - src - 1);    /* count does not include NUL */
}


/* missing in linux and windows */
long long int
strtonum(const char *nptr, long long minval, long long maxval,
         const char **errstr)
{
	long long ret;
	int errno_c = errno;	/* save actual errno */

	errno = 0;
#ifdef TCC
	ret = strtol(nptr, (char **)errstr, 0);
#else
	ret = strtoll(nptr, (char **)errstr, 0);
#endif
	/* We accept only a string that represent exactly a number (ie. start
	 * and end with a digit).
	 * FreeBSD version wants errstr==NULL if no error occurs, otherwise
	 * errstr should point to an error string.
	 * For our purspose, we implement only the invalid error, ranges
	 * error aren't checked
	 */
	if (errno != 0 || nptr == *errstr || **errstr != '\0')
		*errstr = "invalid";
	else  {
		*errstr = NULL;
		errno = errno_c;
	}
	return ret;
}

#if defined (_WIN32) || defined (EMULATE_SYSCTL)
//XXX missing prerequisites
#include <net/if.h> 		//openwrt
#include <netinet/ip.h> 	//openwrt
#include <netinet/ip_fw.h>
#include <netinet/ip_dummynet.h>
#endif

/*
 * set or get system information
 * XXX lock acquisition/serialize calls
 *
 * we export this as sys/module/ipfw_mod/parameters/___
 * This function get or/and set the value of the sysctl passed by
 * the name parameter. If the old value is not desired,
 * oldp and oldlenp should be set to NULL.
 *
 * XXX
 * I do not know how this works in FreeBSD in the case
 * where there are no write permission on the sysctl var.
 * We read the value and set return variables in any way
 * but returns -1 on write failures, regardless the
 * read success.
 *
 * Since there is no information on types, in the following
 * code we assume a length of 4 is a int.
 *
 * Returns 0 on success, -1 on errors.
 */
int
sysctlbyname(const char *name, void *oldp, size_t *oldlenp, void *newp,
         size_t newlen)
{
#if defined (_WIN32) || defined (EMULATE_SYSCTL)
	/*
	 * we embed the sysctl request in the usual sockopt mechanics.
	 * the sockopt buffer il filled with a dn_id with IP_DUMMYNET3
	 * command, and the special DN_SYSCTL_GET and DN_SYSCTL_SET
	 * subcommands.
	 * the syntax of this function is fully compatible with
	 * POSIX sysctlby name:
	 * if newp and newlen are != 0 => this is a set
	 * else if oldp and oldlen are != 0 => this is a get
	 *		to avoid too much overhead in the module, the whole
	 *		sysctltable is returned, and the parsing is done in userland,
	 *		a probe request is done to retrieve the size needed to
	 *		transfer the table, before the real request
	 * if both old and new params = 0 => this is a print
	 *		this is a special request, done only by main()
	 *		to implement the extension './ipfw sysctl',
	 *		a command that bypasses the normal getopt, and that
	 *		is available on those platforms that use this
	 *		sysctl emulation.
	 *		in this case, a negative oldlen signals that *oldp
	 *		is actually a FILE* to print somewhere else than stdout
	 */

	int l;
	int ret;
	struct dn_id* oid;
	struct sysctlhead* entry;
	char* pstring;
	char* pdata;
	FILE* fp;

	if((oldlenp != NULL) && (*oldlenp < 0))
		fp = (FILE*)oldp;
	else
		fp = stdout;
	if(newp != NULL && newlen != 0)
	{
		//this is a set
		l = sizeof(struct dn_id) + sizeof(struct sysctlhead) + strlen(name)+1 + newlen;
		oid = malloc(l);
		if (oid == NULL)
			return -1;
		oid->len = l;
		oid->type = DN_SYSCTL_SET;
		oid->id = DN_API_VERSION;

		entry = (struct sysctlhead*)(oid+1);
		pdata = (char*)(entry+1);
		pstring = pdata + newlen;

		entry->blocklen = ((sizeof(struct sysctlhead) + strlen(name)+1 + newlen) + 3) & ~3;
		entry->namelen = strlen(name)+1;
		entry->flags = 0;
		entry->datalen = newlen;

		bcopy(newp, pdata, newlen);
		bcopy(name, pstring, strlen(name)+1);

		ret = do_cmd(IP_DUMMYNET3, oid, (uintptr_t)l);
		if (ret != 0)
			return -1;
	}
	else
	{
		//this is a get or a print
		l = sizeof(struct dn_id);
		oid = malloc(l);
		if (oid == NULL)
			return -1;
		oid->len = l;
		oid->type = DN_SYSCTL_GET;
		oid->id = DN_API_VERSION;

		ret = do_cmd(-IP_DUMMYNET3, oid, (uintptr_t)&l);
		if (ret != 0)
			return -1;

		l=oid->id;
		free(oid);
		oid = malloc(l);
		if (oid == NULL)
			return -1;
		oid->len = l;
		oid->type = DN_SYSCTL_GET;
		oid->id = DN_API_VERSION;

		ret = do_cmd(-IP_DUMMYNET3, oid, (uintptr_t)&l);
		if (ret != 0)
			return -1;

		entry = (struct sysctlhead*)(oid+1);
		while(entry->blocklen != 0)
		{
			pdata = (char*)(entry+1);
			pstring = pdata+entry->datalen;

			//time to check if this is a get or a print
			if(name != NULL && oldp != NULL && *oldlenp > 0)
			{
				//this is a get
				if(strcmp(name,pstring) == 0)
				{
					//match found, sanity chech on len
					if(*oldlenp < entry->datalen)
					{
						printf("%s error: buffer too small\n",__FUNCTION__);
						return -1;
					}
					*oldlenp = entry->datalen;
					bcopy(pdata, oldp, *oldlenp);
					return 0;
				}
			}
			else
			{
				//this is a print
				if( name == NULL )
					goto print;
				if ( (strncmp(pstring,name,strlen(name)) == 0) && ( pstring[strlen(name)]=='\0' || pstring[strlen(name)]=='.' ) )
						goto print;
				else
						goto skip;
print:
				fprintf(fp, "%s: ",pstring);
				switch( entry->flags >> 2 )
				{
					case SYSCTLTYPE_LONG:
						fprintf(fp, "%li ", *(long*)(pdata));
						break;
					case SYSCTLTYPE_UINT:
						fprintf(fp, "%u ", *(unsigned int*)(pdata));
						break;
					case SYSCTLTYPE_ULONG:
						fprintf(fp, "%lu ", *(unsigned long*)(pdata));
						break;
					case SYSCTLTYPE_INT:
					default:
						fprintf(fp, "%i ", *(int*)(pdata));
				}
				if( (entry->flags & 0x00000003) == CTLFLAG_RD )
					fprintf(fp, "\t(read only)\n");
				else
					fprintf(fp, "\n");
skip:			;
			}
			entry = (struct sysctlhead*)((unsigned char*)entry + entry->blocklen);
		}
		free(oid);
		return 0;
	}
	//fallback for invalid options
	return -1;

#else /* __linux__ */
	FILE *fp;
	char *basename = "/sys/module/ipfw_mod/parameters/";
	char filename[256];	/* full filename */
	char *varp;
	int ret = 0;		/* return value */
	long d;

	if (name == NULL) /* XXX set errno */
		return -1;

	/* locate the filename */
	varp = strrchr(name, '.');
	if (varp == NULL) /* XXX set errno */
		return -1;

	snprintf(filename, sizeof(filename), "%s%s", basename, varp+1);

	/*
	 * XXX we could open the file here, in rw mode
	 * but need to check if a file have write
	 * permissions.
	 */

	/* check parameters */
	if (oldp && oldlenp) { /* read mode */
		fp = fopen(filename, "r");
		if (fp == NULL) {
			fprintf(stderr, "%s fopen error reading filename %s\n", __FUNCTION__, filename);
			return -1;
		}
		if (fscanf(fp, "%ld", &d) != 1) {
			ret = -1;
		} else if (*oldlenp == sizeof(int)) {
			int dst = d;
			memcpy(oldp, &dst, *oldlenp);
		} else if (*oldlenp == sizeof(long)) {
			memcpy(oldp, &d, *oldlenp);
		} else {
			fprintf(stderr, "unknown paramerer len %d\n",
				(int)*oldlenp);
		}
		fclose(fp);
	}

	if (newp && newlen) { /* write */
		fp = fopen(filename, "w");
		if (fp == NULL) {
			fprintf(stderr, "%s fopen error writing filename %s\n", __FUNCTION__, filename);
			return -1;
		}
		if (newlen == sizeof(int)) {
			if (fprintf(fp, "%d", *(int *)newp) < 1)
				ret = -1;
		} else if (newlen == sizeof(long)) {
			if (fprintf(fp, "%ld", *(long *)newp) < 1)
				ret = -1;
		} else {
			fprintf(stderr, "unknown paramerer len %d\n",
				(int)newlen);
		}

		fclose(fp);
	}

	return ret;
#endif /* __linux__ */
}

#ifdef _WIN32
/*
 * On windows, set/getsockopt are mapped to DeviceIoControl()
 */
int
wnd_setsockopt(int s, int level, int sopt_name, const void *optval,
                socklen_t optlen)
{
    size_t len = sizeof (struct sockopt) + optlen;
    struct sockopt *sock;
    DWORD n;
    BOOL result;
    HANDLE _dev_h = (HANDLE)s;

    /* allocate a data structure for communication */
    sock = malloc(len);
    if (sock == NULL)
        return -1;

    sock->sopt_dir = SOPT_SET;
    sock->sopt_name = sopt_name;
    sock->sopt_valsize = optlen;
    sock->sopt_val = (void *)(sock+1);

    memcpy(sock->sopt_val, optval, optlen);
    result = DeviceIoControl (_dev_h, IP_FW_SETSOCKOPT, sock, len,
		NULL, 0, &n, NULL);
    free (sock);

    return (result ? 0 : -1);
}

int
wnd_getsockopt(int s, int level, int sopt_name, void *optval,
                socklen_t *optlen)
{
    size_t len = sizeof (struct sockopt) + *optlen;
    struct sockopt *sock;
    DWORD n;
    BOOL result;
    HANDLE _dev_h = (HANDLE)s;

    sock = malloc(len);
    if (sock == NULL)
        return -1;

    sock->sopt_dir = SOPT_GET;
    sock->sopt_name = sopt_name;
    sock->sopt_valsize = *optlen;
    sock->sopt_val = (void *)(sock+1);

    memcpy (sock->sopt_val, optval, *optlen);

    result = DeviceIoControl (_dev_h, IP_FW_GETSOCKOPT, sock, len,
		sock, len, &n, NULL);
	//printf("len = %i, returned = %u, valsize = %i\n",len,n,sock->sopt_valsize);
    *optlen = sock->sopt_valsize;
    memcpy (optval, sock->sopt_val, *optlen);
    free (sock);
    return (result ? 0 : -1);
}

int
my_socket(int domain, int ty, int proto)
{
    TCHAR *pcCommPort = TEXT("\\\\.\\Ipfw");
    HANDLE _dev_h = INVALID_HANDLE_VALUE;

    /* Special Handling For Accessing Device On Windows 2000 Terminal Server
       See Microsoft KB Article 259131 */
    if (_dev_h == INVALID_HANDLE_VALUE) {
        _dev_h = CreateFile (pcCommPort,
		GENERIC_READ | GENERIC_WRITE,
		0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    if (_dev_h == INVALID_HANDLE_VALUE) {
	printf("%s failed %u, cannot talk to kernel module\n",
		__FUNCTION__, (unsigned)GetLastError());
        return -1;
    }
    return (int)_dev_h;
}

struct hostent* gethostbyname2(const char *name, int af)
{
	return gethostbyname(name);
}

struct ether_addr* ether_aton(const char *a)
{
	fprintf(stderr, "%s empty\n", __FUNCTION__);
	return NULL;
}

#ifdef TCC
int     opterr = 1,             /* if error message should be printed */
        optind = 1,             /* index into parent argv vector */
        optopt,                 /* character checked for validity */
        optreset;               /* reset getopt */
char    *optarg;                /* argument associated with option */

#define BADCH   (int)'?'
#define BADARG  (int)':'
#define EMSG    ""

#define PROGNAME	"ipfw"
/*
 * getopt --
 *      Parse argc/argv argument vector.
 */
int
getopt(nargc, nargv, ostr)
        int nargc;
        char * const nargv[];
        const char *ostr;
{
        static char *place = EMSG;              /* option letter processing */
        char *oli;                              /* option letter list index */

        if (optreset || *place == 0) {          /* update scanning pointer */
                optreset = 0;
                place = nargv[optind];
                if (optind >= nargc || *place++ != '-') {
                        /* Argument is absent or is not an option */
                        place = EMSG;
                        return (-1);
                }
                optopt = *place++;
                if (optopt == '-' && *place == 0) {
                        /* "--" => end of options */
                        ++optind;
                        place = EMSG;
                        return (-1);
                }
                if (optopt == 0) {
                        /* Solitary '-', treat as a '-' option
                           if the program (eg su) is looking for it. */
                        place = EMSG;
                        if (strchr(ostr, '-') == NULL)
                                return (-1);
                        optopt = '-';
                }
        } else
                optopt = *place++;

        /* See if option letter is one the caller wanted... */
        if (optopt == ':' || (oli = strchr(ostr, optopt)) == NULL) {
                if (*place == 0)
                        ++optind;
                if (opterr && *ostr != ':')
                        (void)fprintf(stderr,
                            "%s: illegal option -- %c\n", PROGNAME,
                            optopt);
                return (BADCH);
        }

        /* Does this option need an argument? */
        if (oli[1] != ':') {
                /* don't need argument */
                optarg = NULL;
                if (*place == 0)
                        ++optind;
        } else {
                /* Option-argument is either the rest of this argument or the
                   entire next argument. */
                if (*place)
                        optarg = place;
                else if (nargc > ++optind)
                        optarg = nargv[optind];
                else {
                        /* option-argument absent */
                        place = EMSG;
                        if (*ostr == ':')
                                return (BADARG);
                        if (opterr)
                                (void)fprintf(stderr,
                                    "%s: option requires an argument -- %c\n",
                                    PROGNAME, optopt);
                        return (BADCH);
                }
                place = EMSG;
                ++optind;
        }
        return (optopt);                        /* return option letter */
}

//static FILE *err_file = stderr;
void
verrx(int ex, int eval, const char *fmt, va_list ap)
{
        fprintf(stderr, "%s: ", PROGNAME);
        if (fmt != NULL)
                vfprintf(stderr, fmt, ap);
        fprintf(stderr, "\n");
	if (ex)
		exit(eval);
}
void
errx(int eval, const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        verrx(1, eval, fmt, ap);
        va_end(ap);
}

void
warnx(const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
	verrx(0, 0, fmt, ap);
        va_end(ap);
}

char *
strsep(char **stringp, const char *delim)
{
        char *s;
        const char *spanp;
        int c, sc;
        char *tok;

        if ((s = *stringp) == NULL)
                return (NULL);
        for (tok = s;;) {
                c = *s++;
                spanp = delim;
                do {
                        if ((sc = *spanp++) == c) {
                                if (c == 0)
                                        s = NULL;
                                else
                                        s[-1] = 0;
                                *stringp = s;
                                return (tok);
                        }
                } while (sc != 0);
        }
        /* NOTREACHED */
}

static unsigned char
tolower(unsigned char c)
{
	return (c >= 'A' && c <= 'Z') ? c + 'a' - 'A' : c;
}

static int isdigit(unsigned char c)
{
	return (c >= '0' && c <= '9');
}

static int isxdigit(unsigned char c)
{
	return (strchr("0123456789ABCDEFabcdef", c) ? 1 : 0);
}

static int isspace(unsigned char c)
{
	return (strchr(" \t\n\r", c) ? 1 : 0);
}

static int isascii(unsigned char c)
{
	return (c < 128);
}

static int islower(unsigned char c)
{
	return (c >= 'a' && c <= 'z');
}

int
strcasecmp(const char *s1, const char *s2)
{
        const unsigned char
                        *us1 = (const unsigned char *)s1,
                        *us2 = (const unsigned char *)s2;

        while (tolower(*us1) == tolower(*us2++))
                if (*us1++ == '\0')
                        return (0);
        return (tolower(*us1) - tolower(*--us2));
}

intmax_t
strtoimax(const char * restrict nptr, char ** restrict endptr, int base)
{
	return strtol(nptr, endptr,base);
}

void
setservent(int a)
{
}

#define NS_INADDRSZ 128

int
inet_pton(int af, const char *src, void *dst)
{
        static const char digits[] = "0123456789";
        int saw_digit, octets, ch;
        u_char tmp[NS_INADDRSZ], *tp;

	if (af != AF_INET) {
		errno = EINVAL;
		return -1;
	}

        saw_digit = 0;
        octets = 0;
        *(tp = tmp) = 0;
        while ((ch = *src++) != '\0') {
                const char *pch;

                if ((pch = strchr(digits, ch)) != NULL) {
                        u_int new = *tp * 10 + (pch - digits);

                        if (saw_digit && *tp == 0)
                                return (0);
                        if (new > 255)
                                return (0);
                        *tp = new;
                        if (!saw_digit) {
                                if (++octets > 4)
                                        return (0);
                                saw_digit = 1;
                        }
                } else if (ch == '.' && saw_digit) {
                        if (octets == 4)
                                return (0);
                        *++tp = 0;
                        saw_digit = 0;
                } else
                        return (0);
        }
        if (octets < 4)
                return (0);
        memcpy(dst, tmp, NS_INADDRSZ);
        return (1);
}

const char *
inet_ntop(int af, const void *_src, char *dst, socklen_t size)
{
        static const char fmt[] = "%u.%u.%u.%u";
        char tmp[sizeof "255.255.255.255"];
	const u_char *src = _src;
        int l;
	if (af != AF_INET) {
		errno = EINVAL;
		return NULL;
	}

        l = snprintf(tmp, sizeof(tmp), fmt, src[0], src[1], src[2], src[3]);
        if (l <= 0 || (socklen_t) l >= size) {
                errno = ENOSPC;
                return (NULL);
        }
        strlcpy(dst, tmp, size);
        return (dst);
}

/*%
 * Check whether "cp" is a valid ascii representation
 * of an Internet address and convert to a binary address.
 * Returns 1 if the address is valid, 0 if not.
 * This replaces inet_addr, the return value from which
 * cannot distinguish between failure and a local broadcast address.
 */
int
inet_aton(const char *cp, struct in_addr *addr) {
        u_long val;
        int base, n;
        char c;
        u_int8_t parts[4];
        u_int8_t *pp = parts;
        int digit;

        c = *cp;
        for (;;) {
                /*
                 * Collect number up to ``.''.
                 * Values are specified as for C:
                 * 0x=hex, 0=octal, isdigit=decimal.
                 */
                if (!isdigit((unsigned char)c))
                        return (0);
                val = 0; base = 10; digit = 0;
                if (c == '0') {
                        c = *++cp;
                        if (c == 'x' || c == 'X')
                                base = 16, c = *++cp;
                        else {
                                base = 8;
                                digit = 1 ;
                        }
                }
                for (;;) {
                        if (isascii(c) && isdigit((unsigned char)c)) {
                                if (base == 8 && (c == '8' || c == '9'))
                                        return (0);
                                val = (val * base) + (c - '0');
                                c = *++cp;
                                digit = 1;
                        } else if (base == 16 && isascii(c) &&
                                   isxdigit((unsigned char)c)) {
                                val = (val << 4) |
                                        (c + 10 - (islower((unsigned char)c) ? 'a' : 'A'));
                                c = *++cp;
                                digit = 1;
                        } else
                                break;
                }
                if (c == '.') {
                        /*
                         * Internet format:
                         *      a.b.c.d
                         *      a.b.c   (with c treated as 16 bits)
                         *      a.b     (with b treated as 24 bits)
                         */
                        if (pp >= parts + 3 || val > 0xffU)
                                return (0);
                        *pp++ = val;
                        c = *++cp;
                } else
                        break;
        }
        /*
         * Check for trailing characters.
         */
        if (c != '\0' && (!isascii(c) || !isspace((unsigned char)c)))
                return (0);
        /*
         * Did we get a valid digit?
         */
        if (!digit)
                return (0);
        /*
         * Concoct the address according to
         * the number of parts specified.
         */
        n = pp - parts + 1;
        switch (n) {
        case 1:                         /*%< a -- 32 bits */
                break;

        case 2:                         /*%< a.b -- 8.24 bits */
                if (val > 0xffffffU)
                        return (0);
                val |= parts[0] << 24;
                break;

        case 3:                         /*%< a.b.c -- 8.8.16 bits */
                if (val > 0xffffU)
                        return (0);
                val |= (parts[0] << 24) | (parts[1] << 16);
                break;

        case 4:                         /*%< a.b.c.d -- 8.8.8.8 bits */
                if (val > 0xffU)
                        return (0);
                val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
                break;
        }
        if (addr != NULL)
                addr->s_addr = htonl(val);
        return (1);
}

#endif /* TCC */

#endif /* _WIN32 */
