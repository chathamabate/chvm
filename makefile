CC 		:= gcc
CFLAGS	:= 

all: chvm
test: test_chvm

# The index directory will hold definitions which
# can be used anywhere. (Probably all macros)
index_hdrs		:= $(wildcard index/*.h)

# Testing code will not be templated in the makefile
# as all modules depend on it. Testing code
# will depend on nothing.
testing_hdrs	:= $(wildcard testing_src/*.h)
testing_srcs 	:= $(wildcard testing_src/*.c)
testing_objs 	:= $(subst .c,.o,$(testing_srcs))

testing_src/%.o: testing_src/%.c $(testing_hdrs) $(index_hdrs)
	@printf "T %-25.25s <- [$?]\n" "$@"
	@$(CC) -c -o $@ $< $(CFLAGS)

modules	:=

# $(1) = module name
# $(2) = module dependencies separated by spaces.
define module_template 
modules		+= $(1)
$(1)_hdrs	:= $$(wildcard $(1)_src/*.h)
$(1)_srcs	:= $$(wildcard $(1)_src/*.c)
$(1)_objs	:= $$(subst .c,.o,$$($(1)_srcs))

$(1)_dep_hdrs	:= $$(foreach mod,$(2),$$(wildcard $$(mod)_src/*.h)) 

$(1)_test_hdrs	:= $$(wildcard $(1)_src/test/*.h)
$(1)_test_srcs	:= $$(wildcard $(1)_src/test/*.c)
$(1)_test_objs	:= $$(subst .c,.o,$$($(1)_test_srcs))

# A test object file is dependent on many things...
#
# The test file source.
# The headers in this modules test folder.
# The headers in this module.
# The headers in this module's dependency modules.
# Finally, the headers in the testing src folder.

$(1)_test_dep_hdrs	:= $$($(1)_test_hdrs) $$($(1)_hdrs) $$($(1)_dep_hdrs) $(testing_hdrs)


$(1)_src/test/%.o: $(1)_src/test/%.c $$($(1)_test_dep_hdrs) $(index_hdrs)
	@printf "T %-25.25s <- [$$?]\n" "$$@"
	@$(CC) -c -o $$@ $$< $(CFLAGS)

$(1)_src/%.o: $(1)_src/%.c $$($(1)_hdrs) $$($(1)_dep_hdrs) $(index_hdrs)
	@printf "S %-25.25s <- [$$?]\n" "$$@"
	@$(CC) -c -o $$@ $$< $(CFLAGS)
endef

# PUT MODULE DEFINITIONS HERE---------------------------

$(eval $(call module_template,util,))
$(eval $(call module_template,chvm,util))
$(eval $(call module_template,chasm,util chvm))

# ------------------------------------------------------

%.o: %.c
	@echo "Rule not found for " $< " -> " $@

# all_objs refers to pure source files objects. (NOT any test files)
all_objs		:= $(foreach mod,$(modules),$($(mod)_objs))

# all_test_objs refers to all objs relating to testing.
all_test_objs	:= $(testing_objs) $(foreach mod,$(modules),$($(mod)_test_objs))

# Main will be entry point for normal program.
./main.o: ./main.c $(all_objs)
	@printf "S %-25.25s <- [$?]\n" "$@"
	@$(CC) -c -o $@ $< $(CFLAGS)

chvm: ./main.o 
	@printf "Building $@\n"
	@$(CC) -o $@ ./main.o $(all_objs) $(CFLAGS)

./test_main.o: ./test_main.c $(all_objs) $(all_test_objs)
	@printf "T %-25.25s <- [$?]\n" "$@"
	@$(CC) -c -o $@ $< $(CFLAGS)

test_chvm: ./test_main.o 
	@printf "Building $@\n"
	@$(CC) -o $@ ./test_main.o $(all_objs) $(all_test_objs) $(CFLAGS)

clean:
	rm chvm ./main.o $(all_objs) test_chvm ./test_main.o $(all_test_objs)

