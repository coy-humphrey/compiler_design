// Coy Humphrey 

#include "symutils.h"

#include <bitset>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

#include <stdio.h>

#include "astree.h"
#include "auxlib.h"
#include "stringset.h"
#include "lyutils.h"
#include "yyparse.h"

vector<symbol_table*> symbol_stack;
vector<size_t> blocknum_stack;
size_t next_block = 1;
symbol_table struct_table;

attr_bitset return_type;
symbol_table* return_fields;

const string* curr_struct = nullptr;
bool in_array = false;

bool in_function_header = false;

FILE* out;

int visit (astree* root);


void print_sym (const string *name, symbol* sym)
{
   attr_bitset attr = sym->attributes;
   if (sym->block_nr)
   {
      fprintf (out, "   ");
   }
   fprintf (out, "%s ", name->c_str());
   fprintf (out, 
      "(%ld.%ld.%ld) ", sym->filenr, sym->linenr, sym->offset);
   fprintf (out, "{%ld} ", sym->block_nr);
   if (attr[ATTR_struct]) fprintf (out, "struct ");
   if (attr[ATTR_array]) fprintf (out, "array ");
   if (attr[ATTR_bool]) fprintf (out, "bool ");
   if (attr[ATTR_char]) fprintf (out, "char ");
   if (attr[ATTR_int]) fprintf (out, "int ");
   if (attr[ATTR_string]) fprintf (out, "string ");
   if (attr[ATTR_variable]) fprintf (out, "variable ");
   if (attr[ATTR_lval]) fprintf (out, "lval ");
   if (attr[ATTR_param]) fprintf (out, "param");
   fprintf (out, "\n");
}

void print_struct_sym (const string* name, symbol* sym)
{
   fprintf (out, "%s ", name->c_str());
   fprintf (out, 
      "(%ld.%ld.%ld) ", sym->filenr, sym->linenr, sym->offset);
   fprintf (out, "{%ld} ", sym->block_nr);
   fprintf (out, "struct \"%s\"", name->c_str());
   fprintf (out, "\n");
}

void print_func_sym (const string* name, symbol* sym)
{
   attr_bitset attr = sym->attributes;
   fprintf (out, "%s ", name->c_str());
   fprintf (out, 
      "(%ld.%ld.%ld) ", sym->filenr, sym->linenr, sym->offset);
   fprintf (out, "{%ld} ", sym->block_nr);
   if (attr[ATTR_struct]) fprintf (out, "struct ");
   if (attr[ATTR_array]) fprintf (out, "array ");
   if (attr[ATTR_bool]) fprintf (out, "bool ");
   if (attr[ATTR_char]) fprintf (out, "char ");
   if (attr[ATTR_int]) fprintf (out, "int ");
   if (attr[ATTR_string]) fprintf (out, "string ");
   if (attr[ATTR_variable]) fprintf (out, "variable ");
   if (attr[ATTR_lval]) fprintf (out, "lval ");
   fprintf (out, "function\n");
}

int visit_root (astree* root)
{
   for ( auto itor : root->children )
   {
      if (visit (itor)) return 1;
   }
   return 0;
}

