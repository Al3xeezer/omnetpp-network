# omnetpp-network
En este repositorio se ha subido la resolución del caso 3 desarrollado en OMNeT++. A continuación se explica en mayor detalle el procedimiento seguido, así como una explicación de los métodos implementados, problemáticas encontradas o algunos comentarios acerca del mismo.

## Estructura del proyecto
Antes de comenzar a explicar el funcionamiento de cada uno de los módulos, se ha creído conveniente explicar brevemente la estructura de ficheros del proyecto de modo que sea más sencilla la comprensión posterior:

* `network.ned`: en este fichero se definen los módulos que se van a utilizar, así como sus enlaces, conexiones y otros parámetros básicos. Además, aquí es donde se definen las redes o *networks* (agrupaciones de módulos que están conectados y que interaccionan entre sí durante la simulación).

* `omnetpp.ini`: en este fichero se permiten inicializar/modificar ciertos parámetros de los módulos del network.ned. De esta forma, en el .ned se definen los parámetros por defecto de cada módulo y en el .ini se inicializan con los valores que correspondan (para cada instancia). 

* `myPacket.msg`: en este fichero se define la estructura de paquetes que se va utilizar en la simulación. A partir de este fichero, el compilador genera los ficheros `myPacket_m.cc` y `myPacket_m.h`.

* `Source.cc`: este fichero define el comportamiento del módulo de fuente o generador de paquetes. De esta forma, este módulo se encarga de generar paquetes con una tasa exponencial y con la estructura definida en `myPacket.msg`.

* `Sender.cc`: este fichero define el comportamiento del módulo de emisor. Éste es el encargado de gestionar el envío de paquetes a otros nodos de la red.

* `Receiver.cc`: este fichero define el comportamiento del módulo de receptor. Éste es el encargado de gestionar la recepción de paquetes provenientes de otros módulos, comprobar si tienen errores y generar los paquetes de *acknoledge* ack/nack correspondientes.

## Topología de red
El esquema de red que se ha definido es el siguiente:


En él se ha definido un módulo compuesto al que se ha llamado *nodo*, el cual esta conformado por los tres módulos simples que se han mencionado anteriormente: source, sender y receiver.

De este módulo *nodo* se han hecho 5 instancias que se han conectado entre sí para definir la red completa. A su vez, cada uno de los nodos está conectado internamente de forma que sus puntos de entrada (in[]) están conectados a los puntos de entrada de su submódulo *receiver* (receiver.in[]) y los puntos de salida (out[]) a los puntos de salida de su submódulo *sender* (sender.out[]).

Por último, para simular el envío de paquetes a través de enlaces "reales" (con probabilidad de error, retardo, etc.) se ha definido el canal *Ethernet100*. Este canal se ha utilizado para unir entre sí cada uno de los nodos.
