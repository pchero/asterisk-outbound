#Makefile
# Created on: Nov 9, 2015
#     Author: pchero


#### Compiler and tool definitions shared by all build targets #####
CC = gcc
UNAME := $(shell uname)
# -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations
#BASICOPTS = -g -pthread -pipe -g3 -O6 -fPIC -DAST_MODULE=\"res_outbound\"
BASICOPTS = -Wall -g -pthread -pipe -g3 -O6 -fPIC -DAST_MODULE=\"res_outbound\"
CFLAGS = $(BASICOPTS)
PKGCONFIG="pkg-config"
OSLDLIBS=

# Define the target directories.
TARGETDIR_res_outbound.so=build

ifeq ($(UNAME), Linux)
	SHAREDLIB_FLAGS_res_outbound.so = -shared -Xlinker -x -Wl,--hash-style=gnu -Wl,--as-needed -rdynamic
endif

ifeq ($(UNAME), Darwin)
	PKGCONFIG=$(shell if [ "x$(HOMEBREW_PREFIX)" == "x" ];then echo "/usr/local/bin/pkg-config"; else echo "$(HOMEBREW_PREFIX)/bin/pkg-config"; fi)

	# Link or archive
	SHAREDLIB_FLAGS_res_outbound.so = -bundle -Xlinker -macosx_version_min -Xlinker 10.4 -Xlinker -undefined -Xlinker dynamic_lookup -force_flat_namespace
	OSLDLIBS=/usr/lib/bundle1.o
endif

all: $(TARGETDIR_res_outbound.so)/res_outbound.so

## Target: res_outbound.so
CFLAGS_res_outbound.so = \
	-I/usr/include/ \
	-I/usr/local/include/ \
	`mysql_config --cflags`
	
#CPPFLAGS_res_outbound.so =

LDLIBS_res_outbound.so = $(OSLDLIBS) `mysql_config --libs` -levent -lpthread -levent_pthreads

OBJS_res_outbound.so =  \
	$(TARGETDIR_res_outbound.so)/res_outbound.o \
	$(TARGETDIR_res_outbound.so)/db_handler.o \
	$(TARGETDIR_res_outbound.so)/event_handler.o \
	$(TARGETDIR_res_outbound.so)/ami_handler.o \
	$(TARGETDIR_res_outbound.so)/dialing_handler.o \
	$(TARGETDIR_res_outbound.so)/cli_handler.o 
	

# WARNING: do not run this directly, it should be run by the master Makefile 
$(TARGETDIR_res_outbound.so)/res_outbound.so: $(TARGETDIR_res_outbound.so) $(OBJS_res_outbound.so) $(DEPLIBS_res_outbound.so)
	$(LINK.c) $(CFLAGS_res_outbound.so) $(CPPFLAGS_res_outbound.so) -o $@ $(OBJS_res_outbound.so) $(SHAREDLIB_FLAGS_res_outbound.so) $(LDLIBS_res_outbound.so)

# Compile source files into .o files
$(TARGETDIR_res_outbound.so)/res_outbound.o: $(TARGETDIR_res_outbound.so) src/res_outbound.c 
	$(COMPILE.c) $(CFLAGS_res_outbound.so) $(CPPFLAGS_res_outbound.so) -o $@ src/res_outbound.c

$(TARGETDIR_res_outbound.so)/db_handler.o: $(TARGETDIR_res_outbound.so) src/db_handler.c 
	$(COMPILE.c) $(CFLAGS_res_outbound.so) $(CPPFLAGS_res_outbound.so) -o $@ src/db_handler.c

$(TARGETDIR_res_outbound.so)/event_handler.o: $(TARGETDIR_res_outbound.so) src/event_handler.c 
	$(COMPILE.c) $(CFLAGS_res_outbound.so) $(CPPFLAGS_res_outbound.so) -o $@ src/event_handler.c

$(TARGETDIR_res_outbound.so)/ami_handler.o: $(TARGETDIR_res_outbound.so) src/ami_handler.c 
	$(COMPILE.c) $(CFLAGS_res_outbound.so) $(CPPFLAGS_res_outbound.so) -o $@ src/ami_handler.c

$(TARGETDIR_res_outbound.so)/dialing_handler.o: $(TARGETDIR_res_outbound.so) src/dialing_handler.c 
	$(COMPILE.c) $(CFLAGS_res_outbound.so) $(CPPFLAGS_res_outbound.so) -o $@ src/dialing_handler.c

$(TARGETDIR_res_outbound.so)/cli_handler.o: $(TARGETDIR_res_outbound.so) src/cli_handler.c 
	$(COMPILE.c) $(CFLAGS_res_outbound.so) $(CPPFLAGS_res_outbound.so) -o $@ src/cli_handler.c


#### Clean target deletes all generated files ####
clean:
	rm -f \
		$(TARGETDIR_res_outbound.so)/res_outbound.so \
		$(TARGETDIR_res_outbound.so)/*.o
	rm -f -r $(TARGETDIR_res_outbound.so)
	
install:
	mv $(TARGETDIR_res_outbound.so)/res_outbound.so /usr/lib/asterisk/modules/


# Create the target directory (if needed)
$(TARGETDIR_res_outbound.so):
	mkdir -p $(TARGETDIR_res_outbound.so)


# Enable dependency checking
#.KEEP_STATE:
#.KEEP_STATE_FILE:.make.state.GNU-amd64-Linux
