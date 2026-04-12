#  Simulador N-Body - Configuración del Entorno (WSL2)

Este documento detalla los pasos seguidos para configurar el entorno de desarrollo en **Windows** utilizando **WSL2**, permitiendo compilar y ejecutar código C++ con soporte nativo para **OpenMP**.

## 1. Instalación de WSL2 (Windows Subsystem for Linux)
WSL2 permite ejecutar un entorno GNU/Linux directamente en Windows sin la sobrecarga de una máquina virtual tradicional.

* Abrir **PowerShell** como Administrador.
* Ejecutar el comando:
    ```powershell
    wsl --install
    ```
* **Reiniciar la computadora.**
* Al reiniciar, se abrirá una consola de Ubuntu. Sigue las instrucciones para crear tu **usuario** y **contraseña** (esta contraseña será necesaria para comandos `sudo`).

---

## 2. Configuración en Visual Studio Code
Para trabajar desde Windows pero compilar en Linux:

1.  Instala la extensión **WSL** (de Microsoft) en VS Code.
2.  En la terminal de Ubuntu, navega a la carpeta de tu proyecto y escribe:
    ```bash
    code .
    ```
3.  VS Code se abrirá en "Modo WSL" (verás un recuadro azul en la esquina inferior izquierda que dice `WSL: Ubuntu`).

---

## 3. Instalación de Herramientas de Desarrollo
Una vez dentro de la terminal de Ubuntu (en VS Code o la consola independiente), instalar los compiladores y librerías necesarias:

### Actualizar repositorios
```bash
sudo apt update
```

### Instalar Compilador, Make y OpenMP
El paquete `build-essential` incluye `g++` y el soporte base para paralelismo:
```bash
sudo apt install build-essential g++ make
```

### Instalar GoogleTest (Pruebas Unitarias)
Requerido por las especificaciones del Laboratorio 1:
```bash
sudo apt install libgtest-dev
```

---

## 4. Verificación del Entorno
Ejecuta los siguientes comandos para asegurar que todo está correctamente instalado:

| Comando | Resultado esperado | Propósito |
| :--- | :--- | :--- |
| `g++ --version` | `g++ (Ubuntu 11.x.x) ...` | Compilador C++17/20 |
| `make --version` | `GNU Make 4.3` | Automatización de builds |
| `ldconfig -p \| grep libgomp` | `libgomp.so.1 (libc6,x86-64)` | Soporte de OpenMP activo |

---

## 5. Comandos Útiles del Proyecto

1. Abrí una terminal **WSL Ubuntu** en VS Code.
2. Navegá al directorio del proyecto:
    ```bash
    cd nbody_2d
    ```
3. Limpiar y compilar:
    ```bash
    make clean
    make
    ```
4. Ejecutar simulación base:
    ```bash
    ./nbody_2d
    ```
5. Ejecutar test básico de aceleración:
    ```bash
    make test
    ```

### Compilación con OpenMP (Semana 2 en adelante)
```bash
g++ -O3 -fopenmp -Wall -Wextra -std=c++17 main.cpp Particle.cpp NBodySystem.cpp NBodySimulator.cpp Integrator.cpp MetricsCalculator.cpp Benchmark.cpp Visualizer.cpp -o nbody_2d
```

---

## 6. Solución de Problemas Comunes
* **Error "Command not found":** Asegúrate de haber ejecutado el paso 3 (`sudo apt install`).
* **Permisos denegados:** Recuerda anteponer `sudo` en comandos de instalación.
* **VS Code no reconoce `omp.h`:** Instala el "C/C++ Extension Pack" dentro de la instancia de WSL en VS Code.

---
