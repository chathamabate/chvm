CC 		:= gcc
CFLAGS	:= 

all: test

# Macro for printing build message.
# $(call build_msg_template,$?,$@,prefix)
define build_msg_template
@printf "$(3) \x1b[37;3m%-30.30s\x1b[0m [$(1)]\n" "$(2)"
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

# This macro defines all headers to watch when updating
# object files in this module.
#
# A module object file is dependent on a few things.
#
# The corresponding C file.
# All header files in the module.
# All header files in all dependency modules.
# All core headers.
$(1)_dep_hdrs	:= $$($(1)_hdrs) $$(foreach mod,$(2),$$($$(mod)_hdrs)) $(core_hdrs)

$(1)_test_hdrs	:= $$(wildcard $(1)_src/test/*.h)
$(1)_test_srcs	:= $$(wildcard $(1)_src/test/*.c)
$(1)_test_objs	:= $$(subst .c,.o,$$($(1)_test_srcs))

# This macro defines all headers to watch when updating
# test object files in this module.
#
# A test object file is dependent on many things...
#
# The test C file.	
# All headers in this module's test folder.
# *All headers in this module.
# *All headers in all dependency modules.
# All core testing headers.
# *All core headers.
#
# * indicates all headers which are already included in 
# the normal dependency headers macro.
$(1)_test_dep_hdrs	:= $$($(1)_test_hdrs) $(testing_hdrs) $$($(1)_dep_hdrs)

$(1)_src/test/%.o: $(1)_src/test/%.c $$($(1)_test_dep_hdrs)
	$$(call build_msg_template,$$?,$$@,$(suite_prefix))
	@$(CC) -c -o $$@ $$< $(CFLAGS)

$(1)_src/%.o: $(1)_src/%.c $$($(1)_dep_hdrs)
	$$(call build_msg_template,$$?,$$@,$(app_prefix))
	@$(CC) -c -o $$@ $$< $(CFLAGS)
endef

# PUT MODULE DEFINITIONS HERE---------------------------

$(eval $(call module_template,util,))
$(eval $(call module_template,chvm,util))
$(eval $(call module_template,chasm,util chvm))

# ------------------------------------------------------

# All module testing headers.
all_mod_test_hdrs := $(foreach mod,$(modules),$($(mod)_test_hdrs))

# test.c should only reference testing code and core code.
# The dependencies will be as follows....
#
# The c file.
# All module testing headers.
# All core testing headers.
# All core headers.
test.o: test.c $(all_mod_test_hdrs) $(testing_hdrs) $(core_hdrs)
	$(call build_msg_template,$?,$@,$(test_prefix))
	@$(CC) -c -o $@ $< $(CFLAGS)

# This is all normal object files.
all_mod_objs 		:= $(core_objs) $(foreach mod,$(modules),$($(mod)_objs))

# This is all testing objects.
all_test_objs 		:= $(testing_objs) $(foreach mod,$(modules),$($(mod)_test_objs)) 

# Combination of the above 2 macros.
all_objs			:= $(all_mod_objs) $(all_test_objs)

# test.c will be the test entry point, always at the top 
# level directory.
# To create the test binary, every single object must be up to date!
test: test.o $(all_objs)
	$(call link_msg_template,$@)
	@$(CC) -o $@ ./test.o $(all_objs) $(CFLAGS)
	$(success_msg)

%.o: %.c
	@echo "Rule not found for " $< " -> " $@

# Here we could figure out all created objects than delete them
# this way there will never be any error messages when 
# cleaning up the project.

existing_core_objs			:= $(wildcard core_src/*.o)
existing_testing_objs		:= $(wildcard testing_src/*.o)
existing_module_objs		:= $(foreach mod,$(modules),$(wildcard $(mod)_src/*.o))
existing_module_test_objs	:= $(foreach mod,$(modules),$(wildcard $(mod)_src/test/*.o))
existing_main_objs			:= $(wildcard *.o)

existing_removeables		:= $(existing_core_objs) 
existing_removeables		+= $(existing_testing_objs)
existing_removeables		+= $(existing_module_objs)
existing_removeables		+= $(existing_module_test_objs)
existing_removeables		+= $(existing_main_objs)

# $(call remove_template,removeable_file)
define remove_template

@printf	"\x1b[31;3m$(1)\x1b[0m\n"
@rm $(1)

endef

clean:
	@printf "\x1b[91mCleaning Up...\x1b[0m\n"
	$(foreach removeable,$(existing_removeables), $(call remove_template,$(removeable)))
