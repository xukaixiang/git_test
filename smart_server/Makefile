.PHONY:sqlite json src obj kernellist

CC=gcc
OBJ=server
OBJS=sqlite3_tool.o smart_lock_server.o kernellist.o json_string_tool.o

SUBDIRS=sqlite src kernellist json obj
BIN_DIR=bin
OBJS_DIR=obj

export CC OBJ OBJS SUBDIRS BIN_DIR OBJS_DIR

all:CHECK_DIR $(SUBDIRS)

CHECK_DIR:
		mkdir -p $(BIN_DIR)

$(SUBDIRS):
		make -C $(@)

cl:
		rm $(OBJS_DIR)/*.o
		rm $(BIN_DIR)/$(OBJ)

								
