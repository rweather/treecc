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
					  gen_ruby.c \
					  gen_java.c \
					  gen_php.c \
				      input.c \
				      literal.c \
				      node.c \
					  oper.c \
					  options.c \
				      parse.c \
					  skeleton.c \
					  skels.c \
					  stream.c

SKELETON_FILES = $(top_srcdir)/etc/c_skel.c \
				 $(top_srcdir)/etc/c_skel.h \
				 $(top_srcdir)/etc/cpp_skel.cc \
				 $(top_srcdir)/etc/cpp_skel.h \
				 $(top_srcdir)/etc/c_gc_skel.h \
				 $(top_srcdir)/etc/c_gc_skel.c \
				 $(top_srcdir)/etc/cpp_gc_skel.h \
				 $(top_srcdir)/etc/cpp_gc_skel.cc

$(srcdir)/skels.c: $(SKELETON_FILES) $(top_srcdir)/mkskel-sh
	$(SHELL) $(top_srcdir)/mkskel-sh $(SKELETON_FILES) >$(srcdir)/skels.c

CLEANFILES = skels.c
