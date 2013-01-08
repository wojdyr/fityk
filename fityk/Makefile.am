EXTRA_DIST = swig/fityk_perl.cpp swig/Fityk.pm swig/fityk_ruby.cpp swig/java/ \
	     swig/fityk.i
BUILT_SOURCES = swig/fityk_lua.cpp swig/luarun.h

lib_LTLIBRARIES = libfityk.la

libfityk_la_LDFLAGS = $(LIBRARY_VERSION_FLAG) -no-undefined
libfityk_la_LIBADD = -lxy -lz $(LUA_LIB)
libfityk_la_CPPFLAGS = $(LUA_INCLUDE)


libfityk_la_SOURCES = logic.cpp view.cpp lexer.cpp eparser.cpp cparser.cpp \
		 runner.cpp info.cpp common.cpp data.cpp var.cpp mgr.cpp \
		 tplate.cpp func.cpp udf.cpp bfunc.cpp ast.cpp \
		 vm.cpp transform.cpp settings.cpp ui.cpp ui_api.cpp \
		 GAfit.cpp LMfit.cpp guess.cpp NMfit.cpp \
		 model.cpp fit.cpp voigt.cpp numfuncs.cpp fityk.cpp \
		 \
                 logic.h view.h lexer.h eparser.h cparser.h \
		 runner.h info.h common.h data.h var.h mgr.h \
		 tplate.h func.h udf.h bfunc.h ast.h \
		 vm.h transform.h settings.h ui.h \
		 GAfit.h LMfit.h guess.h NMfit.h \
		 model.h fit.h voigt.h numfuncs.h gauss-legendre.h \
		 swig/fityk_lua.cpp swig/luarun.h \
		 CMPfit.cpp CMPfit.h cmpfit/mpfit.c cmpfit/mpfit.h

if NLOPT_ENABLED
libfityk_la_SOURCES += NLfit.cpp NLfit.h
endif

pkginclude_HEADERS = fityk.h ui_api.h


# --- directory swig/ ---

if PYTHON_ENABLED
python_PYTHON = $(srcdir)/swig/fityk.py
pyexec_LTLIBRARIES = swig/_fityk.la
endif

swig__fityk_la_SOURCES = swig/fityk_python.cpp
swig__fityk_la_CPPFLAGS = $(PYTHON_CPPFLAGS) -I$(srcdir)
swig__fityk_la_LDFLAGS = -module -avoid-version -shared -no-undefined
swig__fityk_la_LIBADD = libfityk.la

$(srcdir)/swig/luarun.h:
	$(SWIG) -c++ -lua -external-runtime $@

$(srcdir)/swig/fityk_python.cpp: swig/fityk.i fityk.h ui_api.h
	$(SWIG) -c++ -python -I$(srcdir) -o $@ $<
$(srcdir)/swig/fityk.py: $(srcdir)/swig/fityk_python.cpp

$(srcdir)/swig/fityk_lua.cpp: swig/fityk.i fityk.h
	$(SWIG) -c++ -lua -I$(srcdir) -o $@ $<

$(srcdir)/swig/fityk_perl.cpp: swig/fityk.i fityk.h
	$(SWIG) -c++ -perl -I$(srcdir) -o $@ $<
$(srcdir)/swig/Fityk.pm: $(srcdir)/swig/fityk_perl.cpp

$(srcdir)/swig/fityk_ruby.cpp: swig/fityk.i fityk.h
	$(SWIG) -c++ -ruby -I$(srcdir) -o $@ $<

$(srcdir)/swig/java/fityk_java.cpp: swig/fityk.i fityk.h
	mkdir -p $(srcdir)/swig/java
	$(SWIG) -c++ -java -I$(srcdir) -o $@ -package fityk -outdir $(srcdir)/swig/java $<

perl: swig/fityk_perl.o
	mkdir -p swig/perl
	$(CXX) -shared $< -L./.libs -lfityk -o swig/perl/Fityk.so

ruby: swig/fityk_ruby.o
	mkdir -p swig/ruby
	$(CXX) -shared $(CXXFLAGS) $(LDFLAGS) $< -L./.libs -lfityk -o swig/ruby/fityk.so

java: swig/java/fityk_java.o
	$(CXX) -shared $(CXXFLAGS) $(LDFLAGS) $< -L./.libs -lfityk -o swig/java/libfitykJ.so

# requires perl-ExtUtils-Embed
swig/fityk_perl.o: swig/fityk_perl.cpp
	$(CXX) -fPIC `perl -MExtUtils::Embed -e perl_inc` -I$(srcdir) -o $@ -c $<

swig/fityk_ruby.o: swig/fityk_ruby.cpp
	$(CXX) $(CPPFLAGS) -I$(srcdir) \
	    -I`ruby -e "require 'rbconfig'; puts Config::CONFIG['archdir']"` \
	    $(CXXFLAGS) -fPIC -o $@ -c $<

swig/java/fityk_java.o: swig/java/fityk_java.cpp
	mkdir -p swig/java
	$(CXX) $(CPPFLAGS) -I$(srcdir) \
	    $(CXXFLAGS) -fno-strict-aliasing -fPIC -o $@ -c $<
