/*
 * Output and debugging functions
 *
 * Author:
 *   Miguel de Icaza (miguel@novell.com)
 *
 * (C) 2006 Novell, Inc.
 * Copyright 2011 Xamarin Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

/* The current fatal levels, error is always fatal */
static GLogLevelFlags fatal = G_LOG_LEVEL_ERROR;
static GLogFunc default_log_func;
static gpointer default_log_func_user_data;
static GPrintFunc stdout_handler, stderr_handler;

static void default_stdout_handler (const gchar *string);
static void default_stderr_handler (const gchar *string);

void
g_printv (const gchar *format, va_list args)
{
	char *msg;

	if (g_vasprintf (&msg, format, args) < 0)
		return;

	if (!stdout_handler)
		stdout_handler = default_stdout_handler;

	stdout_handler (msg);
	g_free (msg);
}

void
g_print (const gchar *format, ...)
{
	va_list args;
	va_start (args, format);
	g_printv (format, args);
	va_end (args);
}

void
g_printerr (const gchar *format, ...)
{
	char *msg;
	va_list args;

	va_start (args, format);
	if (g_vasprintf (&msg, format, args) < 0) {
		va_end (args);
		return;
	}
	va_end (args);

	if (!stderr_handler)
		stderr_handler = default_stderr_handler;

	stderr_handler (msg);
	g_free (msg);
}

GLogLevelFlags
g_log_set_always_fatal (GLogLevelFlags fatal_mask)
{
	GLogLevelFlags old_fatal = fatal;

	fatal |= fatal_mask;
	
	return old_fatal;
}

GLogLevelFlags
g_log_set_fatal_mask (const gchar *log_domain, GLogLevelFlags fatal_mask)
{
	/*
	 * Mono does not use a G_LOG_DOMAIN currently, so we just assume things are fatal
	 * if we decide to set G_LOG_DOMAIN (we probably should) we should implement
	 * this.
	 */
	return fatal_mask;
}

void
g_logv (const gchar *log_domain, GLogLevelFlags log_level, const gchar *format, va_list args)
{
	char *msg;

	if (!default_log_func)
		default_log_func = g_log_default_handler;
	
	if (g_vasprintf (&msg, format, args) < 0)
		return;

	default_log_func (log_domain, log_level, msg, default_log_func_user_data);
	g_free (msg);
}

void
g_log (const gchar *log_domain, GLogLevelFlags log_level, const gchar *format, ...)
{
	va_list args;

	va_start (args, format);
	g_logv (log_domain, log_level, format, args);
	va_end (args);
}

static char *failure_assertion = NULL;

const char *
g_get_assertion_message (void)
{
	return failure_assertion;
}

void
g_assertion_message (const gchar *format, ...)
{
	va_list args;

	va_start (args, format);

	g_vasprintf (&failure_assertion, format, args);

	g_logv (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR, format, args);
	va_end (args);
	exit (0);
}

#if HOST_ANDROID
#include <android/log.h>

static android_LogPriority
to_android_priority (GLogLevelFlags log_level)
{
	switch (log_level & G_LOG_LEVEL_MASK)
	{
		case G_LOG_LEVEL_ERROR:     return ANDROID_LOG_FATAL;
		case G_LOG_LEVEL_CRITICAL:  return ANDROID_LOG_ERROR;
		case G_LOG_LEVEL_WARNING:   return ANDROID_LOG_WARN;
		case G_LOG_LEVEL_MESSAGE:   return ANDROID_LOG_INFO;
		case G_LOG_LEVEL_INFO:      return ANDROID_LOG_DEBUG;
		case G_LOG_LEVEL_DEBUG:     return ANDROID_LOG_VERBOSE;
	}
	return ANDROID_LOG_UNKNOWN;
}

#define LOG_MESSAGE_MAX_LEN 4096

static void
android_log_line (gint log_priority, const gchar *log_domain, gchar *log_message, gint log_len)
{
	gchar log_buf [LOG_MESSAGE_MAX_LEN];

	g_assert (log_len <= LOG_MESSAGE_MAX_LEN - 1);

	/* If line is longer than LOG_MESSAGE_MAX_LEN - 1, then we simply cut it out. This is consistent with the previous behavior. */
	strncpy (log_buf, log_message, log_len);
	log_buf [log_len] = '\0';

	__android_log_write (log_priority, log_domain, log_buf);
}

