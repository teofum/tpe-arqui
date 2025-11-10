# TP2 - Sistemas Operativos

## Compilación y ejecución



## Comandos disponibles

### Comandos generales

- `help` - Muestra lista de comandos disponibles
- `echo <args>` - Imprime argumentos a stdout
- `clear` - Limpia la terminal
- `history` - Muestra historial de comandos

### Physical Memory Management

- `mem` - Muestra estado de la memoria (total, usada, libre)

### Procesos, context switching y scheduling

- `proc help` - Muestra ayuda de comandos de procesos
- `proc ls` - Lista todos los procesos con sus propiedades (PID, nombre, estado, prioridad, RSP, foreground)
- `proc fg <pid>` - Trae un proceso a foreground
- `proc kill <pid>` - Mata un proceso
- `proc nice <pid> <priority>` - Cambia la prioridad de un proceso (0-4)
- `proc stop <pid>` - Bloquea un proceso
- `proc run <pid>` - Desbloquea un proceso

### Inter-Process Communication

- `cat` - Imprime stdin a stdout
- `wc` - Cuenta líneas del input
- `filter` - Filtra vocales del input
- `mvar <n_writers> <n_readers>` - Problema de múltiples lectores/escritores

### Tests provistos por la Cátedra

- `test help` - Muestra lista de tests disponibles
- `test mm <max_memory>` - Test de memory allocator (ciclo infinito)
- `test sync <n>` - Test de sincronización con semáforos
- `test nosync <n>` - Test de sincronización sin semáforos (muestra race conditions)
- `test processes <max_processes>` - Test de creación, bloqueo y eliminación de procesos
- `test prio <max_value>` - Test de scheduler con prioridades

## Caracteres especiales

- `&` - Ejecuta comando en background
  ```bash
  test mm 1000000 &
  ```

- `|` - Conecta dos procesos mediante pipe
  ```bash
  cat | filter
  echo hola mundo | wc
  ```

## Atajos de teclado

- `Ctrl + C` - Mata el proceso en foreground
- `Ctrl + D` - Envía EOF (End of File)

## Ejemplos de uso por fuera de los tests



## Limitaciones



## Autores

- Arcodaci, Tiziano - tarcodaci@itba.edu.ar
- Fumagalli, Teo - tfumagalli@itba.edu.ar
- Pizzuto Beltran, Lorenzo - lpizzutobeltran@itba.edu.ar
