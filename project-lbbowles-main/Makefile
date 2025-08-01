# Makefile for Lecture 15 (Thread Sync and Thread-Safe Programming) examples

# The "CFLAGS" are included every time the compiler is invoked, whether
# compiling a .c to a .o or linking the .o's to an executable

# A weakness of this Makefile: You can't specify different CFLAGS for
# different .c files -- so you couldn't make some with "-pthread" and
# some without, for example. Since this Makefile is for CSC 362 and we
# won't ever need that, it should be a big deal...

CFLAGS = -Wall -g -pthread

# The names of all the programs to build

PROGRAMS = gndcontrol

# For each program (named "program" for example) you must have a variable
# named "program_OBJS" that lists the .o files needed for that program
# Optimally, you may also have program_LDFLAGS and program_LDLIBS for
# flags and libraries that are needed in the linking stage. For example,
# you could include "program_LDLIBS = -lm" if your program needs the
# math library. It's OK to leave either or both of the LDFLAGS and LDLIBS
# definitions out.

gndcontrol_OBJS = gndcontrol.o airplane.o airs_protocol.o util.o alist.o \
	planelist.o taxi_queue.o

############################################################################
# Makefile magic below here. CSC 362 students don't need to change anything
# below here. If you DO decide to mess with this, you better be very, very
# careful and understand what you're doing!

OBJS_DIR = build
BINS_DIR = bin
SRC_DIR = src

PATH_PROGS = $(PROGRAMS:%=$(BINS_DIR)/%)

.PHONY: all
all: $(OBJS_DIR) $(BINS_DIR) $(PATH_PROGS)

$(OBJS_DIR):
	@mkdir -p $(OBJS_DIR)

$(BINS_DIR):
	@mkdir -p $(BINS_DIR)

# bring in dependencies
-include $(OBJS_DIR)/*.d

define PROGRAM_template =
 $(BINS_DIR)/$(1): $$($(1)_OBJS:%.o=$$(OBJS_DIR)/%.o)
	$$(CC) -o $$@ $$(CFLAGS) $$($(1)_LDFLAGS) $$^ $$($(1)_LDLIBS)
endef

$(foreach prog,$(PROGRAMS),$(eval $(call PROGRAM_template,$(prog))))

# Note that -MMD and -MP are what allows us to handle dependencies automatically
$(OBJS_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJS_DIR)
	$(CC) -c -o $@ $(CFLAGS) -MMD -MP $< $(LDFLAGS)

# Note for curious students: *~ is a "backup file" from the emacs editor...
.PHONY: clean
clean:
	rm -rf $(OBJS_DIR) $(BINS_DIR) *~ */*~