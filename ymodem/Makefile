SRCS = $(wildcard *.c)
TARGET_SRCS = $(wildcard ymodem_test_*.c)
COMMON_SRCS = $(filter-out $(TARGET_SRCS),$(SRCS))
# filter A from B

OBJS = $(SRCS:.c=.o)
TARGET_OBJS = $(TARGET_SRCS:.c=.o)
COMMON_OBJS = $(COMMON_SRCS:.c=.o)

DEPS = $(OBJS:.o=.d)
TARGETS = $(TARGET_SRCS:.c=)
TEST_TARGETS = $(TARGETS:%=%.test)


DEPFLAGS = -MMD -MP
CC = gcc
CXX = g++
CFLAGS += $(DEPFLAGS)
-include $(DEPS)
#.SUFFIXES:

all: $(TARGETS)

$(TARGETS): %:%.o $(COMMON_OBJS)
	@echo [CC] $@
	@$(CC) -o $@ $^

%.o: %.c
	@echo [PRE] $(^:.c=.o)
	@$(CC) -c $^

clean:
	rm $(DEPS) $(COMMON_OBJS) $(TARGETS)

tests: $(TEST_TARGETS)

$(TEST_TARGETS): %.test:%
	@echo [TEST] $@
	@./$(@:.test=)

.PHONY: tests