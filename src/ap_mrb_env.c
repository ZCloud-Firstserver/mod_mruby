/*
// ap_mrb_env.c - to provide env methods
//
// See Copyright Notice in mod_mruby.h
*/

#include "ap_mrb_env.h"
#include "mruby/hash.h"
#include "util_script.h"

static mrb_value ap_mrb_init_env(mrb_state *mrb, mrb_value self)
{
  request_rec *r = ap_mrb_get_request();
  ap_add_common_vars(r);
  ap_add_cgi_vars(r);
  ap_mrb_push_request(r);

  return self;
}

static mrb_value ap_mrb_set_env(mrb_state *mrb, mrb_value str)
{
  mrb_value key, val;

  request_rec *r = ap_mrb_get_request();
  apr_table_t *e = r->subprocess_env;

  mrb_get_args(mrb, "oo", &key, &val);
  apr_table_setn(e, mrb_str_to_cstr(mrb, key), mrb_str_to_cstr(mrb, val));
  return val;
}

static mrb_value ap_mrb_get_env(mrb_state *mrb, mrb_value str)
{
  mrb_value key;
  const char *val;

  request_rec *r = ap_mrb_get_request();
  apr_table_t *e = r->subprocess_env;

  mrb_get_args(mrb, "o", &key);
  val = apr_table_get(e, mrb_str_to_cstr(mrb, key));
  if (val == NULL)
    return mrb_nil_value();
  return mrb_str_new(mrb, val, strlen(val));
}

static mrb_value ap_mrb_get_env_hash(mrb_state *mrb, mrb_value str)
{
  int i, ai;
  mrb_value hash = mrb_hash_new(mrb);
  request_rec *r = ap_mrb_get_request();
  apr_table_t *e = r->subprocess_env;
  const apr_array_header_t *arr = apr_table_elts(e);
  apr_table_entry_t *elts = (apr_table_entry_t *)arr->elts;
  ai = mrb_gc_arena_save(mrb);
  for (i = 0; i < arr->nelts; i++) {
    mrb_hash_set(mrb, hash, mrb_str_new(mrb, elts[i].key, strlen(elts[i].key)),
                 mrb_str_new(mrb, elts[i].val, strlen(elts[i].val)));
    mrb_gc_arena_restore(mrb, ai);
  }
  return hash;
}

void ap_mruby_env_init(mrb_state *mrb, struct RClass *class_core)
{
  struct RClass *class_env;

  int ai = mrb_gc_arena_save(mrb);
  class_env = mrb_define_class_under(mrb, class_core, "Env", mrb->object_class);
  mrb_define_method(mrb, class_env, "initialize", ap_mrb_init_env,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb, class_env, "[]=", ap_mrb_set_env, MRB_ARGS_ANY());
  mrb_define_method(mrb, class_env, "[]", ap_mrb_get_env, MRB_ARGS_ANY());
  mrb_define_method(mrb, class_env, "all", ap_mrb_get_env_hash,
                    MRB_ARGS_NONE());
  mrb_gc_arena_restore(mrb, ai);
}
