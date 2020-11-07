#include <string.h>
#include "type.h"
#include "instr.h"
#include "plug.h"
#include "import.h"

static MFUN(gwf_pair_set) {
  const m_uint sz = sizeof(o->data);
  memcpy(o->data, MEM(SZ_INT), sz);
}

GWION_IMPORT(pair) {
  const m_str list[] = { "A" , "B"};
  const Type t_pair = gwi_mk_type(gwi, "Pair", SZ_INT, t_object);
  GWI_BB(gwi_tmpl_ini(gwi, 2, list))
  GWI_BB(gwi_class_ini(gwi, t_pair, NULL, NULL))
  GWI_BB(gwi_tmpl_end(gwi))
  GWI_BB(gwi_item_ini(gwi, "A", "key"))
  GWI_BB(gwi_item_end(gwi, 0, NULL))
  GWI_BB(gwi_item_ini(gwi, "B", "val"))
  GWI_BB(gwi_item_end(gwi, 0, NULL))
  GWI_BB(gwi_func_ini(gwi, "void", "set", gwf_pair_set))
  GWI_BB(gwi_func_arg(gwi, "A", "key"))
  GWI_BB(gwi_func_arg(gwi, "B", "val"))
  GWI_BB(gwi_func_end(gwi, 0))
  GWI_BB(gwi_class_end(gwi))
  return GW_OK;
}
