#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/wait.h>

#include <readline/readline.h>
#include <readline/history.h>

#define MAXLINE 1024 //Numero maximo de letras por entrada
#define MAXPARAM 100 //Numero maximo de parâmetros por entrada


/**********Funções iniciais**********/

//Imprime o diretório atual
void pegaDir()
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("\nSO1: %s", cwd);
}

//Armazena a entrada da linha de comando
int pegaEntrada(char *entrada)
{
	char *buffer = readline("\n$ ");

	//Se o tamanho do buffer for diferente de 0, adiciona a entrada no histórico e armazena a entrada 
	if (strlen(buffer) != 0) {
		add_history(buffer);
		strcpy(entrada, buffer);
		return 1;
	} else {
		return 0;
	}
}

// Função para executar comandos embutidos (exit e cd)
int execEmbutidos(char** comando)
{
	int i = 0, aux = 0;
	char* listaComandos[2];

	listaComandos[0] = "exit";
	listaComandos[1] = "cd";

	while (i < 2) 
	{
		//Compara se o comando é igual a lista de comandos
		if (strcmp(comando[0], listaComandos[i]) == 0) {
			aux = i + 1;
			break;
		}
		i++;
	}

	//Checa auxiliar  (1 - sai da execução do shell, 2 - muda o diretório)
	switch (aux) 
	{
		case 1:
			exit(0);
		case 2:
			if(chdir(comando[1]) == -1)
				printf("Arquivo ou diretorio não encontrado\n");	
			return 1;
		default:
			break;
	}

	return 0;
}


/**********Funções para processar e separar a entrada**********/

//separa a entrada na ocorrencia de pipe (identificado por " | ")
int separaPipe(char* entrada, char** seprPipe)
{
	for (int i = 0; i < 2; i++) {
		seprPipe[i] = strsep(&entrada, "|");
		if (seprPipe[i] == NULL)
			break;
	}

	//retorna 0 se nao houver a ocorrencia do pipe
	if (seprPipe[1] == NULL)
		return 0; 
	else {
		return 1;
	}
}

// Separa e armazena os parametros da entrada
void separaToken(char *entrada, char **comando)
{
	char *token;
	int i = 0; 

	//Separa os tokens a partir da ocorrencia de espaço
	token = strtok(entrada, " ");
	if(token)
	{
		//Armazena os parametros da linha de comando equanto 
		//houverem tokens ou quantidade de parametros
		while (token != NULL || i < MAXPARAM)
		{
			comando[i] = token;
			if(comando[i] == NULL)
				break;
			token = strtok(NULL," ");
			i++;
		}
		//printf("%s ",comando[1]);

	}

}

//Processa a linha de comando e seapara os parâmetros se houverem
int separaComando(char* entrada, char** comando, char** comandoPipe)
{
	int checaPipe = 0;
	char *seprPipe[2];
	
	checaPipe = separaPipe(entrada, seprPipe);

	//se checaPipe retornar 1, as entradas em seprPipe 
	if (checaPipe) {
		separaToken(seprPipe[0], comando);
		separaToken(seprPipe[1], comandoPipe);

	} else {
		separaToken(entrada, comando);
		int i = 0, checaParalelo = 0;
		while(comando[i] != NULL)
		{
			if(strcmp(comando[i],"&") == 0)
			{
				comando[i] = NULL;
				checaParalelo = 1;
				break;
			}
			i++;
		}
		if(checaParalelo)
			return 3;
	}
	
	if (execEmbutidos(comando))
		return 0;
	else
		return 1 + checaPipe;
}


/**********Funções da família exec**********/

//Executa a linha de comando 
void execComando(char **comando)
{
	pid_t pidFilho = fork();

	//Processo pai cria processo filho para execução
	if (pidFilho == -1) 
	{
		printf("\nErro no fork");
		return;
	} 
	else if (pidFilho == 0) 
	{
		//execvp(*FILE, char const *argv) Irá buscar o arquivo do comando 
		//e executá-lo com seus respectivos argumentos
		if (execvp(comando[0], comando) < 0) 
		{
			printf("\nComando nao encontrado");
		}
		exit(0);
	} 

	else 
	{
		// Processo pai espera o filho terminar
		wait(NULL);
		return;
	}
}

