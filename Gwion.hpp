extern "C"
{
#include "defs.h"
#include "err_msg.h"
//typedef _Complex complex;
extern m_int o_string_data;
#ifndef __ARRAY
#define __ARRAY
#include <complex.h>
#include "map.h"

typedef struct VM_Vector_* VM_Vector;
typedef struct {
  m_uint length;
  m_uint depth;
  Type type;
  m_bool is_obj;
  m_bool is_ref;
  m_uint stack_offset;
} VM_Array_Info;

struct VM_Array_ {
  M_Vector* vector;
//  Map    map;
  Type type;
  m_bool is_obj;
};

m_uint    m_vector_size(M_Vector* v);
m_uint    i_vector_at  (M_Vector* v, m_uint i);
m_float   f_vector_at  (M_Vector* v, m_uint i);
/*
#ifdef USE_DOUBLE
complex double  c_vector_at  (M_Vector* v, m_uint i);
#else
complex float  c_vector_at  (M_Vector* v, m_uint i);
#endif
*/
m_vec3   v3_vector_at  (M_Vector* v, m_uint i);
m_vec4   v4_vector_at  (M_Vector* v, m_uint i);
m_uint*   i_vector_addr(M_Vector* v, m_uint i);
m_float*  f_vector_addr(M_Vector* v, m_uint i);

_Complex c_vector_addr(M_Vector* v, m_uint i);
m_vec3*  v3_vector_addr(M_Vector* v, m_uint i);
m_vec4*  v4_vector_addr(M_Vector* v, m_uint i);
void      i_vector_set (M_Vector* v, m_uint i, m_uint  data);
void      f_vector_set (M_Vector* v, m_uint i, m_float data);
void      c_vector_set (M_Vector* v, m_uint i, _Complex data);
void     v3_vector_set (M_Vector* v, m_uint i, m_vec3  data);
void     v4_vector_set (M_Vector* v, m_uint i, m_vec4  data);

M_Object new_M_Array  (m_uint size, m_uint length);
#endif

#include "type.h"
#include "dl.h"
#include "bbq.h"
extern m_bool initialize_object(M_Object o, Type t);
extern Type import_class_begin(Env env, Type type, Nspc where, f_xtor pre_ctor, f_xtor dtor);
extern m_bool import_class_end(Env env);
extern m_bool add_global_type(Env env, Type t);
m_int import_mvar(Env env, const m_str type,
  const m_str name, m_bool is_const, m_bool is_ref, const m_str doc );
extern Func import_mfun(Env env, DL_Func * mfun);


}

typedef struct VM_Shred_* VM_Shred;
//#define import_mvar(a, b, c, d, e, f) import_mvar(a, b , c, d, e, f)
#define dl_func_add_arg(a, b, c)      dl_func_add_arg(a, (m_str)b , (m_str)c) 
#define new_DL_Func(a, b, c)          new_DL_Func((m_str)a, (m_str)b, (m_uint)c)               
extern "C" {
extern M_Object new_M_Object(VM_Shred shred);
extern M_Object new_M_UGen(VM_Shred shred);
}
