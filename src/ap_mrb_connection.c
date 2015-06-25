/*
// ap_mrb_connection.c - to provide connection api
//
// See Copyright Notice in mod_mruby.h
*/

#include "mod_mruby.h"
#include "ap_mrb_server.h"
//#include "json.h"

//static struct mrb_data_type server_rec_type = {
//  "server_rec", 0,
//};

// char write
/*
static mrb_value ap_mrb_set_server_error_fname(mrb_state *mrb, mrb_value str)
{
  mrb_value val;
  request_rec *r = ap_mrb_get_request();
  mrb_get_args(mrb, "o", &val);
  r->server->error_fname = apr_pstrdup(r->pool, mrb_str_to_cstr(mrb, val));
  return val;
}
*/

// char read
static mrb_value ap_mrb_get_conn_remote_ip(mrb_state *mrb, mrb_value str)
{
  request_rec *r = ap_mrb_get_request();
#ifdef __APACHE24__
  return ap_mrb_str_to_value(mrb, r->pool, r->connection->client_ip);
#else
  return ap_mrb_str_to_value(mrb, r->pool, r->connection->remote_ip);
#endif
}

static mrb_value ap_mrb_get_conn_remote_port(mrb_state *mrb, mrb_value str)
{
  request_rec *r = ap_mrb_get_request();
#ifdef __APACHE24__
  mrb_int val = (mrb_int)r->connection->client_addr->port;
#else
  mrb_int val = (mrb_int)r->connection->remote_addr->port;
#endif
  return mrb_fixnum_value(val);
}

static mrb_value ap_mrb_get_conn_remote_host(mrb_state *mrb, mrb_value str)
{
  request_rec *r = ap_mrb_get_request();
  return ap_mrb_str_to_value(mrb, r->pool, r->connection->remote_host);
}

static mrb_value ap_mrb_get_conn_remote_logname(mrb_state *mrb, mrb_value str)
{
  request_rec *r = ap_mrb_get_request();
  return ap_mrb_str_to_value(mrb, r->pool, r->connection->remote_logname);
}

static mrb_value ap_mrb_get_conn_local_ip(mrb_state *mrb, mrb_value str)
{
  request_rec *r = ap_mrb_get_request();
  return ap_mrb_str_to_value(mrb, r->pool, r->connection->local_ip);
}

static mrb_value ap_mrb_get_conn_local_port(mrb_state *mrb, mrb_value str)
{
  request_rec *r = ap_mrb_get_request();
  mrb_int val = (mrb_int)r->connection->local_addr->port;
  return mrb_fixnum_value(val);
}

static mrb_value ap_mrb_get_conn_local_host(mrb_state *mrb, mrb_value str)
{
  request_rec *r = ap_mrb_get_request();
  return ap_mrb_str_to_value(mrb, r->pool, r->connection->local_host);
}

// int write
/*
static mrb_value ap_mrb_set_server_loglevel(mrb_state *mrb, mrb_value str)
{
  mrb_int val;
  request_rec *r = ap_mrb_get_request();
  mrb_get_args(mrb, "i", &val);
#ifdef __APACHE24__
  r->server->log.level = (int)val;
#else
  r->server->loglevel = (int)val;
#endif
  return str;
}

// int read
static mrb_value ap_mrb_get_server_loglevel(mrb_state *mrb, mrb_value str)
{
  request_rec *r = ap_mrb_get_request();
#ifdef __APACHE24__
  return mrb_fixnum_value(r->server->log.level);
#else
  return mrb_fixnum_value(r->server->loglevel);
#endif
}
*/

static mrb_value ap_mrb_get_conn_keepalives(mrb_state *mrb, mrb_value str)
{
  request_rec *r = ap_mrb_get_request();
  return mrb_fixnum_value(r->connection->keepalives);
}

static mrb_value ap_mrb_get_conn_data_in_input_filters(mrb_state *mrb, 
    mrb_value str)
{
  request_rec *r = ap_mrb_get_request();
  return mrb_fixnum_value(r->connection->data_in_input_filters);
}

void ap_mruby_conn_init(mrb_state *mrb, struct RClass *class_core)
{
  struct RClass *class_conn;

  class_conn = mrb_define_class_under(mrb, class_core, "Connection", mrb->object_class);
  mrb_define_method(mrb, class_conn, "remote_ip", ap_mrb_get_conn_remote_ip, MRB_ARGS_NONE());
  mrb_define_method(mrb, class_conn, "remote_port", ap_mrb_get_conn_remote_port, MRB_ARGS_NONE());
  mrb_define_method(mrb, class_conn, "remote_host", ap_mrb_get_conn_remote_host, MRB_ARGS_NONE());
  mrb_define_method(mrb, class_conn, "remote_logname", ap_mrb_get_conn_remote_logname, MRB_ARGS_NONE());
  mrb_define_method(mrb, class_conn, "local_ip", ap_mrb_get_conn_local_ip, MRB_ARGS_NONE());
  mrb_define_method(mrb, class_conn, "local_port", ap_mrb_get_conn_local_port, MRB_ARGS_NONE());
  mrb_define_method(mrb, class_conn, "local_host", ap_mrb_get_conn_local_host, MRB_ARGS_NONE());
  mrb_define_method(mrb, class_conn, "keepalives", ap_mrb_get_conn_keepalives, MRB_ARGS_NONE());
  mrb_define_method(mrb, class_conn, "data_in_input_filters", ap_mrb_get_conn_data_in_input_filters, MRB_ARGS_NONE());
}
