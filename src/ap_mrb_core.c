/*
// ap_mrb_core.c - to proveid mod_mruby core
//
// See Copyright Notice in mod_mruby.h
*/

#include "mod_mruby.h"
#include "ap_mrb_core.h"
#include "ap_mrb_request.h"
#include <sys/syscall.h>
#include <sys/types.h>

#ifndef _WIN32
#define SUPPORT_SYSLOG
#else
#define sleep(x) Sleep(x * 1000)
#endif

int mod_mruby_return_code;

int ap_mrb_get_status_code()
{
  return mod_mruby_return_code;
}

int ap_mrb_set_status_code(int val)
{
  mod_mruby_return_code = val;
  return 0;
}

void ap_mrb_raise_error(mrb_state *mrb, mrb_value obj, mod_mruby_code_t *code)
{
  struct RString *str;
  char *err_out, *cache, *type;

  obj = mrb_funcall(mrb, obj, "inspect", 0);

  if (mrb_type(obj) == MRB_TT_STRING) {
    str = mrb_str_ptr(obj);
    err_out = str->as.heap.ptr;
    if (code->type == MOD_MRUBY_STRING) {
      type = "STRING";
    } else {
      type = "FILE";
    }
    if (code->cache == CACHE_ENABLE) {
      cache = "ENABLE";
    } else {
      cache = "DISABLE";
    }
    ap_log_error(
        APLOG_MARK, APLOG_ERR, 0, NULL,
        "%s ERROR %s: mrb_run failed: [TYPE: %s] [CACHED: %s] mruby raise: %s",
        MODULE_NAME, __func__, type, cache, err_out);
  }
}

static mrb_value ap_mrb_return(mrb_state *mrb, mrb_value self)
{

  mrb_int ret;

  mrb_get_args(mrb, "i", &ret);
  ap_mrb_set_status_code((int)ret);

  return self;
}

static mrb_value ap_mrb_get_mod_mruby_name(mrb_state *mrb, mrb_value str)
{
  return mrb_str_new(mrb, MODULE_NAME, strlen(MODULE_NAME));
}

static mrb_value ap_mrb_get_mod_mruby_version(mrb_state *mrb, mrb_value str)
{
  return mrb_str_new(mrb, MODULE_VERSION, strlen(MODULE_VERSION));
}

static mrb_value ap_mrb_get_server_version(mrb_state *mrb, mrb_value str)
{
#if AP_SERVER_PATCHLEVEL_NUMBER > 3
  return mrb_str_new(mrb, ap_get_server_description(),
                     strlen(ap_get_server_description()));
#else
  return mrb_str_new_lit(mrb, AP_SERVER_BASEVERSION " (" PLATFORM ")");
#endif
}

static mrb_value ap_mrb_get_server_build(mrb_state *mrb, mrb_value str)
{
  return mrb_str_new(mrb, ap_get_server_built(), strlen(ap_get_server_built()));
}

static mrb_value ap_mrb_sleep(mrb_state *mrb, mrb_value str)
{

  mrb_int time;

  mrb_get_args(mrb, "i", &time);
  sleep((int)time);

  return str;
}

static mrb_value ap_mrb_errlogger(mrb_state *mrb, mrb_value str)
{

  mrb_value *argv;
  mrb_int argc;

  mrb_get_args(mrb, "*", &argv, &argc);
  if (argc != 2) {
    ap_log_error(APLOG_MARK, APLOG_WARNING, 0, NULL,
                 "%s ERROR %s: argument is not 2", MODULE_NAME, __func__);
    return str;
  }

  ap_log_error(APLOG_MARK, mrb_fixnum(argv[0]), 0,
               (ap_mrb_get_request())->server, "%s",
               mrb_str_to_cstr(mrb, argv[1]));

  return str;
}

static mrb_value ap_mrb_syslogger(mrb_state *mrb, mrb_value str)
{

#ifdef SUPPORT_SYSLOG
  mrb_int pri;
  char *msg;

  mrb_get_args(mrb, "iz", &pri, &msg);

  openlog(NULL, LOG_PID, LOG_SYSLOG);
  syslog(pri, "%s", msg);
  closelog();

  return str;
#else
  ap_log_error(APLOG_MARK, APLOG_ERR, 0, NULL,
               "%s ERROR %s: syslog was not supported", MODULE_NAME, __func__);

  return mrb_nil_value();
#endif
}

static mrb_value ap_mrb_rputs(mrb_state *mrb, mrb_value str)
{
  mrb_value msg;

  int ai = mrb_gc_arena_save(mrb);
  mrb_get_args(mrb, "o", &msg);

  if (mrb_type(msg) != MRB_TT_STRING) {
    msg = mrb_funcall(mrb, msg, "to_s", 0, NULL);
  }
  ap_rputs(mrb_str_to_cstr(mrb, msg), ap_mrb_get_request());
  mrb_gc_arena_restore(mrb, ai);

  return str;
}

