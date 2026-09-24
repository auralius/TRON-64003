################################################################################
# micro T-Kernel 3.00.03  makefile
################################################################################

TEMP_C_SRCS = $(wildcard ../app_sample/*.c)
TEMP_CPP_SRCS = $(wildcard ../app_sample/*.cpp)

TEMP_C_OBJS = $(TEMP_C_SRCS:.c=.o)
TEMP_CPP_OBJS = $(TEMP_CPP_SRCS:.cpp=.o)

TEMP_C_DEPS = $(TEMP_C_SRCS:.c=.d)
TEMP_CPP_DEPS = $(TEMP_CPP_SRCS:.cpp=.d)

OBJS += $(subst ../, ./mtkernel_3/, $(TEMP_C_OBJS))
OBJS += $(subst ../, ./mtkernel_3/, $(TEMP_CPP_OBJS))

C_DEPS += $(subst ../, ./mtkernel_3/, $(TEMP_C_DEPS))
C_DEPS += $(subst ../, ./mtkernel_3/, $(TEMP_CPP_DEPS))


mtkernel_3/app_sample/%.o: ../app_sample/%.c
	@echo 'Building C file: $<'
	$(GCC) $(CFLAGS) -D$(TARGET) $(INCPATH) \
		-MF"$(@:%.o=%.d)" -MT"$(@)" \
		-c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


mtkernel_3/app_sample/%.o: ../app_sample/%.cpp
	@echo 'Building C++ file: $<'
	$(CXX) $(CXXFLAGS) -D$(TARGET) $(INCPATH) \
		-MF"$(@:%.o=%.d)" -MT"$(@)" \
		-c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