static void
android_log (gint log_priority, const gchar *log_domain, const gchar *log_message)
{
	gint log_message_len, log_message_p_len;
	gchar *log_message_p;

	log_message_len = strlen (log_message);
	if (log_message_len <= LOG_MESSAGE_MAX_LEN) {
		__android_log_write (log_priority, log_domain, log_message);
		return;
	}

	for (log_message_p = log_message; log_message_p < log_message + log_message_len;) {
		gchar *p = strstr (log_message_p, "\n");
		if (p == NULL) {
			/* There is no more "\n". */
			android_log_line (log_priority, log_domain, log_message_p, LOG_MESSAGE_MAX_LEN - 1);
			break;
		}

		log_message_p_len = p - log_message_p;
		if (log_message_p_len > LOG_MESSAGE_MAX_LEN - 1)
			log_message_p_len = LOG_MESSAGE_MAX_LEN - 1;

		android_log_line (log_priority, log_domain, log_message_p, log_message_p_len);

		/* Set `log_message_p` to the character right after "\n" */
		log_message_p = p + 1;
	}
}

void
g_log_default_handler (const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer unused_data)
{
	android_log (to_android_priority (log_level), log_domain, message);
	if (log_level & fatal)
		abort ();
}

static void
default_stdout_handler (const gchar *message)
{
	/* TODO: provide a proper app name */
	android_log (ANDROID_LOG_ERROR, "mono", message);
}

static void
default_stderr_handler (const gchar *message)
{
	/* TODO: provide a proper app name */
	android_log (ANDROID_LOG_ERROR, "mono", message);
}


#elif defined(HOST_IOS)
#include <asl.h>

static int
to_asl_priority (GLogLevelFlags log_level)
{
	switch (log_level & G_LOG_LEVEL_MASK)
	{
		case G_LOG_LEVEL_ERROR:     return ASL_LEVEL_CRIT;
		case G_LOG_LEVEL_CRITICAL:  return ASL_LEVEL_ERR;
		case G_LOG_LEVEL_WARNING:   return ASL_LEVEL_WARNING;
		case G_LOG_LEVEL_MESSAGE:   return ASL_LEVEL_NOTICE;
		case G_LOG_LEVEL_INFO:      return ASL_LEVEL_INFO;
		case G_LOG_LEVEL_DEBUG:     return ASL_LEVEL_DEBUG;
	}
	return ASL_LEVEL_ERR;
}

void
g_log_default_handler (const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer unused_data)
{
	asl_log (NULL, NULL, to_asl_priority (log_level), "%s", message);
	if (log_level & fatal)
		abort ();
}

static void
default_stdout_handler (const gchar *message)
{
	asl_log (NULL, NULL, ASL_LEVEL_WARNING, "%s", message);
}

static void
default_stderr_handler (const gchar *message)
{
	asl_log (NULL, NULL, ASL_LEVEL_WARNING, "%s", message);
}

#else

void
g_log_default_handler (const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer unused_data)
{
	FILE *target = stdout;

	fprintf (target, "%s%s%s\n",
		log_domain != NULL ? log_domain : "",
		log_domain != NULL ? ": " : "",
		message);

	if (log_level & fatal) {
		fflush (stdout);
		fflush (stderr);
		abort ();
	}
}

static void
default_stdout_handler (const gchar *string)
{
	fprintf (stdout, "%s", string);
}

static void
default_stderr_handler (const gchar *string)
{
	fprintf (stderr, "%s", string);
}

#endif

GLogFunc
g_log_set_default_handler (GLogFunc log_func, gpointer user_data)
{
	GLogFunc old = default_log_func;
	default_log_func = log_func;
	default_log_func_user_data = user_data;
	return old;
}

GPrintFunc
g_set_print_handler (GPrintFunc func)
{
	GPrintFunc old = stdout_handler;
	stdout_handler = func;
	return old;
}

GPrintFunc
g_set_printerr_handler (GPrintFunc func)
{
	GPrintFunc old = stderr_handler;
	stderr_handler = func;
	return old;
}

