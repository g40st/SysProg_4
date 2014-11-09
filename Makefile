# Makefile für das Multiplayer-Quiz
# Stefan Gast, 2013, 2014
#
# Thomas Buck, 2014, added WxWidgets GUI support (currently Darwin only)
###############################################################################

#################
# Konfiguration #
#################

OUTPUT_TARGETS = bin/server bin/client bin/loader bin/guiTest
WARNINGS = -Wall
DIALECT_OPTS = -std=gnu99

ARCH = $(shell uname -m)
ARCH2 = $(shell uname -s)
CCARCH_32 = -march=i686
CCARCH_64 =

ifndef CFLAGS
CFLAGS = -pipe -ggdb
ifeq ($(ARCH2),Darwin)
LIBQUIZGUI=
GTK_LIBS=$(shell wx-config --libs) -lstdc++
CFLAGS+=$(shell wx-config --cflags) -pthread
RT=
else
LIBQUIZGUI = client/gui/libquizgui-$(ARCH).a
GTK_LIBS = `pkg-config --libs gtk+-2.0` `pkg-config --libs gthread-2.0`
CFLAGS += -pthread
RT=-lrt
ifeq ($(ARCH),i686)
CFLAGS += $(CCARCH_32)
else
ifeq ($(ARCH),x86_64)
CFLAGS += $(CCARCH_64)
endif
endif
endif
endif

DIALECT_OPTS += $(WARNINGS)

CLEANFILES = server/*.o server/*.dep \
	     client/*.o client/*.dep \
	     loader/*.o loader/*.dep \
	     common/*.o common/*.dep \
		 client/gui/*.o client/gui/*.dep
MRPROPERFILES = $(OUTPUT_TARGETS)

###############################################################################

################################################
# Module der Programme Server, Client und Loader
################################################

SERVER_MODULES=server/catalog.o \
	       server/clientthread.o \
	       server/login.o \
	       server/main.o \
	       server/score.o \
	       server/user.o \
	       common/rfc.o \
	       common/util.o

CLIENT_MODULES=client/fragewechsel.o \
		   client/gui.o \
	       client/listener.o \
	       client/main.o \
	       common/rfc.o \
	       common/util.o

LOADER_MODULES=loader/browse.o \
	       loader/load.o \
	       loader/main.o \
	       loader/parser.o \
	       loader/util.o \
	       common/util.o

GUI_MODULES=client/gui/guiApp.o \
			client/gui/guiGame.o \
			client/gui/guiMain.o \
			client/gui/guiPreparation.o

TEST_MODULES = client/gui/guiTest.o
TEST_MODULES += common/util.o

ifeq ($(ARCH2),Darwin)
CLIENT_MODULES += $(GUI_MODULES)
TEST_MODULES += $(GUI_MODULES)
endif

###############################################################################

########################################
# Target zum Kompilieren aller Programme
########################################

.PHONY: all
all: $(OUTPUT_TARGETS)

###############################################################################

########################################################
# Includes für eventuell generierte Abhängigkeiten + GUI
########################################################

-include client/gui/gui.mk
-include $(SERVER_MODULES:.o=.dep)
-include $(CLIENT_MODULES:.o=.dep)
-include $(LOADER_MODULES:.o=.dep)

ifeq ($(ARCH2),Darwin)
-include $(GUI_MODULES:.o=.dep)
endif

###############################################################################

#######################
# Targets zum Aufräumen
#######################

.PHONY: clean
clean:
	@for i in $(CLEANFILES) ; do \
		[ -f "$$i" ] && rm -v "$$i" ;\
	done ;\
	exit 0

.PHONY: mrproper
mrproper: clean
	@for i in $(MRPROPERFILES) ; do \
		[ -f "$$i" ] && rm -v "$$i" ;\
	done ;\
	exit 0

###############################################################################

##################################################
# Targets zum Linken von Server, Client und Loader
##################################################

bin/server: $(SERVER_MODULES)
	$(CC) $(CFLAGS) -o $@ $^ $(RT)

bin/client: $(CLIENT_MODULES) $(LIBQUIZGUI)
	$(CC) $(CFLAGS) -o $@ $^ $(GTK_LIBS) $(RT)

bin/loader: $(LOADER_MODULES)
	$(CC) $(CFLAGS) -o $@ $^ $(RT)

bin/guiTest: $(TEST_MODULES) $(LIBQUIZGUI)
	$(CC) $(CFLAGS) -o $@ $^ $(GTK_LIBS) $(RT)

###############################################################################

###########################################################
# Kompilieren der Module und Erzeugen der Abhängigkeiten
# (siehe http://scottmcpeak.com/autodepend/autodepend.html)
###########################################################

%.o: %.c
	$(CC) -c -I. $(CFLAGS) $(DIALECT_OPTS) -o $@ $<
	@$(CC) -MM -I. $(CFLAGS) $< > $*.dep
	@mv -f $*.dep $*.dep.tmp
	@sed -e 's|.*:|$*.o:|' < $*.dep.tmp > $*.dep
	@sed -e 's/.*://' -e 's/\\$$//' < $*.dep.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.dep
	@rm -f $*.dep.tmp

%.o: %.cpp
	$(CC) -c -I. $(CFLAGS) $(WARNINGS) -o $@ $<
	@$(CC) -MM -I. $(CFLAGS) $< > $*.dep
	@mv -f $*.dep $*.dep.tmp
	@sed -e 's|.*:|$*.o:|' < $*.dep.tmp > $*.dep
	@sed -e 's/.*://' -e 's/\\$$//' < $*.dep.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.dep
	@rm -f $*.dep.tmp

