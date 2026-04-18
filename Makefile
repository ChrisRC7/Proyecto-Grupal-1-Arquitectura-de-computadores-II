# Rutas de SystemC (se auto-detectan; SYSTEMC_HOME sigue siendo configurable)
SYSTEMC_HOME ?= /usr/local/systemc-2.3.3
TARGET_ARCH  ?= linux64

# Priorizamos una instalación local en /usr/local; si no existe, usamos la del sistema.
ifneq ($(wildcard $(SYSTEMC_HOME)/include/systemc.h),)
SYSTEMC_INCLUDE := $(SYSTEMC_HOME)/include
SYSTEMC_LIBDIR  := $(SYSTEMC_HOME)/lib-$(TARGET_ARCH)
else ifneq ($(wildcard /usr/include/systemc),)
SYSTEMC_INCLUDE := /usr/include
SYSTEMC_LIBDIR  := /usr/lib/x86_64-linux-gnu
else
SYSTEMC_INCLUDE :=
SYSTEMC_LIBDIR  :=
endif

CXX = g++
CXX_STD ?= -std=c++17
CXXFLAGS = $(CXX_STD) -I$(SYSTEMC_INCLUDE) -Wno-deprecated
LDFLAGS = -L$(SYSTEMC_LIBDIR) -lsystemc -lm -Wl,-rpath=$(SYSTEMC_LIBDIR)

# Archivos
SRC_DIR = src
OBJ_DIR = obj
SIM_EXEC = sim_proyecto
GEN_EXEC = trace_generator

all: check_systemc $(GEN_EXEC) $(SIM_EXEC)

check_systemc:
	@if [ -z "$(SYSTEMC_INCLUDE)" ] || [ -z "$(SYSTEMC_LIBDIR)" ]; then \
		echo "Error: No se encontró SystemC."; \
		echo "Instala el paquete con: sudo apt install -y libsystemc-dev"; \
		echo "O define SYSTEMC_HOME=<ruta> si lo tienes en /usr/local."; \
		exit 1; \
	fi

# Compilar el Generador de Traces (No requiere SystemC)
$(GEN_EXEC): $(SRC_DIR)/trace_generator.cpp
	$(CXX) $(CXX_STD) $< -o $@

# Compilar el Simulador SystemC
$(SIM_EXEC): $(SRC_DIR)/main.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(SIM_EXEC) $(GEN_EXEC) traces/*.txt