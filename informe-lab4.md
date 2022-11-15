
INFORME: Laboratorio 4 BigBrother FS

Grupo 23

Integrantes :

-   Bazan tomas
    
-   Carabelos Milagros
    
-   Pereyra Carrillo Juan Cruz
    
-   Scavuzzo Ignacio
    

  

ÍNDICE

1.[Preguntas](##preguntas)

2.[Explicación por partes](##Explicación-por-partes)
​	[Primera parte](###primera-parte)

​	[Segunda parte](###segunda-parte)

​	[Tercera parte](###tercera-parte)

3.[Como correrlo](##como-correrlo)

  

## Preguntas

1.  Cuando se ejecuta el main con la opción -d, ¿qué se está mostrando en la pantalla?

    "-d" habilita una opción de debug donde básicamente podemos ver un archivo de log que hace verboso el funcionamiento de nuestro sistema de archivos, podemos mantener este archivo comunicándose con nosotros en una terminal mientras utilizamos nuestro volumen en otra terminal distinta.

  

2.  ¿Hay alguna manera de saber el nombre del archivo guardado en el cluster 157?
    
    Podría lograrse esto pero necesitariamos utilizar la fat table para luego aplicar algunas funciones y tener acceso finalmente a la entrada de la tabla correspondiente, la cual nos otorga, entre otras cosas, dos punteros a char: *dir_entry->base_name y dir_entry->extension* los cuales nos dan información sobre el nombre del archivo, este procedimiento es parecido al que utilizamos en la función search_bb_orphan_dir_cluster

 


3.  ¿Dónde se guardan las entradas de directorio? ¿Cuántos archivos puede tener adentro un directorio en FAT32?  
    
    Las entradas de directorio se guardan en los cluster que ocupa cada directorio, en la versión que se nos entrega de FAT32 los directorios pueden tener como máximo 16 archivos ya que cada cluster es de 512 bytes y cada entrada de directorio ocupa 32 bytes y los directorios no ocupan más de un cluster
    

  

4.  Cuando se ejecuta el comando como ls -l, el sistema operativo, ¿llama a algún programa de usuario? ¿A alguna llamada al sistema? ¿Cómo se conecta esto con FUSE? ¿Qué funciones de su código se ejecutan finalmente?  
    
    Ls se ejecuta a través de la función de FUSE fat_fuse_readdir la cual utiliza fat_fuse_read_children y devuelve la información sobre los archivos que viven en nuestro volumen en forma de array. Para generar esta array lo que debemos hacer es leer cada uno de los hijos de nuestro volumen; iterandolos en la funcion h_tree_flatten_h_children y rellenando el array. No llama a programas de usuario, ni llamadas al sistema.
      Para ocultar nuestros archivos debemos ignorar los hijos que correspondan a bb y a fs.log.

  


5.  ¿Por qué tienen que escribir las entradas de directorio manualmente pero no tienen que guardar la tabla FAT cada vez que la modifican?
    

    A diferencia de las dentries la tabla fat está mapeada en memoria para un acceso más eficiente y luego, se puede persistir para mantener los cambios realizados mientras utilizamos el volumen. Incluso podemos chequear cambios con la copia de fat que se encuentra a disposición del file system para un mejorar la seguridad de la persistencia de los datos, mientras que por otro lado las dentries necesitan ser escritas en los cluster que se encuentran en disco para un correcto funcionamiento del sistema.


6.  Para los sistemas de archivos FAT32, la tabla FAT, ¿siempre tiene el mismo tamaño? En caso de que sí, ¿qué tamaño tiene?
    

    Durante la ejecución la tabla fat tiene siempre el mismo tamaño y ocupa en FAT32 la cantidad de cluster que tenemos multiplicado por los 32 bits por entrada. Sin embargo la cantidad de cluster puede variar según la arquitectura donde montemos nuestra imagen por lo tanto el tamaño de la fat puede ser diferente entre dispositivos diferentes.

  

## Explicación por partes

  

### Primera parte

En la primera parte del proyecto decidimos implementar una función que funcione parecido a mknod, ésta es la que se encarga de crear archivos. Lo que debíamos hacer era crear un archivo que se llame fs.log para espiar al usuario. Antes de crear el archivo chequeamos que no exista dicho archivo recorriendo el árbol de archivos. Una vez que la imagen esté montada, cada vez que el usuario escriba o lea un archivo esta información se escribirá en fs.log. Luego para esconder dicho archivo, modificamos las funcion que se encarga de buscar las entradas y mostrarlas por pantalla, cuando encuentre un archivo de nombre fs.log lo omite.  



  

### Segunda parte

En la segunda parte del proyecto intentamos reutilizar lo hecho en la primera parte pero nos dimos cuenta de que necesitamos refaccionar el código.

Decidimos dividir el código en distintas partes:  

Búsqueda (search_bb_orphan_dir_cluster): Nuestro algoritmo de búsqueda se encarga de ir iterando todos los clusters hasta encontrar algún cluster marcado como BAD. Una vez que encontramos un cluster “malo” necesitamos chequear que este cluster es el que aloja a nuestro directorio “bb”.
  -Caso en el que lo encuentra: Devolvemos el número de cluster en el cual esté nuestro bb.
  -Caso en el que no lo encuentra: Devolvemos el siguiente cluster vacío y lo marcamos como un cluster malo y creamos el directorio bb.

Creación del directorio (bb_init_log_dir): Creamos un directorio desde 0, sin usar una direntry, para luego insertarlo en nuestro file_tree.

Creación del fs.log (create_fs_file): Esta es la función más importante. Lo que necesitamos hacer aquí es utilizar el fat_tree_get_file para obtener el directorio de bb. Luego leeremos todos los hijos de bb, buscando a ver si existe un fs.log
  -Caso en el que exista: Lo insertamos a nuestro file_tree, no es necesario crearlo de vuelta, sino perdemos persistencia sobrescribiendo el archivo.
  -Caso en el que no exista: Necesitamos crearlo, lo inicializamos con la función fat_file_init, luego, lo insertamos en nuestro file_tree y lo seteamos como hijo de bb_file.

  
### Tercera parte


En esta etapa del proyecto implementamos dos funciones básicas unlink que se utiliza mediante rm para borrar archivos y rmdir para eliminar directorios, para esto nos basamos en el funcionamiento de unix por lo tanto rmdir solo funciona para directorios vacíos. En cada una de las funciones que implementamos las tareas realizadas son, liberar todos los cluster (lo cual hacemos con truncate para eliminar todos menos el primero y después borramos el primero manualmente), eliminar la dentry del directorio padre, actualizar el árbol de directorios, y finalmente persistir los cambios necesarios.

  

## Como correrlo

Script para correr el file system:

Se creó un script para testear funciones y automatizar el testeo de las implementaciones. Para correrlo ejectutar ./montaje.sh test. Se encarga de desmontar la imagen, limpiar los archivos ejecutables mediante make clean, compilar con make y ejecutar la linea de comando ‘./fat-fuse ../resources/bb_fs.img mnt’ que, básicamente, monta la imagen bb_fs en la carpeta mnt. Si se daña la imagen se puede ejecutar ‘./montaje.sh restore’. Esto se encarga de restaurar la imagen a un estado previo del repositorio, desmontar la imagen con permisos de superusuario, eliminar la carpeta y volver a crearla. También cuentan con funcionalidades para montar, desmontar. Todo esto se puede ver ejecutando ‘./montaje.sh help’ . Nota: si no se puede ejecutar dar permisos de ejecutable al archivo con chmod +x montaje.sh.INFORME: Laboratorio 4 BigBrother FS
