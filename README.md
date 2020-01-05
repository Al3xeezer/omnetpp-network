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

![Esquema de red](/img/esquema.PNG)

En él se ha definido un módulo compuesto al que se ha llamado *nodo*, el cual esta formado por los tres módulos simples que se han mencionado anteriormente: source, sender y receiver.

De este módulo *nodo* se han hecho 5 instancias que se han conectado entre sí para definir la red completa. A su vez, cada uno de los nodos está conectado internamente de forma que sus puntos de entrada (in[ ]) están conectados a los puntos de entrada de su submódulo *receiver* (receiver.in[ ]) y los puntos de salida (out[ ]) a los puntos de salida de su submódulo *sender* (sender.out[ ]).

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

* `createPck()`: esta función se encarga de generar un nuevo paquete de *acknoledge* ack o nack, con nodo origen su propio nodo y como nodo destino el nodo que envió el paquete pendiente de confirmar.

* `handleMessage(cMessage *msg)`: esta función se ejecuta cada vez que el módulo recibe un nuevo evento/paquete. Cada vez que llega un evento, se comprueba si es un paquete (TYPE_PCK), un ack (TYPE_ACK) o un nack (TYPE_NACK):

  1. Si es un paquete, se comprueba si tiene error: si tiene, se crea un nack y si no tiene un ack (vía `createPck()`). Finalmente, se envía el ack/nack **al emisor**. En caso de que no haya habido error, se envía también el paquete **al emisor**.

  2. Si es un ack, este es un paquete de confirmación que proviene de otro nodo y que está confirmando un paquete enviado por este nodo. Por lo tanto, este ack se reenvía directamente al emisor.

  3. Si es un nack, se trata de un paquete que proviene de otro nodo que está indicando que le ha llegado un paquete desde este nodo con error. Por lo tanto, se reenvía al emisor para que éste retransmita el paquete en cuestión.

### Sender.cc
Este módulo cuenta con las siguientes funciones:
* `initialize()`: esta función se encarga de inicializar las colas y las máquinas de estados para cada una de las conexiones que tenga el nodo. Para simular el comportamiento de un nodo que implementa una política FIFO y Stop & Wait para el envío de paquetes, es necesario **definir una máquina de estados para cada uno de los enlaces que permita controlar cuando se puede enviar un nuevo paquete a través de cada uno de ellos y una cola para almacenar los paquetes que lleguen al nodo mientras el enlace por el que se quiera transmitir se encuentre ocupado.**

* `router(myPacket *pck)`: esta función se encarga de obtener el enlace a través del cual se tiene que enviar el paquete que se le pasa para cada uno de los nodos de la red. Esta función, por lo tanto, actua a modo de tabla de rutado para los paquetes entrantes.

* `sendCopyOf(myPacket *pck, int index)`: esta función se encarga de enviar un paquete a través del enlace que se le indica a través del parámetro *index*. En primer lugar duplica el paquete, envía el paquete original y la copia generada la almacena en la cola correspondiente a su enlace de salida (mismo índice para ese destino, txQueue[índice] y out[índice]) **en primera posición, de modo que en caso de que llegue una retransmisión se pueda extraer el paquete facilmente de la cola.** Después, se cambia la máquina de estados (mismo índice) a *STATE_BUSY* para que no se pueda volver a enviar paquetes a través de ese enlace hasta que no llegue la confirmación y, finalmente, se programa un evento de *timeout* para que en caso de que se pierda el paquete se vuelva a retransmitir.

