#!/bin/bash

# Ejecutar ./ex5 en segundo plano
./ex5 4 &
pid_ex5=$!

# Ejecutar ./realzador en segundo plano
./realzador &
pid_realzador=$!

# Esperar a que ./ex5 y ./realzador terminen
wait $pid_ex5
wait $pid_realzador

# Una vez que ambos hayan terminado, ejecutar ./combine
./combine

echo "Proceso completado: Imágenes combinadas."
