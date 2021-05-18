int main(int argc, char **argv){
 
    if (argc == 1){
        return 0;
    }
 
    char **command = &argv[1];
 
    int *pipe_pos = get_pipe_pos(argc-1, command);
    //Vector with positions of pipes found, position 0 reserved for the total amount of commands.
 
    char **com[pipe_pos[0]]; //Commands vector
    get_commands(&*com);
 
    //EXECUTE COMMANDS
    pid_t fork1, fork2, fork3;
    int fd1[2], fd2[2];
 
    if(pipe(fd1) < 0){
        perror("pipe1");
    }
    if(pipe(fd2) < 0){
        perror("pipe2");
    }
 
    //COMMAND 1
    fork1 = fork();
    if(fork1 == 0){
        dup2(fd1[1], STDOUT_FILENO);
        execvp(com[0][0], com[0]);
    }else{
        waitpid(fork1, NULL, 0);
    }
 
    //COMMAND 2
    fork2 = fork();
    if(fork2 == 0){
        dup2(fd1[0], STDIN_FILENO);
        dup2(fd2[1], STDOUT_FILENO);
        execvp(com[1][0], com[1]);
    }else{
        waitpid(fork2, NULL, 0);
    }
 
    //COMMAND 3
    fork3 = fork();
    if(fork3 == 0){
        dup2(fd2[0], STDIN_FILENO);
        execvp(com[2][0], com[2]);
    }else{
        waitpid(fork3, NULL, 0);
    }
 
    return 0;
}
