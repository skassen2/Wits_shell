#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

#define MAX_WORDS 100  // Maximum number of words in the input
#define MAX_WORD_LEN 100  // Maximum length of each word

// Function to split the input into words and return the array of words
char **split_words(char *input, int *word_count) {
    char **words = malloc(MAX_WORDS * sizeof(char *));
    char *word;
    int count = 0;

    // Tokenize the input by spaces
    word = strtok(input, " \n");
    while (word != NULL && count < MAX_WORDS) {
        words[count] = malloc((strlen(word) + 1) * sizeof(char));
        strcpy(words[count], word);
        count++;
        word = strtok(NULL, " \n");
    }

    *word_count = count;
    return words;
}

bool is_redirect_present(char **words, int word_count) {
    for (int i = 0; i < word_count; i++) {
        if (strcmp(words[i], ">") == 0) {
            return true;
        }
    }
    return false;
}

bool is_parallel_errors(char **words, int word_count){// checks if only & and if & is the last in the array
    if(strcmp(words[0],"&")==0 && word_count==1){
        return true;//error
    }
    else{
        return false;// no error
    }

}

bool is_parallel(char **new_words, int new_word_count) {
    for (int i = 0; i < new_word_count; i++) {
        if (strcmp(new_words[i], "&") == 0) {
            return true;
        }
    }
    return false;
}

bool is_redirect_second_last_and_unique(char **words, int word_count) {
    if (word_count < 2 ||strcmp(words[0], ">") == 0) {//test case 12
        return false;
    }
    
    bool found = false;
    for (int i = 0; i < word_count; i++) {
        if (strcmp(words[i], ">") == 0) {
            if (found) {
                return false;  // More than one ">" found
            }
            if (i == word_count - 2) {
                found = true;  // The ">" is the second last element
            } else {
                return false;  // ">" is not in the correct position
            }
        }
    }
    
    return found;  // True if exactly one ">" is found and it's the second last element
}

void execute_file_in_new_process(char paths[15][100], char **words) {
    char full_path[1000]="";
    bool isFound=false;
    int index=-1;
    char found_path[1000]="";
    for(int i=0;i<15;i++){
        full_path[0] = '\0';//clear string
        strcat(full_path, paths[i]);
        strcat(full_path, words[0]);
        //printf("the path is %s\n",full_path);
        if (access(full_path, F_OK) == 0) {//check if path exists
            isFound=true;
            index=i;
            strcpy(found_path,full_path);
            //printf("Found it %s\n",found_path);

        }
    }

    if(isFound){
            pid_t pid = fork();  // Create a new process

            if(pid==0){
                char *Arg[] = {"/bin/bash", found_path, NULL};
                if (execv("/bin/bash", Arg) == -1) {
                    
                }
            }
            else{
                wait(NULL);
            }
        }
        else{
            char error_message[30] = "An error has occurred\n" ;
            write(STDERR_FILENO, error_message, strlen(error_message));
        }

}

char *add_trailing_slash(char *word) {//adds / to the end a word if its not given in the path word
    int length = strlen(word);

    // Check if the last character is '/'
    if (word[length - 1] != '/') {
        // Allocate memory for the new string (+1 for '/' and +1 for '\0')
        char *new_word = malloc((length + 2) * sizeof(char));

        // Copy the original word to the new word
        strcpy(new_word, word);

        // Add the '/' at the end and null-terminate the string
        new_word[length] = '/';
        new_word[length + 1] = '\0';

        return new_word;
    } else {
        // If the last character is already '/', return the original word
        return strdup(word);
    }
}

