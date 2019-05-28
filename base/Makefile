.PHONY:all clean
#****************************************************************************
# Targets of the build
#****************************************************************************
OUTPUT := libbase.a
CXX = clang++
OUTPUT_INCLUDE := ../includes/base
OUTPUT_LIBS    := ../libs

all: ${OUTPUT}
	cp ${OUTPUT} ${OUTPUT_LIBS}

#****************************************************************************
# Source files
#****************************************************************************
#
C_SRCS=$(shell find -maxdepth 1 -iname "*.cc")
#
CXX_SRCS=$(shell find -maxdepth 1 -iname "*.cpp")

OBJS=$(subst .cc,.o,$(C_SRCS))
OBJS+=$(subst .cpp,.o,$(CXX_SRCS))

#****************************************************************************
# Output
#****************************************************************************
${OUTPUT}: ${OBJS}
	${AR} rc $@ ${OBJS} ${LIBS} ${EXTRA_LIBS}
#	${LD} -shared -fPIC -o $@ ${LDFLAGS} ${OBJS} ${LIBS} ${EXTRA_LIBS}

#****************************************************************************
# common rules
#****************************************************************************

clean:
	-rm -f core ${OBJS} ${OUTPUT}
	-rm -f ${OUTPUT_LIBS}/${OUTPUT}
