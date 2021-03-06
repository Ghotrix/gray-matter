#==============================================================================#
#	GNUmakefile                                                                #
#                                                                              #
#	Copyright � 2005-2008, The Gray Matter Team, original authors.             #
#==============================================================================#

# Your operating system.  This must be LINUX, OS_X, or WINDOWS.
# BSD is based on OS X.  If you insist on running BSD, set this to OS_X.  ;-)
PLAT = LINUX

# Your processor.  This must be i386, i486, i586, pentium, pentium-mmx, i686,
# pentiumpro, pentium2, pentium3, pentium3m, pentium-m, pentium4, pentium4m,
# prescott, nocona, k6, k6-2, k6-3, athlon, athlon-tbird, athlon-4, athlon-xp,
# athlon-mp, k8, opteron, athlon64, athlon-fx, winchip-c6, winchip2, c3, or
# c3-2.
ARCH = native

# Subversion macros.
SVNDEF := -D'SVN_REV="$(shell svnversion -n .)"'

CXX  = g++
LANG = -ansi
WARN = -Wall #-Werror
OPTI = -g -O3 $(SVNDEF)
#OPTI += -fomit-frame-pointer
#OPTI += -DDEBUG_SEARCH
PREP = -D$(PLAT)
LINK = -lpthread
DIR  = -Iinc
MACH = -march=$(ARCH)

SRCS = $(wildcard src/*.cpp)
OBJS = $(addprefix bin/,$(notdir $(subst .cpp,.o,$(SRCS))))
DEPS = $(addsuffix .d,$(basename $(OBJS)))

# Uncomment for profile information
PFC = #-pg -fno-inline
PFL = #-pg

.PHONY: all clean doc
all : bin/gray

# If not cleaning, etc., use dependencies.
ifeq ($(filter clean,$(MAKECMDGOALS)),)
-include $(DEPS)
endif

clean :
	rm -f $(OBJS) $(DEPS) bin/gray

bin/%.o : src/%.cpp inc/%.h
	$(CXX) -c -o $@ $< $(LANG) $(WARN) $(OPTI) $(PREP) $(DIR) $(MACH) $(PFC)

bin/gray : $(OBJS)
	$(CXX) -o $@ $(LANG) $(WARN) $(OPTI) $(PREP) $(LINK) $(DIR) $(MACH) $(OBJS) $(PFL)

bin/%.d: src/%.cpp
	@set -e; $(CC) $(CPPFLAGS) -Iinc -DBUILDDEPS -MM $< \
		| sed 's/\($*\)\.o[ :]*/bin\/\1.o bin\/$*.d : GNUmakefile /g' > $@; \
		[ -s $@ ] || rm -f $@

doc:
	@doxygen doxygen.cfg
