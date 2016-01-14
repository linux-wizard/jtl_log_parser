CUST_RECURSIVE_TARGETS = coverage-clean-recursive coverage-recursive

$(CUST_RECURSIVE_TARGETS):
	@fail= failcom='exit 1'; \
	for f in $$MAKEFLAGS; do \
		case $$f in \
		*=* | --[!k]*);; \
		*k*) failcom='fail=yes';; \
		esac; \
	done; \
	dot_seen=no; \
	target=`echo $@ | sed s/-recursive//`; \
	list='$(SUBDIRS)'; for subdir in $$list; do \
		if test "$$subdir" = "."; then \
			dot_seen=yes; \
			local_target="$$target-am"; \
		else \
			local_target="$$target"; \
		fi; \
		($(am__cd) $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$local_target) || eval $$failcom; \
	done; \
	if test "$$dot_seen" = "no"; then \
		$(MAKE) $(AM_MAKEFLAGS) "$$target-am" || exit 1; \
	fi; test -z "$$fail"

coverage: coverage-recursive
coverage-clean: coverage-clean-recursive

coverage-clean-am:
	-rm -f *.info *.gcda

coverage-check:
	$(MAKE) CXXFLAGS="-O0 -ggdb -fno-inline -fprofile-arcs -ftest-coverage" check

@snippet@

.PHONY: $(CUST_RECURSIVE_TARGETS) \
	coverage coverage-am \
	coverage-clean coverage-clean-am
