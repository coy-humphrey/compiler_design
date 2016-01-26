// Coy Humphrey 

#include <sstream>
#include <string>
#include <vector>

using namespace std;

#include <stdio.h>

#include "auxlib.h"
#include "lyutils.h"
#include "stringset.h"
#include "symutils.h"
#include "yyparse.h"

FILE* outf;

string eight_space = "        ";

vector<astree*> global_decl;

size_t rnum = 1;

void e_visit (astree* root);
// Visit expression, return string with variable holding value
string expr_visit (astree* root);

void emit_basetype (astree* root)
{
   if (root->sym->attributes[ATTR_int])
      fprintf (outf, "int");
   else if (root->sym->attributes[ATTR_char])
      fprintf (outf, "char");
   else if (root->sym->attributes[ATTR_bool])
      fprintf (outf, "char");
   else if (root->sym->attributes[ATTR_void])
      fprintf (outf, "void");
   else if (root->sym->attributes[ATTR_string])
      fprintf (outf, "char*");
   else if (root->sym->attributes[ATTR_struct])
      fprintf (outf, "struct %s*", root->sym->struct_name->c_str());
   
   if (root->sym->attributes[ATTR_array])
      fprintf (outf, "*");
      
   fprintf (outf, " ");
}

void emit_field_name (astree* root, const string* struct_name)
{
   const string* fname;
   fname = root->children.back()->lexinfo;
   fprintf (outf, "f_%s_%s", struct_name->c_str(), fname->c_str());
}

void emit_struct (astree* root)
{
   const string* struct_name = root->children[0]->lexinfo;
   fprintf (outf, "struct s_%s {\n", struct_name->c_str());
   for (size_t itor = 1; itor < root->children.size(); ++itor)
   {
      fprintf (outf, "%s", eight_space.c_str());
      emit_basetype (root->children[itor]);
      emit_field_name (root->children[itor], struct_name);
      fprintf (outf, ";\n");
   }
   fprintf (outf, "};\n");
}



void emit_structs (astree* root)
{
   for (auto itor : root->children)
      if (itor->symbol == TOK_STRUCT)
         emit_struct (itor);
}

void emit_string_cons ()
{
   int num = 1;
   for (auto itor : stringcons)
   {
      fprintf(outf, "char* s%d = %s;\n", num++, itor->lexinfo->c_str());
   }
}

void emit_global_vars(astree* root)
{
   for (auto itor : root->children)
      if (itor->symbol == TOK_VARDECL)
         global_decl.push_back (itor);
   for (auto itor : global_decl)
   {
      emit_basetype (itor->children[0]);
      fprintf (outf, "__%s;\n", 
                  itor->children[0]->children.back()->lexinfo->c_str());
   }
}

void emit_parameters (astree* root)
{
   for (size_t itor = 0; itor < root->children.size(); ++itor)
   {
      fprintf (outf, "%s", eight_space.c_str());
      emit_basetype (root->children[itor]);
      fprintf (outf, "_%ld_%s", root->children[itor]->sym->block_nr,
            root->children[itor]->children.back()->lexinfo->c_str());
      if (itor < root->children.size() - 1)
         fprintf (outf, ",\n");
   }
   fprintf (outf, ")\n");
}

void emit_block (astree* root)
{
   for (auto itor : root->children)
      e_visit (itor);
}

void emit_function (astree* root)
{
   emit_basetype (root->children[0]);
   fprintf (outf, "__%s (\n", 
            root->children[0]->children.back()->lexinfo->c_str());
   emit_parameters (root->children[1]);
   fprintf (outf, "{\n");
   emit_block (root->children[2]);
   fprintf (outf, "}\n");
}

void emit_functions (astree* root)
{
   for (auto itor : root->children)
   if (itor->symbol == TOK_FUNCTION)
      emit_function (itor);
}

void emit_while (astree* root)
{
   fprintf (outf, "while_%ld_%ld_%ld:;\n", 
            root->filenr, root->linenr, root->offset);
   string op = expr_visit (root->children[0]);
   fprintf (outf, "%s", eight_space.c_str());
   fprintf (outf, "if (!%s) goto break_%ld_%ld_%ld;\n", op.c_str(),
            root->filenr, root->linenr, root->offset);
   e_visit (root->children[1]);
   fprintf (outf, "%s", eight_space.c_str());
   fprintf (outf, "goto while_%ld_%ld_%ld;\n",
            root->filenr, root->linenr, root->offset);
   fprintf (outf, "break_%ld_%ld_%ld:;\n",
            root->filenr, root->linenr, root->offset);
}