static mrb_value ap_mrb_echo(mrb_state *mrb, mrb_value str)
{
  mrb_value msg;

  int ai = mrb_gc_arena_save(mrb);
  mrb_get_args(mrb, "o", &msg);

  if (mrb_type(msg) != MRB_TT_STRING) {
    msg = mrb_funcall(mrb, msg, "to_s", 0, NULL);
  }
  ap_rputs(
      mrb_str_to_cstr(mrb, mrb_str_plus(mrb, msg, mrb_str_new_lit(mrb, "\n"))),
      ap_mrb_get_request());
  mrb_gc_arena_restore(mrb, ai);

  return str;
}

static mrb_value ap_mrb_server_name(mrb_state *mrb, mrb_value self)
{
  return mrb_str_new_lit(mrb, AP_SERVER_BASEPRODUCT);
}

static mrb_value ap_mrb_f_global_remove(mrb_state *mrb, mrb_value self)
{
  mrb_sym id;
  mrb_get_args(mrb, "n", &id);
  mrb_gv_remove(mrb, id);

  return mrb_f_global_variables(mrb, self);
}

static mrb_value ap_mrb_f_count_arena(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value(mrb_gc_arena_save(mrb));
}

static mrb_value ap_mrb_f_get_tid(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value((mrb_int)syscall(SYS_gettid));
}

