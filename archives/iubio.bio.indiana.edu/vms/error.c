/*Last Modified:   5-FEB-1992 08:52:48.43, By: MARK */
/* error.c
 *
 * Error handling routines from Stevens: UNIX network programming pp 722-731
 *
 * The functions in this file are independent of any application
 * variables, and may be used in any C program.  Either of the names
 * CLIENT of SERVER may be defined when compiling this function.
 * If neither are defined, we assume SERVER.
 */

#include "gopher.h"

#include <varargs.h>

#ifdef IS_BSD
#define BSD 1
#endif

#ifdef CLIENT
#ifdef SERVER
/*no way jose, can't define both! (CLIENT and SERVER)*/
#endif
#endif

#ifndef CLIENT
#ifndef SERVER
#define CLIENT 1
#endif
#endif

#ifndef NULL
#define NULL ((void *) 0)
#endif

char *pname = NULL;

#ifdef CLIENT  /* these all output to stderr */

/*
 * Fatal error.  Print a message and terminate.
 * Don't dump core and don't print the system's errno value.
 *
 * 	err_quit(str, arg1, arg2, .....)
 *
 * The string "str" must specify the conversion specification for any args.
 *
 */

/*VARARGS1*/
err_quit(va_alist)
va_dcl
{
	va_list args;
	char *fmt;

	va_start(args);
	if (pname != NULL)
		fprintf(stderr, "%s: ",pname);
	fmt = va_arg(args, char *);

#ifndef NO_VPRINTF
	vfprintf(stderr, fmt, args);
#endif
	fputc('\n', stderr);
	va_end(args);

	exit(1);
}

/*
 * Fatal error related to a system call. Print the message and terminate.
 * Don't dump core, but do print the systems errno value and its
 * associated message.
 *
 *	err_sys(str, arg1, arg2, ...)
 *
 */

/*VARARGS1*/
err_sys(va_alist)
va_dcl
{
	va_list args;
	char *fmt;

	va_start(args);
	if (pname != NULL)
		fprintf(stderr, "%s: ", pname);
	fmt = va_arg(args, char *);

#ifndef NO_VFPRINTF
	vfprintf(stderr, fmt, args);
#endif

	va_end(args);

	my_perror();

	exit(1);
}


/*
 * Recoverable error. print a message and return to the caller.
 *
 * 	err_ret(str, arg1, arg2, ...)
 */

/*VARARGS1*/
err_ret(va_alist)
va_dcl
{
	va_list	args;
	char	*fmt;

	va_start(args);
	if (pname != NULL)
		fprintf(stderr, "%s: ", pname);
	fmt = va_arg(args, char *);

#ifndef NO_VFPRINTF
	vfprintf(stderr, fmt, args);
#endif

	va_end(args);

	my_perror();

	fflush(stdout);
	fflush(stderr);
	return;
}


/*
 * Fatal error.  Print a message, dump core and terminate.
 *
 *	err_dump(str, arg1, arg2, ....)
 *
 */

/*VARARGS1*/
err_dump(va_alist)
va_dcl
{
	va_list args;
	char *fmt;

	va_start(args);
	if (pname != NULL)
		fprintf(stderr, "%s: ", pname);
	fmt = va_arg(args, char *);

#ifndef NO_VFPRINTF
	vfprintf(stderr, fmt, args);
#endif

	va_end(args);

	my_perror();

	fflush(stdout);		/* abort doesn't flush stdio buffers */
	fflush(stderr);

	abort();		/* dump core and terminate */
	exit(1);
}

/*
 * Print the UNIX errno value
 */


my_perror()
{
	char *sys_err_str();

	fprintf(stderr, " %s\n", sys_err_str());
}

#endif /* CLIENT */

#ifdef SERVER

#ifdef IS_BSD
 /*
  * Under BSD, these server routines use the syslog(3) facility.
  * They don't append a newline for example.
  */

#include <syslog.h>


#else /* not BSD */
/*
 * There really ought to be a better way to handle server logging
 * under System V
 */

