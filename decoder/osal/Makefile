include ../../Makefile.pub

DEFS =
CFLAGS := -Wall $(DEFS) -I. -O0 -g
CFLAGS +=  -I./   -I../
CXXFLAGS:=$(CFLAGS)
CCFLAGS:=$(CFLAGS)
LIBPATH := 
LIBS := -lpthread 


INSTALL_DIR=/home/work/tftpboot

##### End of variables to change
.PHONY :prepare all clean install

TARGET = osal.a
ALL = $(TARGET)
all: prepare $(ALL)

SOURCES =  $(wildcard *.c) 

#change .cpp files  to .o files
OBJFILES = $(SOURCES:%.c=obj/%.o)

obj/%.o:%.c 
	$(COMPILE_CC)
	
$(TARGET):	$(OBJFILES)  
	$(AR_LIB)
prepare:
	@echo "preparing..."
	@if ! [ -d obj ]; then mkdir obj; fi;
clean:
	-rm -rf *.o $(ALL) *~ obj/* obj
install:
	install $(TARGET) $(INSTALL_DIR)

##### Any additional, platform-specific rules come here:
