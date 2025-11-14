################################################################################
# Automatically-generated file. Do not edit!
################################################################################
#-march=armv7-a -mfloat-abi=softfp -mfpu=neon-vfpv4 -mno-unaligned-access -fno-aggressive-loop-optimizations
include ../Makefile.pub 

TOP_DIR 	:= $(shell pwd | awk '{split($$0, f, "/rtspYolov5/decoder"); print f[1]}')
CLASS_DIR 	:= $(TOP_DIR)/samples/common/classes



CFLAGS :=   -Wall     
CFLAGS +=  -I./  -I/usr/local/cuda/include    -I./osal -I../connet -I"$(TOP_DIR)/include" -I"$(TOP_DIR)/include/libjpeg-8b"  -I"$(TARGET_ROOTFS)/usr/include/libdrm"
CXXFLAGS:=$(CFLAGS) -std=c++11
CCFLAGS:=$(CFLAGS)

# CUDA code generation flags
GENCODE_SM53 := -gencode arch=compute_53,code=sm_53
GENCODE_SM62 := -gencode arch=compute_62,code=sm_62
GENCODE_SM72 := -gencode arch=compute_72,code=sm_72
GENCODE_SM_PTX := -gencode arch=compute_72,code=compute_72
GENCODE_FLAGS := -I./ -I/usr/local/cuda/include $(GENCODE_SM53) $(GENCODE_SM62) $(GENCODE_SM72) $(GENCODE_SM_PTX)



LIBPATH = -L/usr/lib/aarch64-linux-gnu/tegra/ -L/usr/local/cuda/lib64 
LIBS := -lpthread -lv4l2 -lEGL -lGLESv2 -lX11  -lnvjpeg\
		 $(shell pkg-config --libs opencv4)  -lnvinfer_plugin -lnvinfer -lcudart  -lcublas -lcusolver\
	-lnvbuf_utils   -ldrm  ./osal/osal.a ../connet/connet.a


SOURCES_CU =  $(wildcard *.cu) 
OBJFILES_CUO = $(SOURCES_CU:%.cu=obj/%.cuo) 


CLASS_CPP:=	$(wildcard $(CLASS_DIR)/*.cpp)
CLASS_OBJS := $(CLASS_CPP:.cpp=.o) 

##### End of variables to change
SOURCES =  $(wildcard *.c) 
OBJFILES = $(SOURCES:%.c=obj/%.o) $(CLASS_OBJS) $(OBJFILES_CUO)

.PHONY :clean install prepare all
TARGET = dec_bin
all : prepare $(TARGET) 


#change .cpp files  to .o files


#$@--目标文件，$^--所有的依赖文件，$<--第一个依赖文件。

obj/%.o:%.c 
	$(COMPILE_CXX)
	
obj/%.cuo:%.cu 
	/usr/local/cuda/bin/nvcc  $(GENCODE_FLAGS)  -c -o $@ $<

dec_bin:$(OBJFILES) 
	$(LINK_CXX)  
	
prepare:
	@echo "preparing..."
	@if ! [ -d obj ]; then mkdir obj; fi;
	@echo "Top dir: " $(TOP_DIR)
	

clean:
	-rm -rf *.o obj/*.o $(TARGET) *~ obj/

install:
	cp $(TARGET) /home/work/nfs/nvr


##### Any additional, platform-specific rules come here:

