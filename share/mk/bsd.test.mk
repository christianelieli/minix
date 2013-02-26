# $NetBSD: bsd.test.mk,v 1.24 2013/02/23 22:01:51 jmmv Exp $
#

.include <bsd.init.mk>

_TESTS:=	# empty

.if defined(TESTS_SUBDIRS)
SUBDIR+=	${TESTS_SUBDIRS}
.endif

.include <bsd.subdir.mk>

.if defined(TESTS_C)
_TESTS+=	${TESTS_C}
PROGS+=		${TESTS_C}
.  for _T in ${TESTS_C}
BINDIR.${_T}=	${TESTSDIR}
LDADD.${_T}+=	-latf-c
DPADD.${_T}+=	${LIBATF_C}
MAN.${_T}?=	# empty
.  endfor
.endif

.if defined(TESTS_CXX)
_TESTS+=	${TESTS_CXX}
PROGS_CXX+=	${TESTS_CXX}
.  for _T in ${TESTS_CXX}
BINDIR.${_T}=	${TESTSDIR}
LDADD.${_T}+=	-latf-c++ -latf-c
DPADD.${_T}+=	${LIBATF_CXX} ${LIBATF_C}
MAN.${_T}?=	# empty
#.if defined(__MINIX) # LSC: Can't test as bsd.own.mk may not have been sourced
# Somehow it resets / ignore the settings in bsd.prog.mk, so duplicate here
LDADD.${_T}+= -lgcc_s
DPADD.${_T}+= ${DESTDIR}/usr/lib/libgcc_s.a
#.endif # defined(__MINIX)
.  endfor
.endif

.if defined(TESTS_SH)
_TESTS+=		${TESTS_SH}
CLEANDIRFILES+=		${TESTS_SH}

.  for _T in ${TESTS_SH}
SCRIPTS+=		${_T}
SCRIPTSDIR_${_T}=	${TESTSDIR}

CLEANDIRFILES+=		${_T}.tmp

TESTS_SH_SRC_${_T}?=	${_T}.sh
${_T}: ${TESTS_SH_SRC_${_T}}
	${_MKTARGET_BUILD}
	echo '#! /usr/bin/atf-sh' >${.TARGET}.tmp
	cat ${.ALLSRC} >>${.TARGET}.tmp
	chmod +x ${.TARGET}.tmp
	mv ${.TARGET}.tmp ${.TARGET}
.  endfor
.endif

ATFFILE?=	auto
.if ${MKKYUA} != "no"
KYUAFILE?=	${ATFFILE}
.else
KYUAFILE=	no
.endif

# Additional list of 'tp' entries to add to the Atffile when ATFFILE=auto.
# These entries must all correspond to subdirectories to descend into.
#
# This is useful in the cases where the tests in a single directory come
# from various sources (e.g. src/tests and src/external/.../tests) and
# the installation of some of those tests rely on MK* variables being set.
ATFFILE_EXTRA_SUBDIRS?=

.if ${ATFFILE:tl} != "no"
FILES+=			Atffile
FILESDIR_Atffile=	${TESTSDIR}

.  if ${ATFFILE:tl} == "auto"
CLEANDIRFILES+=	Atffile Atffile.tmp

realall: Atffile
Atffile: Makefile
	${_MKTARGET_CREATE}
	@{ echo 'Content-Type: application/X-atf-atffile; version="1"'; \
	echo; \
	echo '# Automatically generated by bsd.test.mk.'; \
	echo; \
	echo 'prop: test-suite = "NetBSD"'; \
	echo; \
	for tp in ${TESTS_SUBDIRS:N.WAIT} ${_TESTS} ${ATFFILE_EXTRA_SUBDIRS}; \
	do \
	    echo "tp: $${tp}"; \
	done; } >Atffile.tmp
	@mv Atffile.tmp Atffile
.  endif
.endif

.if ${KYUAFILE:tl} != "no"
FILES+=			Kyuafile
FILESDIR_Kyuafile=	${TESTSDIR}

.  if ${KYUAFILE:tl} == "auto"
CLEANDIRFILES+=	Kyuafile Kyuafile.tmp

realall: Kyuafile
Kyuafile: Makefile
	${_MKTARGET_CREATE}
	@{ \
	    echo '-- Automatically generated by bsd.test.mk.'; \
	    echo; \
	    echo 'syntax(2)'; \
	    echo; \
	    echo 'test_suite("NetBSD")'; \
	    if [ -n "${_TESTS}" ]; then \
	        echo; \
	        for tp in ${_TESTS}; do \
	            echo "atf_test_program{name=\"$${tp}\"}"; \
	        done; \
	    fi; \
	    if [ -n "${TESTS_SUBDIRS:N.WAIT}" -o \
	         -n "${ATFFILE_EXTRA_SUBDIRS}" ]; then \
	        echo; \
	        for subdir in ${TESTS_SUBDIRS:N.WAIT} \
	                      ${ATFFILE_EXTRA_SUBDIRS}; do \
	            echo "include(\"$${subdir}/Kyuafile\")"; \
	        done; \
	    fi; \
	} >Kyuafile.tmp
	@mv Kyuafile.tmp Kyuafile
.  endif
.endif

.if ${ATFFILE:tl} != "no" || ${KYUAFILE:tl} != "no"
.  include <bsd.files.mk>
.endif

.if !empty(SCRIPTS) || !empty(PROGS) || !empty(PROGS_CXX)
.  include <bsd.prog.mk>
.endif

#
# Definition of the "make test" target and supporting variables.
#
# This target, by necessity, can only work for native builds (i.e. a NetBSD
# host building a release for the same system).  The target runs ATF, which is
# not in the toolchain, and the tests execute code built for the target host.
#
# Due to the dependencies of the binaries built by the source tree and how they
# are used by tests, it is highly possible for a execution of "make test" to
# report bogus results unless the new binaries are put in place.
#

TESTS_PATH += ${DESTDIR}/bin ${DESTDIR}/sbin ${DESTDIR}/usr/bin ${DESTDIR}/usr/sbin
TESTS_LD_LIBRARY_PATH += ${DESTDIR}/lib ${DESTDIR}/usr/lib

TESTS_ENV += ATF_BUILD_CC=${DESTDIR}/usr/bin/cc
TESTS_ENV += ATF_BUILD_CPP=${DESTDIR}/usr/bin/cpp
TESTS_ENV += ATF_BUILD_CXX=${DESTDIR}/usr/bin/c++
TESTS_ENV += ATF_CONFDIR=${DESTDIR}/etc
TESTS_ENV += ATF_INCLUDEDIR=${DESTDIR}/usr/include
TESTS_ENV += ATF_LIBDIR=${DESTDIR}/usr/lib
TESTS_ENV += ATF_LIBEXECDIR=${DESTDIR}/usr/libexec
TESTS_ENV += ATF_PKGDATADIR=${DESTDIR}/usr/share/atf
TESTS_ENV += ATF_SHELL=${DESTDIR}/bin/sh
TESTS_ENV += LD_LIBRARY_PATH=${TESTS_LD_LIBRARY_PATH:tW:S/ /:/g}
TESTS_ENV += PATH=${TESTS_PATH:tW:S/ /:/g}

_TESTS_FIFO = ${.OBJDIR}/atf-run.fifo
_TESTS_LOG = ${.OBJDIR}/atf-run.log
CLEANDIRFILES += ${_TESTS_FIFO} ${_TESTS_LOG}

.PHONY: test
.if defined(TESTSDIR)
.  if ${TESTSDIR} == ${TESTSBASE}
# Forbid this case.  It is likely to cause false positives/negatives and it
# does not cover all the tests (e.g. it misses testing software in external).
test:
	@echo "*** Sorry, you cannot use make test from src/tests.  Install the"
	@echo "*** tests into their final location and run them from /usr/tests"
	@false
.  else
test:
	@echo "*** WARNING: make test is experimental"
	@echo "***"
	@echo "*** Using this test does not preclude you from running the tests"
	@echo "*** installed in /usr/tests.  This test run may raise false"
	@echo "*** positives and/or false negatives."
	@echo
	@set -e; \
	cd ${DESTDIR}${TESTSDIR}; \
	rm -f ${_TESTS_FIFO}; \
	mkfifo ${_TESTS_FIFO}; \
	cat ${_TESTS_FIFO} | tee ${_TESTS_LOG} | \
	    ${TESTS_ENV} ${DESTDIR}/usr/bin/atf-report & \
	result=0; \
	${TESTS_ENV} ${DESTDIR}/usr/bin/atf-run >>${_TESTS_FIFO} || result=1; \
	wait; \
	rm -f ${_TESTS_FIFO}; \
	echo; \
	echo "*** The verbatim output of atf-run has been saved to ${_TESTS_LOG}"; \
	echo "*** Once again, note that "make test" is unsupported."; \
	test $${result} -eq 0
.  endif
.else
test:
	@echo "*** No TESTSDIR defined; nothing to do."
.endif

##### Pull in related .mk logic
.include <bsd.clean.mk>
