FROM gcc:11

# Evitar prompts interactivos durante la instalación de paquetes
ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /app

# 1. Dependencias del sistema y de Python
# Usamos apt-get para matplotlib en lugar de pip para evitar advertencias de 
# "entorno administrado externamente" y aprovechar el caché de la imagen base.
RUN apt-get update && apt-get install -y \
    make \
    git \
    cmake \
    python3 \
    python3-matplotlib \
    && rm -rf /var/lib/apt/lists/*

# 2. Instalar Catch2 v3 optimizado
# Añadimos --depth 1 para no descargar el historial de Git (más rápido)
# Añadimos -DBUILD_TESTING=OFF para no compilar los tests del propio Catch2
# Añadimos -j$(nproc) para compilar usando todos los núcleos del CPU
RUN cd /tmp && \
    git clone --depth 1 --branch v3.4.0 https://github.com/catchorg/Catch2.git && \
    cd Catch2 && \
    mkdir build && cd build && \
    cmake -DBUILD_TESTING=OFF .. && \
    make -j$(nproc) && make install && \
    ldconfig && \
    cd / && rm -rf /tmp/Catch2

# 3. Copiar archivos del proyecto
COPY . /app

# 4. Compilar la aplicación principal
RUN make clean && make all

# 5. Ejecutar tests durante la construcción de la imagen
# Al estar en un RUN, si un test falla, el build de Docker falla inmediatamente.
# Esto asegura que a tu GHCR SOLO se suban imágenes 100% estables.
RUN echo "--==### Ejecutando Tests con Catch2 ###==--" && make test

# 6. Comando por defecto: el Dockerfile debe permitir una demostración del simulador
CMD ["./nbody_2d"]