void emit_ifelse (astree* root)
{
   string op = expr_visit (root->children[0]);
   fprintf (outf, "%s", eight_space.c_str());
   fprintf (outf, "if (!%s) goto else_%ld_%ld_%ld;\n", op.c_str(),
            root->filenr, root->linenr, root->offset);
   e_visit (root->children[1]);
   fprintf (outf, "%s", eight_space.c_str());
   fprintf (outf, "goto fi_%ld_%ld_%ld;\n",
            root->filenr, root->linenr, root->offset);
   fprintf (outf, "else_%ld_%ld_%ld:;\n",
            root->filenr, root->linenr, root->offset);
   e_visit (root->children[2]);
   fprintf (outf, "fi_%ld_%ld_%ld:;\n",
            root->filenr, root->linenr, root->offset);
}

void emit_if (astree* root)
{
   string op = expr_visit (root->children[0]);
   fprintf (outf, "%s", eight_space.c_str());
   fprintf (outf, "if (!%s) goto fi_%ld_%ld_%ld;\n", op.c_str(),
            root->filenr, root->linenr, root->offset);
   e_visit (root->children[1]);
   fprintf (outf, "fi_%ld_%ld_%ld:;\n",
            root->filenr, root->linenr, root->offset);
}

void emit_return (astree* root)
{
   string op = expr_visit (root->children[0]);
   fprintf (outf, "%s", eight_space.c_str());
   fprintf (outf, "return %s;\n", op.c_str());
}

void emit_returnvoid (astree* root)
{
   (void) root;
   fprintf (outf, "%s", eight_space.c_str());
   fprintf (outf, "return;");
}

void emit_vardecl (astree* root)
{
   string op = expr_visit (root->children[1]);
   const string* name = root->children[0]->children.back()->lexinfo;
   size_t block = root->children[0]->children.back()->sym->block_nr;
   fprintf (outf, "%s", eight_space.c_str());
   emit_basetype (root->children[0]);
   fprintf (outf, "_%ld_%s = %s;\n", block, name->c_str(), op.c_str());
}

string emit_assignment (astree* root)
{
   string op1 = expr_visit (root->children[0]);
   string op2 = expr_visit (root->children[1]);
   fprintf (outf, "%s", eight_space.c_str());
   fprintf (outf, "%s = %s;\n", op1.c_str(), op2.c_str());
   return op1;
}

string emit_ident (astree* root)
{
   if (root->sym->block_nr == 0)
   {
      stringstream result;
      result << "__" << root->lexinfo->c_str();
      return result.str();
   }
   stringstream result;
   result<< "_" << root->sym->block_nr << "_" << root->lexinfo->c_str();
   return result.str();
}

string emit_math_binop (astree* root)
{
   string op1 = expr_visit (root->children[0]);
   string op2 = expr_visit (root->children[1]);
   
   fprintf (outf, "%s", eight_space.c_str());
   fprintf (outf, "int i%ld = %s %s %s;\n",
            rnum, op1.c_str(), root->lexinfo->c_str(), op2.c_str());
   stringstream result;
   result << "i" << rnum++;
   return result.str();
}

string emit_bool_binop (astree* root)
{
   string op1 = expr_visit (root->children[0]);
   string op2 = expr_visit (root->children[1]);
   
   fprintf (outf, "%s", eight_space.c_str());
   fprintf (outf, "char c%ld = %s %s %s;\n",
            rnum, op1.c_str(), root->lexinfo->c_str(), op2.c_str());
   stringstream result;
   result << "c" << rnum++;
   return result.str();
}

string emit_math_unop (astree* root)
{
   string op1 = expr_visit (root->children[0]);
   fprintf (outf, "%s", eight_space.c_str());
   fprintf (outf, "int i%ld = %s%s;\n",
            rnum, root->lexinfo->c_str(), op1.c_str());
   stringstream result;
   result << "i" << rnum++;
   return result.str();
}

