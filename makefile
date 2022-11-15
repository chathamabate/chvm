CC 		:= gcc
CFLAGS	:= 

all: chvm
test: test_chvm

# Macro for printing build message.
# $(call build_msg_template,$?,$@,prefix)
define build_msg_template
@printf "$(3) \x1b[37;3m%-25.25s\x1b[0m [$(1)]\n" "$(2)"
endef

# $(call link_msg_template,$@)
define link_msg_template 
@printf	"\n\x1b[3mLinking \x1b[1m$(1)\x1b[0m\n"
endef

define success_msg
@printf "\n\x1b[92mBuild Successful\x1b[0m\n\n"
endef

core_prefix 	:= \x1b[95mC\x1b[0m
app_prefix		:= \x1b[1mA\x1b[0m
test_prefix 	:= \x1b[93mT\x1b[0m
suite_prefix	:= \x1b[92mS\x1b[0m

# Core code will depend on nothing.
# Everything will depend on core code.
# Core code will not have tests.
core_hdrs		:= $(wildcard core_src/*.h)
core_srcs		:= $(wildcard core_src/*.c)
core_objs		:= $(subst .c,.o,$(core_srcs))

core_src/%.o: core_src/%.c $(core_hdrs)
	$(call build_msg_template,$?,$@,$(core_prefix))
	@$(CC) -c -o $@ $< $(CFLAGS)

# Testing code will not be templated in the makefile
# as all modules depend on it. Testing code
# will depend only on core code.
testing_hdrs	:= $(wildcard testing_src/*.h)
testing_srcs 	:= $(wildcard testing_src/*.c)
testing_objs 	:= $(subst .c,.o,$(testing_srcs))

testing_src/%.o: testing_src/%.c $(testing_hdrs) $(core_hdrs)
	$(call build_msg_template,$?,$@,$(test_prefix))
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

# Regardless, in both situations, the core hdrs are dependencies also...

$(1)_src/test/%.o: $(1)_src/test/%.c $$($(1)_test_dep_hdrs) $(core_hdrs)
	$$(call build_msg_template,$$?,$$@,$(suite_prefix))
	@$(CC) -c -o $$@ $$< $(CFLAGS)

$(1)_src/%.o: $(1)_src/%.c $$($(1)_hdrs) $$($(1)_dep_hdrs) $(core_hdrs)
	$$(call build_msg_template,$$?,$$@,$(app_prefix))
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
all_objs		:= $(core_objs) $(foreach mod,$(modules),$($(mod)_objs))

# all_test_objs refers to all objs relating to testing.
all_test_objs	:= $(testing_objs) $(foreach mod,$(modules),$($(mod)_test_objs))

# Main will be entry point for normal program.
./main.o: ./main.c $(all_objs)
	$(call build_msg_template,$?,$@,$(app_prefix))
	@$(CC) -c -o $@ $< $(CFLAGS)

chvm: ./main.o 
	$(call link_msg_template,$@)
	@$(CC) -o $@ ./main.o $(all_objs) $(CFLAGS)
	$(success_msg)

./test_main.o: ./test_main.c $(all_objs) $(all_test_objs)
	$(call build_msg_template,$?,$@,$(test_prefix))
	@$(CC) -c -o $@ $< $(CFLAGS)

test_chvm: ./test_main.o 
	$(call link_msg_template,$@)
	@$(CC) -o $@ ./test_main.o $(all_objs) $(all_test_objs) $(CFLAGS)
	$(success_msg)

# Here we could figure out all created objects than delete them
# this way there will never be any error messages when 
# cleaning up the project.

existing_core_objs			:= $(wildcard core_src/*.o)
existing_testing_objs		:= $(wildcard testing_src/*.o)
existing_module_objs		:= $(foreach mod,$(modules),$(wildcard $(mod)_src/*.o))
existing_module_test_objs	:= $(foreach mod,$(modules),$(wildcard $(mod)_src/test/*.o))
existing_main_objs			:= $(wildcard *.o)
existing_binaries			:= $(wildcard *chvm)

existing_removeables		:= $(existing_core_objs) 
existing_removeables		+= $(existing_testing_objs)
existing_removeables		+= $(existing_module_objs)
existing_removeables		+= $(existing_module_test_objs)
existing_removeables		+= $(existing_main_objs)
existing_removeables		+= $(existing_binaries)

# $(call remove_template,removeable_file)
define remove_template

@printf	"\x1b[31;3m$(1)\x1b[0m\n"
@rm $(1)

endef

clean:
	@printf "\x1b[91mCleaning Up...\x1b[0m\n"
	$(foreach removeable,$(existing_removeables), $(call remove_template,$(removeable)))