void execute_redirect(char paths[15][100], char **words, int word_count) {
    char full_path[1000] = "";
    bool isFound = false;
    char found_path[1000] = "";

    // Check if the command exists in any of the provided paths
    for (int i = 0; i < 15; i++) {
        full_path[0] = '\0'; // Clear string
        strcat(full_path, paths[i]);
        strcat(full_path, "/");
        strcat(full_path, words[0]);
        if (access(full_path, F_OK) == 0) { // Check if path exists
            isFound = true;
            strcpy(found_path, full_path);
            break;
        }
    }

    if (isFound) {
        pid_t pid = fork(); // Create a new process

        if (pid == 0) { // Child process
            int fd = -1;

            // Check for output redirection
            for (int i = 0; i < word_count; i++) {
                if (strcmp(words[i], ">") == 0) {
                    // Open the file for redirection
                    fd = open(words[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd == -1) {
                        perror("open");
                        exit(EXIT_FAILURE);
                    }

                    // Redirect stdout to the file
                    if (dup2(fd, STDOUT_FILENO) == -1) {
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }

                    // Remove ">" and the filename from the arguments
                    word_count = i;
                    break;
                }
            }

            // Determine if the command is a shell script (ends with ".sh")
            char *extension = strrchr(found_path, '.');
            bool is_shell_script = extension && strcmp(extension, ".sh") == 0;

            // Prepare the arguments for execv
            char **Arg = malloc((word_count + (is_shell_script ? 2 : 1)) * sizeof(char *));
            if (is_shell_script) {
                Arg[0] = "/bin/bash";
                Arg[1] = found_path; // The shell script to execute
                for (int i = 1; i < word_count; i++) {
                    Arg[i + 1] = words[i];
                }
                Arg[word_count + 1] = NULL;
            } else {
                for (int i = 0; i < word_count; i++) {
                    Arg[i] = words[i];
                }
                Arg[word_count] = NULL;
            }

            // Execute the command
            if (execv(is_shell_script ? "/bin/bash" : found_path, Arg) == -1) {
                perror("execv");
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
            }

            free(Arg);

            // Close the file descriptor if it was used
            if (fd != -1) {
                close(fd);
            }

            exit(EXIT_SUCCESS); // Exit the child process
        } else if (pid > 0) { // Parent process
            wait(NULL); // Wait for the child process to finish
        } else {
            // Fork failed
            perror("fork");
            exit(EXIT_FAILURE);
        }
    } else {
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
}

void execute_command(char paths[15][100], char **words,int word_count) {
    char full_path[1000]="";
    bool isFound=false;
    int index=-1;
    char found_path[1000]="";
    for(int i=0;i<15;i++){
        full_path[0] = '\0';//clear string
        strcat(full_path, paths[i]);
        strcat(full_path, words[0]);
        //printf("%s\n",full_path);
        if (access(full_path, F_OK) == 0) {//check if path exists
            isFound=true;
            index=i;
            strcpy(found_path,full_path);
            //printf("Found it %s\n",found_path);

        }
    }

    if(isFound){
            pid_t pid = fork();  // Create a new process

            if(pid==0){
                //char *Arg[]={found_path,NULL};
                char **Arg = malloc((word_count + 1) * sizeof(char *));
                for (int i = 0; i < word_count; i++) {
                    Arg[i] = words[i];
                }
                Arg[word_count] = NULL;
                if (execv(found_path, Arg) == -1) {
                    char error_message[30] = "An error has occurred\n" ;
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }

                free(Arg);
            }
            else{
                wait(NULL);
            }
        }
        else{
            char error_message[30] = "An error has occurred\n" ;
            write(STDERR_FILENO, error_message, strlen(error_message));
        }

}

bool is_shell_script(const char *word) {
    const char *extension = strrchr(word, '.'); // Find the last occurrence of '.'

    // Check if the extension is ".sh"
    if (extension != NULL && strcmp(extension, ".sh") == 0) {
        return true;
    }

    return false;
}

void basicLs(char paths[15][100], char **words,int word_count){
    char full_path[1000]="";
    bool isFound=false;
    int index=-1;
    char found_path[1000]="";
    for(int i=0;i<15;i++){
        full_path[0] = '\0';//clear string
        strcat(full_path, paths[i]);
        strcat(full_path, words[0]);
        //printf("%s\n",full_path);
        if (access(full_path, F_OK) == 0) {//check if path exists
            isFound=true;
            index=i;
            strcpy(found_path,full_path);
            //printf("Found it %s\n",found_path);

        }
    }

    if(isFound){
            pid_t pid = fork();  // Create a new process

            if(pid==0){
                char *Arg[]={"ls","-v",NULL};
                if (execv(found_path, Arg) == -1) {
                    char error_message[30] = "An error has occurred\n" ;
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
            }
            else{
                wait(NULL);
            }
        }
        else{
            char error_message[30] = "An error has occurred\n" ;
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
}

char** split_up_and_check_redirection(char **words, int word_count, int *new_word_count){
    int max_new_words = word_count * 3; // A rough estimate to avoid multiple reallocations
    char **new_words = malloc(max_new_words * sizeof(char *));
    *new_word_count = 0;

    for (int i = 0; i < word_count; i++) {
        char *current_word = words[i];
        char *gt_ptr = strchr(current_word, '>');

        if (gt_ptr) {
            // If the '>' is the first character
            if (gt_ptr == current_word) {
                new_words[(*new_word_count)++] = strdup(">");
                char *remaining_word = gt_ptr + 1;
                if (*remaining_word != '\0') {
                    new_words[(*new_word_count)++] = strdup(remaining_word);
                }
            }
            // If the '>' is somewhere in the middle or end
            else {
                char *before_gt = strndup(current_word, gt_ptr - current_word);
                new_words[(*new_word_count)++] = before_gt;
                new_words[(*new_word_count)++] = strdup(">");
                char *after_gt = gt_ptr + 1;
                if (*after_gt != '\0') {
                    new_words[(*new_word_count)++] = strdup(after_gt);
                }
            }
        } else {
            new_words[(*new_word_count)++] = strdup(current_word);
        }
    }

    return new_words;
    
}

int count_ampersands(const char *word) {
    int count = 0;
    while (*word) {
        if (*word == '&') {
            count++;
        }
        word++;
    }
    return count;
}

// Function to split words containing & into individual components
char** split_up_ampersand(char **words, int word_count, int *new_word_count) {
    // Calculate the maximum possible number of new words
    int max_new_words = 0;
    for (int i = 0; i < word_count; i++) {
        if (strcmp(words[i], "&") == 0) {
            max_new_words++; // Count as one word if it's just "&"
        } else {
            max_new_words += count_ampersands(words[i]) * 2 + 1; // for each & and its adjacent parts
        }
    }

    // Allocate memory for the new array of words
    char **new_words = (char **)malloc(max_new_words * sizeof(char *));
    int new_idx = 0;

    for (int i = 0; i < word_count; i++) {
        char *current_word = words[i];
        
        if (strcmp(current_word, "&") == 0) {
            // If the word is just "&", add it as is
            new_words[new_idx++] = strdup(current_word);
        } else {
            // Split the word by &
            char *part = strtok(current_word, "&");
            while (part != NULL) {
                new_words[new_idx++] = strdup(part); // Add part before &
                part = strtok(NULL, "&");
                if (part != NULL) {
                    new_words[new_idx++] = strdup("&"); // Add &
                }
            }
        }
    }

    // Update the new word count
    *new_word_count = new_idx;

    // Reallocate memory to fit the exact number of new words
    new_words = (char **)realloc(new_words, new_idx * sizeof(char *));
    
    return new_words;
}

void cdErrors(){//happens if cd has 0 or more than 1 command or any errors
    char error_message[30] = "An error has occurred\n" ;
    write(STDERR_FILENO, error_message, strlen(error_message));

}

char ***create_command_array(char **new_words, int new_word_count, int **row_sizes, int *num_rows) {
    // Allocate memory for the 2D array of strings
    char ***array = malloc(new_word_count * sizeof(char **));
    *row_sizes = malloc(new_word_count * sizeof(int));
    int row = 0, col = 0;

    array[row] = malloc(new_word_count * sizeof(char *));
    for (int i = 0; i < new_word_count; i++) {
        if (strcmp(new_words[i], "&") == 0) {
            // Check if '&' is the last element in the array
            if (i != new_word_count - 1) {
                // End of the current command, move to the next row
                (*row_sizes)[row] = col;  // Store the number of elements in the current row
                array[row][col] = NULL;    // Mark the end of the current row
                row++;
                col = 0;
                array[row] = malloc(new_word_count * sizeof(char *));
            }
        } else {
            // Copy the word to the current row and column
            array[row][col] = strdup(new_words[i]);
            col++;
        }
    }
    
    // Finalize the last row
    (*row_sizes)[row] = col;
    array[row][col] = NULL;
    *num_rows = row + 1;
    
    return array;
}

void free_command_array(char ***array, int *row_sizes, int num_rows) {
    for (int i = 0; i < num_rows; i++) {
        int j = 0;
        while (array[i][j] != NULL) {
            free(array[i][j]);
            j++;
        }
        free(array[i]);
    }
    free(array);
    free(row_sizes);
}

void execute_parallel(char**words,int word_count,char paths[15][100],int pathSize){
    
    int new_word_count;
    char **new_words = split_up_and_check_redirection(words, word_count, &new_word_count);//account if > is in a word not seperated by spaces

    if(is_redirect_present(words,word_count) || is_redirect_present(new_words,new_word_count)){//checks if there is redirect first, if its right then do it else error
        if(is_redirect_second_last_and_unique(words,word_count)){//checks to see if > appears only once and second last
            execute_redirect(paths,words,word_count);
        }
        else if(is_redirect_second_last_and_unique(new_words,new_word_count)){
            execute_redirect(paths,new_words,new_word_count);
        }
        else{
            cdErrors();
        }
    }
    else if ((strcmp(words[0], "ls") == 0) && word_count==1) { //ls input
        basicLs(paths,words,word_count);
    }
    else if(strcmp(words[0], "exit") == 0){//if try to exit with arguements, throw error
        if(word_count==1){
            exit(0);
        }
        else{
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
    }
    else if((strcmp(words[0], "cd") == 0 && word_count==1) || (strcmp(words[0], "cd") == 0 && word_count>2)){//cd but not correct num of args
        cdErrors();
    }
    else if(strcmp(words[0], "cd") == 0 && word_count==2){//cd with correct num args
        if (chdir(words[1]) != 0) {
            cdErrors();
        }
    }
    else if((strcmp(words[0], "path") == 0) && word_count==1){//empty path so reset
        pathSize=0;
        for (int i = 0; i < 15; i++) {
            strcpy(paths[i], "");
        }
    }
    else if((strcmp(words[0], "path") == 0)){//build path if given path command
        for (int i = 1; i < word_count; i++) {
            strcpy(paths[pathSize], words[i]);
            pathSize+=1;
        }
    }
    else if(is_shell_script(words[0])){//if it is a shell script, add file to path and try run, if path doesnt exist, show error
        //printf("aaa\n");
        execute_file_in_new_process(paths, words);
    }
    else{
        execute_command(paths,words,word_count);
    }



    
}

void interactive(){
    char paths[15][100]={"","","","","","","","","","","","","","",""};
    int pathSize=0;

    pathSize=2;
    strcpy(paths[0], "/bin/");
    strcpy(paths[1], "/usr/bin/");

    while (1) {  // Infinite loop
        printf("witsshell> ");  // Custom prompt

        char *input = NULL;
        size_t len = 0;
        int word_count;
        char **words;
        

        getline(&input, &len, stdin);
        

        // Split the input into words
        words = split_words(input, &word_count);

        bool error_parallel=is_parallel_errors(words,word_count);

        int new_word_count;
        char **new_words = split_up_and_check_redirection(words, word_count, &new_word_count);

        new_words=split_up_ampersand(new_words,new_word_count,&new_word_count);
        /*for(int i=0;i<new_word_count;i++){
            printf("%s \n",new_words[i]);
        }*/
        

        bool parallel=is_parallel(new_words,new_word_count);

        if(error_parallel){
            //do nothing
        }
        else if(!error_parallel && parallel){
            //printf("gae");
            int num_rows;
            int *row_sizes;
            char ***command_array = create_command_array(new_words, new_word_count, &row_sizes, &num_rows);
            /*for(int i=0;i<num_rows;i++){
                for(int j=0;j<row_sizes[i];j++){
                    printf("%s ",command_array[i][j]);
                }
                printf("\n");
            }*/
            for(int i=0; i<num_rows; i++){
                pid_t pid = fork();  // Create a child process
                if(pid == 0) {  // Child process
                    execute_parallel(command_array[i], row_sizes[i], paths, pathSize);
                    exit(0);  // Exit the child process when done
                } else if(pid < 0) {  // Error in forking
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
            }

            // Wait for all child processes to finish
            for(int i=0; i<num_rows; i++){
                wait(NULL);
            }
            

            free_command_array(command_array, row_sizes, num_rows);
        }
        else if(is_redirect_present(words,word_count) || is_redirect_present(new_words,new_word_count)){//checks if there is redirect first, if its right then do it else error
            if(is_redirect_second_last_and_unique(words,word_count)){//checks to see if > appears only once and second last
                //printf("in the two\n");
                execute_redirect(paths,words,word_count);
            }
            else if(is_redirect_second_last_and_unique(new_words,new_word_count)){
                //printf("in the one\n");
                execute_redirect(paths,new_words,new_word_count);
            }
            else{
                cdErrors();
            }
        }
		else if ((strcmp(words[0], "ls") == 0) && word_count==1) { //ls input
            basicLs(paths,words,word_count);    
        }
        else if(strcmp(words[0], "exit") == 0){
            if(word_count==1){
                exit(0);
            }
            else{
                cdErrors();
            }
        }
        else if((strcmp(words[0], "cd") == 0 && word_count==1) || (strcmp(words[0], "cd") == 0 && word_count>2)){//cd but not correct num of args
            cdErrors();
        }
        else if(strcmp(words[0], "cd") == 0 && word_count==2){//cd with correct num args
            if (chdir(words[1]) != 0) {
                cdErrors();
            }
        }
        else if((strcmp(words[0], "path") == 0) && word_count==1){//empty path so reset
            pathSize=0;
            for (int i = 0; i < 15; i++) {
                strcpy(paths[i], "");
            }
        }
        else if((strcmp(words[0], "path") == 0)){//build path if given path command
            for (int i = 1; i < word_count; i++) {
                char *add_slash_to_end=add_trailing_slash(words[i]);
                strcpy(paths[pathSize], add_slash_to_end);
                pathSize+=1;
            }
        }
        else if(is_shell_script(words[0])){//if it is a shell script, add file to path and try run, if path doesnt exist, show error
            execute_file_in_new_process(paths, words);
        }
        else{
            execute_command(paths,words,word_count);
        }

        

        free(words);
        free(input);
        }
}

int batchMode(char *MainArgv[]){
        char paths[15][100]={"","","","","","","","","","","","","","",""};
        int pathSize=0;//keep track where to insert
        char *input = NULL;  // Pointer to hold the input
        size_t len = 0;     // Length of the buffer
        ssize_t nread;      // Number of characters read
        FILE *file;
        int word_count;
        char **words;

        //add /bin and /usr/bin
        pathSize=2;
        strcpy(paths[0], "/bin/");
        strcpy(paths[1], "/usr/bin/");


        //open file and check if empty
        file = fopen(MainArgv[1], "r");
        if (file == NULL) {
            cdErrors();
            return 1;
        }
        //read in each line
        while ((nread = getline(&input, &len, file)) != -1) {
            if (nread > 0 && input[nread - 1] == '\n') { //remove next line \n
                input[nread - 1] = '\0';
            }

            if (strlen(input) == 0) {//handles empty line
                continue;
            }

            words = split_words(input, &word_count);// words[0] will be cd,ls,..

            bool error_parallel=is_parallel_errors(words,word_count);
            

            
            int new_word_count;
            char **new_words = split_up_and_check_redirection(words, word_count, &new_word_count);//account if > is in a word not seperated by spaces

            new_words=split_up_ampersand(new_words,new_word_count,&new_word_count);
            

            bool parallel=is_parallel(new_words,new_word_count);

            if(error_parallel){
                //do nothing
            }
            else if(!error_parallel && parallel){//if paralle is fine create a 2d array of arguements
                //printf("gae");
                int num_rows;
                int *row_sizes;
                char ***command_array = create_command_array(new_words, new_word_count, &row_sizes, &num_rows);
                for(int i=0; i<num_rows; i++){
                    pid_t pid = fork();  // Create a child process
                    if(pid == 0) {  // Child process
                        execute_parallel(command_array[i], row_sizes[i], paths, pathSize);
                        exit(0);  // Exit the child process when done
                    } else if(pid < 0) {  // Error in forking
                        perror("fork");
                        exit(EXIT_FAILURE);
                    }
                }

                // Wait for all child processes to finish
                for(int i=0; i<num_rows; i++){
                    wait(NULL);
                }

                free_command_array(command_array, row_sizes, num_rows);
            }
            else if(is_redirect_present(words,word_count) || is_redirect_present(new_words,new_word_count)){//checks if there is redirect first, if its right then do it else error
                if(is_redirect_second_last_and_unique(words,word_count)){//checks to see if > appears only once and second last
                    execute_redirect(paths,words,word_count);
                }
                else if(is_redirect_second_last_and_unique(new_words,new_word_count)){
                    execute_redirect(paths,new_words,new_word_count);
                }
                else{
                    cdErrors();
                }
            }
            else if ((strcmp(words[0], "ls") == 0) && word_count==1) { //ls input
                basicLs(paths,words,word_count);
            }
            else if(strcmp(words[0], "exit") == 0){//if try to exit with arguements, throw error
                if(word_count==1){
                    exit(0);
                }
                else{
                    char error_message[30] = "An error has occurred\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
            }
            else if((strcmp(words[0], "cd") == 0 && word_count==1) || (strcmp(words[0], "cd") == 0 && word_count>2)){//cd but not correct num of args
                cdErrors();
            }
            else if(strcmp(words[0], "cd") == 0 && word_count==2){//cd with correct num args
                if (chdir(words[1]) != 0) {
                    cdErrors();
                }
            }
            else if((strcmp(words[0], "path") == 0) && word_count==1){//empty path so reset
                pathSize=0;
                for (int i = 0; i < 15; i++) {
                    strcpy(paths[i], "");
                }
            }
            else if((strcmp(words[0], "path") == 0)){//build path if given path command
                for (int i = 1; i < word_count; i++) {
                    char *add_slash_to_end=add_trailing_slash(words[i]);
                    strcpy(paths[pathSize], add_slash_to_end);
                    pathSize+=1;
                }
            }
            else if(is_shell_script(words[0])){//if it is a shell script, add file to path and try run, if path doesnt exist, show error
                execute_file_in_new_process(paths, words);
            }
            else{
                execute_command(paths,words,word_count);
            }    

        }

        fclose(file);
        free(words);
        free(input);
        return 0;
}

int main(int MainArgc, char *MainArgv[]){

    if(MainArgc==2){//batch mode
        int done=batchMode(MainArgv);
        if(done==1){
            return 1;
        }
    }
    else if(MainArgc>2){//too many arguements for batch mode
        cdErrors();
        return 1;
    }
    else{
        interactive();
    }
    return 0;
}
