# Coy Humphrey 
# $Id: Makefile,v 1.11 2013-10-15 16:37:56-07 - - $
ASG       = asg5

MKFILE    = Makefile
DEPSFILE  = ${MKFILE}.deps
NOINCLUDE = ci clean spotless
NEEDINCL  = ${filter ${NOINCLUDE}, ${MAKECMDGOALS}}
VALGRIND  = valgrind --leak-check=full --show-reachable=yes

#
# Definitions of list of files:
#
HSOURCES  = astree.h  lyutils.h  auxlib.h  stringset.h symutils.h emit.h
CSOURCES1 = astree.cc lyutils.cc auxlib.cc stringset.cc main.cc
CSOURCES  = ${CSOURCES1} symutils.cc emit.cc
LSOURCES  = scanner.l
YSOURCES  = parser.y
ETCSRC    = README ${MKFILE} ${DEPSFILE}
CLGEN     = yylex.cc
HYGEN     = yyparse.h
CYGEN     = yyparse.cc
CGENS     = ${CLGEN} ${CYGEN}
ALLGENS   = ${HYGEN} ${CGENS}
EXECBIN   = oc
ALLCSRC   = ${CSOURCES} ${CGENS}
OBJECTS   = ${ALLCSRC:.cc=.o}
LREPORT   = yylex.output
YREPORT   = yyparse.output
REPORTS   = ${LREPORT} ${YREPORT}
ALLSRC    = ${ETCSRC} ${YSOURCES} ${LSOURCES} ${HSOURCES} ${CSOURCES}
LISTSRC   = ${ALLSRC} ${HYGEN}

SUBMITSRC   = ${HSOURCES} ${CSOURCES} ${LSOURCES} ${YSOURCES}
SUBMITFILES = ${SUBMITSRC} README ${MKFILE}

#
# Definitions of the compiler and compilation options:
#
GCC       = g++ -g -O0 -Wall -Wextra -std=gnu++11
MKDEPS    = g++ -MM -std=gnu++11

#
# The first target is always ``all'', and hence the default,
# and builds the executable images
#
all : ${EXECBIN}

#
# Build the executable image from the object files.
#
${EXECBIN} : ${OBJECTS}
	${GCC} -o${EXECBIN} ${OBJECTS}

#
# Build an object file form a C source file.
#
%.o : %.cc
	${GCC} -c $<


#
# Build the scanner.
#
${CLGEN} : ${LSOURCES}
	flex --outfile=${CLGEN} ${LSOURCES} 2>${LREPORT}
	- grep -v '^  ' ${LREPORT}

#
# Build the parser.
#
${CYGEN} ${HYGEN} : ${YSOURCES}
	bison --defines=${HYGEN} --output=${CYGEN} ${YSOURCES}

#
# Check sources into a git subdirectory.
#
ci : ${SUBMITFILES}
	git add ${SUBMITFILES}
	git commit -m 'Makefile commit'


#
# Clean and spotless remove generated files.
#
clean :
	- rm ${OBJECTS} ${ALLGENS} ${REPORTS} ${DEPSFILE}

spotless : clean
	- rm ${EXECBIN}

#
# Submit files for grading
#
submit : ${SUBMITFILES}
	submit cmps104a-wm.f14 ${ASG} ${SUBMITFILES}

#
# Run checksource on files to be submitted
#
check : ${SUBMITFILES}
	checksource ${SUBMITFILES}


#
# Build the dependencies file using the C preprocessor
#
deps : ${ALLCSRC}
	@ echo "# ${DEPSFILE} created `date` by ${MAKE}" >${DEPSFILE}
	${MKDEPS} ${ALLCSRC} >>${DEPSFILE}

${DEPSFILE} :
	@ touch ${DEPSFILE}
	${MAKE} --no-print-directory deps

#
# Everything
#
again :
	make --no-print-directory spotless deps ci all
	
ifeq "${NEEDINCL}" ""
include ${DEPSFILE}
endif
