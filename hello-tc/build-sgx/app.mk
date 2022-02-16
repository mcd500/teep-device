
# # Three configuration modes - Debug, prerelease, release
#   Debug - Macro DEBUG enabled.
#   Prerelease - Macro NDEBUG and EDEBUG enabled.
#   Release - Macro NDEBUG enabled.
ifeq ($(DEBUG_TYPE), DEBUG)
DEBUG_FLAGS += -DDEBUG -UNDEBUG -UEDEBUG
else ifeq ($(DEBUG_TYPE), PRERELEASE)
DEBUG_FLAGS += -DNDEBUG -DEDEBUG -UDEBUG
else ifeq ($(DEBUG_TYPE), RELEASE)
DEBUG_FLAGS += -DNDEBUG -UEDEBUG -UDEBUG
else
DEBUG_FLAGS += -DNDEBUG -UEDEBUG -UDEBUG
endif

ifeq ($(MACHINE), SIM)
LIBRARY_SUFFIX = _sim
else
LIBRARY_SUFFIX =
endif

CXXFLAGS = $(SGX_CXXFLAGS) $(DEBUG_FLAGS) \
	-I$(TAREF_DIR)/build/include

LIBRARIES=sgx_urts sgx_uae_service
LOAD_LIBRARIES=$(patsubst %,-l%$(LIBRARY_SUFFIX), $(LIBRARIES))
LOAD_LIBRARIES += $(addprefix -l, Enclave_u pthread)

LDFLAGS = \
	-L$(SGX_LIBRARY_DIR) \
	-L$(TAREF_DIR)/build/lib \
	-lsgx_urts$(LIBRARY_SUFFIX) \
	-lsgx_uae_service$(LIBRARY_SUFFIX) \
	-lEnclave_u -lpthread

.PHONY: all clean
all: App_sgx

App_sgx.o: ../App-sgx.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

App_sgx: App_sgx.o
	$(CXX) $^ -o $@ $(LDFLAGS)

clean:
	rm -f *.o App_sgx