#define syslog(a,b)	fprintf(stderr, "%s\n", (b))
#define openlog(a,b,c)	fprintf(stderr, "%s\n", (a))

#endif  /* BSD */

char emesgst[255] = {0};  /* used by all server routines */

/*
 * Identify ourselves, for syslog() messages.
 *
 * LOG_PID is an option that says prepend each message with our pid.
 * LOG_CONS is an option that says write to the console if unable to
 * send the message to syslogd.
 * LOG_DAEMON is our facility.
 */

err_init(ident)
char *ident);
{
	openlog(ident, (LOG_PID|LOG_CONS), LOG_DAEMON);
}

/*
 * Fatal error.  Print a message and terminate.
 * Don't print the system's errno value.
 *
 *	err_quit(str, arg1, arg2, ...)
 */

/*VARARGS1*/
err_quit(va_alist)
va_dcl
{
	va_list args;
	char *fmt;

	va_start(args);
	fmt = va_arg(args, char *);
	vsprintf(emesgstr, fmt, args);
	va_end(args);

	syslog(LOG_ERR, emesgstr);

	exit(1)
}


/*
 * Fatal error related to a system call.  Print the message and terminate.
 * Don't dump core, but do print the systems errno value and its associated
 * message.
 */

/*VARARGS*/
err_sys(va_alist)
va_dcl
{
	va_list args;
	char *fmt;

	va_start(args);
	fmt = va_arg(args, char *);
	vsprintf(emesgstr, fmt, args);
	va_end(args);

	my_perror();
	syslog(LOG_ERR, emesgstr);

	exit(1);
}


/*
 * Recoverable error.  Print a message, and return to caller.
 */

/*VARARGS1*/
err_ret(va_alist)
va_dcl
{
	va_alist args;
	char *fmt;

	va_start(args);
	fmt = va_arg(args, char *);
	vsprintf(emergstr, fmt, args);
	va_end(args);

	my_perror();
	syslog(LOG_ERR, emergstr);

	return;
}


/*
 * Fatal error.  Print a message, dump core and terminate.
 */

/*VARARGS1*/
err_dump(va_alist)
va_dcl
{
	va_list args;
	char *fmt;

	va_start(args);
	fmt = va_arg(args, char *);
	vsprintf(emesgstr, fmt, args);
	va_end(args);

	my_perror();
	syslog(LOG_ERR, emesgstr);

	abort();
	exit(1);
}

/*
 * Print the UNIX errno value.
 * We just append it the the end of the emesgstr[] array.
 */

void
my_perror()
{
	int len;
	char *sys_err_str();

	len = strlen(emesgstr);
	sprintf(emesgstr + len, " %s", sys_err_str());
}

#endif  /* SERVER */


extern int errno;		/* UNIX error number */
extern int sys_nerr;		/* # of error message strings in sys table */
extern char *sys_errlist[];	/* the system error message table */

#ifdef SYS5
int t_errno;
int t_nerr;
char *t_errlist[1];
#endif

/*
 * Return a string containing some additional operating system
 * dependent information.
 * Note that different versions of UNIX assign different meanings
 * to the same value of "errno".  This means that if an error
 * condition is being sent ot another UNIX system, we must interpret
 * the errno value on the system that generated the error, and not
 * just send the decimal value of errno to the other system.
 */

char *
sys_err_str()
{
	static char msgstr[200];

	if (errno != 0) {
		if (errno >0 && errno < sys_nerr)
			sprintf(msgstr, "(%s)", sys_errlist[errno]);
		else
			sprintf(msgstr, "(errno = %d)", errno);
	} else {
		msgstr[0] = '\0';
	}

#ifdef SYS5

	if (t_errno != 0) {
		char tmsgstr[100];

		if (t_errno > 0 && t_errno < sys_nerr)
			sprintf(tmsgstr, " (%s)", t_errlist[t_errno]);
		else
			sprintf(tmsgstr, ", (t_errno = %d)", t_errno);

		strcat(msgstr, tmsgstr);
	}
#endif

	return(msgstr);
}


