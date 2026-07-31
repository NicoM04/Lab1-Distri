# Laboratorio 2: Programación GPGPU con CUDA
## Simulador gravitatorio N-cuerpos en 2D (C++/CUDA)

Este proyecto extiende el simulador N-cuerpos del Laboratorio 1, migrando el núcleo computacional a la GPU utilizando **CUDA** con un diseño orientado a coalescing (SoA) y memoria compartida. Además, incorpora prácticas rigurosas de Git, Integración Continua (CI) y agentes de Inteligencia Artificial para automatizar tareas.

### Estado Actual:
- Kernels CUDA (básico y memoria compartida) implementados y validados.
- Transferencias optimizadas Host/Device mediante la clase RAII `CudaBuffer`.
- Pruebas unitarias e integración en CPU y GPU operativas con tolerancias definidas.
- Benchmark integrado (`-benchmark-cuda`) para generar reportes en clúster.

---

## 👥 Equipo y Roles

De acuerdo con las especificaciones del Laboratorio 2, el equipo se distribuye en 5 roles principales:

| Rol | Responsable | Tareas Principales |
| :--- | :--- | :--- |
| **1. Kernels CUDA** | Francisco Riquelme | Desarrollo de `computeAccelerationsKernel` (básico y shared). Manejo de índices 1D, validación de bordes y macros `CUDA_CHECK`. |
| **2. Host/Device y memoria** | Gabriel Cabrera | Diseño de `CudaBuffer`. Layout SoA (`d_mass`, `d_x`, etc.). Minimización de transferencias `cudaMemcpy` por paso temporal. |
| **3. Integración y validación** | Amaru Monje | Integrador de Euler sincronizado con Device. Tests de CPU vs GPU (con tolerancias `rtol=1e-4`, `atol=1e-8`). Kernels de reducción (Energía). |
| **4. Git, releases y agentes** | Nicolás Morales | Gestión de ramas (`main` protegida, `feature/*`, `fix/*`). Mantenimiento del `CHANGELOG.md`. Configuración de los agentes de IA (Documentador, Bugs, MRs). |
| **5. Calidad, CI y visualización**| Thomas Gustafsson | Mantenimiento de `Makefile` y Dockerfile. Configuración de CI (`make test` en MRs). Ejecución de benchmarks en clúster y generación de gráficas `.png`. |

*(Reemplazar los nombres entre corchetes antes de la entrega final)*

---

## 🧠 Estructura CUDA y Layout SoA

El simulador GPU maneja la memoria a través de **Structure of Arrays (SoA)** para garantizar el *memory coalescing*. 
La memoria se administra dinámicamente usando la plantilla `CudaBuffer<T>`, la cual abstrae `cudaMalloc` y `cudaFree`.

### Variantes de Ejecución (Kernels)
- **Variante 0 (Básica)**: Cada hilo calcula la aceleración de un cuerpo `i` iterando sobre el resto `j`. Usa memoria global directamente.
- **Variante 1 (Shared Memory)**: Utiliza *tiling* (bloques de memoria compartida) para cargar partes del sistema y reducir las lecturas a la memoria global. Se sincroniza internamente usando `__syncthreads()`.

---

## 🤖 Agentes de IA en el Repositorio

El proyecto utiliza scripts/pipelines automatizados configurados para revisar partes clave del ciclo de desarrollo. (Revisar los scripts en `.github/workflows/` o `scripts/agents/`):
- **Documentador:** Revisa que el README, CHANGELOG y comentarios no estén desactualizados. Crea *issues* o MRs si la reparación es mecánica (`agent:auto-fix`).
- **Revisor de Bugs:** Análisis diario sobre `main` en busca de regresiones, uso de `CUDA_CHECK` y memory leaks de CUDA.
- **Revisor de MRs:** Se dispara al abrir/actualizar un Pull/Merge Request. Clasifica el cambio como automático o detiene el merge indicando **"Requiere intervención humana"** si involucra cambios físicos o arquitectónicos complejos.

---

## 🚀 Cómo Compilar y Ejecutar

### Dependencias
- Driver de NVIDIA y CUDA Toolkit (versión >= 12.x recomendada).
- Compiladores: `nvcc` y `g++` (soporte para C++17).
- GNU Make.
- Python 3 + Matplotlib + Numpy (para visualización).

### Compilación Local
```bash
make clean
make
```
*(Si deseas compilar con Docker: `docker build -t nbody_cuda .`)*

### Ejecutar Suite de Tests
Es imperativo que todas las pruebas pasen antes de cualquier MR.
```bash
make test
make cuda-test
```

### Ejecutar Benchmarks (Clúster DIINF)

Para generar los datos reales de ejecución en la GPU (Speedup, blockDim, ley de Amdahl), el proyecto cuenta con un script de SLURM (`pipeline_lab2.slurm`) automatizado.

1. **Sube tu código al clúster** (o haz un `git pull` desde tu sesión SSH):
   ```bash
   scp -r /ruta/local/nbody_2d usuario@ssh.diinf.usach.cl:~/
   ```

2. **Inicia el pipeline automatizado**:
   ```bash
   ssh usuario@ssh.diinf.usach.cl
   cd nbody_2d
   sbatch pipeline_lab2.slurm
   ```

3. **Monitorea tu trabajo**:
   ```bash
   squeue -u usuario
   ```

4. **Recupera los resultados**:
   Una vez finalizado el trabajo en la cola, el script habrá compilado, ejecutado todos los análisis requeridos y generado tu gráfico consolidado en:
   `lab2_plots/performance_plots.png`
   Descarga esa imagen a tu PC local y adjúntala a tu reporte.

> [!NOTE] 
> Asegúrate de ejecutar esto **solo en el nodo GPU del clúster** mediante SLURM para que los tiempos sean los oficiales de la entrega.

---

## 🔄 Tolerancias Numéricas
Para que las aceleraciones calculadas por la GPU (`float` o `double`) se asuman correctas frente a la versión puramente serial de CPU, la función de testeo admite los siguientes márgenes (documentados y justificados):
- `rtol = 1e-4`
- `atol = 1e-8`

Estas tolerancias acomodan el error de redondeo acumulativo y las discrepancias de FMA (Fused Multiply-Add) que el compilador `nvcc` inyecta durante las optimizaciones.