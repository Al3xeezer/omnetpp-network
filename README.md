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

## Funcionamiento
En este apartado se explican en mayor detalle los módulos de fuente, emisor y receptor de paquetes.
### Source.cc
Este módulo cuenta con las siguientes funciones:
* `initialize()`: en esta función se inicializa la tasa de generación de paquetes, el número de secuencia y el parámetro *hasSource*. Este último parámetro se utiliza para diferenciar los nodos con fuente de los que no (todos tienen un módulo de fuente pero no todos envían paquetes). Además, en esta función se programa un evento propio o *selfmessage* que se utilizará para crear nuevos paquetes al de un tiempo determinado.

* `createPck()`: esta función se encarga de generar un nuevo paquete que sigue la estructura de `myPacket.msg`.

* `getDestination()`: esta función genera de forma aleatoria el destino de cada paquete.

* `handleMessage(cMessage *msg)`: esta función se lanza cada vez que el módulo recibe un nuevo evento. Con cada evento, se genera un nuevo paquete con `createPck()`, se envía y se programa un nuevo evento para dentro de un tiempo determinado (tasa exponencial).

### Receiver.cc
Este módulo cuenta con las siguientes funciones:
* `initialize()`: no hace nada.

* `createPck()`: esta función se encarga de generar un nuevo paquete de *acknoledge* ack o nack, con nodo origen el propio nodo y como nodo destino el nodo que envió el paquete pendiente de confirmar.

* `handleMessage(cMessage *msg)`: esta función se lanza cada vez que el módulo recibe un nuevo evento/paquete. Cada vez que llega un paquete, se comprueba si es un paquete normal, un ack o un nack:

  1. Si es un paquete, se comprueba si tiene error: si tiene, se crea un nack y si no tiene, un ack (vía `createPck()`). Finalmente, se envía el ack/nack **al sender**. En caso de que no haya habido error, se envía también el paquete **al sender**.

  2. Si es un ack, este es un paquete de confirmación que proviene de otro nodo y que esta confirmando un paquete enviado por este propio nodo (en el sender). De esta forma, se reenvía directamente al sender.

  3. Si es un nack, es un paquete que proviene de otro nodo y que está indicando que le ha llegado un paquete enviado por este nodo con error. Se reenvía al sender para que este lo retransmita.

### Sender.cc
Este módulo cuenta con las siguientes funciones:
* `initialize()`: esta función se encarga de inicializar las colas y las máquinas de estados para cada una de las conexiones que tenga el nodo. Para simular el comportamiento de un nodo que implementa una política FIFO y Stop & Wait para el envío de paquetes, es necesario **definir una maquina de estados para cada uno de los enlaces que permita controlar cuando se puede enviar un nuevo paquete a través de cada uno de ellos y una cola para almacenar los paquetes que lleguen al nodo mientras el enlace por el que quieran salir se encuentre ocupado.**

* `router(myPacket *pck)`: esta función se encarga de obtener el enlace a través del cual se tiene que enviar el paquete que se le pasa para cada uno de los nodos de la red. Esta función, por lo tanto, actua a modo de tabla de rutado para los paquetes entrantes.

* `sendCopyOf(myPacket *pck, int index)`: esta función se encarga de enviar un paquete a través del enlace que se le indica a través del parámetro *index*. En primer lugar duplica el paquete, envía el original y la copia la almacena en la cola correspondiente al enlace de salida (mismo índice para ese destino, txQueue[indice] y out[indice]) **en primera posición, de modo que en caso de que llegue una retransmisión se pueda obtener facilmente de la cola.** Después, se cambia la maquina de estados (mismo índice) a *TYPE_BUSY* para que no se pueda volver a enviar paquetes a través de ese enlace hasta que no llegue la confirmación y, finalmente, se programa un evento de *timeout* para que en caso de que se pierda el paquete se vuelva a retransmitir.

* `handleMessage(cMessage *msg)`:esta función es la encargada de manejar todas las casuísticas para el correcto funcionamiento del emisor. La forma de operar es la siguiente:

  1. En primer lugar se comprueba si el evento recibido es de *timeout*. Si lo es, comprueba cual es (de que cola), extrae el paquete de la cola correspondiente y lo retransmite (vía `sendCopyOf()`).

  2. En segundo lugar, comprueba si es un paquete que ha llegado desde su `Source.cc`. Si es así, llamando a la función `router()` obtiene el índice de la cola/enlace/máquina de estados y, en función de si el enlace se encuentra ocupado (BUSY) o libre (IDLE) lo inserta en la cola correspondiente o lo envía mediante la función `sendCopyOf()`.

  3. Si no es ninguno de los casos anteriores, quiere decir que lo que ha llegado al emisor es un paquete externo, ya sea de su propio receptor o desde otro nodo. Entonces, se obtiene el índice del enlace por el que ha llegado, **gateIndex** y el índice de rutado, **routerIndex**. Posteriormente se comprueba que tipo de paquete es: paquete (TYPE_PCK), ack (TYPE_ACK) o nack (TYPE_NACK):

    * **TYPE_PCK:** si el índice de rutado es distinto de -1 (en `router()` se utiliza -1 para indicar que el nodo es final), se comprueba el estado de la maquina de estados, state_machine[routerIndex]: si es IDLE se envía y si es BUSY se inserta en la cola.
    
    * **TYPE_ACK:** se comprueba el atributo *pck.destination* del paquete y, si el destino es este nodo, quiere decir que el ack viene desde otro nodo avisando que el paquete enviado ha llegado correctamente. Por lo tanto, se puede eliminar la copia almacenada en la cola de ese paquete y enviar el siguiente en el caso de que esa cola no estuviese vacía. Si el destino no es este nodo, el ack recibido ha sido creado por el receptor de este mismo nodo para avisar a otro que el paquete ha llegado correctamente. De esta forma, el emisor solo tiene que enviar el ack al nodo correspondiente (out[gateIndex]).
  
    * **TYPE_NACK:** se comprueba el atributo *pck.destination* del paquete y, si el destino es este nodo, quiere decir que otro nodo ha recibido el paquete que se ha enviado con errores y pide la retransmisión. Por lo tanto, el emisor tiene que volver a enviar el paquete (la copia que se habia almacenado en la cola correspondiente) vía `sendCopyOf()` (out[gateIndex]). Si el destino no es este nodo, el nack ha sido creado por el receptor de este mismo nodo para avisar a otro de que el paquete ha llegado con errores. De esta forma , el emisor solo tiene que enviar el nack al nodo correspondiente (out[gateIndex]).