int visit_binop (astree* root)
{
   if (visit (root->children[0])) return 1;
   if (visit (root->children[1])) return 1;
   
   if ((root->children[0]->sym->attributes[ATTR_int] &&
       root->children[1]->sym->attributes[ATTR_int]) &&
       !(root->children[0]->sym->attributes[ATTR_array] ||
       root->children[1]->sym->attributes[ATTR_array]))
   {
      root->sym->attributes[ATTR_int] = true;
      root->sym->attributes[ATTR_vreg] = true;
   }
   else
   {
      errprintf ("(%ld,%ld,%ld): binop: Incompatible types\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   return 0;
}

int visit_int_unop (astree* root)
{
   if (visit (root->children[0])) return 1;
   
   if (root->children[0]->sym->attributes[ATTR_int] &&
       !root->children[0]->sym->attributes[ATTR_array])
   {
      root->sym->attributes[ATTR_int] = true;
      root->sym->attributes[ATTR_vreg] = true;
   }
   else
   {
      errprintf ("(%ld,%ld,%ld): unop: Incompatible types\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   return 0;
}

int visit_not (astree* root)
{
   if (visit (root->children[0])) return 1;
   
   if (root->children[0]->sym->attributes[ATTR_bool] &&
       !root->children[0]->sym->attributes[ATTR_array])
   {
      root->sym->attributes[ATTR_bool] = true;
      root->sym->attributes[ATTR_vreg] = true;
   }
   else
   {
      errprintf ("(%ld,%ld,%ld): not: Incompatible types\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   return 0;
}

int visit_chr (astree* root)
{
   if (visit (root->children[0])) return 1;
   
   if (root->children[0]->sym->attributes[ATTR_int] &&
       !root->children[0]->sym->attributes[ATTR_array])
   {
      root->sym->attributes[ATTR_char] = true;
      root->sym->attributes[ATTR_vreg] = true;
   }
   else
   {
      errprintf ("(%ld,%ld,%ld): chr: Incompatible types\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   return 0;
}

int visit_ord (astree* root)
{
   if (visit (root->children[0])) return 1;
   
   if (root->children[0]->sym->attributes[ATTR_char] &&
       !root->children[0]->sym->attributes[ATTR_array])
   {
      root->sym->attributes[ATTR_int] = true;
      root->sym->attributes[ATTR_vreg] = true;
   }
   else
   {
      errprintf ("(%ld,%ld,%ld): ord: Incompatible types\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   return 0;
}

int visit_while (astree* root)
{
   if (visit (root->children[0])) return 1;
   if (visit (root->children[1])) return 1;
   
   if (root->children[0]->sym->attributes[ATTR_bool] &&
       !root->children[0]->sym->attributes[ATTR_array])
   {
      
   }
   else
   {
      errprintf ("(%ld,%ld,%ld): while: Incompatible types\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   return 0;
}

int visit_block (astree* root)
{
   symbol_stack.push_back (nullptr);
   blocknum_stack.push_back (next_block++);
   
   root->sym->block_nr = blocknum_stack.back();
   
   for (auto itor : root->children)
   {
      if (visit (itor)) return 1;
   }
   
   symbol_stack.pop_back();
   blocknum_stack.pop_back();
   return 0;
}

int visit_intcon (astree* root)
{
   root->sym->attributes[ATTR_int] = true;
   root->sym->attributes[ATTR_const] = true;
   return 0;
}

int visit_charcon (astree* root)
{
   root->sym->attributes[ATTR_char] = true;
   root->sym->attributes[ATTR_const] = true;
   return 0;
}

int visit_stringcon (astree* root)
{
   root->sym->attributes[ATTR_string] = true;
   root->sym->attributes[ATTR_const] = true;
   return 0;
}

int visit_boolcon (astree* root)
{
   root->sym->attributes[ATTR_bool] = true;
   root->sym->attributes[ATTR_const] = true;
   return 0;
}

int visit_nullcon (astree* root)
{
   root->sym->attributes[ATTR_null] = true;
   root->sym->attributes[ATTR_const] = true;
   return 0;
}

// Works for both if and ifelse
int visit_if (astree* root)
{
   for (auto itor : root->children)
   {
      if (visit (itor)) return 1;
   }
   
   if (root->children[0]->sym->attributes[ATTR_bool] &&
       !root->children[0]->sym->attributes[ATTR_array])
   {
      
   }
   else
   {
      errprintf ("(%ld,%ld,%ld): if: Incompatible types\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   return 0;
}



int visit_struct (astree* root)
{
   const string* type_name = root->children[0]->lexinfo;
   curr_struct = type_name;
   // count returns number of instances of type_name in set.
   // either 1 if type_name is in set, or 0 if not
   // If struct has been declared, but no fields have been set
   // or if struct has not been declared
   if ((struct_table.count (type_name) && 
       struct_table.at (type_name) == nullptr) ||
       !struct_table.count (type_name))
   {
      root->sym->fields = new symbol_table();
      struct_table.insert ({type_name, root->sym});
      root->sym->struct_name = type_name;
      root->sym->owning_struct = type_name;
      print_struct_sym (type_name, root->sym);
   }
   else
   {
      errprintf ("(%ld,%ld,%ld): Duplicate struct declaration\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   
   // Process fielddecls
   for (size_t itor = 1; itor < root->children.size(); ++itor)
   {
      if (visit (root->children[itor])) return 1;
   }
   
   curr_struct = nullptr;
   return 0;
}

int visit_void (astree* root)
{
   for ( auto itor : root->children ) 
      if (visit (itor)) return 1;
   root->sym->attributes[ATTR_void] = true;
   return 0;
}

int visit_bool (astree* root)
{
   for ( auto itor : root->children ) 
      if (visit (itor)) return 1;
   // Check if in struct, if not add to stack, else to struct table
   // If no children, let parent handle things
   root->sym->attributes[ATTR_bool] = true;
   if (!(root->children.size() == 0))
   {
      root->children[0]->sym->attributes[ATTR_bool] = true;
      // root->children[0]->sym->attributes[ATTR_field] = true;
      // Check if in struct, if not add to stack, else to struct table
      if (curr_struct != nullptr)
      {
         root->children[0]->sym->owning_struct = curr_struct;
         struct_table.at (curr_struct)->fields->insert 
            ({root->children[0]->lexinfo, root->children[0]->sym});
         print_sym (root->children[0]->lexinfo, root->children[0]->sym);
      }
      else
      {
         if (in_function_header) return 0;
         if (symbol_stack.back() == nullptr)
         {
            symbol_stack.pop_back();
            symbol_stack.push_back (new symbol_table());
         }
         root->children[0]->sym->attributes[ATTR_variable] = true;
         root->children[0]->sym->attributes[ATTR_lval] = true;
         symbol_stack.back()->insert 
            ({root->children[0]->lexinfo, root->children[0]->sym});
         print_sym (root->children[0]->lexinfo, root->children[0]->sym);
      }
   }
   return 0;
}

// For this, array, struct and other primitive alter code to not
// do complex things if in function header
int visit_char (astree* root)
{
   for ( auto itor : root->children ) 
      if (visit (itor)) return 1;
   // Check if in struct, if not add to stack, else to struct table
   // If no children, let parent handle things
   root->sym->attributes[ATTR_char] = true;
   if (!(root->children.size() == 0))
   {
      root->children[0]->sym->attributes[ATTR_char] = true;
      // root->children[0]->sym->attributes[ATTR_field] = true;
      // Check if in struct, if not add to stack, else to struct table
      if (curr_struct != nullptr)
      {
         root->children[0]->sym->owning_struct = curr_struct;
         struct_table.at (curr_struct)->fields->insert 
            ({root->children[0]->lexinfo, root->children[0]->sym});
         print_sym (root->children[0]->lexinfo,root->children[0]->sym);
      }
      else
      {
         if (in_function_header) return 0;
         if (symbol_stack.back() == nullptr)
         {
            symbol_stack.pop_back();
            symbol_stack.push_back (new symbol_table());
         }
         root->children[0]->sym->attributes[ATTR_variable] = true;
         root->children[0]->sym->attributes[ATTR_lval] = true;
         symbol_stack.back()->insert 
            ({root->children[0]->lexinfo, root->children[0]->sym});
         print_sym (root->children[0]->lexinfo,root->children[0]->sym);
      }
   }
   return 0;
}

int visit_int (astree* root)
{
   for ( auto itor : root->children ) 
      if (visit (itor)) return 1;
   // Check if in struct, if not add to stack, else to struct table
   // If no children, let parent handle things
   root->sym->attributes[ATTR_int] = true;
   if (!(root->children.size() == 0))
   {
      root->children[0]->sym->attributes[ATTR_int] = true;
      // root->children[0]->sym->attributes[ATTR_field] = true;
      // Check if in struct, if not add to stack, else to struct table
      if (curr_struct != nullptr)
      {
         root->children[0]->sym->owning_struct = curr_struct;
         struct_table.at (curr_struct)->fields->insert 
            ({root->children[0]->lexinfo, root->children[0]->sym});
         print_sym (root->children[0]->lexinfo, root->children[0]->sym);
      }
      else
      {
         if (in_function_header) return 0;
         if (symbol_stack.back() == nullptr)
         {
            symbol_stack.pop_back();
            symbol_stack.push_back (new symbol_table());
         }
         root->children[0]->sym->attributes[ATTR_variable] = true;
         root->children[0]->sym->attributes[ATTR_lval] = true;
         symbol_stack.back()->insert 
            ({root->children[0]->lexinfo, root->children[0]->sym});
         print_sym (root->children[0]->lexinfo, root->children[0]->sym);
      }
   }
   return 0;
}

int visit_string (astree* root)
{
   for ( auto itor : root->children ) 
      if (visit (itor)) return 1;
   root->sym->attributes[ATTR_string] = true;
   if (!(root->children.size() == 0))
   {
      root->children[0]->sym->attributes[ATTR_string] = true;
      // root->children[0]->sym->attributes[ATTR_field] = true;
      // Check if in struct, if not add to stack, else to struct table
      if (curr_struct != nullptr)
      {
         root->children[0]->sym->owning_struct = curr_struct;
         struct_table.at (curr_struct)->fields->insert 
            ({root->children[0]->lexinfo, root->children[0]->sym});
         print_sym (root->children[0]->lexinfo, root->children[0]->sym);
      }
      else
      {
         if (in_function_header) return 0;
         if (symbol_stack.back() == nullptr)
         {
            symbol_stack.pop_back();
            symbol_stack.push_back (new symbol_table());
         }
         root->children[0]->sym->attributes[ATTR_variable] = true;
         root->children[0]->sym->attributes[ATTR_lval] = true;
         symbol_stack.back()->insert 
            ({root->children[0]->lexinfo, root->children[0]->sym});
         print_sym (root->children[0]->lexinfo, root->children[0]->sym);
      }
   }
   // If no children, let parent handle things
   return 0;
}

int visit_typeid (astree* root)
{
   for ( auto itor : root->children ) 
      if (visit (itor)) return 1;
   if (!struct_table.count (root->lexinfo))
   {
      errprintf ("(%ld,%ld,%ld): typeid: Undeclared type\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   else
   {
      
      
      root->sym->attributes[ATTR_typeid] = true;
      root->sym->attributes[ATTR_struct] = true;
      root->sym->fields = struct_table.at (root->lexinfo)->fields;
      root->sym->struct_name = root->lexinfo;
      root->sym->owning_struct = root->lexinfo;
      
      if (!(root->children.size() == 0))
      {
         root->children[0]->sym->fields = root->sym->fields;
         root->children[0]->sym->attributes[ATTR_struct] = true;
         root->children[0]->sym->struct_name = root->lexinfo;
         // root->children[0]->sym->attributes[ATTR_field] = true;
         // Check if in struct, if not add to stack,else to struct table
         if (curr_struct != nullptr)
         {
            root->children[0]->sym->owning_struct = curr_struct;
            struct_table.at (curr_struct)->fields->insert 
               ({root->children[0]->lexinfo, root->children[0]->sym});
            print_sym (root->children[0]->lexinfo,
                       root->children[0]->sym);
         }
         else
         {
            if ( in_function_header ) return 0;
            if (symbol_stack.back() == nullptr)
            {
               symbol_stack.pop_back();
               symbol_stack.push_back (new symbol_table());
            }
            root->children[0]->sym->attributes[ATTR_variable] = true;
            root->children[0]->sym->attributes[ATTR_lval] = true;
            symbol_stack.back()->insert 
               ({root->children[0]->lexinfo, root->children[0]->sym});
            print_sym(root->children[0]->lexinfo,
                      root->children[0]->sym);
         }
      }
      // If no children, let parent handle things
   }
   return 0;
}

int visit_returnvoid (astree* root)
{
   if (return_type[ATTR_void])
   {
      
   }
   else
   {
      errprintf ("(%ld,%ld,%ld): Incompatible types\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   return 0;
}

int visit_return (astree* root)
{
   if (visit (root->children[0])) return 1;
   
   attr_bitset attr = root->children[0]->sym->attributes;
   
   if (return_type[ATTR_void])
   {
      errprintf ("(%ld,%ld,%ld): Returning void in non-void function\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   else if (return_type[ATTR_array] != attr[ATTR_array])
   {
      if (!(return_type[ATTR_array] && attr[ATTR_null]))
      {
         errprintf ("(%ld,%ld,%ld): Incompatible types\n",
                  root->filenr, root->linenr, root->offset);
         return 1;
      }
   }
   else if (return_type[ATTR_bool] != attr[ATTR_bool])
   {
      errprintf ("(%ld,%ld,%ld): Incompatible types\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   else if (return_type[ATTR_char] != attr[ATTR_char])
   {
      errprintf ("(%ld,%ld,%ld): Incompatible types\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   else if (return_type[ATTR_int] != attr[ATTR_int])
   {
      errprintf ("(%ld,%ld,%ld): Incompatible types\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   else if (return_type[ATTR_string])
   {
      if (!(attr[ATTR_string] || attr[ATTR_null]))
      {
         errprintf ("(%ld,%ld,%ld): Incompatible types\n",
                  root->filenr, root->linenr, root->offset);
         return 1;
      }
   }
   else if (return_type[ATTR_struct])
   {
      if (!attr[ATTR_struct])
      {
         if (!attr[ATTR_null])
         {
            errprintf ("(%ld,%ld,%ld): Incompatible types\n",
                  root->filenr, root->linenr, root->offset);
            return 1;
         }
      }
      // If typeids don't match 
      else if (root->children[0]->sym->fields != return_fields)
      {
         errprintf ("(%ld,%ld,%ld): Incompatible types\n",
                  root->filenr, root->linenr, root->offset);
         return 1;
      }
   }
   // else all is good
   return 0;
}

int visit_comparison (astree* root)
{
   if (visit (root->children[0])) return 1;
   if (visit (root->children[1])) return 1;
   
   if ((root->children[0]->sym->attributes[ATTR_bool] &&
        root->children[1]->sym->attributes[ATTR_bool]) ||
       (root->children[0]->sym->attributes[ATTR_int] &&
         root->children[1]->sym->attributes[ATTR_int]) ||
       (root->children[0]->sym->attributes[ATTR_char] &&
        root->children[1]->sym->attributes[ATTR_char]))
   {
      // valid comparison
      root->sym->attributes[ATTR_bool] = true;
      root->sym->attributes[ATTR_vreg] = true;
   }
   else
   {
      errprintf ("(%ld,%ld,%ld): comparison: Incompatible types\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   return 0;
}

// 
int visit_array (astree* root)
{
   in_array = true;
   // process children, then add to relevant symbol table
   // top of stack if not in struct, else struct table
   if (visit (root->children[0])) return 1;
   if (visit (root->children[1])) return 1;
   
   if (root->children[0]->sym->attributes[ATTR_typeid])
   {
      root->sym->attributes[ATTR_struct] = true;
      root->sym->fields = root->children[0]->sym->fields;
      root->sym->struct_name = root->children[0]->sym->struct_name;
   }
   else
   {
      root->sym->attributes = root->children[0]->sym->attributes;
   }
   root->sym->attributes[ATTR_array] = true;
   
   
   root->children[1]->sym->attributes = root->sym->attributes;
   root->children[1]->sym->fields = root->sym->fields;
   root->children[1]->sym->struct_name = root->sym->struct_name;
   
   // If currently in a struct, insert into struct field
   if (curr_struct != nullptr)
   {
      struct_table.at (curr_struct)->fields->insert 
         ({root->children[1]->lexinfo, root->children[1]->sym});
      print_sym (root->children[1]->lexinfo, root->children[1]->sym);
   }
   else if (in_function_header)
   {
      // Do nothing
   }
   // Otherwise, put in top of symbol_stack
   else
   {
      if (symbol_stack.back() == nullptr)
      {
         symbol_stack.pop_back();
         symbol_stack.push_back (new symbol_table());
      }
      root->children[1]->sym->attributes[ATTR_variable] = true;
      root->children[1]->sym->attributes[ATTR_lval] = true;
      symbol_stack.back()->insert 
         ({root->children[1]->lexinfo, root->children[1]->sym});
      print_sym (root->children[1]->lexinfo, root->children[1]->sym);
   }
   
   in_array = false;
   return 0;
}

int visit_field (astree* root)
{
   // within a struct, field decl
   if (curr_struct != nullptr)
   {
      // If name is already in use
      if (struct_table.at (curr_struct)->fields->count (root->lexinfo))
      {
         errprintf ("(%ld,%ld,%ld): Field name already in use\n",
                  root->filenr, root->linenr, root->offset);
         return 1;
      }
   }
   // 
   else
   {
      root->sym->attributes[ATTR_field] = true;
   }
   return 0;
}

int visit_declid (astree* root)
{
   if (in_function_header) return 0;
   if (symbol_stack.back() == nullptr)
   {
      symbol_stack.pop_back();
      symbol_stack.push_back (new symbol_table());
   }
   if (symbol_stack.back()->count (root->lexinfo))
   {
      errprintf ("(%ld,%ld,%ld): declid: Duplicate declaration\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   return 0;
}

int visit_vardecl (astree* root)
{
   if (visit (root->children[0])) return 1;
   if (visit (root->children[1])) return 1;
   
   // If both are arrays
   if (root->children[0]->sym->attributes[ATTR_array])
   {
      if (!(root->children[1]->sym->attributes[ATTR_null] ||
             root->children[1]->sym->attributes[ATTR_array]))
      {
         errprintf ("(%ld,%ld,%ld): vardecl1: Incompatible types\n",
                  root->filenr, root->linenr, root->offset);
         return 1;
      }
   }
   // If both are structs
   if (root->children[0]->sym->attributes[ATTR_struct])
   {
      if (root->children[1]->sym->attributes[ATTR_struct])
      {
         // and both have the same fields
         if (root->children[0]->sym->fields == 
             root->children[1]->sym->fields)
         {
            // match
         }
         else
         {
            errprintf ("(%ld,%ld,%ld): vardecl2: Incompatible types\n",
                  root->filenr, root->linenr, root->offset);
            return 1;
         }
      }
      else if (!(root->children[1]->sym->attributes[ATTR_null]))
      {
         errprintf ("(%ld,%ld,%ld): vardecl3: Incompatible types\n",
                  root->filenr, root->linenr, root->offset);
         return 1;
      }
   }
   // If they aren't the same primitive type, mismatch
   else if (!((root->children[0]->sym->attributes[ATTR_int] ==
             root->children[1]->sym->attributes[ATTR_int]) &&
            (root->children[0]->sym->attributes[ATTR_char] ==
             root->children[1]->sym->attributes[ATTR_char]) &&
            (root->children[0]->sym->attributes[ATTR_string] ==
             root->children[1]->sym->attributes[ATTR_string]) &&
            (root->children[0]->sym->attributes[ATTR_bool] ==
             root->children[1]->sym->attributes[ATTR_bool])))
   {
      errprintf ("(%ld,%ld,%ld): vardecl4: Incompatible types\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   return 0;
}

int visit_new (astree* root)
{
   // If it doesn't error out, must be valid
   if (visit (root->children[0])) return 1;
   if (struct_table.at (root->children[0]->lexinfo) == nullptr)
   {
      errprintf ("(%ld,%ld,%ld): new: Incomplete type\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   root->sym->attributes[ATTR_struct] = true;
   root->sym->attributes[ATTR_vreg] = true;
   root->sym->fields = root->children[0]->sym->fields;
   root->sym->struct_name = root->children[0]->sym->struct_name;
   return 0;
}
int visit_newstring (astree* root)
{
   if (visit (root->children[0])) return 1;
   if (!(root->children[0]->sym->attributes[ATTR_int]))
   {
      errprintf ("(%ld,%ld,%ld): newstring: Incompatible type\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   root->sym->attributes[ATTR_string] = true;
   root->sym->attributes[ATTR_vreg] = true;
   return 0;
}

int visit_newarray (astree* root)
{
   if (visit (root->children[0])) return 1;
   if (visit (root->children[1])) return 1;
   if (!(root->children[1]->sym->attributes[ATTR_int]))
   {
      errprintf ("(%ld,%ld,%ld): newarray: Incompatible type\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   if (root->children[0]->sym->attributes[ATTR_typeid])
   {
      root->sym->attributes[ATTR_struct] = true;
      root->sym->fields = root->children[0]->sym->fields;
      root->sym->struct_name = root->children[0]->sym->struct_name;
   }
   else
   {
      root->sym->attributes = root->children[0]->sym->attributes;
   }
   root->sym->attributes[ATTR_array] = true;
   root->sym->attributes[ATTR_vreg] = true;
   return 0;
}

int visit_ident (astree* root)
{
   for (int itor = symbol_stack.size() - 1; itor >= 0; --itor)
   {
      if (symbol_stack[itor] == nullptr) continue;
      if (symbol_stack[itor]->count (root->lexinfo))
      {
         root->sym->attributes = 
            symbol_stack[itor]->at (root->lexinfo)->attributes;
         root->sym->fields = 
            symbol_stack[itor]->at (root->lexinfo)->fields;
         root->sym->struct_name =
            symbol_stack[itor]->at (root->lexinfo)->struct_name;
         root->sym->owning_struct = 
            symbol_stack[itor]->at (root->lexinfo)->owning_struct;
         root->sym->parameters = 
            symbol_stack[itor]->at (root->lexinfo)->parameters;
         root->sym->block_nr = 
            symbol_stack[itor]->at (root->lexinfo)->block_nr;
         return 0;
      }
   }
   // If ident is not declared
   errprintf ("(%ld,%ld,%ld): ident: Identifier not declared\n",
               root->filenr, root->linenr, root->offset);
   return 1;
}

int visit_assignment (astree* root)
{
   if (visit (root->children[0])) return 1;
   if (visit (root->children[1])) return 1;
   
   if (!(root->children[0]->sym->attributes[ATTR_lval]))
   {
      errprintf ("(%ld,%ld,%ld): assign: Left operand not an lval\n",
               root->filenr, root->linenr, root->offset);
      return 1;
   }
   if (root->children[0]->sym->attributes[ATTR_array])
   {
      if (!(root->children[1]->sym->attributes[ATTR_null] ||
             root->children[1]->sym->attributes[ATTR_array]))
      {
         errprintf ("(%ld,%ld,%ld): assign: Incompatible types\n",
                  root->filenr, root->linenr, root->offset);
         return 1;
      }
      else
      {
         root->sym->attributes[ATTR_array] = true;
      }
   }
   if (root->children[0]->sym->attributes[ATTR_struct])
   {
      if (!(root->children[1]->sym->attributes[ATTR_struct]))
      {
         if (!(root->children[1]->sym->attributes[ATTR_null]))
         {
            errprintf ("(%ld,%ld,%ld): assign: Incompatible types\n",
                       root->filenr, root->linenr, root->offset);
            return 1;
         }
         else
         {
            root->sym->attributes[ATTR_null] = true;
         }
      }
      else
      {
         if (!(root->children[0]->sym->fields == 
               root->children[1]->sym->fields))
         {
            errprintf ("(%ld,%ld,%ld): assign: Incompatible types\n",
                       root->filenr, root->linenr, root->offset);
            return 1;
         }
         else
         {
            root->sym->attributes[ATTR_struct] = true;
            root->sym->fields = root->children[0]->sym->fields;
            root->sym->struct_name =root->children[0]->sym->struct_name;
            root->sym->owning_struct = 
               root->children[0]->sym->owning_struct;
         }
      }
   }
   else if (!((root->children[0]->sym->attributes[ATTR_int] ==
             root->children[1]->sym->attributes[ATTR_int]) &&
            (root->children[0]->sym->attributes[ATTR_char] ==
             root->children[1]->sym->attributes[ATTR_char]) &&
            (root->children[0]->sym->attributes[ATTR_string] ==
             root->children[1]->sym->attributes[ATTR_string]) &&
            (root->children[0]->sym->attributes[ATTR_bool] ==
             root->children[1]->sym->attributes[ATTR_bool])))
   {
      errprintf ("(%ld,%ld,%ld): assign: Incompatible types\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   else
   {
      root->sym->attributes[ATTR_int] = 
         root->children[0]->sym->attributes[ATTR_int];
      root->sym->attributes[ATTR_char] = 
         root->children[0]->sym->attributes[ATTR_char];
      root->sym->attributes[ATTR_bool] = 
         root->children[0]->sym->attributes[ATTR_bool];
      root->sym->attributes[ATTR_string] = 
         root->children[0]->sym->attributes[ATTR_string];
   }
   root->sym->attributes[ATTR_vreg];
   return 0;
}

int visit_index (astree* root)
{
   if (visit (root->children[0])) return 1;
   if (visit (root->children[1])) return 1;
   
   if (!(root->children[1]->sym->attributes[ATTR_int]))
   {
      errprintf ("(%ld,%ld,%ld): Right operand of [ not an int\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   if (!(root->children[0]->sym->attributes[ATTR_array]))
   {
      if (!(root->children[0]->sym->attributes[ATTR_string]))
      {
         errprintf ("(%ld,%ld,%ld): Left operand of [ not an array\n",
                     root->filenr, root->linenr, root->offset);
         return 1;
      }
   }
   
   if (root->children[0]->sym->attributes[ATTR_string])
   {
      if (root->children[0]->sym->attributes[ATTR_array])
         root->sym->attributes[ATTR_string] = true;
      else
         root->sym->attributes[ATTR_char] = true;
   }
   else
   {
      root->sym->attributes[ATTR_int] = 
         root->children[0]->sym->attributes[ATTR_int];
      root->sym->attributes[ATTR_char] = 
         root->children[0]->sym->attributes[ATTR_char];
      root->sym->attributes[ATTR_bool] = 
         root->children[0]->sym->attributes[ATTR_bool];
      root->sym->attributes[ATTR_struct] = 
         root->children[0]->sym->attributes[ATTR_struct];
      root->sym->fields = root->children[0]->sym->fields;
      root->sym->struct_name = root->children[0]->sym->struct_name;
      root->sym->owning_struct = root->children[0]->sym->owning_struct;
   }
   root->sym->attributes[ATTR_vaddr] = true;
   root->sym->attributes[ATTR_lval] = true;
   return 0;
}

int visit_dot (astree* root)
{
   if (visit (root->children[0])) return 1;
   if (visit (root->children[1])) return 1;
   
   if (!(root->children[0]->sym->attributes[ATTR_struct]))
   {
      errprintf ("(%ld,%ld,%ld): dot: Left operant of . not a struct\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   symbol_table* fields = root->children[0]->sym->fields;
   if (!(fields->count (root->children[1]->lexinfo)))
   {
      errprintf ("(%ld,%ld,%ld): dot: Right operant of . not a field\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   root->sym->attributes = 
      fields->at (root->children[1]->lexinfo)->attributes;
   root->sym->fields = 
      fields->at (root->children[1]->lexinfo)->fields;
   root->sym->struct_name = 
      fields->at (root->children[1]->lexinfo)->struct_name;
   root->sym->owning_struct = 
      fields->at (root->children[1]->lexinfo)->owning_struct;
   root->children[1]->sym->attributes = 
      fields->at (root->children[1]->lexinfo)->attributes;
   root->children[1]->sym->fields = 
      fields->at (root->children[1]->lexinfo)->fields;
   root->children[1]->sym->struct_name = 
      fields->at (root->children[1]->lexinfo)->struct_name;
   root->children[1]->sym->owning_struct = 
      fields->at (root->children[1]->lexinfo)->owning_struct;
   
   root->sym->attributes[ATTR_lval] = true;
   root->sym->attributes[ATTR_variable] = true;
   return 0;
}

int visit_prototype (astree* root)
{
   symbol_stack.push_back (nullptr);
   blocknum_stack.push_back (next_block++);
   
   in_function_header = true;
   if (visit (root->children[0])) return 1;
   in_function_header = false;
   
   // Attributes are equal to basetype, unless basetype is typeid
   // then replace typeid with struct
   root->sym->attributes = root->children[0]->sym->attributes;
   root->sym->attributes[ATTR_prototype] = true;
   if (root->sym->attributes[ATTR_typeid])
   {
      root->sym->attributes[ATTR_typeid] = false;
      root->sym->attributes[ATTR_struct] = true;
      root->sym->fields = root->children[0]->sym->fields;
      root->sym->struct_name = root->children[0]->sym->struct_name;
      root->sym->owning_struct = root->children[0]->sym->owning_struct;
   }
   
   if (visit (root->children[1])) return 1;
   // TODO
   // Children of paramlist add all of them to param vector
   if (root->sym->parameters == nullptr)
   {
      root->sym->parameters = new vector<symbol*>();
   }
   for (auto itor : root->children[1]->children)
   {
      root->sym->parameters->push_back(itor->sym);
   }
   if (symbol_stack[0] == nullptr)
   {
      symbol_stack[0] = new symbol_table();
   }
   // Add to symbol table
   // Key is declid, which is either the left child of the basetype, or
   // the right child of the TOK_ARRAY
   const string* key = root->children[0]->children[0]->lexinfo;
   if (root->sym->attributes[ATTR_array])
      key = root->children[0]->children[1]->lexinfo;
   if (symbol_stack[0]->count (key))
   {
      errprintf ("(%ld,%ld,%ld): Prototype already declared\n",
                  root->filenr, root->linenr, root->offset);
      return 1;
   }
   symbol_stack[0]->insert ({key, root->sym});
   print_func_sym (key, root->sym);
   
   symbol_stack.pop_back();
   blocknum_stack.pop_back();
   return 0;
}

int visit_function (astree* root)
{
   symbol_stack.push_back (nullptr);
   blocknum_stack.push_back (next_block++);
   
   in_function_header = true;
   if (visit (root->children[0])) return 1;
   in_function_header = false;
   
   // Attributes are equal to basetype, unless basetype is typeid
   // then replace typeid with struct
   root->sym->attributes = root->children[0]->sym->attributes;
   // set to prototype for now, for easy comparison to other function
   // declarations
   root->sym->attributes[ATTR_prototype] = true;
   if (root->sym->attributes[ATTR_typeid])
   {
      root->sym->attributes[ATTR_typeid] = false;
      root->sym->attributes[ATTR_struct] = true;
      root->sym->fields = root->children[0]->sym->fields;
      root->sym->struct_name = root->children[0]->sym->struct_name;
      root->sym->owning_struct = root->children[0]->sym->owning_struct;
   }
   return_type = root->sym->attributes;
   return_type[ATTR_prototype] = false;
   
   return_fields = root->sym->fields;
   
   if (visit (root->children[1])) return 1;
   // TODO
   // Children of paramlist add all of them to param vector
   if (root->sym->parameters == nullptr)
   {
      root->sym->parameters = new vector<symbol*>();
   }
   for (auto itor : root->children[1]->children)
   {
      root->sym->parameters->push_back(itor->sym);
   }
   if (symbol_stack[0] == nullptr)
   {
      symbol_stack[0] = new symbol_table();
   }
   // Add to symbol table
   // Key is declid, which is either the left child of the basetype, or
   // the right child of the TOK_ARRAY
   const string* key = root->children[0]->children[0]->lexinfo;
   if (root->sym->attributes[ATTR_array])
      key = root->children[0]->children[1]->lexinfo;
   if (symbol_stack[0]->count (key))
   {
      if (root->sym->attributes == symbol_stack[0]->at(key)->attributes)
      {
         if (root->sym->fields == symbol_stack[0]->at(key)->fields)
         {
            if (root->sym->parameters->size() != 
                symbol_stack[0]->at(key)->parameters->size())
            {
               errprintf ("(%ld,%ld,%ld): func: Confliction function\n",
                  root->filenr, root->linenr, root->offset);
               return 1;
            }
            else
            {
               for (size_t i = 0;i < root->sym->parameters->size(); ++i)
               {
                  if ((root->sym->parameters[i] != 
                      symbol_stack[0]->at(key)->parameters[i]))
                  {
                     errprintf ("(%ld,%ld,%ld): Conflicting function\n",
                              root->filenr, root->linenr, root->offset);
                  }
                  if ((root->sym->fields[i] != 
                      symbol_stack[0]->at(key)->fields[i]))
                  {
                     errprintf ("(%ld,%ld,%ld): Conflicting function\n",
                              root->filenr, root->linenr, root->offset);
                     return 1;
                  }
               }
               // They are equal
               symbol_stack[0]->at(key)->attributes[ATTR_prototype] =
                                                                false;
               symbol_stack[0]->at(key)->attributes[ATTR_function] = 
                                                                true;
            }
         }
         else
         {
            errprintf ("(%ld,%ld,%ld): func: Conflicting function\n",
                  root->filenr, root->linenr, root->offset);
            return 1;
         }
      }
      else
      {
         errprintf ("(%ld,%ld,%ld): func: Conflicting function\n",
                  root->filenr, root->linenr, root->offset);
         return 1;
      }
   }
   else
   {
      root->sym->attributes[ATTR_prototype] = false;
      root->sym->attributes[ATTR_function] = true;
      symbol_stack[0]->insert ({key, root->sym});
      print_func_sym (key, root->sym);
   }
   
   if (visit (root->children[2])) return 1;
   
   symbol_stack.pop_back();
   blocknum_stack.pop_back();
   return_type = 0;
   return_fields = nullptr;
   return 0;
}

int visit_call (astree* root)
{
   for ( auto itor : root->children ) 
      if (visit (itor)) return 1;
      
   if (!(root->children[0]->sym->attributes[ATTR_function] ||
         root->children[0]->sym->attributes[ATTR_prototype]))
   {
      errprintf ("(%ld,%ld,%ld): call: Not a function\n",
               root->filenr, root->linenr, root->offset);
      return 1;
   }
   
   root->sym->attributes = root->children[0]->sym->attributes;
   root->sym->fields = root->children[0]->sym->fields;
   root->sym->struct_name = root->children[0]->sym->struct_name;
   root->sym->owning_struct = root->children[0]->sym->owning_struct;
   root->sym->attributes[ATTR_function] = false;
   root->sym->attributes[ATTR_vreg] = true;
   // Check parameters
   size_t len = root->children[0]->sym->parameters->size();
   // First child is ident, all others are parameters
   if (len != root->children.size() - 1)
   {
      errprintf ("(%ld,%ld,%ld): Incorrect number of parameters\n",
               root->filenr, root->linenr, root->offset);
      return 1;
   }
   
   for (size_t itor = 0; itor < len; ++itor)
   {
      symbol* sym1 = root->children[itor+1]->sym;
      symbol* sym2 = (*(root->children[0]->sym->parameters))[itor];
      if (sym2->attributes[ATTR_array])
      {
         if (!(sym1->attributes[ATTR_array] ||
               sym1->attributes[ATTR_null]))
         {
            errprintf ("(%ld,%ld,%ld): call2: Incorrect parameters\n",
               root->filenr, root->linenr, root->offset);
            return 1;
         }
      }
      if (sym2->attributes[ATTR_struct])
      {
         if (sym2->fields != sym1->fields && 
             !(sym1->attributes[ATTR_null]))
         {
            errprintf ("(%ld,%ld,%ld): call3: Incorrect parameters\n",
               root->filenr, root->linenr, root->offset);
            return 1;
         }
      }
      // If they aren't the same primitive type, mismatch
      else if (!((sym2->attributes[ATTR_int] ==
                  sym1->attributes[ATTR_int]) &&
                 (sym2->attributes[ATTR_char] ==
                  sym1->attributes[ATTR_char]) &&
                 (sym2->attributes[ATTR_string] ==
                  sym1->attributes[ATTR_string]) &&
                 (sym2->attributes[ATTR_bool] ==
                  sym1->attributes[ATTR_bool])))
      {
         errprintf ("(%ld,%ld,%ld): call4: Incorrect parameters\n",
                     root->filenr, root->linenr, root->offset);
         return 1;
      }
   }
   return 0;
}

int visit_paramlist (astree* root)
{
   for ( auto itor : root->children ) 
   {
      if (visit (itor)) return 1;
      itor->sym->attributes[ATTR_param] = true;
   }
   return 0;
}

int visit_eqne (astree* root)
{
   for ( auto itor : root->children ) 
      if (visit (itor)) return 1;
   root->sym->attributes[ATTR_bool] = true;
   root->sym->attributes[ATTR_vreg] = true;
   return 0;
}

int visit (astree* root)
{
   root->sym->block_nr = blocknum_stack.back();
   int status = 0;
   switch (root->symbol)
   {
      case TOK_VOID: status = visit_void (root); break;
      case TOK_BOOL: status = visit_bool (root); break;
      case TOK_CHAR: status = visit_char (root); break;
      case TOK_INT:  status = visit_int  (root); break;
      case TOK_STRING: status = visit_string (root); break;
      case TOK_IF:
      case TOK_IFELSE: status = visit_if (root); break;
      case TOK_WHILE: status = visit_while (root); break;
      case TOK_RETURN: status = visit_return (root); break;
      case TOK_STRUCT: status = visit_struct (root); break;
      case TOK_FALSE:
      case TOK_TRUE: status = visit_boolcon (root); break;
      case TOK_NULL: status = visit_nullcon (root); break;
      case TOK_NEW: status = visit_new (root); break;
      case TOK_ARRAY: status = visit_array (root); break;
      case TOK_EQ:
      case TOK_NE: status = visit_eqne (root); break;
      
      case TOK_LT:
      case TOK_LE:
      case TOK_GT:
      case TOK_GE: status = visit_comparison (root); break;
      case '+':
      case '-':
      case '*':
      case '/':
      case '%': status = visit_binop (root); break;
      case '=': status = visit_assignment (root); break;
      case TOK_IDENT: status = visit_ident (root); break;
      case TOK_INTCON: status = visit_intcon (root); break;
      case TOK_CHARCON: status = visit_charcon (root); break;
      case TOK_STRINGCON: status = visit_stringcon (root); break;

      case TOK_BLOCK: status = visit_block (root); break;
      case TOK_CALL: status = visit_call (root); break;
      
      // case TOK_INITDECL: 
      case TOK_CHR: status = visit_chr (root); break;
      case TOK_ORD: status = visit_ord (root); break;
      case '!': status = visit_not (root); break;
      case TOK_POS:
      case TOK_NEG: status = visit_int_unop (root); break;
      case TOK_NEWARRAY: status = visit_newarray (root); break;
      case TOK_TYPEID: status = visit_typeid (root); break;
      case TOK_FIELD: status = visit_field (root); break;
      case TOK_DECLID: status = visit_declid (root); break;
      case TOK_FUNCTION: status = visit_function (root); break;
      case TOK_PARAMLIST: status = visit_paramlist (root); break;
      case TOK_PROTOTYPE: status = visit_prototype (root); break;
      case TOK_VARDECL: status = visit_vardecl (root); break;
      case TOK_RETURNVOID: status = visit_returnvoid (root); break;
      case TOK_NEWSTRING: status = visit_newstring (root); break;
      case TOK_INDEX: status = visit_index (root); break;
      case '.': status = visit_dot (root); break;
      case ROOT: status = visit_root (root); break;
      default: break;
   }
   return status;
}

int type_check (astree* root, FILE* output)
{
   symbol_stack.push_back (nullptr);
   blocknum_stack.push_back (0);
   
   out = output;
   
   // Clear return type
   return_type = 0;
   return_fields = nullptr;
   
   return visit (root);
}


















