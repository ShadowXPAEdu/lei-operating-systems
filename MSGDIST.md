# MSGDIST
---

  Aceitar mensagens, armazená-las e redistribuí-las a quem estiver interessado nelas.

### Sistema MSGDIST:

  - `Pessoas:` Administradores e Utilizadores.
  - `Processos e Programas:` Gestor (Servidor), Verificador (Fornecido) e Clientes (Cliente).
  - `Mensagem e Tópico:`
    - Tópico    char[50]
    - Título    char[50]
    - Corpo     char[1000]
    - Duração   int
    (Mensagem terá que ter um ID, provávelmente atribuido pelo servidor)

  O gestor adiciona novos tópicos à sua lista de tópicos.
  Duração é o tempo em segundos que a mensagem fica no servidor.

---

  Os clientes enviam mensagens ao Gestor que serão depois entregues aos clientes consoante as escolhas e consultas que estes fazem.

---

# Cliente

  O cliente poderá:
  - Escrever uma nova mensagem, especificando todos os campos, e enviá-la ao gestor.
    - Se o cliente estiver subscrito ao tópico de mensagem enviada, deve ser imediatamente alertado.
  - Consultar a lista de tópicos atualmente existentes (tenham ou não mensagens).
  - Consultar a lista de títulos de mensagens de um determinado tópico.
  - Consultar uma mensagem de um tópico.
  - Subscrever/cancelar subscrição de um tópico.

---

  O cliente é alertado sempre que uma nova mensagem de um tópico por ele subscrito fica disponível no gestor.
  O utilizador indica o seu username ao programa cliente como argomento da linha de comandos. Caso exista um username com esse mesmo nome, o gestor acrescenta um número ao username e avisa o cliente.

---

# Interação automática Cliente-Servidor

  - O cliente recusar-se-á a prosseguir a sua execução se perceber que o gestor não está em execução.
  - Quando o utilizador encerra o seu processo cliente, este deverá informar o gestor.
  - O gestor, ao terminar, informará os clientes conhecidos, e que se encontram em execução, que vai terminar. Os clientes deverão também terminar, informando o utilizador.
  - O gestor detetará automáticamente que o cliente deixou de existir com uma precisão de 10 segundos. (Heartbeat/Keep alive)
  - O cliente detetará que o gestor deixou de estar a correr com uma precisão de, no máximo, 10 segundos. (Heartbeat/Keep alive)

---

# Servidor

  O gestor deverá concretizar as funcionalidades:
  - Receber mensagens enviadas pelos clientes, armazena-as durante o tempo indicado no campo duração da mensagem e elimina-as automáticamente quando a duração se esgotar. O número máximo de mensagens a armazenar é fixo durante a execução do gestor e é fornecido pelo valor da variável de ambiente MAXMSG.
  - Avisa os clientes acerca da existência de novas mensagens recebidas nos tópicos por eles subscritos.
  - Mantém informação acerca de que clientes e utilizadores se encontram a interagir com ele.
  - Mantém a informação acerca de que tópicos estão subscritos por que clientes/utilizadores.
  - É lancado pelo administrador e aceita, por parte deste, comandos escritos.
  - Deteta que clientes terminaram a sua execução.
  - Avisa que clientes conhecidos que vai terminar, e permite que o cliente saiba que ainda está em execução.
  - Reporta de imediato no `stderr` todas as ações feitas que alteram a lista de mensagens ou tópicos, e as ações que alteram o conjunto de clientes/utilizadores. (Logs do que está a acontecer mandados para o `stderr`)
  Exemplos:
    - Mensagem recebida.
    - Eliminação das mensagens publicadas quando as mesmas expiraram.
    - Criação de novos tópicos e a eliminação de tópicos.
    - Utilizador entrou.
    - Cliente deixou de existir.
    - Etc.

  Cada mensagem recebida pelo gestor é analizada com o auxílio do programa independente `verificador`, que recebe o corpo da mensagem e identifica o número de palavras proibidas nele existente. Se o número de palavras proibidas exceder o valor indicado na variável de ambiente `MAXNOT`, então a mensagem é rejeitada e o cliente é avisado. As palavras proibidas estão no ficheiro de texto cujo nome está na variável de ambiente `WORDSNOT`.

---

  O gestor deve efetuar toda a verificação da duração temporal das mensagens, a notificação aos clientes de novas mensagens, a verificação da existência dos clientes e o que for necessário para que os clientes percebam que o gestor ainda existe.

---

# Administração do gestor

  As ações que o administrador pode desencadear são:
  - Ligar/desligar a filtragem de palavras proibidas: `filter [on/off]`
  - Listar utilizadores: `users`
  - Listar tópicos: `topics`
  - Listar mensagens: `msg`
  - Listar mensagens de um tópico: `topic [tópico]`
  - Apagar mensagens: `del [id_mensagem]`
  - Excluir um utilizador: `kick [user]`
  - Desligar o gestor: `shutdown`
  - Eliminar tópicos que não tenham nenhuma mensagem de momento (subscrições são canceladas): `prune`

  Os seguintes mecanismos devem poder funcionar de forma independente/paralela uns dos outros de forma a que o atraso num deles não ponha diretamente em causa a execução dos restantes.
  - Verificação de palavras através do verificador.
  - Processamente de mensagens enviadas pelos clientes.
  - Atendimento dos comandos do administrador.
  - Deteção de clientes em encerramento ou incontactáveis.
  - Verificação das idades das mensagens e eliminação das que expiram.

---

# Considerações adicionais

  - Os programas cliente e gestor são lançados através da linha de comandos.
  - Apenas deve existir um processo gestor em execução. Ao detetar uma nova instância termina de imediato.
  - Caso existam diversas tarefas feitas em simultâneo envolvendo os mesmos dados, deverá haver o cuidado de sincronizar o acesso aos mesmos usando mecanismos adequados de modo a assegurar sempre a consistência da informação manipulada.
