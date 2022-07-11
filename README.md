<b>Requisitos:</b><br/>
Banco de Dados Postgres intalado ou através do Docker com docker-compose<br/>
GCC Compiler<br/>
Linux OS<br/>

<br/>
Caso opte por utilizar o Docker e o docker-compose siga os seguintes passos. <br/>
Para criar o container que será usado como banco de dados use o comando: <br/>

```
docker-compose up -d
```

Com o banco de dados online, crie as tabelas e insira um usuário e um arquivo, esse arquivo precisa estar na pasta files na raiz do projeto, como exemplo temos o arquivo teste.txt dentro da pasta files. <br/>
Após isso configure as variáveis de conexão com o banco de dados nos parametros da função "connection()" no arquivo "db.c".<br/>

<strong>É necessário ter um usuário raiz com seus arquivos para serem compartilhados com os outros usuários</strong><br/>

A seguir se apresentam os comandos de compilação de cada arquivo. <br/>
Arquivo <b>udpServer.c</b>: 
```
 Compilar: gcc src/udpServer.c src/db.c src/communication.c -o udpServer -lpq
 Executar: ./udpServer
```
<br/>
Arquivo <b>udpProvider.c</b>: 

```
 Compilar: gcc src/udpProvider.c src/utilities.c -o udpProvider -lm
 Executar: ./udpProvider
```

<br/>
Arquivo <b>udpClient.c</b>: 

```
 Compilar: gcc src/udpClient.c src/utilities.c -o udpClient -lm
 Executar: ./udpClient
```

Lembrando que as constantes presentes em cada arquivo devem ser alteradas afim de se obter o resultado esperado.

