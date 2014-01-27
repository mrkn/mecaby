#include <mecab.h>

#include <ruby/ruby.h>
#include <ruby/encoding.h>

#ifndef UNREACHABLE
# define UNREACHABLE	/* unreachable */
#endif

#ifdef PRIsVALUE
# define RB_CLASSNAME(klass) klass
#else
# define PRIsVALUE "s"
# define RB_CLASSNAME(klass) rb_class2name(klass)
#endif

#ifndef RARRAY_CONST_PTR
# define RARRAY_CONST_PTR(a) RARRAY_PTR(a)
#endif
#ifndef RARRAY_AREF
# define RARRAY_AREF(a, i) (RARRAY_CONST_PTR(a)[i])
#endif

#define DEFINE_GETTER_AND_CHECKER(type, klass) \
void \
check_##type##_initialized(mecaby_##type##_t* ptr, VALUE obj, VALUE err) \
{ \
  if (ptr->type == NULL) { \
    rb_raise(err, "uninitialized %"PRIsVALUE, RB_CLASSNAME(rb_obj_class(obj))); \
  } \
} \
static mecaby_##type##_t* \
get_##type(VALUE obj) \
{ \
  mecaby_##type##_t* ptr; \
  TypedData_Get_Struct(obj, mecaby_##type##_t, &mecaby_##type##_data_type, ptr); \
  return ptr; \
} \
 \
static mecaby_##type##_t* \
get_##type##_initialized(VALUE obj) \
{ \
  mecaby_##type##_t* ptr = get_##type(obj); \
  return ptr->type != NULL ? ptr : NULL; \
} \
 \
mecaby_##type##_t* \
check_get_##type(VALUE obj) \
{ \
  if (!rb_typeddata_is_kind_of(obj, &mecaby_##type##_data_type)) { \
    rb_raise(rb_eTypeError, "%"PRIsVALUE" object is required but %"PRIsVALUE" is given", \
             RB_CLASSNAME(mecaby_c##klass), RB_CLASSNAME(rb_obj_class(obj))); \
  } \
  return DATA_PTR(obj); \
} \
 \
mecaby_##type##_t* \
check_get_##type##_initialized(VALUE obj, VALUE err) \
{ \
  mecaby_##type##_t* ptr = check_get_##type(obj); \
  check_##type##_initialized(ptr, obj, err); \
  return ptr; \
}

/*
 * Types
 */

#ifdef HAVE_MECAB_MODEL_NEW
typedef struct mecaby_model {
  VALUE arg;
  mecab_model_t* model;
} mecaby_model_t;

typedef struct mecaby_lattice {
  VALUE generator;
  mecab_lattice_t* lattice;
} mecaby_lattice_t;
#endif

typedef struct mecaby_tagger {
  VALUE generator;
  mecab_t* tagger;
} mecaby_tagger_t;

typedef struct mecaby_dictionary_info {
  VALUE generator;
  mecab_dictionary_info_t const* dictionary_info;
} mecaby_dictionary_info_t;

typedef struct mecaby_node {
  VALUE generator;
  mecab_node_t const* node;
} mecaby_node_t;

typedef struct mecaby_path {
  VALUE generator;
  mecab_path_t const* path;
} mecaby_path_t;

/*
 * Global objects
 */

static VALUE mecaby_mMecaby;
static VALUE mecaby_eError;
static VALUE mecaby_eRuntimeError;
#ifdef HAVE_MECAB_MODEL_NEW
static VALUE mecaby_cModel;
static VALUE mecaby_cLattice;
#endif
static VALUE mecaby_cTagger;
static VALUE mecaby_cDictionaryInfo;
static VALUE mecaby_cNode;
static VALUE mecaby_cPath;

/*
 * Pointer-to-object map
 */

#if SIZEOF_VALUE == SIZEOF_LONG || (SIZEOF_VALUE == SIZEOF_UINTPTR_T && SIZEOF_UINTPTR_T == SIZEOF_LONG)
# define VALUE_TO_NUM(obj) ULONG2NUM((unsigned long)obj >> 2)
# define POINTER_TO_NUM(ptr) VALUE_TO_NUM((VALUE)ptr)
# define NUM_TO_VALUE(num) ((VALUE)NUM2ULONG(num))
#elif SIZEOF_VALUE == SIZEOF_LONG_LONG || (SIZEOF_VALUE == SIZEOF_UINTPTR_T && SIZEOF_UINTPTR_T == SIZEOF_LONG_LONG)
# define VALUE_TO_NUM(obj) ULL2NUM((unsigned LONG_LONG)obj >> 2)
# define POINTER_TO_NUM(ptr) VALUE_TO_NUM((VALUE)ptr)
# define NUM_TO_VALUE(num) ((VALUE)NUM2ULL(num))
#else
# error ---->> assume sizeof(void*) == sizeof(long) or sizeof(LONG_LONG) to be compiled. <<----
#endif

static VALUE mecaby_pointer_object_map = Qnil;

static void
mecaby_init_pointer_object_map()
{
  mecaby_pointer_object_map = rb_hash_new();
  rb_gc_register_address(&mecaby_pointer_object_map);
}

static VALUE
mecaby_lookup_object(void const* ptr)
{
  VALUE key, val;
  
  if (ptr == NULL) return Qnil;

  key = POINTER_TO_NUM(ptr);
  val = rb_hash_aref(mecaby_pointer_object_map, key);

  return NIL_P(val) ? Qnil : NUM_TO_VALUE(val);
}

static void
mecaby_register_pointer_object(void const* ptr, VALUE obj)
{
  VALUE key, val;

  key = POINTER_TO_NUM(ptr);
  val = VALUE_TO_NUM(obj);
  rb_hash_aset(mecaby_pointer_object_map, key, val);
}

static void
mecaby_unregister_pointer(void const* ptr)
{
  VALUE key;

  key = POINTER_TO_NUM(ptr);
  rb_hash_delete(mecaby_pointer_object_map, key);
}

/*
 * Typed data preparations
 */

#ifdef HAVE_MECAB_MODEL_NEW
static void
mecaby_model_mark(void *ptr)
{
  mecaby_model_t* model = ptr;

  if (model != NULL) {
    rb_gc_mark(model->arg);
  }
}

static void
mecaby_model_free(void *ptr)
{
  mecaby_model_t* model = ptr;

  if (model != NULL) {
    if (model->model != NULL) {
      mecaby_unregister_pointer(model->model);
      mecab_model_destroy(model->model);
    }
    model->arg = Qnil;
    xfree(model);
  }
}

static size_t
mecaby_model_memsize(void const *ptr)
{
  return sizeof(mecaby_model_t);
}

static const rb_data_type_t mecaby_model_data_type = {
  "Mecaby::Model",
  {
    mecaby_model_mark,
    mecaby_model_free,
    mecaby_model_memsize,
  }
#ifdef RUBY_TYPED_FREE_IMMEDIATELY
  , NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
#endif
};

#define MECABY_OBJ_IS_MODEL(obj) rb_typeddata_is_kind_of((obj), &mecaby_model_data_type)

DEFINE_GETTER_AND_CHECKER(model, Model);

static void
mecaby_lattice_mark(void *ptr)
{
  mecaby_lattice_t* lattice = ptr;

  if (lattice != NULL) {
    rb_gc_mark(lattice->generator);
  }
}

static void
mecaby_lattice_free(void *ptr)
{
  mecaby_lattice_t* lattice = ptr;

  if (lattice != NULL) {
    if (lattice->lattice != NULL) {
      mecaby_unregister_pointer(lattice->lattice);
      mecab_lattice_destroy(lattice->lattice);
    }
    lattice->generator = Qnil;
    xfree(lattice);
  }
}

static size_t
mecaby_lattice_memsize(void const *ptr)
{
  return sizeof(mecaby_lattice_t);
}

static const rb_data_type_t mecaby_lattice_data_type = {
  "Mecaby::Lattice",
  {
    mecaby_lattice_mark,
    mecaby_lattice_free,
    mecaby_lattice_memsize,
  }
#ifdef RUBY_TYPED_FREE_IMMEDIATELY
  , NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
#endif
};

#define MECABY_OBJ_IS_LATTICE(obj) rb_typeddata_is_kind_of((obj), &mecaby_lattice_data_type)

DEFINE_GETTER_AND_CHECKER(lattice, Lattice);
#endif /* HAVE_MECAB_MODEL_NEW */

static void
mecaby_tagger_mark(void *ptr)
{
  mecaby_tagger_t* tagger = ptr;

  if (tagger != NULL) {
    rb_gc_mark(tagger->generator);
  }
}

static void
mecaby_tagger_free(void *ptr)
{
  mecaby_tagger_t* tagger = ptr;

  if (tagger != NULL) {
    if (tagger->tagger != NULL) {
      mecaby_unregister_pointer(tagger->tagger);
      mecab_destroy(tagger->tagger);
    }
    tagger->generator = Qnil;
    xfree(tagger);
  }
}

static size_t
mecaby_tagger_memsize(void const *ptr)
{
  return sizeof(mecaby_tagger_t);
}

static const rb_data_type_t mecaby_tagger_data_type = {
  "mecaby::Tagger",
  {
    mecaby_tagger_mark,
    mecaby_tagger_free,
    mecaby_tagger_memsize,
  }
#ifdef RUBY_TYPED_FREE_IMMEDIATELY
  , NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
#endif
};

#define MECABY_OBJ_IS_TAGGER(obj) rb_typeddata_is_kind_of((obj), &mecaby_tagger_data_type)

DEFINE_GETTER_AND_CHECKER(tagger, Tagger);

static void
mecaby_dictionary_info_mark(void *ptr)
{
  mecaby_dictionary_info_t* di = ptr;

  if (di != NULL) {
    rb_gc_mark(di->generator);
  }
}

static void
mecaby_dictionary_info_free(void *ptr)
{
  mecaby_dictionary_info_t* di = ptr;

  if (di != NULL) {
    if (di->dictionary_info != NULL) {
      mecaby_unregister_pointer(di->dictionary_info);
      /* shouldn't free di->dictionary_info pointer. */
    }
    di->generator = Qnil;
    xfree(di);
  }
}

static size_t
mecaby_dictionary_info_memsize(void const *ptr)
{
  return sizeof(mecaby_dictionary_info_t);
}

static const rb_data_type_t mecaby_dictionary_info_data_type = {
  "Mecaby::DictionaryInfo",
  {
    mecaby_dictionary_info_mark,
    mecaby_dictionary_info_free,
    mecaby_dictionary_info_memsize,
  }
#ifdef RUBY_TYPED_FREE_IMMEDIATELY
  , NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
#endif
};

#define MECABY_OBJ_IS_DICTIONARY_INFO(obj) rb_typeddata_is_kind_of((obj), &mecaby_dictionary_info_data_type)

DEFINE_GETTER_AND_CHECKER(dictionary_info, DictionaryInfo);

static VALUE mecaby_create_dictionary_info(mecab_dictionary_info_t const*, VALUE);

static void
mecaby_node_mark(void *ptr)
{
  mecaby_node_t* node = ptr;
  if (node != NULL) {
    rb_gc_mark(node->generator);
  }
}

static void
mecaby_node_free(void *ptr)
{
  mecaby_node_t* node = ptr;
  if (node != NULL) {
    if (node->node) {
      mecaby_unregister_pointer(node->node);
      /* shouldn't free node->node pointer. */
    }
    node->generator = Qnil;
    xfree(node);
  }
}

static size_t
mecaby_node_memsize(void const *ptr)
{
  return sizeof(mecaby_node_t);
}

static const rb_data_type_t mecaby_node_data_type = {
  "Mecaby::Node",
  {
    mecaby_node_mark,
    mecaby_node_free,
    mecaby_node_memsize,
  }
#ifdef RUBY_TYPED_FREE_IMMEDIATELY
  , NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
#endif
};

#define MECABY_OBJ_IS_NODE(obj) rb_typeddata_is_kind_of((obj), &mecaby_node_data_type)

DEFINE_GETTER_AND_CHECKER(node, Node);

static VALUE mecaby_create_node(mecab_node_t const*, VALUE);

static void
mecaby_path_mark(void *ptr)
{
  mecaby_path_t* path = ptr;

  if (path != NULL) {
    rb_gc_mark(path->generator);
  }
}

static void
mecaby_path_free(void *ptr)
{
  mecaby_path_t* path = ptr;

  if (path != NULL) {
    if (path->path != NULL) {
      mecaby_unregister_pointer(path->path);
      /* shouldn't free path->path pointer. */
    }
    path->generator = Qnil;
    xfree(path);
  }
}

static size_t
mecaby_path_memsize(void const *ptr)
{
  return sizeof(mecaby_path_t);
}

static const rb_data_type_t mecaby_path_data_type = {
  "Mecaby::Path",
  {
    mecaby_path_mark,
    mecaby_path_free,
    mecaby_path_memsize,
  }
#ifdef RUBY_TYPED_FREE_IMMEDIATELY
  , NULL, NULL, RUBY_TYPED_FREE_IMMEDIATELY
#endif
};

#define MECABY_OBJ_IS_PATH(obj) rb_typeddata_is_kind_of((obj), &mecaby_path_data_type)

DEFINE_GETTER_AND_CHECKER(path, Path);

/*
 * Allocate methods
 */

#ifdef HAVE_MECAB_MODEL_NEW
static VALUE
mecaby_model_s_allocate(VALUE klass)
{
  mecaby_model_t* model;
  VALUE obj = TypedData_Make_Struct(klass, mecaby_model_t, &mecaby_model_data_type, model);
  model->arg = Qnil;
  model->model = NULL;
  return obj;
}

static VALUE
mecaby_lattice_s_allocate(VALUE klass)
{
  mecaby_lattice_t* lattice;
  VALUE obj = TypedData_Make_Struct(klass, mecaby_lattice_t, &mecaby_lattice_data_type, lattice);
  lattice->generator = Qnil;
  lattice->lattice = NULL;
  return obj;
}
#endif /* HAVE_MECAB_MODEL_NEW */

static VALUE
mecaby_tagger_s_allocate(VALUE klass)
{
  mecaby_tagger_t* tagger;
  VALUE obj = TypedData_Make_Struct(klass, mecaby_tagger_t, &mecaby_tagger_data_type, tagger);
  tagger->generator = Qnil;
  tagger->tagger = NULL;
  return obj;
}

static VALUE
mecaby_dictionary_info_s_allocate(VALUE klass)
{
  mecaby_dictionary_info_t* di;
  VALUE obj = TypedData_Make_Struct(klass, mecaby_dictionary_info_t, &mecaby_dictionary_info_data_type, di);
  di->generator = Qnil;
  di->dictionary_info = NULL;
  return obj;
}

static VALUE
mecaby_node_s_allocate(VALUE klass)
{
  mecaby_node_t* node;
  VALUE obj = TypedData_Make_Struct(klass, mecaby_node_t, &mecaby_node_data_type, node);
  node->generator = Qnil;
  node->node = NULL;
  return obj;
}

static VALUE
mecaby_path_s_allocate(VALUE klass)
{
  mecaby_path_t* path;
  VALUE obj = TypedData_Make_Struct(klass, mecaby_path_t, &mecaby_path_data_type, path);
  path->generator = Qnil;
  path->path = NULL;
  return obj;
}

#ifdef HAVE_MECAB_MODEL_NEW
/*
 * Mecaby::Model
 */

static VALUE
mecaby_model_initialize(int argc, VALUE* argv, VALUE self)
{
  VALUE arg;
  mecaby_model_t* model = check_get_model(self);

  if (model->model != NULL) {
    rb_raise(rb_eRuntimeError, "already initialized");
  }

  rb_scan_args(argc, argv, "01", &arg);
  if (argc == 0) {
    model->model = mecab_model_new2("-C");
  }
  else {
    VALUE ary = rb_check_array_type(arg);

    if (NIL_P(ary)) {
      char const* str = StringValueCStr(arg);
      model->arg = rb_obj_dup(arg);
      model->model = mecab_model_new2(str);
    }
    else {
      int i, n;
      char** args;

      n = RARRAY_LEN(ary);
      args = ALLOCA_N(char*, n);
      for (i = 0; i < n; ++i) {
        VALUE item = RARRAY_AREF(ary, i);
        args[i] = StringValueCStr(item);
      }
      model->arg = rb_obj_dup(ary);
      model->model = mecab_model_new(n, args);
    }
  }

  if (!NIL_P(model->arg)) {
    rb_obj_freeze(model->arg);
  }

  mecaby_register_pointer_object(model->model, self);
  return self;
}

static VALUE
mecaby_model_inspect(VALUE self)
{
  VALUE str, cname;
  mecaby_model_t* model = check_get_model(self);

  if (NIL_P(model->arg)) {
    return rb_call_super(0, NULL);
  }

  cname = rb_class_name(CLASS_OF(self));
  if (model->model != NULL) {
    str = rb_sprintf("#<%"PRIsVALUE":%p arg=%"PRIsVALUE">", cname, (void*)self, rb_inspect(model->arg));
  }
  else {
    str = rb_sprintf("#<%"PRIsVALUE":%p uninitialized>", cname, (void*)self);
  }
  OBJ_INFECT(str, self);

  return str;
}

static VALUE
mecaby_model_dictionary_info(VALUE self)
{
  VALUE obj;
  mecab_dictionary_info_t const* mecab_di;
  mecaby_model_t* model = check_get_model_initialized(self, rb_eRuntimeError);

  mecab_di = mecab_model_dictionary_info(model->model);
  return mecaby_create_dictionary_info(mecab_di, self);
}

static VALUE
mecaby_model_transition_cost(VALUE self, VALUE rc_attr, VALUE lc_attr)
{
  return Qnil;
}

static VALUE
mecaby_model_create_tagger(VALUE self)
{
  VALUE obj;

  obj = rb_obj_alloc(mecaby_cTagger);
  rb_funcallv(obj, rb_intern("initialize"), 1, &self);

  return obj;
}

static VALUE
mecaby_model_create_lattice(VALUE self)
{
  VALUE obj;

  obj = rb_obj_alloc(mecaby_cLattice);
  rb_funcallv(obj, rb_intern("initialize"), 1, &self);

  return obj;
}

static VALUE
mecaby_model_swap(VALUE self, VALUE other)
{
  mecaby_model_t* model_self;
  mecaby_model_t* model_other;

  model_self = get_model_initialized(self);
  model_other = get_model_initialized(other);

  if (model_self != NULL && model_other != NULL) {
    mecab_model_swap(model_self->model, model_other->model);
  }

  return self;
}

/*
 * Mecaby::Lattice
 */

static VALUE
mecaby_lattice_initialize(int argc, VALUE* argv, VALUE self)
{
  VALUE arg;
  mecaby_lattice_t* lattice = check_get_lattice(self);

  if (lattice->lattice != NULL) {
    rb_raise(rb_eRuntimeError, "already initialized");
  }

  rb_scan_args(argc, argv, "01", &arg);
  if (argc == 0) {
    lattice->lattice = mecab_lattice_new();
  }
  else if (MECABY_OBJ_IS_MODEL(arg)) {
    mecaby_model_t* model = check_get_model_initialized(arg, rb_eArgError);
    lattice->generator = arg;
    lattice->lattice = mecab_model_new_lattice(model->model);
    OBJ_INFECT(self, arg);
  }
  else {
    rb_raise(rb_eArgError, "invalid argument: %"PRIsVALUE, rb_inspect(arg));
  }

  return self;
}

static VALUE
mecaby_lattice_sentence(VALUE self)
{
  char const* sentence;
  mecaby_lattice_t* lattice = check_get_lattice_initialized(self, rb_eRuntimeError);

  sentence = mecab_lattice_get_sentence(lattice->lattice);

  return rb_external_str_new_with_enc(sentence, strlen(sentence), rb_default_external_encoding());
}

static VALUE
mecaby_lattice_sentence_eq(VALUE self, VALUE vsentence)
{
  char const* sentence;
  mecaby_lattice_t* lattice = check_get_lattice_initialized(self, rb_eRuntimeError);

  sentence = StringValueCStr(vsentence);
  mecab_lattice_set_sentence(lattice->lattice, sentence);

  return vsentence;
}

static VALUE
mecaby_lattice_set_sentence(VALUE self, VALUE vsentence)
{
  mecaby_lattice_sentence_eq(self, vsentence);
  return self;
}

static VALUE
mecaby_lattice_to_s(VALUE self)
{
  char const* str;
  mecaby_lattice_t* lattice = check_get_lattice_initialized(self, rb_eRuntimeError);

  str = mecab_lattice_tostr(lattice->lattice);

  return rb_external_str_new_with_enc(str, strlen(str), rb_default_external_encoding());
}
#endif /* HAVE_MECAB_MODEL_NEW */

/*
 * Mecaby::Tagger
 */

static VALUE
mecaby_tagger_initialize(int argc, VALUE* argv, VALUE self)
{
  VALUE arg;
  mecaby_tagger_t* tagger = check_get_tagger(self);

  if (tagger->tagger != NULL) {
    rb_raise(rb_eRuntimeError, "already initialized");
  }

  rb_scan_args(argc, argv, "01", &arg);
  if (argc == 0) {
    tagger->tagger = mecab_new2("-C");
  }
  else {
    VALUE ary = rb_check_array_type(arg);

    if (NIL_P(ary)) {
#ifdef HAVE_MECAB_MODEL_NEW
      if (MECABY_OBJ_IS_MODEL(arg)) {
        mecaby_model_t* model = check_get_model_initialized(arg, rb_eArgError);
        tagger->generator = arg;
        tagger->tagger = mecab_model_new_tagger(model->model);
        OBJ_INFECT(self, arg);
      }
      else
#endif
      {
        char const* str = StringValueCStr(arg);
        tagger->generator = rb_obj_dup(arg);
        tagger->tagger = mecab_new2(str);
      }
    }
    else {
      int i, n;
      char** args;

      n = RARRAY_LEN(ary);
      args = ALLOCA_N(char*, n);
      for (i = 0; i < n; ++i) {
        VALUE item = RARRAY_AREF(ary, i);
        args[i] = StringValueCStr(item);
      }
      tagger->generator = rb_obj_dup(ary);
      tagger->tagger = mecab_new(n, args);
    }
  }

  if (tagger->tagger == NULL) {
    rb_raise(rb_eRuntimeError, "%s:%d: mecab_tagger_initialize: %s", __FILE__, __LINE__, mecab_strerror(NULL));
  }

  if (!NIL_P(tagger->generator)
#ifdef HAVE_MECAB_MODEL_NEW
      && !MECABY_OBJ_IS_MODEL(tagger->generator)
#endif
      ) {
    rb_obj_freeze(tagger->generator);
  }

  mecaby_register_pointer_object(tagger->tagger, self);
  return self;
}

static VALUE
mecaby_tagger_inspect(VALUE self)
{
  VALUE str, cname;
  mecaby_tagger_t* tagger = check_get_tagger(self);

  if (NIL_P(tagger->generator)) {
    return rb_call_super(0, NULL);
  }

  cname = rb_class_name(CLASS_OF(self));
  if (tagger->tagger != NULL) {
#ifdef HAVE_MECAB_MODEL_NEW
    if (MECABY_OBJ_IS_MODEL(tagger->generator)) {
      str = rb_sprintf("#<%"PRIsVALUE":%p model=%"PRIsVALUE">", cname, (void*)self, tagger->generator);
    }
    else
#endif
    {
      str = rb_sprintf("#<%"PRIsVALUE":%p arg=%"PRIsVALUE">", cname, (void*)self, rb_inspect(tagger->generator));
    }
  }
  else {
      str = rb_sprintf("#<%"PRIsVALUE":%p uninitialized>", cname, (void*)self);
  }
  OBJ_INFECT(str, self);

  return str;
}

static VALUE
mecaby_tagger_dictionary_info(VALUE self)
{
  VALUE obj;
  mecab_dictionary_info_t const* mecab_di;
  mecaby_tagger_t* tagger = check_get_tagger(self);

  mecab_di = mecab_dictionary_info(tagger->tagger);
  return mecaby_create_dictionary_info(mecab_di, self);
}

#ifdef HAVE_MECAB_MODEL_NEW
static VALUE
mecaby_tagger_parse_lattice(VALUE self, VALUE vlattice)
{
  int result;
  mecaby_tagger_t* tagger = check_get_tagger_initialized(self, rb_eRuntimeError);
  mecaby_lattice_t* lattice = check_get_lattice_initialized(vlattice, rb_eArgError);

  result = mecab_parse_lattice(tagger->tagger, lattice->lattice);

  return result ? Qtrue : Qfalse;
}
#endif

static VALUE
mecaby_tagger_parse_string(VALUE self, VALUE vinput)
{
  int result;
  const char* input;
  const char* output;
  mecaby_tagger_t* tagger = check_get_tagger_initialized(self, rb_eRuntimeError);

  input = StringValueCStr(vinput);
  output = mecab_sparse_tostr(tagger->tagger, input);

  return rb_external_str_new_with_enc(output, strlen(output), rb_default_external_encoding());
}

static VALUE
mecaby_tagger_parse(VALUE self, VALUE target)
{
#ifdef HAVE_MECAB_MODEL_NEW
  if (MECABY_OBJ_IS_LATTICE(target)) {
    return mecaby_tagger_parse_lattice(self, target);
  }
#endif

  return mecaby_tagger_parse_string(self, target);
}

static VALUE
mecaby_tagger_nbest_parse(VALUE self, VALUE vn, VALUE vinput)
{
  size_t n;
  const char* input;
  const char* output;
  mecaby_tagger_t* tagger = check_get_tagger_initialized(self, rb_eRuntimeError);

  input = StringValueCStr(vinput);
  n = NUM2SIZET(vn);
  output = mecab_nbest_sparse_tostr(tagger->tagger, n, input);
  if (output == NULL) {
    rb_raise(mecaby_eError, "%s", mecab_strerror(tagger->tagger));
  }

  return rb_external_str_new_with_enc(output, strlen(output), rb_default_external_encoding());
}

static VALUE
mecaby_tagger_nbest_init(VALUE self, VALUE vinput)
{
  const char* input;
  int result;
  mecaby_tagger_t* tagger = check_get_tagger_initialized(self, rb_eRuntimeError);

  input = StringValueCStr(vinput);
  result = mecab_nbest_init(tagger->tagger, input);

  return result ? Qtrue : Qfalse;
}

static VALUE
mecaby_tagger_nbest_next(VALUE self)
{
  char const* output;
  mecaby_tagger_t* tagger = check_get_tagger_initialized(self, rb_eRuntimeError);

  output = mecab_nbest_next_tostr(tagger->tagger);
  if (output == NULL) return Qnil;

  return rb_external_str_new_with_enc(output, strlen(output), rb_default_external_encoding());
}

static VALUE
mecaby_tagger_parse_to_node(VALUE self, VALUE vinput)
{
  char const* input;
  mecaby_tagger_t* tagger = check_get_tagger_initialized(self, rb_eRuntimeError);
  mecab_node_t const* mecab_node;

  input = StringValueCStr(vinput);
  mecab_node = mecab_sparse_tonode(tagger->tagger, input);

  return mecaby_create_node(mecab_node, self);
}

/*
 * Mecaby::DictionaryInfo
 */

static VALUE
mecaby_create_dictionary_info(mecab_dictionary_info_t const* mecab_di, VALUE generator)
{
  VALUE vdi;
  mecaby_dictionary_info_t* di;

  vdi = mecaby_lookup_object(mecab_di);
  if (!NIL_P(vdi)) return vdi;

  vdi = rb_obj_alloc(mecaby_cDictionaryInfo);
  di = get_dictionary_info(vdi);
  di->generator = generator;
  di->dictionary_info = mecab_di;
  OBJ_INFECT(vdi, generator);

  mecaby_register_pointer_object(mecab_di, vdi);
  return vdi;
}

static VALUE
mecaby_dictionary_info_next(VALUE self)
{
  mecaby_dictionary_info_t* di = check_get_dictionary_info_initialized(self, rb_eRuntimeError);

  if (di->dictionary_info->next == NULL) return Qnil;

  return mecaby_create_dictionary_info(di->dictionary_info->next, self);
}

static VALUE
mecaby_dictionary_info_filename(VALUE self)
{
  mecaby_dictionary_info_t* di = check_get_dictionary_info_initialized(self, rb_eRuntimeError);

  if (di->dictionary_info->filename == NULL) return Qnil;

  return rb_filesystem_str_new_cstr(di->dictionary_info->filename);
}

static VALUE
mecaby_dictionary_info_charset(VALUE self)
{
  mecaby_dictionary_info_t* di = check_get_dictionary_info_initialized(self, rb_eRuntimeError);

  if (di->dictionary_info->charset == NULL) return Qnil;

  return rb_external_str_new_with_enc(di->dictionary_info->charset, strlen(di->dictionary_info->charset), rb_default_external_encoding());
}

static VALUE
mecaby_dictionary_info_size(VALUE self)
{
  mecaby_dictionary_info_t* di = check_get_dictionary_info_initialized(self, rb_eRuntimeError);

  return UINT2NUM(di->dictionary_info->size);
}

static VALUE
mecaby_dictionary_info_type(VALUE self)
{
  mecaby_dictionary_info_t* di = check_get_dictionary_info_initialized(self, rb_eRuntimeError);

  switch (di->dictionary_info->type) {
    case MECAB_USR_DIC:
      return rb_intern("USR_DIC");

    case MECAB_SYS_DIC:
      return rb_intern("SYS_DIC");

    case MECAB_UNK_DIC:
      return rb_intern("UNK_DIC");
  }

  UNREACHABLE;
  return Qnil;
}

static VALUE
mecaby_dictionary_info_lsize(VALUE self)
{
  mecaby_dictionary_info_t* di = check_get_dictionary_info_initialized(self, rb_eRuntimeError);

  return UINT2NUM(di->dictionary_info->lsize);
}

static VALUE
mecaby_dictionary_info_rsize(VALUE self)
{
  mecaby_dictionary_info_t* di = check_get_dictionary_info_initialized(self, rb_eRuntimeError);

  return UINT2NUM(di->dictionary_info->rsize);
}

static VALUE
mecaby_dictionary_info_version(VALUE self)
{
  mecaby_dictionary_info_t* di = check_get_dictionary_info_initialized(self, rb_eRuntimeError);

  return UINT2NUM(di->dictionary_info->version);
}

/*
 * Mecaby::Node
 */

static VALUE
mecaby_create_node(mecab_node_t const* mecab_node, VALUE generator)
{
  VALUE vnode;
  mecaby_node_t* node;

  vnode = mecaby_lookup_object(mecab_node);
  if (!NIL_P(vnode)) return vnode;

  vnode = rb_obj_alloc(mecaby_cNode);
  node = get_node(vnode);
  node->generator = generator;
  node->node = mecab_node;
  OBJ_INFECT(vnode, generator);

  mecaby_register_pointer_object(mecab_node, vnode);
  return vnode;
}

static VALUE
mecaby_node_prev(VALUE self)
{
  mecaby_node_t* node = check_get_node_initialized(self, rb_eRuntimeError);

  if (node->node->prev == NULL) {
    return Qnil;
  }

  return mecaby_create_node(node->node->prev, self);
}

static VALUE
mecaby_node_next(VALUE self)
{
  mecaby_node_t* node = check_get_node_initialized(self, rb_eRuntimeError);

  if (node->node->next == NULL) {
    return Qnil;
  }

  return mecaby_create_node(node->node->next, self);
}

static VALUE
mecaby_node_surface(VALUE self)
{
  mecaby_node_t* node = check_get_node_initialized(self, rb_eRuntimeError);

  return rb_external_str_new_with_enc(node->node->surface, node->node->length, rb_default_external_encoding());
}

static VALUE
mecaby_node_feature(VALUE self)
{
  mecaby_node_t* node = check_get_node_initialized(self, rb_eRuntimeError);

  return rb_external_str_new_with_enc(node->node->feature, strlen(node->node->feature), rb_default_external_encoding());
}

#define DEFINE_NODE_STATUS_PREDICATOR(name, NAME) \
static VALUE \
mecaby_node_status_is_##name(VALUE self) \
{ \
  mecaby_node_t* node = check_get_node_initialized(self, rb_eRuntimeError); \
 \
  return node->node->stat == MECAB_##NAME##_NODE ? Qtrue : Qfalse; \
}

DEFINE_NODE_STATUS_PREDICATOR(nor, NOR);
DEFINE_NODE_STATUS_PREDICATOR(unk, UNK);
DEFINE_NODE_STATUS_PREDICATOR(bos, BOS);
DEFINE_NODE_STATUS_PREDICATOR(eos, EOS);
DEFINE_NODE_STATUS_PREDICATOR(eon, EON);

#undef DEFINE_NODE_STATUS_PREDICATOR

void
Init_mecaby(void)
{
  mecaby_init_pointer_object_map();

  mecaby_mMecaby = rb_define_module("Mecaby");
  rb_define_const(mecaby_mMecaby, "MECAB_VERSION", rb_str_new2(mecab_version()));

  mecaby_eError = rb_define_class_under(mecaby_mMecaby, "Error", rb_eStandardError);

#ifdef HAVE_MECAB_MODEL_NEW
  mecaby_cModel = rb_define_class_under(mecaby_mMecaby, "Model", rb_cData);
  rb_define_alloc_func(mecaby_cModel, mecaby_model_s_allocate);
  rb_define_method(mecaby_cModel, "initialize", mecaby_model_initialize, -1);
  rb_define_method(mecaby_cModel, "inspect", mecaby_model_inspect, 0);
  rb_define_method(mecaby_cModel, "dictionary_info", mecaby_model_dictionary_info, 0);
  rb_define_method(mecaby_cModel, "transition_cost", mecaby_model_transition_cost, 2);
  rb_define_method(mecaby_cModel, "create_tagger", mecaby_model_create_tagger, 0);
  rb_define_alias(mecaby_cModel, "createTagger", "create_tagger");
  rb_define_alias(mecaby_cModel, "new_tagger", "create_tagger");
  rb_define_method(mecaby_cModel, "create_lattice", mecaby_model_create_lattice, 0);
  rb_define_alias(mecaby_cModel, "createLattice", "create_lattice");
  rb_define_alias(mecaby_cModel, "new_lattice", "create_lattice");
  rb_define_method(mecaby_cModel, "swap", mecaby_model_swap, 1);

  mecaby_cLattice = rb_define_class_under(mecaby_mMecaby, "Lattice", rb_cData);
  rb_define_alloc_func(mecaby_cLattice, mecaby_lattice_s_allocate);
  rb_define_method(mecaby_cLattice, "initialize", mecaby_lattice_initialize, -1);
  rb_define_method(mecaby_cLattice, "sentence", mecaby_lattice_sentence, 0);
  rb_define_method(mecaby_cLattice, "sentence=", mecaby_lattice_sentence_eq, 1);
  rb_define_method(mecaby_cLattice, "set_sentence", mecaby_lattice_set_sentence, 1);
  rb_define_method(mecaby_cLattice, "to_s", mecaby_lattice_to_s, 0);
#endif /* HAVE_MECAB_MODEL_NEW */

  mecaby_cTagger = rb_define_class_under(mecaby_mMecaby, "Tagger", rb_cData);
  rb_define_alloc_func(mecaby_cTagger, mecaby_tagger_s_allocate);
  rb_define_method(mecaby_cTagger, "initialize", mecaby_tagger_initialize, -1);
  rb_define_method(mecaby_cTagger, "inspect", mecaby_tagger_inspect, 0);
  rb_define_method(mecaby_cTagger, "dictionary_info", mecaby_tagger_dictionary_info, 0);
  rb_define_method(mecaby_cTagger, "parse", mecaby_tagger_parse, 1);
  rb_define_method(mecaby_cTagger, "nbest_parse", mecaby_tagger_nbest_parse, 2);
  rb_define_method(mecaby_cTagger, "nbest_init", mecaby_tagger_nbest_init, 1);
  rb_define_method(mecaby_cTagger, "nbest_next", mecaby_tagger_nbest_next, 0);
  /*rb_define_method(mecaby_cTagger, "nbest_next_node", mecaby_tagger_nbest_next_node, 0);*/
  rb_define_method(mecaby_cTagger, "parse_to_node", mecaby_tagger_parse_to_node, 1);

  mecaby_cDictionaryInfo = rb_define_class_under(mecaby_mMecaby, "DictionaryInfo", rb_cData);
  rb_define_alloc_func(mecaby_cDictionaryInfo, mecaby_dictionary_info_s_allocate);
  rb_define_method(mecaby_cDictionaryInfo, "next", mecaby_dictionary_info_next, 0);
  rb_define_method(mecaby_cDictionaryInfo, "filename", mecaby_dictionary_info_filename, 0);
  rb_define_method(mecaby_cDictionaryInfo, "charset", mecaby_dictionary_info_charset, 0);
  rb_define_method(mecaby_cDictionaryInfo, "size", mecaby_dictionary_info_size, 0);
  rb_define_method(mecaby_cDictionaryInfo, "type", mecaby_dictionary_info_type, 0);
  rb_define_method(mecaby_cDictionaryInfo, "lsize", mecaby_dictionary_info_lsize, 0);
  rb_define_method(mecaby_cDictionaryInfo, "rsize", mecaby_dictionary_info_rsize, 0);
  rb_define_method(mecaby_cDictionaryInfo, "version", mecaby_dictionary_info_version, 0);

  mecaby_cNode = rb_define_class_under(mecaby_mMecaby, "Node", rb_cData);
  rb_define_alloc_func(mecaby_cNode, mecaby_node_s_allocate);
  rb_define_method(mecaby_cNode, "prev", mecaby_node_prev, 0);
  rb_define_method(mecaby_cNode, "next", mecaby_node_next, 0);
  rb_define_method(mecaby_cNode, "surface", mecaby_node_surface, 0);
  rb_define_method(mecaby_cNode, "feature", mecaby_node_feature, 0);
  rb_define_method(mecaby_cNode, "status_nor?", mecaby_node_status_is_nor, 0);
  rb_define_method(mecaby_cNode, "status_unk?", mecaby_node_status_is_unk, 0);
  rb_define_method(mecaby_cNode, "status_bos?", mecaby_node_status_is_bos, 0);
  rb_define_method(mecaby_cNode, "status_eos?", mecaby_node_status_is_eos, 0);
  rb_define_method(mecaby_cNode, "status_eon?", mecaby_node_status_is_eon, 0);

  mecaby_cPath = rb_define_class_under(mecaby_mMecaby, "Path", rb_cData);
  rb_define_alloc_func(mecaby_cPath, mecaby_path_s_allocate);
}
