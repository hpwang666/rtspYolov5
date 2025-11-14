INCLUDES =   -I/usr/include/opencv4  -I/usr/local/cuda/include
		
##### Change the following for your environment: 
CROSS_COMPILE =
C_COMPILER =		$(CROSS_COMPILE)gcc

CU_FLAGS = $(INCLUDES)
CU_COMPILER =           $(CROSS_COMPILE)nvcc

COMPILE_OPTS =		$(INCLUDES)  -std=c++11 -Wall -Ofast -g -Wfatal-errors -D_MWAITXINTRIN_H_INCLUDED
C_FLAGS =		$(COMPILE_OPTS) 
#-D可以把编译宏传进去

# -ffunction-sections、-fdata-sections，在编译生成的.o目标文件中，会将每个函数或数据段，放在各种单独独立的section中；
# 链接时候使用这个参数 -Wl,–gc-sections 就不会把不用的函数链接进执行文件
CPLUSPLUS_COMPILER =	$(CROSS_COMPILE)g++ 
CPLUSPLUS_FLAGS =	$(COMPILE_OPTS) -Wall #-DBSD=1 -g -fPIC

LINK  =	 $(CROSS_COMPILE)g++  

##### End of variables to change
ALL = prepare $(TARGET) 
TARGET = yolov5_rt_bin
all:  $(ALL)


SOURCES =  $(wildcard *.cpp)
SOURCES_CU =  $(wildcard *.cu)

#change .cpp files  to .o files
OBJFILES = $(SOURCES:%.cpp=obj/%.o)
OBJFILES_CU = $(SOURCES_CU:%.cu=obj/%.ocu)

#$@--目标文件，$^--所有的依赖文件，$<--第一个依赖文件。
obj/%.o:%.cpp    
	@echo -e CC $<
	@$(CPLUSPLUS_COMPILER)  $(CPLUSPLUS_FLAGS) -c -o $@ $< 

obj/%.ocu:%.cu
	@echo -e CU $<
	@$(CU_COMPILER)  $(CU_FLAGS) -c -o $@ $<


LIBPATH = -L./  -L/usr/local/cuda/lib64
#注意lib的顺序，最底层的放在最后面，以便于被前面的库引用,a需要b里面的文件，b需要c里面的文件
LIBS =    $(shell pkg-config --libs opencv4)  -lnvinfer_plugin -lnvinfer -lcudart  -lcublas -lcusolver
ALLLIBS =  $(LIBPATH) $(LIBS)

#不会删除中间文件
.SECONDARY:
	
$(TARGET):	$(OBJFILES) $(OBJFILES_CU)
	@echo "linking $(TARGET) from ..."
	$(LINK) -o $@  $^  $(ALLLIBS)
	

prepare:
	@echo "preparing..."
	@if ! [ -d obj ]; then mkdir obj; fi;

clean:
	-rm -rf *.o obj/*.o $(ALL) *~ 
install:
	install $(TARGET) $(INSTALL_DIR)
	
.PHONY :clean install prepare all

##### Any additional, platform-specific rules come here:
