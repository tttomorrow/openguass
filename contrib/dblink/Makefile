# contrib/dblink/Makefile

MODULE_big = dblink
OBJS	= dblink.o
PG_CPPFLAGS = -I$(libpq_srcdir)
SHLIB_LINK = $(libpq) -L$(LIBODBC_LIB_PATH) -lodbc
SHLIB_PREREQS = submake-libpq

EXTENSION = dblink
DATA = dblink--1.0.sql dblink--unpackaged--1.0.sql

exclude_option = -fPIE
override CPPFLAGS := $(filter-out $(exclude_option),$(CPPFLAGS))

REGRESS = dblink

# the db name is hard-coded in the tests
override USE_MODULE_DB =

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/dblink
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
