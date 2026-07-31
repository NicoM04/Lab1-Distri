#!/bin/bash
#SBATCH --job-name=lab2_cuda
#SBATCH --partition=GPU
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=12
#SBATCH --mem=64G
#SBATCH --gres=gpu:1
#SBATCH --time=04:00:00
#SBATCH --output=logs%j.out

echo "======================================================="
echo "Iniciando Pipeline Laboratorio 2 (CUDA N-Cuerpos)"
echo "Fecha y hora:" $(date)
echo "Nodo:" $SLURM_NODELIST
echo "======================================================="

echo "Información de la GPU:"
nvidia-smi

echo ""
echo "Compilando proyecto (limpieza previa)..."
make clean benchmark

echo ""
echo "Creando carpeta de salida..."
mkdir -p lab2_plots

echo ""
echo "Ejecutando simulación y benchmark CUDA en C++..."
# El programa generará automáticamente los archivos .dat correspondientes 
# (benchmark_results, scaling_analysis, blockdim_study) dentro de lab2_plots.
# Guardamos la salida del terminal en el log que pide la rúbrica.
./nbody_2d -benchmark-cuda > lab2_plots/cluster_run.log

echo ""
echo "Exportando serie de tiempo de energías y trayectorias..."
# Ejecutamos con el flag simulate para generar el energy_timeseries.dat y trajectories.dat
./nbody_2d -simulate -export-energy -N 1000 -steps 1000 -dt 0.01

# Movemos los datos de simulación físicos a la carpeta de gráficas
mv trajectories.dat lab2_plots/ 2>/dev/null
mv energy_timeseries.dat lab2_plots/ 2>/dev/null

echo ""
echo "Generando el gráfico final consolidado..."
cd lab2_plots
python3 plot_real_data.py
cd ..

echo ""
echo "======================================================="
echo "Pipeline finalizado exitosamente."
echo "Tus gráficas están en lab2_plots/performance_plots.png"
echo "======================================================="
