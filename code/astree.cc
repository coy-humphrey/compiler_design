// Coy Humphrey
#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "astree.h"
#include "stringset.h"
#include "lyutils.h"

astree* new_astree (int symbol, int filenr, int linenr,
                    int offset, const char* lexinfo) {
   astree* tree = new astree();
   tree->symbol = symbol;
   tree->filenr = filenr;
   tree->linenr = linenr;
   tree->offset = offset;
   tree->lexinfo = intern_stringset (lexinfo);
   tree->sym = new struct symbol;
   tree->sym->attributes = 0;
   tree->sym->fields = nullptr;
   tree->sym->block_nr = 0;
   tree->sym->parameters = nullptr;
   tree->sym->filenr = filenr;
   tree->sym->linenr = linenr;
   tree->sym->offset = offset;
   tree->sym->struct_name = nullptr;
   tree->sym->owning_struct = nullptr;
   tree->rnum = 0;
   DEBUGF ('f', "astree %p->{%d:%d.%d: %s: \"%s\"}\n",
           tree, tree->filenr, tree->linenr, tree->offset,
           get_yytname (tree->symbol), tree->lexinfo->c_str());
   return tree;
}

astree* adopt1 (astree* root, astree* child) {
   root->children.push_back (child);
   DEBUGF ('a', "%p (%s) adopting %p (%s)\n",
           root, root->lexinfo->c_str(),
           child, child->lexinfo->c_str());
   return root;
}

astree* adopt2 (astree* root, astree* left, astree* right) {
   adopt1 (root, left);
   adopt1 (root, right);
   return root;
}

astree* adopt1sym (astree* root, astree* child, int symbol) {
   root = adopt1 (root, child);
   root->symbol = symbol;
   return root;
}


static void dump_node (FILE* outfile, astree* node) {
   fprintf (outfile, "%s \"%s\" (%ld.%ld.%ld) {%ld} ",
            get_yytname (node->symbol), node->lexinfo->c_str(), 
            node->filenr, node->linenr, node->offset, 
            node->sym->block_nr);
   attr_bitset attr = node->sym->attributes;
   if (attr[ATTR_struct]) fprintf (outfile, "struct ");
   if (attr[ATTR_array]) fprintf (outfile, "array ");
   if (attr[ATTR_bool]) fprintf (outfile, "bool ");
   if (attr[ATTR_char]) fprintf (outfile, "char ");
   if (attr[ATTR_int]) fprintf (outfile, "int ");
   if (attr[ATTR_string]) fprintf (outfile, "string ");
   if (attr[ATTR_variable]) fprintf (outfile, "variable ");
   if (attr[ATTR_lval]) fprintf (outfile, "lval ");
   fprintf (outfile, "\n");
}

static void dump_astree_rec (FILE* outfile, astree* root,
                             int depth) {
   if (root == NULL) return;
   for (int itor = 0; itor < depth; ++itor)
   {
      fprintf (outfile, "|  ");
   }
   dump_node (outfile, root);
   for (size_t child = 0; child < root->children.size();
        ++child) {
      dump_astree_rec (outfile, root->children[child],
                       depth + 1);
   }
}

void dump_astree (FILE* outfile, astree* root) {
   dump_astree_rec (outfile, root, 0);
   fflush (NULL);
}

/*
void yyprint (FILE* outfile, unsigned short toknum,
              astree* yyvaluep) {
   if (is_defined_token (toknum)) {
      dump_node (outfile, yyvaluep);
   }else {
      fprintf (outfile, "%s(%d)\n",
               get_yytname (toknum), toknum);
   }
   fflush (NULL);
}
*/

void free_ast (astree* root) {
   while (not root->children.empty()) {
      astree* child = root->children.back();
      root->children.pop_back();
      free_ast (child);
   }
   DEBUGF ('f', "free [%p]-> %d:%d.%d: %s: \"%s\")\n",
           root, root->filenr, root->linenr, root->offset,
           get_yytname (root->symbol), root->lexinfo->c_str());
   delete root;
}

void free_ast2 (astree* tree1, astree* tree2) {
   free_ast (tree1);
   free_ast (tree2);
}

RCSC("$Id: astree.cpp,v 1.1 2014-10-03 18:22:05-07 - - $")