* `handleMessage(cMessage *msg)`: esta función es la encargada de manejar todas las casuísticas para el correcto funcionamiento del emisor. Su forma de operar es la siguiente:

  1. En primer lugar se comprueba si el evento recibido es de *timeout*. Si lo es, comprueba cual es (de que cola), extrae el paquete de la cola correspondiente y lo retransmite (vía `sendCopyOf()`).

  2. En segundo lugar, comprueba si es un paquete que ha llegado desde su fuente. Si es así, llamando a la función `router()` obtiene el índice de la cola/enlace/máquina de estados y, en función de si el enlace se encuentra ocupado (STATE_BUSY) o libre (STATE_IDLE) lo inserta en la cola correspondiente o lo envía mediante la función `sendCopyOf()`.

  3. Si no es ninguno de los casos anteriores, quiere decir que se trata de un paquete generado en su propio receptor o en otro nodo. Entonces, se obtiene el índice de la puerta por la que ha llegado, **gateIndex** y el índice de rutado, **routerIndex**. Posteriormente se comprueba que tipo de paquete es: paquete (TYPE_PCK), ack (TYPE_ACK) o nack (TYPE_NACK):

    * **TYPE_PCK:** si el índice de rutado es distinto de -1 (en `router()` se utiliza -1 para indicar que un nodo es final), se comprueba el estado de la máquina de estados, state_machine[routerIndex]: si es STATE_IDLE se envía y si es STATE_BUSY se inserta en la cola.
    
    * **TYPE_ACK:** se comprueba el atributo *pck.destination* del paquete y, si el destino es este nodo, quiere decir que el ack viene desde otro nodo avisando que el paquete enviado ha llegado correctamente. Por lo tanto, se puede eliminar la copia de ese paquete almacenada en una cola y enviar el siguiente en el caso de que esa cola no estuviese vacía. Si el destino no es este nodo, el ack recibido ha sido generado por el receptor de este mismo nodo para avisar a otro que el paquete ha llegado correctamente. Por lo tanto, el emisor solo tiene que enviar el ack al nodo correspondiente (out[gateIndex]).
  
    * **TYPE_NACK:** se comprueba el atributo *pck.destination* del paquete y, si el destino es este nodo, quiere decir que otro nodo ha recibido un paquete con errores de este nodo. Por lo tanto, el emisor tiene que volver a enviar el paquete (la copia que se habia almacenado en la cola correspondiente) vía `sendCopyOf()` (out[gateIndex]). Si el destino no es este nodo, el nack ha sido creado por el receptor de este nodo para avisar a otro de que el paquete ha llegado con errores. De esta forma, el emisor solo tiene que enviar el nack al nodo correspondiente (out[gateIndex]).
    
## Comentarios/Problemáticas
* En el código subido al repositorio se ha implementado el uso de *timeouts*. Sin embargo, en realidad éstos son innecesarios (además de una fuente de problemas). Tras investigar como funciona la clase *DatarateChannel*, he visto que la forma que tiene el programa de tratar el *ber* y el *per* es la misma, es decir, que en OMNeT++ realmente no se contempla la pérdida de paquetes (solo la llegada con errores). Por lo tanto y, teniendo en cuenta como se ha definido el receptor, todos los paquetes que se envíen llegarán al receptor y éste solo podrá diferenciar si tiene error o no. De este modo, siempre se responderá con un ack o un nack (evitando así la necesidad de un *timeout* puesto que todos los mensajes tendrán su respuesta y ésta siempre llegará).  

* Relacionado con el punto anterior (uso de *timeouts*), he visto que al ejecutar la simulación ésta suele acabar con error (si se hace en debug no, no entiendo muy bien el por qué). Esto se debe a como OMNeT++ maneja los tiempos de llegada:
```c++
simtime_t FinishTime = getParentModule()->gate("out",index)->getTransmissionChannel()->getTransmissionFinishTime();
    simtime_t nextTime = simTime()+300*(FinishTime-simTime());
    scheduleAt(nextTime,timeout[index]);
```
Para obtener los *timeouts*, en OMNeT++ se utiliza la función `getTransmissionFinishTime()`. **Este tiempo de finalización es una estimación que realiza el programa de cual sería el tiempo de simulación en el que el paquete debería llegar, no el real**. Por lo tanto hay una pequeña variación entre el real y el estimado. Además, éste método tiene otro problema. Tal y como se ha planteado el escenario, se tienen modulos compuestos (nodos) por otros módulos simples (emisor, receptor y fuente). Entonces, cuando se llama a esta función desde el emisor, el tiempo que devuelve no es el tiempo que tarda el mensaje recién enviado en llegar al siguiente nodo, ya que éste todavía no ha salido al exterior (se encontraría en el enlace **sender.out[ ] --> out[ ]** del nodo, por lo que te devuelve el tiempo estimado de ese enlace, no el externo). En caso de haber usado módulos simples si que devolvería correctamente el tiempo ya que el paquete se insertaría directamente en el enlace de salida del nodo.

En mi implementación, he puesto un multiplicador (x300) a ese tiempo de propagación del paquete para que no salten continuamente los *timeouts*. Sin embargo, a veces no es suficiente (lo que provoca que de error la simulación).   

**NOTA: Ejecutar la simulación en modo debug**