string emit_not (astree* root)
{
   string op1 = expr_visit (root->children[0]);
   fprintf (outf, "%s", eight_space.c_str());
   fprintf (outf, "char c%ld = %s%s;\n",
            rnum, root->lexinfo->c_str(), op1.c_str());
   stringstream result;
   result << "c" << rnum++;
   return result.str();
}

string emit_ord (astree* root)
{
   string op1 = expr_visit (root->children[0]);
   fprintf (outf, "%s", eight_space.c_str());
   fprintf (outf, "int i%ld = (int)%s;\n",
            rnum, op1.c_str());
   stringstream result;
   result << "i" << rnum++;
   return result.str();
}

string emit_chr (astree* root)
{
   string op1 = expr_visit (root->children[0]);
   fprintf (outf, "%s", eight_space.c_str());
   fprintf (outf, "char c%ld = (char)%s;\n",
            rnum, op1.c_str());
   stringstream result;
   result << "c" << rnum++;
   return result.str();
}

string emit_new (astree* root)
{
   fprintf (outf, "%s", eight_space.c_str());
   emit_basetype (root->children[0]);
   fprintf (outf, "p%ld = xcalloc (1, sizeof(struct %s));\n", rnum,
             root->children[0]->sym->struct_name->c_str());
   stringstream result;
   result << "p" << rnum++;
   return result.str();
}

string emit_newstring (astree* root)
{
   string op = expr_visit (root->children[0]);
   fprintf (outf, "%s", eight_space.c_str());
   fprintf (outf, "char* p%ld = xcalloc (%s, sizeof (char));\n",
             rnum, op.c_str());
   stringstream result;
   result << "p" << rnum++;
   return result.str();
}

string emit_newarray (astree* root)
{
   string op = expr_visit (root->children[1]);
   fprintf (outf, "%s", eight_space.c_str());
   emit_basetype (root->children[0]);
   fprintf (outf, "* p%ld = xcalloc (%s, sizeof (",
             rnum, op.c_str());
   emit_basetype (root->children[0]);
   fprintf (outf, "));\n");
   stringstream result;
   result << "p" << rnum++;
   return result.str();
}

string emit_charcon (astree* root)
{
   stringstream result;
   result << root->lexinfo->c_str();
   return result.str();
}

string emit_true (astree* root)
{
   (void)root;
   stringstream result;
   result << 1;
   return result.str();
}

string emit_false (astree* root)
{
   (void)root;
   stringstream result;
   result << 0;
   return result.str();
}

string emit_stringcon (astree* root)
{
   size_t num = 0;
   for (size_t itor = 0; itor < stringcons.size(); ++itor)
   {
      if (root == stringcons[itor])
      {
         num = itor + 1;
         break;
      }
   }
   stringstream result;
   result << "s" << num;
   return result.str();
}

string emit_intcon (astree* root)
{
   string val = root->lexinfo->c_str();
   size_t pos = val.find_first_not_of ("0");
   string result;
   if (pos >= val.size()) result = "0";
   else result = val.substr (pos);
   return result;
}

string emit_index (astree* root)
{
   string op1 = expr_visit (root->children[0]);
   string op2 = expr_visit (root->children[1]);
   
   fprintf (outf, "%s", eight_space.c_str());
   emit_basetype (root->children[0]);
   fprintf (outf, "a%ld = &%s[%s];\n", rnum, op1.c_str(), op2.c_str());
   stringstream result;
   result << "*a" << rnum++;
   return result.str();
}

string emit_field (astree* root)
{
   if (root->sym->owning_struct == nullptr) return "";
   stringstream result;
   result << "f_" << root->sym->owning_struct->c_str() << "_" <<
               root->lexinfo->c_str();
   return result.str();
}

string emit_dot (astree* root)
{
   string op1 = expr_visit (root->children[0]);
   string op2 = expr_visit (root->children[1]);
   
   fprintf (outf, "%s", eight_space.c_str());
   emit_basetype (root->children[1]);
   fprintf (outf, "*a%ld = &%s->%s;\n", rnum, op1.c_str(), op2.c_str());
   stringstream result;
   result << "*a" << rnum++;
   return result.str();
}

