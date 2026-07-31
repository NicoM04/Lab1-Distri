import numpy as np
import matplotlib.pyplot as plt
import os
import sys

def load_data(filename):
    if not os.path.exists(filename):
        print(f"Error: No se encontró el archivo {filename}")
        sys.exit(1)
    # Skipping header row
    return np.loadtxt(filename, skiprows=1)

def main():
    print("Cargando datos reales...")
    
    # 1. benchmark_results.dat
    # Format: N, BlockDim, CPU_Time(ms), GPU_Basic_Kernel(ms), GPU_Basic_E2E(ms), GPU_Shared_Kernel(ms), GPU_Shared_E2E(ms)
    bench_data = load_data('benchmark_results.dat')
    
    # Filter for BlockDim = 256 for scaling analysis
    mask_256 = bench_data[:, 1] == 256
    N_values = bench_data[mask_256, 0]
    cpu_time = bench_data[mask_256, 2]
    gpu_basic_kernel = bench_data[mask_256, 3]
    gpu_basic_e2e = bench_data[mask_256, 4]
    gpu_shared_kernel = bench_data[mask_256, 5]
    gpu_shared_e2e = bench_data[mask_256, 6]

    # Calculate Speedup and Amdahl dynamically
    speedup_basic = cpu_time / gpu_basic_e2e
    speedup_shared = cpu_time / gpu_shared_e2e
    sp_k = cpu_time / gpu_shared_kernel
    transfer_time = gpu_shared_e2e - gpu_shared_kernel
    f_serial = transfer_time / (cpu_time + transfer_time)
    amdahl_pred = 1.0 / (f_serial + (1.0 - f_serial) / sp_k)

    # Filter for N = 2000 for blockDim study
    mask_2000 = bench_data[:, 0] == 2000
    blockdims = bench_data[mask_2000, 1]
    bdim_basic = bench_data[mask_2000, 3]
    bdim_shared = bench_data[mask_2000, 5]

    # 4. energy_timeseries.dat
    # Format: Step, Time, Kinetic, Potential, Total
    energy_data = load_data('energy_timeseries.dat')
    steps = energy_data[:, 0]
    times = energy_data[:, 1]
    k = energy_data[:, 2]
    u = energy_data[:, 3]
    total_e = energy_data[:, 4]

    print("Generando gráficos...")
    fig, axs = plt.subplots(3, 2, figsize=(15, 15))
    fig.suptitle('Laboratorio 2: Rendimiento y Simulador CUDA (Datos DIINF)', fontsize=16)

    # 1. Speedup GPU vs. CPU frente a N
    axs[0, 0].plot(N_values, speedup_basic, 'o-', label='Básica (E2E)')
    axs[0, 0].plot(N_values, speedup_shared, 's-', label='Shared (E2E)')
    axs[0, 0].set_xlabel('N (cuerpos)')
    axs[0, 0].set_ylabel('Speedup (T_CPU / T_GPU)')
    axs[0, 0].set_title('1. Speedup GPU vs. CPU frente a N')
    axs[0, 0].grid(True)
    axs[0, 0].legend()

    # 2. Tiempo kernel-only vs. end-to-end
    width = 0.35
    x = np.arange(len(N_values))
    axs[0, 1].bar(x - width/2, gpu_shared_kernel, width, label='Kernel-Only (Shared)')
    axs[0, 1].bar(x + width/2, gpu_shared_e2e, width, label='End-to-End (Shared)')
    axs[0, 1].set_xticks(x)
    axs[0, 1].set_xticklabels([str(int(n)) for n in N_values])
    axs[0, 1].set_xlabel('N (cuerpos)')
    axs[0, 1].set_ylabel('Tiempo (ms)')
    axs[0, 1].set_title('2. Tiempo Kernel-only vs End-to-End')
    axs[0, 1].legend()
    axs[0, 1].grid(axis='y')

    # 3. Tiempo frente a blockDim.x
    axs[1, 0].plot(blockdims, bdim_basic, 'o-', label='Básica')
    axs[1, 0].plot(blockdims, bdim_shared, 's-', label='Shared Memory')
    axs[1, 0].set_xlabel('blockDim.x (hilos por bloque)')
    axs[1, 0].set_ylabel('Tiempo del kernel (ms)')
    axs[1, 0].set_title('3. Tiempo frente a blockDim.x (N=2000)')
    axs[1, 0].set_xscale('log', base=2)
    axs[1, 0].set_xticks(blockdims)
    axs[1, 0].set_xticklabels([str(int(b)) for b in blockdims])
    axs[1, 0].grid(True)
    axs[1, 0].legend()

    # 4. Curva de Amdahl: predicción vs medición
    axs[1, 1].plot(N_values, amdahl_pred, 'k--', label='Predicción Amdahl')
    axs[1, 1].plot(N_values, speedup_shared, 'ro-', label='Medición Shared (E2E)')
    axs[1, 1].set_xlabel('N (cuerpos)')
    axs[1, 1].set_ylabel('Speedup')
    axs[1, 1].set_title('4. Curva de Amdahl: Predicción vs Medición')
    axs[1, 1].grid(True)
    axs[1, 1].legend()

    # 5. Trayectorias y E(t)
    axs[2, 0].plot(times, k, label='Cinética (K)')
    axs[2, 0].plot(times, u, label='Potencial (U)')
    axs[2, 0].plot(times, total_e, 'k-', linewidth=2, label='Total (E)')
    axs[2, 0].set_xlabel('Tiempo de simulación (s)')
    axs[2, 0].set_ylabel('Energía')
    axs[2, 0].set_title('5. Evolución de la Energía E(t) (Deriva Euler)')
    axs[2, 0].grid(True)
    axs[2, 0].legend()

    # 6. Comparación básica vs. shared memory
    axs[2, 1].plot(N_values, gpu_basic_kernel, 'o-', label='Kernel Básico')
    axs[2, 1].plot(N_values, gpu_shared_kernel, 's-', label='Kernel Shared Memory')
    axs[2, 1].set_xlabel('N (cuerpos)')
    axs[2, 1].set_ylabel('Tiempo del kernel (ms)')
    axs[2, 1].set_title('6. Comparación Básica vs. Shared Memory')
    axs[2, 1].grid(True)
    axs[2, 1].legend()

    plt.tight_layout()
    plt.subplots_adjust(top=0.92)
    plt.savefig('performance_plots.png', dpi=300)
    plt.close()

    print("Gráficos generados y guardados en performance_plots.png!")

if __name__ == '__main__':
    main()
