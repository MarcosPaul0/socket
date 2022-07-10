<b>Requisitos:</b><br/>
Docker-compose<br/>
GCC Compiler<br/>
Linux OS<br/>

<br/>

Para criar o container que será usado como banco de dados use o comando: <br/>

```
docker-compose up -d
```
A seguir se apresentam os comandos de compilação de cada arquivo. <br/>
Arquivo <b>udpServer.c</b>: 
```
 Compilar: gcc src/udpServer.c src/db.c src/communication.c -o udpServer -lpq
 Executar: ./udpServer
```
<br/>
Arquivo <b>udpProvider.c</b>: 

```
 Compilar: gcc src/udpProvider.c src/verify.c -o udpProvider -lm
 Executar: ./udpProvider
```

<br/>
Arquivo <b>udpClient.c</b>: 

```
 Compilar: gcc src/udpClient.c -o udpClient -lm
 Executar: ./udpClient
```

Lembrando que as constantes presentes em cada arquivo devem ser alteradas afim de se obter o resultado esperado.