string emit_call (astree* root)
{
   string op1 = expr_visit (root->children[0]);
   vector<string> ops;
   for (size_t itor = 1; itor < root->children.size(); ++itor)
   {
      ops.push_back (expr_visit (root->children[itor]));
   }
   fprintf (outf, "%s", eight_space.c_str());
   stringstream result;
   if (!(root->sym->attributes[ATTR_void]))
   {
      emit_basetype (root->children[0]);
      char type;
      if (root->sym->attributes[ATTR_array]) type = 'a';
      else if (root->sym->attributes[ATTR_string]) type = 's';
      else if (root->sym->attributes[ATTR_struct]) type = 'p';
      else if (root->sym->attributes[ATTR_int]) type = 'i';
      else type = 'c';
      fprintf (outf, "%c%ld = ", type, rnum);
      result << type << rnum++;
   }
   fprintf (outf, "%s (", op1.c_str());
   bool first = true;
   for (string itor : ops)
   {
      if (!first)
      {
         fprintf (outf, ",");
      }
      first = false;
      fprintf (outf, "%s", itor.c_str()); 
   }
   fprintf (outf, ");\n");
   return result.str();
}

void emit_global_var_decls (astree* root)
{
   (void)root;
   for (auto itor : global_decl)
   {
      string op = expr_visit (itor->children[1]);
      fprintf (outf, "%s", eight_space.c_str());
      fprintf (outf, "__%s = %s;\n", 
                  itor->children[0]->children.back()->lexinfo->c_str(),
                  op.c_str());
   }
}

void emit_main (astree* root)
{
   fprintf (outf, "void __ocmain (void)\n{\n");
   
   emit_global_var_decls (root);
   
   for (auto itor : root->children)
   {
      if (itor->symbol != TOK_VARDECL && itor->symbol != TOK_STRUCT
          && itor->symbol != TOK_FUNCTION && itor->symbol !=
          TOK_PROTOTYPE)
      {
         e_visit (itor);
      }
   }
   
   fprintf (outf, "}\n");
}

void e_visit (astree* root)
{
   switch (root->symbol)
   {
      case TOK_BLOCK: emit_block (root); break;
      case TOK_WHILE: emit_while (root); break;
      case TOK_IF: emit_if (root); break;
      case TOK_IFELSE: emit_ifelse (root); break;
      case TOK_RETURNVOID: emit_returnvoid (root); break;
      case TOK_RETURN: emit_return (root); break;
      case TOK_VARDECL: emit_vardecl (root); break;
      default: expr_visit (root);
   }
}

// each one returns string of variable holding result
string expr_visit (astree* root)
{
   string op;
   switch (root->symbol)
   {
      case '=': op = emit_assignment (root); break;
      case TOK_IDENT: op = emit_ident (root); break;
      case '+':
      case '-':
      case '/':
      case '*':
      case '%': op = emit_math_binop (root); break;
      case TOK_EQ:
      case TOK_NE:
      case TOK_LT:
      case TOK_GT:
      case TOK_LE:
      case TOK_GE: op = emit_bool_binop (root); break;
      case TOK_POS:
      case TOK_NEG: op = emit_math_unop (root); break;
      case '!': op = emit_not (root); break;
      case TOK_ORD: op = emit_ord (root); break;
      case TOK_CHR: op = emit_chr (root); break;
      case TOK_NEW: op = emit_new (root); break;
      case TOK_NEWARRAY: op = emit_newarray (root); break;
      case TOK_NEWSTRING: op = emit_newstring (root); break;
      case TOK_CHARCON: op = emit_charcon (root); break;
      case TOK_STRINGCON: op = emit_stringcon (root); break;
      case TOK_INTCON: op = emit_intcon (root); break;
      case TOK_TRUE: op = emit_true (root); break;
      case TOK_FALSE: op = emit_false (root); break;
      case TOK_NULL: op = emit_false (root); break;
      case TOK_INDEX: op = emit_index (root); break;
      case '.': op = emit_dot (root); break;
      case TOK_FIELD: op = emit_field (root); break;
      case TOK_CALL: op = emit_call (root); break;
   }
   return op;
}

void emit_prolog()
{
   fprintf (outf, "#define __OCLIB_C__\n");
   fprintf (outf, "#include \"oclib.oh\"\n");
}

void emit (astree* root, FILE* o)
{
   outf = o;
   emit_prolog ();
   emit_structs (root);
   emit_string_cons();
   emit_global_vars (root);
   emit_functions (root);
   emit_main (root);
}
