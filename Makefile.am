SUBDIRS = . etc doc tests examples

bin_PROGRAMS = treecc
noinst_LIBRARIES = libtreecc.a

treecc_SOURCES = main.c
treecc_LDADD = libtreecc.a

libtreecc_a_SOURCES = context.c \
					  errors.c \
					  gen.c \
					  gen_c.c \
					  gen_cpp.c \
					  gen_cs.c \
					  gen_java.c \
				      input.c \
				      literal.c \
				      node.c \
					  oper.c \
					  options.c \
				      parse.c \
					  skeleton.c \
					  stream.c

if USINGGCC
INCLUDES = -Wall -DTREECC_DATA_DIR=\"$(datadir)/treecc\"
else
INCLUDES = -DTREECC_DATA_DIR=\"$(datadir)/treecc\"
endif
