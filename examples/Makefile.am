noinst_PROGRAMS = expr_c @EXPR_CPP@
EXTRA_PROGRAMS  = expr_cpp

expr_c_SOURCES = expr_c.c gram_c.y scan_c.l
expr_c_LDADD = -lm

expr_cpp_SOURCES = expr_cpp.cc gram_cpp.yy scan_cpp.ll

expr_c.c expr_c.h: expr_c.tc
	../treecc -s $(srcdir)/../etc expr_c.tc

expr_cpp.cc expr_cpp.h: expr_cpp.tc
	../treecc -s $(srcdir)/../etc expr_cpp.tc

AM_YFLAGS = -d

CLEANFILES = expr_c.c expr_c.h expr_cpp.cc expr_cpp.h expr_cs.cs \
			 gram_c.c gram_c.h gram_cpp.cc gram_cpp.h \
			 scan_c.c scan_cpp.cc

if USINGGCC
INCLUDES = -Wall
endif
