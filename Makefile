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
SIM_SOURCES = $(SRC_DIR)/main.cpp $(wildcard $(SRC_DIR)/sim/*.cpp)

.PHONY: all build test test_extra check_systemc clean

all: test

build: check_systemc $(GEN_EXEC) $(SIM_EXEC)

test: build
	@echo "Ejecutando pruebas..."
	@mkdir -p traces
	./$(GEN_EXEC)
	@echo "Propuesta 1: Multiplicacion de Matrices Bloqueada (Alta Contencion)"
	./$(SIM_EXEC) traces/trace_matmul.txt msi
	./$(SIM_EXEC) traces/trace_matmul.txt firefly
	@echo "Propuesta 2: Pipeline de Criptografia/Red (Productor-Consumidor)"
	./$(SIM_EXEC) traces/trace_pipeline.txt msi
	./$(SIM_EXEC) traces/trace_pipeline.txt firefly
	@echo "Pruebas finalizadas correctamente."

test_extra: build
	@echo "Ejecutando pruebas extra..."
	@mkdir -p traces
	./$(GEN_EXEC)
	@echo "Escenario extra (no parte de las 2 propuestas): Patron migratorio"
	./$(SIM_EXEC) traces/trace_migratorio.txt msi
	./$(SIM_EXEC) traces/trace_migratorio.txt firefly
	@echo "Pruebas extra finalizadas correctamente."

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
$(SIM_EXEC): $(SIM_SOURCES)
	$(CXX) $(CXXFLAGS) $(SIM_SOURCES) -o $@ $(LDFLAGS)

clean:
	rm -f $(SIM_EXEC) $(GEN_EXEC) traces/*.txt