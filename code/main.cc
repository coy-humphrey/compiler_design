// Coy Humphrey 

#include <string>
#include <vector>
using namespace std;

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "auxlib.h"
#include "stringset.h"
#include "lyutils.h"
#include "astree.h"
#include "emit.h"

const size_t LINESIZE = 1024;
const string SUFFIX_STR = ".str";
const string SUFFIX_TOK = ".tok";
const string SUFFIX_AST = ".ast";
const string SUFFIX_SYM = ".sym";
const string SUFFIX_OIL = ".oil";
const string cpp_name = "/usr/bin/cpp";
string yyin_cpp_command;
string cpp_argv = "";
string fname = "";

// Declaring yyin in main for part 1. In later assignments, the extern
//yyin from lyutils will be used instead.
// FILE *yyin;

// Open a pipe from the C preprocessor.
// Exit failure if can't.
// Assignes opened pipe to FILE* yyin.
void yyin_cpp_popen (const char* filename, const char* argv) {
   // Command starts with name of cpp
   yyin_cpp_command = cpp_name;
   yyin_cpp_command += " ";
   // Then argument list, if any
   yyin_cpp_command += argv;
   yyin_cpp_command += " ";
   // Finally filename
   yyin_cpp_command += filename;
   // Open cpp pipe
   DEBUGF ('m', "CPP Command: %s\n", yyin_cpp_command.c_str());
   yyin = popen (yyin_cpp_command.c_str(), "r");
   if (yyin == NULL) {
      syserrprintf (yyin_cpp_command.c_str());
      // Set non-zero exit status
      set_exitstatus (EXIT_FAILURE);
      exit (get_exitstatus());
   }
}

void yyin_cpp_pclose (void) {
   int pclose_rc = pclose (yyin);
   eprint_status (yyin_cpp_command.c_str(), pclose_rc);
   if (pclose_rc != 0) set_exitstatus (EXIT_FAILURE);
}

bool want_echo () {
   return not (isatty (fileno (stdin)) and isatty (fileno (stdout)));
}

// Chomp the last character from a buffer if it is delim.
void chomp (char* string, char delim) {
   size_t len = strlen (string);
   if (len == 0) return;
   char* nlpos = string + len - 1;
   if (*nlpos == delim) *nlpos = '\0';
}


void scan_opts (int argc, char** argv) {
   int option;
   // Silences getopt error messages
   opterr = 0;
   yy_flex_debug = 0;
   yydebug = 0;
   for(;;) {
      option = getopt (argc, argv, "@:D:ly");
      if (option == EOF) break;
      switch (option) {
         case '@': set_debugflags (optarg);       break;
         case 'D': cpp_argv += optarg;            break;
         case 'l': yy_flex_debug = 1;             break;
         case 'y': yydebug = 1;               break;
         default:  
            errprintf ("%:bad option (%c)\n", optopt);
            set_exitstatus (EXIT_FAILURE); 
            break;
      }
   }
   DEBUGF ('m', "Check optind bounds\n");
   // Optind >= argc -> No filename argument
   // optind != argc - 1 -> Multiple filename arguments
   if (optind >= argc || optind != argc - 1) 
   {
      errprintf ("Usage: %s [-ly] [-@ flag] [-D string] [filename]\n",
                  get_execname());
      exit (get_exitstatus());
   }
   DEBUGF ('m', "Checked optind bounds: %d\n", optind);
   const char* filename = optind == argc ? "-" : argv[optind];
   
   
   // scanner_newfilename (filename);
   fname += filename;
   // Take basename of filename, strip oc off the end
   string tmp = "" + fname;
   fname = basename (tmp.c_str());
   string suffix = ".oc";
   size_t last_dot = fname.find_last_of (suffix.front());
   if (last_dot >= fname.length())
   {
      errprintf ("%s: Input file must end in %s\n", 
                   get_execname(), suffix.c_str());
      exit (get_exitstatus());
   }
   else if (fname.length() - last_dot != suffix.length())
   {
      errprintf ("%s: Input file must end in %s\n", 
                   get_execname(), suffix.c_str());
      exit (get_exitstatus());
   }
   else
   {
      // Compare last characters to see if they are same as suffix
      if (fname.compare (last_dot, suffix.length(), suffix) == 0)
      {
         // Ends in suffix
      }
      else
      {
         errprintf ("%s: Input file must end in %s\n", 
                   get_execname(), suffix.c_str());
         exit (get_exitstatus());
      }
   }
   // Strip suffix
   fname = fname.substr (0, last_dot);
   // If file has valid suffix, run cpp
   yyin_cpp_popen (filename, cpp_argv.c_str());
   DEBUGF ('m', "filename = %s, yyin = %p, fileno (yyin) = %d\n",
           filename, yyin, fileno (yyin));
   DEBUGF ('m', "Stripped filename = %s\n", fname.c_str());
}

int main (int argc, char** argv) {
   set_execname (argv[0]);
   scan_opts (argc, argv);
   // Open .tok output file declared in lyutils.
   string outputn_tok = fname + SUFFIX_TOK;
   output_tok = fopen (outputn_tok.c_str(), "w");
   
   // Run yylex until it hits EOF
   yyparse();
   //yyparse();
   
   yyin_cpp_pclose();
   
   string outputn_sym = fname + SUFFIX_SYM;
   FILE *outputf_sym = fopen (outputn_sym.c_str(), "w");
   if (outputf_sym == NULL)
   {
      errprintf ("%s: Could not open %s for writing\n", 
                  get_execname(), outputn_sym.c_str());
      exit (get_exitstatus());
   }
   
   type_check (yyparse_astree, outputf_sym);
   fclose (outputf_sym);
   
   // Write .str file
   string outputn_str = fname + SUFFIX_STR;
   FILE *outputf_str = fopen (outputn_str.c_str(), "w");
   if (outputf_str == NULL)
   {
      errprintf ("%s: Could not open %s for writing\n", 
                  get_execname(), outputn_str.c_str());
      exit (get_exitstatus());
   }
   // Dump stringset into file
   // stringset is updated in lyutils each time a token is found
   dump_stringset (outputf_str);
   // Dump AST into file
   string outputn_ast = fname + SUFFIX_AST;
   FILE *outputf_ast = fopen (outputn_ast.c_str(), "w");
   dump_astree (outputf_ast, yyparse_astree);
   string outputn_oil = fname + SUFFIX_OIL;
   FILE *outputf_oil = fopen (outputn_oil.c_str(), "w");
   emit (yyparse_astree, outputf_oil);
   fclose (outputf_oil);
   fclose (outputf_ast);
   fclose (output_tok);
   fclose (outputf_str);
   yylex_destroy();
   // Free ast objects created by yylex
   free_ast (yyparse_astree);
   return get_exitstatus();
}
















