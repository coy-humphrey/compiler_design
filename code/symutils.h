// Coy Humphrey 

#ifndef __SYMUTILS__
#define __SYMUTILS__

#include <bitset>
#include <string>
#include <unordered_map>

using namespace std;

#include <stdio.h>

#include "astree.h"
#include "auxlib.h"

enum
{
   ATTR_void, ATTR_bool, ATTR_char, ATTR_int, ATTR_null, ATTR_string,
   ATTR_struct, ATTR_array, ATTR_function, ATTR_prototype,ATTR_variable,
   ATTR_field, ATTR_typeid, ATTR_param, ATTR_lval, ATTR_const,
   ATTR_vreg, ATTR_vaddr, ATTR_bitset_size,
};
using attr_bitset = bitset<ATTR_bitset_size>;

struct symbol;
using symbol_table = unordered_map<const string*, symbol*>;
using symbol_entry = pair<const string*, symbol*>;
struct symbol
{
   attr_bitset attributes;
   // When comparing structs, compare this field pointer
   // If field pointers are the same, they point to the same element
   // in the symbol_table so they must have the same typeid
   symbol_table* fields;
   const string* struct_name;
   const string* owning_struct;
   size_t filenr, linenr, offset;
   size_t block_nr;
   vector<symbol*>* parameters;
};

int type_check (struct astree* root, FILE* output);

#endif