//Executa a linha de comando com pipe (argumento | )
void execComandoPipe(char** comando, char** comandoPipe)
{
	int fd[2];
	pid_t pid1, pid2; //Filho 1 e 2

	//Processo pai cria um pipe
	if (pipe(fd) < 0) {
		printf("\nErro na criacao do pipe");
		return;
	}

	//Processo pai cria filho 1
	pid1 = fork();
	if (pid1 < 0) {
		printf("\nErro 1° fork");
		return;
	}

	//Checa filho 1 para ser excutado
	if (pid1 == 0) {

		close(fd[0]); //Fecha o descritor de leitura do pipe
		dup2(fd[1], 1); //Redireciona a saida padrao para o pipe
		
		//execvp(*FILE, char const *argv) Irá buscar o arquivo do comando 
		// e executá-lo com seus respectivos argumentos
		if (execvp(comando[0], comando) < 0) 
		{
			printf("\nErro na execucao do comando 1");
			exit(0);
		}
	} 
	else 
	{
		//Processo pai cria filho 2
		pid2 = fork();
		if (pid2 < 0) {
			printf("\nErro no 2° fork");
			return;
		}

		//Checa filho 2 para ser excutado
		if (pid2 == 0) {
			close(fd[1]); //fecha o descritor de escrita do pipe
			dup2(fd[0], 0); //redireciona a entrada padrao para o pipe

			//execvp(*FILE, char const *argv) Irá buscar o arquivo do comando 
			//e executá-lo com seus respectivos argumentos
			if (execvp(comandoPipe[0], comandoPipe) < 0) {
				printf("\nComando 2 nao encontrado");
				exit(0);
			}
		} 
		//Processo pai esperando pelo segundo filho
		else 
		{
			//Fecha os descritores de leitura e escrita do pipe
			close(fd[0]);
			close(fd[1]);

			int status;
			for (int i = 0; i < 2; i++)
			{
				wait(&status); //Espera pelo filho 2
      		}
		}
	}
	
}

//Executa o processo filho em background (comando seguido pelo argumento &)
int execParalelo(char **comando)
{
	pid_t pidFilho = fork();
	
	//Processo pai cria processo filho para execução
	if (pidFilho == -1) 
	{
		printf("\nErro no fork");
		return 0;
	} 
	else if (pidFilho == 0) 
	{
		//execvp(*FILE, char const *argv) Irá buscar o arquivo do comando 
		//e executá-lo com seus respectivos argumentos
		if (execvp(comando[0], comando) < 0) 
		{
			printf("\nComando nao encontrado");
		}
		exit(0);
	} 
	return 1;
}

//Finaliza processo filho em segundo plano
int finalizaFilho(int comParalelo)
{
	//Se comParalelo = 0, então não há processo filho em segundo plano (execução com &).
	//se comParalelo  = 1, então o processo em segundo plano deverá ser 
	//finalizado antes da execução do próximo comando
	if(comParalelo)
	{
		pid_t filhoParalelo = waitpid(-1, NULL, WNOHANG); // finaliza o filho em segundo plano 
		//Checa o retorno da finalização do filho
		if( filhoParalelo == -1)
			printf(" Erro ao finalizar processo filho");
		else
		{
			printf("\n%d  Finalizado",filhoParalelo);
			//comParalelo volta para 0 possibilitando uma nova execução da execParalelo
			comParalelo = 0; 
		}
	}
	return comParalelo;
}

int main(void)
{
	char entrada[MAXLINE];
	char *comando[MAXPARAM]; //comando com parametros separados
	char *comandoPipe[MAXPARAM]; //comando do pipe com parametros separados
	
	int checaExec = 0;		//flag para tipo de execução (com ou sem pipe)
	int comParalelo = 0;	//flag para execução do filho em segundo plano
	
	while (1)
	{
		pegaDir();

		if (pegaEntrada(entrada))
		{
            checaExec = separaComando(entrada,
            comando, comandoPipe);
			
			comParalelo = finalizaFilho(comParalelo);
			
            // checaExec retorna zero se não houver comando
            // Do contrario,
            // 1 se for um comando
            // 2 se estiver incluindo um pipe.
			// 3 se apresentar &, denotando execução do filho em background
			
			if (checaExec == 1)
                execComando(comando);
            if (checaExec == 2)
                execComandoPipe(comando, comandoPipe);
			if(checaExec == 3)
				comParalelo = execParalelo(comando);
        }
	}
	return 0;
}



