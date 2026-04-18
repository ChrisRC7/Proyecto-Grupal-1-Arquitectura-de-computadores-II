# Proyecto Grupal 1 - Arquitectura de Computadores II

Guia rapida para instalar dependencias, compilar y ejecutar el proyecto en Linux.

## 1. Requisitos

- Linux (probado en Ubuntu 24.04)
- `g++` con soporte C++17
- `make`
- Biblioteca de desarrollo de SystemC

## 2. Instalacion de dependencias (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install -y build-essential libsystemc-dev
```

Verificar instalacion de SystemC:

```bash
apt-cache policy libsystemc-dev
ls -l /usr/include/systemc /usr/lib/x86_64-linux-gnu/libsystemc.so
```

## 3. Compilar

Desde la raiz del repositorio:

```bash
make clean
make
```

Esto genera:

- `trace_generator` (generador de trazas)
- `sim_proyecto` (simulador con SystemC)

## 4. Ejecutar

### 4.1 Generar trazas

```bash
./trace_generator
```

### 4.2 Ejecutar simulador

```bash
./sim_proyecto
```

## 5. Limpiar binarios y trazas

```bash
make clean
```

## 6. Problemas comunes

### Error: `fatal error: systemc.h: No existe el archivo o el directorio`

Instala la dependencia faltante:

```bash
sudo apt install -y libsystemc-dev
```

### Error de enlace con `sc_api_version_..._cxx201402L`

Ese error aparece cuando el codigo se compila con un estandar C++ distinto al de la biblioteca instalada.
Este repositorio ya esta configurado para compilar con **C++17**, que coincide con `libsystemc-dev` de Ubuntu reciente.

## 7. Nota sobre rutas de SystemC

El `Makefile` detecta automaticamente dos esquemas:

1. Instalacion local en `/usr/local/systemc-2.3.3`
2. Instalacion de paquete del sistema (`/usr/include` y `/usr/lib/x86_64-linux-gnu`)

Si usan una instalacion local distinta, pueden sobrescribir la ruta al compilar:

```bash
make SYSTEMC_HOME=/ruta/a/systemc
```
