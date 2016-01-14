AC_INIT([jmx_log_parser], [0.0.0], [bug-automake@gnu.org])
AM_INIT_AUTOMAKE([foreign -Wall -Werror -Wno-portability -Wno-unsupported])

AC_PROG_CXX
AM_PROG_AR

AC_CONFIG_MACRO_DIR([m4])

AC_DEFUN([APR_CONFIG],
[
	APR_FIND_APR([], [], 1, [1])

	if ! test "$apr_found" = "yes"; then
		AC_MSG_ERROR([no apache runtime libraries])
	fi
	APR_INCLUDES=`$apr_config --includes`
	APR_CXXFLAGS=`$apr_config --cppflags`
	APR_LIBS=`$apr_config --link-libtool --libs`
	APR_LDFLAGS=`$apr_config --ldflags`

	AC_SUBST([APR_INCLUDES])
	AC_SUBST([APR_CXXFLAGS])
	AC_SUBST([APR_LIBS])
	AC_SUBST([APR_LDFLAGS])
])

AC_DEFUN([APU_CONFIG],
[
	APR_FIND_APU([], [], 1, [1])

	if ! test "$apu_found" = "yes"; then
		AC_MSG_ERROR([no pache utils libraries])
	fi
	APU_INCLUDES=`$apu_config --includes`
	APU_LIBS=`$apu_config --link-libtool --libs`
	APU_LDFLAGS=`$apu_config --ldflags`

	AC_SUBST([APU_INCLUDES])
	AC_SUBST([APU_LIBS])
	AC_SUBST([APU_LDFLAGS])
])

AC_SUBST([APACHE2_INCLUDES], [-I/usr/include/apache2])

APR_CONFIG
APU_CONFIG

AC_ENABLE_STATIC(no)
AC_PROG_LIBTOOL
AM_CONDITIONAL(is_x86_64, [ test "$HOSTTYPE" = x86_64 ])

AC_ARG_ENABLE(debug, [AS_HELP_STRING([--enable-debug], [enable debug @<:@default=no@:>@])], [ if test "${enable_debug:-no}" != no; then CXXFLAGS="${CXXFLAGS} -Wall -Wnon-virtual-dtor -g -O0 -fno-inline -std=c++1y"; fi ], [CXXFLAGS="${CXXFLAGS} -Wall -Wnon-virtual-dtor -DNDEBUG=1 -O2 -std=c++1y"])

AC_PREFIX_DEFAULT([/usr])
AC_CONFIG_COMMANDS_PRE([
if test "$sysconfdir" = '${prefix}/etc'; then sysconfdir=/etc; fi;
if test "$localstatedir" = '${prefix}/var'; then localstatedir=/var; fi
])

dnl
snippet='
ifeq ($(strip $(foreach d,$(SUBDIRS),$(wildcard $(d)/*.gcda))),)
ifeq ($(strip $(foreach d,$(SUBDIRS),$(wildcard $(d)/$(d).info))),)
coverage-am:
else
coverage-am: $(notdir $(CURDIR)).info
$(notdir $(CURDIR)).info: $(foreach d,$(SUBDIRS),$(wildcard $(d)/$(d).info))
	if ! test -r $@; then \
		lcovargs=""; \
	else \
		lcovargs=" --add-tracefile $@"; \
	fi; \
	for p in $?; do \
		if test -s $${p}; \
		then \
			lcovargs+=" --add-tracefile $${p}"; \
		fi; \
	done; \
	if ! test -z "$${lcovargs}"; \
	then \
		lcov -q --no-external --base-directory $(top_srcdir)/src --compat libtool=on,split_crc=on --test-name $(notdir $(CURDIR)) --output-file $@.tmp $${lcovargs}; \
	fi; \
	mv $@.tmp $@
endif
else
coverage-am: $(notdir $(CURDIR)).info
ifeq ($(strip $(foreach d,$(SUBDIRS),$(wildcard $(d)/$(d).info))),)
$(notdir $(CURDIR)).info: $(foreach d,$(SUBDIRS) .libs,$(wildcard $(d)/*.gcda))
	dirs=""; \
	for p in $(sort $(dir $(filter %.gcda,$?))); do \
		dirs+=" --directory $${p}"; \
	done; \
	lcov -q --no-external --base-directory $(top_srcdir)/src --compat libtool=on,split_crc=on --output-file $@.full --capture $$dirs --test-name $(notdir $(CURDIR)); \
	lcov -q --no-external --base-directory $(top_srcdir)/src --compat libtool=on,split_crc=on --output-file $@.tmp --extract $@.full "*/$(notdir $(top_srcdir))/*"; \
	lcov -q --no-external --base-directory $(top_srcdir)/src --compat libtool=on,split_crc=on --output-file $@ --remove $@.tmp "*/unit/*" "*/TestHelpers/*" "*/Cassandra/*"; \
	rm $@.full $@.tmp
else
$(notdir $(CURDIR)).info: $(foreach d,$(SUBDIRS) .libs,$(wildcard $(d)/*.gcda)) $(foreach d,$(SUBDIRS),$(wildcard $(d)/$(d).info))
	dirs=""; \
	for p in $(sort $(dir $(filter %.gcda,$?))); do \
		dirs+=" --directory $${p}"; \
	done; \
	lcov -q --no-external --base-directory $(top_srcdir)/src --compat libtool=on,split_crc=on --output-file $@.full --capture $$dirs --test-name $(notdir $(CURDIR)); \
	lcov -q --no-external --base-directory $(top_srcdir)/src --compat libtool=on,split_crc=on --output-file $@.tmp --extract $@.full "*/$(notdir $(top_srcdir))/*" ; \
	lcov -q --no-external --base-directory $(top_srcdir)/src --compat libtool=on,split_crc=on --output-file $@.tmpinfo --remove $@.tmp "*/unit/*" "*/TestHelpers/*" "*/Cassandra/*"; \
	rm $@.full $@.tmp; \
	lcovargs=" --add-tracefile $@.tmpinfo"; \
	if test -r $@; then \
		lcovargs=" --add-tracefile $@"; \
	fi; \
	for p in $(filter %.info,$?); do \
		lcovargs+=" --add-tracefile $${p}"; \
	done; \
	lcov -q --no-external --base-directory $(top_srcdir)/src --compat libtool=on,split_crc=on --test-name $(notdir $(CURDIR)) --output-file $@.tmp $$lcovargs; \
	rm $@.tmpinfo; \
	mv $@.tmp $@
endif
endif
'

AC_SUBST([snippet])
AM_SUBST_NOTMAKE([snippet])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
	Makefile
	src/Makefile
	conf/Makefile
])

AC_OUTPUT