#define AP_MRB_DEFINE_CORE_CONST_FIXNUM(val)                                   \
  mrb_define_const(mrb, class_core, #val, mrb_fixnum_value(val));

void ap_mruby_core_init(mrb_state *mrb, struct RClass *class_core)
{

  mrb_define_method(mrb, mrb->kernel_module, "server_name", ap_mrb_server_name,
                    MRB_ARGS_NONE());

  AP_MRB_DEFINE_CORE_CONST_FIXNUM(APLOG_EMERG);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(APLOG_ALERT);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(APLOG_CRIT);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(APLOG_ERR);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(APLOG_WARNING);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(APLOG_NOTICE);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(APLOG_INFO);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(APLOG_DEBUG);

  mrb_define_const(mrb, class_core, "LOG_EMERG", mrb_fixnum_value(APLOG_EMERG));
  mrb_define_const(mrb, class_core, "LOG_ALERT", mrb_fixnum_value(APLOG_ALERT));
  mrb_define_const(mrb, class_core, "LOG_CRIT", mrb_fixnum_value(APLOG_CRIT));
  mrb_define_const(mrb, class_core, "LOG_ERR", mrb_fixnum_value(APLOG_ERR));
  mrb_define_const(mrb, class_core, "LOG_WARN",
                   mrb_fixnum_value(APLOG_WARNING));
  mrb_define_const(mrb, class_core, "LOG_NOTICE",
                   mrb_fixnum_value(APLOG_NOTICE));
  mrb_define_const(mrb, class_core, "LOG_INFO", mrb_fixnum_value(APLOG_INFO));
  mrb_define_const(mrb, class_core, "LOG_DEBUG", mrb_fixnum_value(APLOG_DEBUG));

  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_GET);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_PUT);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_POST);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_DELETE);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_CONNECT);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_OPTIONS);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_TRACE);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_PATCH);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_PROPFIND);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_PROPPATCH);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_MKCOL);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_COPY);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_MOVE);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_LOCK);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_UNLOCK);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_VERSION_CONTROL);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_CHECKOUT);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_UNCHECKOUT);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_CHECKIN);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_UPDATE);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_LABEL);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_REPORT);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_MKWORKSPACE);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_MKACTIVITY);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_BASELINE_CONTROL);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_MERGE);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(M_INVALID);

  AP_MRB_DEFINE_CORE_CONST_FIXNUM(LOG_ALERT);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(LOG_CRIT);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(LOG_DEBUG);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(LOG_EMERG);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(LOG_ERR);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(LOG_INFO);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(LOG_NOTICE);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(LOG_EMERG);
  AP_MRB_DEFINE_CORE_CONST_FIXNUM(LOG_WARNING);

  mrb_define_const(mrb, class_core, "OK", mrb_fixnum_value(OK));
  mrb_define_const(mrb, class_core, "DECLINED", mrb_fixnum_value(DECLINED));
  mrb_define_const(mrb, class_core, "HTTP_SERVICE_UNAVAILABLE",
                   mrb_fixnum_value(HTTP_SERVICE_UNAVAILABLE));
  mrb_define_const(mrb, class_core, "HTTP_CONTINUE",
                   mrb_fixnum_value(HTTP_CONTINUE));
  mrb_define_const(mrb, class_core, "HTTP_SWITCHING_PROTOCOLS",
                   mrb_fixnum_value(HTTP_SWITCHING_PROTOCOLS));
  mrb_define_const(mrb, class_core, "HTTP_PROCESSING",
                   mrb_fixnum_value(HTTP_PROCESSING));
  mrb_define_const(mrb, class_core, "HTTP_OK", mrb_fixnum_value(HTTP_OK));
  mrb_define_const(mrb, class_core, "HTTP_CREATED",
                   mrb_fixnum_value(HTTP_CREATED));
  mrb_define_const(mrb, class_core, "HTTP_ACCEPTED",
                   mrb_fixnum_value(HTTP_ACCEPTED));
  mrb_define_const(mrb, class_core, "HTTP_NON_AUTHORITATIVE",
                   mrb_fixnum_value(HTTP_NON_AUTHORITATIVE));
  mrb_define_const(mrb, class_core, "HTTP_NO_CONTENT",
                   mrb_fixnum_value(HTTP_NO_CONTENT));
  mrb_define_const(mrb, class_core, "HTTP_RESET_CONTENT",
                   mrb_fixnum_value(HTTP_RESET_CONTENT));
  mrb_define_const(mrb, class_core, "HTTP_PARTIAL_CONTENT",
                   mrb_fixnum_value(HTTP_PARTIAL_CONTENT));
  mrb_define_const(mrb, class_core, "HTTP_MULTI_STATUS",
                   mrb_fixnum_value(HTTP_MULTI_STATUS));
  mrb_define_const(mrb, class_core, "HTTP_MULTIPLE_CHOICES",
                   mrb_fixnum_value(HTTP_MULTIPLE_CHOICES));
  mrb_define_const(mrb, class_core, "HTTP_MOVED_PERMANENTLY",
                   mrb_fixnum_value(HTTP_MOVED_PERMANENTLY));
  mrb_define_const(mrb, class_core, "HTTP_MOVED_TEMPORARILY",
                   mrb_fixnum_value(HTTP_MOVED_TEMPORARILY));
  mrb_define_const(mrb, class_core, "HTTP_SEE_OTHER",
                   mrb_fixnum_value(HTTP_SEE_OTHER));
  mrb_define_const(mrb, class_core, "HTTP_NOT_MODIFIED",
                   mrb_fixnum_value(HTTP_NOT_MODIFIED));
  mrb_define_const(mrb, class_core, "HTTP_USE_PROXY",
                   mrb_fixnum_value(HTTP_USE_PROXY));
  mrb_define_const(mrb, class_core, "HTTP_TEMPORARY_REDIRECT",
                   mrb_fixnum_value(HTTP_TEMPORARY_REDIRECT));
  mrb_define_const(mrb, class_core, "HTTP_BAD_REQUEST",
                   mrb_fixnum_value(HTTP_BAD_REQUEST));
  mrb_define_const(mrb, class_core, "HTTP_UNAUTHORIZED",
                   mrb_fixnum_value(HTTP_UNAUTHORIZED));
  mrb_define_const(mrb, class_core, "HTTP_PAYMENT_REQUIRED",
                   mrb_fixnum_value(HTTP_PAYMENT_REQUIRED));
  mrb_define_const(mrb, class_core, "HTTP_FORBIDDEN",
                   mrb_fixnum_value(HTTP_FORBIDDEN));
  mrb_define_const(mrb, class_core, "HTTP_NOT_FOUND",
                   mrb_fixnum_value(HTTP_NOT_FOUND));
  mrb_define_const(mrb, class_core, "HTTP_METHOD_NOT_ALLOWED",
                   mrb_fixnum_value(HTTP_METHOD_NOT_ALLOWED));
  mrb_define_const(mrb, class_core, "HTTP_NOT_ACCEPTABLE",
                   mrb_fixnum_value(HTTP_NOT_ACCEPTABLE));
  mrb_define_const(mrb, class_core, "HTTP_PROXY_AUTHENTICATION_REQUIRED",
                   mrb_fixnum_value(HTTP_PROXY_AUTHENTICATION_REQUIRED));
  mrb_define_const(mrb, class_core, "HTTP_REQUEST_TIME_OUT",
                   mrb_fixnum_value(HTTP_REQUEST_TIME_OUT));
  mrb_define_const(mrb, class_core, "HTTP_CONFLICT",
                   mrb_fixnum_value(HTTP_CONFLICT));
  mrb_define_const(mrb, class_core, "HTTP_GONE", mrb_fixnum_value(HTTP_GONE));
  mrb_define_const(mrb, class_core, "HTTP_LENGTH_REQUIRED",
                   mrb_fixnum_value(HTTP_LENGTH_REQUIRED));
  mrb_define_const(mrb, class_core, "HTTP_PRECONDITION_FAILED",
                   mrb_fixnum_value(HTTP_PRECONDITION_FAILED));
  mrb_define_const(mrb, class_core, "HTTP_REQUEST_ENTITY_TOO_LARGE",
                   mrb_fixnum_value(HTTP_REQUEST_ENTITY_TOO_LARGE));
  mrb_define_const(mrb, class_core, "HTTP_REQUEST_URI_TOO_LARGE",
                   mrb_fixnum_value(HTTP_REQUEST_URI_TOO_LARGE));
  mrb_define_const(mrb, class_core, "HTTP_UNSUPPORTED_MEDIA_TYPE",
                   mrb_fixnum_value(HTTP_UNSUPPORTED_MEDIA_TYPE));
  mrb_define_const(mrb, class_core, "HTTP_RANGE_NOT_SATISFIABLE",
                   mrb_fixnum_value(HTTP_RANGE_NOT_SATISFIABLE));
  mrb_define_const(mrb, class_core, "HTTP_EXPECTATION_FAILED",
                   mrb_fixnum_value(HTTP_EXPECTATION_FAILED));
  mrb_define_const(mrb, class_core, "HTTP_UNPROCESSABLE_ENTITY",
                   mrb_fixnum_value(HTTP_UNPROCESSABLE_ENTITY));
  mrb_define_const(mrb, class_core, "HTTP_LOCKED",
                   mrb_fixnum_value(HTTP_LOCKED));
  mrb_define_const(mrb, class_core, "HTTP_NOT_EXTENDED",
                   mrb_fixnum_value(HTTP_NOT_EXTENDED));
  mrb_define_const(mrb, class_core, "HTTP_INTERNAL_SERVER_ERROR",
                   mrb_fixnum_value(HTTP_INTERNAL_SERVER_ERROR));
  mrb_define_const(mrb, class_core, "HTTP_NOT_IMPLEMENTED",
                   mrb_fixnum_value(HTTP_NOT_IMPLEMENTED));
  mrb_define_const(mrb, class_core, "HTTP_BAD_GATEWAY",
                   mrb_fixnum_value(HTTP_BAD_GATEWAY));
  mrb_define_const(mrb, class_core, "HTTP_VARIANT_ALSO_VARIES",
                   mrb_fixnum_value(HTTP_VARIANT_ALSO_VARIES));

  mrb_define_const(mrb, class_core, "PROXYREQ_NONE",
                   mrb_fixnum_value(PROXYREQ_NONE));
  mrb_define_const(mrb, class_core, "PROXYREQ_PROXY",
                   mrb_fixnum_value(PROXYREQ_PROXY));
  mrb_define_const(mrb, class_core, "PROXYREQ_REVERSE",
                   mrb_fixnum_value(PROXYREQ_REVERSE));
  mrb_define_const(mrb, class_core, "PROXYREQ_RESPONSE",
                   mrb_fixnum_value(PROXYREQ_RESPONSE));

  mrb_define_class_method(mrb, class_core, "sleep", ap_mrb_sleep,
                          MRB_ARGS_ANY());
  mrb_define_class_method(mrb, class_core, "rputs", ap_mrb_rputs,
                          MRB_ARGS_ANY());
  mrb_define_class_method(mrb, class_core, "echo", ap_mrb_echo, MRB_ARGS_ANY());
  mrb_define_class_method(mrb, class_core, "return", ap_mrb_return,
                          MRB_ARGS_ANY());
  mrb_define_class_method(mrb, class_core, "errlogger", ap_mrb_errlogger,
                          MRB_ARGS_ANY());
  mrb_define_class_method(mrb, class_core, "log", ap_mrb_errlogger,
                          MRB_ARGS_ANY());
  mrb_define_class_method(mrb, class_core, "syslogger", ap_mrb_syslogger,
                          MRB_ARGS_ANY());
  mrb_define_class_method(mrb, class_core, "syslog", ap_mrb_syslogger,
                          MRB_ARGS_ANY());
  // mrb_define_class_method(mrb, class, "write_request", ap_mrb_write_request,
  // MRB_ARGS_ANY());
  mrb_define_class_method(mrb, class_core, "module_name",
                          ap_mrb_get_mod_mruby_name, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, class_core, "module_version",
                          ap_mrb_get_mod_mruby_version, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, class_core, "server_version",
                          ap_mrb_get_server_version, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, class_core, "server_build",
                          ap_mrb_get_server_build, MRB_ARGS_NONE());
  mrb_define_class_method(mrb, class_core, "remove_global_variable",
                          ap_mrb_f_global_remove, MRB_ARGS_REQ(1));
  mrb_define_class_method(mrb, class_core, "count_arena", ap_mrb_f_count_arena,
                          MRB_ARGS_NONE());
  mrb_define_class_method(mrb, class_core, "get_tid", ap_mrb_f_get_tid,
                          MRB_ARGS_NONE());
}
