noinst_PROGRAMS = test_input test_parse test_output normalize

test_input_SOURCES = test_input.c
test_input_LDADD = ../libtreecc.a

test_parse_SOURCES = test_parse.c
test_parse_LDADD = ../libtreecc.a

test_output_SOURCES = test_output.c
test_output_LDADD = ../libtreecc.a

normalize_SOURCES = normalize.c

TESTS = run_tests
TEST_ENVIRONMENT = $(SHELL)

AM_CFLAGS = -I$(srcdir)/..
