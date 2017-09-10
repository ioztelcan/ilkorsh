#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define INPUT_BUFFER_MIN 64
#define INPUT_BUFFER_MAX 32768
#define TOKEN_BUFFER_MIN 64
#define TOKEN_BUFFER_MAX 1024

#define DEBUG 0

#if DEBUG
#define PRINTF(...)     printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static char *read_input(void)
{
    unsigned int input_cnt = 0;
    size_t input_buf_size = INPUT_BUFFER_MIN;
    char c = '\0';
    char *input = malloc(input_buf_size * sizeof(char));

    if (input == NULL) {
        printf("ERROR: Input buffer allocation failed! \n");
        exit(EXIT_FAILURE);
    }

    /* Read characters until newline. */
    while (1) {
        c = getchar();
        if (c == EOF || c == '\n') {
            input[input_cnt] = '\0';
            return input;
        }
        input[input_cnt] = c;
        input_cnt++;

        /* Double the input buffer size if input size exceeds the buffer size. */
        if (input_cnt == input_buf_size + 1) {
            input_buf_size = input_buf_size * 2;
            if (input_buf_size > INPUT_BUFFER_MAX) {
                printf("ERROR: Input size too big! \n");
                free(input);
                exit(EXIT_FAILURE);
            }
            input = realloc(input, input_buf_size * sizeof(char));
            if (input == NULL) {
                printf("ERROR: Input buffer re-allocation failed! \n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

char **parse_arguments(char *input) {
    unsigned int token_cnt = 0;
    size_t token_list_size = TOKEN_BUFFER_MIN;
    char *token = NULL;

    /* Fantastic explanation of why I failed while trying to allocate memory with size of char from stackoverlow:
     * ~~~~~~~~~~~~~~~~~~~~~
     * If you are trying to allocate space for an array of pointers, such as
     *
     * char** my_array_of_strings;  // or some array of pointers such as int** or even void**
     *
     * then you will need to consider word size (8 bits in a 64-bit system, 4 bytes in a 32-bit system) when allocating space for n pointers.
     * The size of a pointer is the same of your word size.
     *
     * So while you may wish to allocate space for n pointers, you are actually going to need n times 8 or 4 (for 64-bit or 32-bit systems, respectively)
     *
     * To avoid overflowing your allocated memory for n elements of 8 bytes:
     *      my_array_of_strings = (char**) malloc( n * 8 );  // for 64-bit systems
     *      my_array_of_strings = (char**) malloc( n * 4 );  // for 32-bit systems
     *
     * This will return a block of n pointers, each consisting of 8 bytes (or 4 bytes if you're using a 32-bit system)
     * I have noticed that Linux will allow you to use all n pointers when you haven't compensated for word size,
     * but when you try to free that memory it realizes its mistake and it gives out that rather nasty error.
     * And it is a bad one, when you overflow allocated memory, many security issues lie in wait.
     * ~~~~~~~~~~~~~~~~~~~~~
     */
    char **token_list = malloc(token_list_size * sizeof(token));

    if (token_list == NULL) {
        printf("ERROR: Input buffer allocation failed! \n");
        exit(EXIT_FAILURE);
    }

    /* strtok function splits a string using delimiters into smaller "tokens"
     * It works in 2 phases:
     *      - If the input string is not NULL, it splits the string using delimiters and returns
     *        the first substring.
     *      - If the input string is NULL, then it continues returning the tokenized substrings
     *        in each subsequent call.
     *
     *  Took me some time to find out :/ */
    token = strtok(input, " \n\r\t");
    while (token != NULL) {
        token_list[token_cnt] = token;
        token_cnt++;

        if (token_cnt == token_list_size + 1) {
            token_list_size = token_list_size * 2;
            if (token_list_size > TOKEN_BUFFER_MAX) {
                printf("ERROR: Max argument size exceeded! \n");
                free(token_list);
                exit(EXIT_FAILURE);
            }
            token_list = realloc(token_list, token_list_size * sizeof(token));
            if (token_list == NULL) {
                printf("ERROR: Argument buffer re-allocation failed! \n");
                exit(EXIT_FAILURE);
            }
        }
        /* Aforementioned subsequent calls. */
        token = strtok(NULL, " \n\r\t");
    }

    token_list[token_cnt] = NULL;
    return token_list;
}

void run_command(char **arguments)
{
    pid_t c_pid, wait_pid;
    int status, ret;

    c_pid = fork();
    /* If pid is 0, we are in child process. */
    if (c_pid == 0) {
        ret = execvp(arguments[0], arguments);
        if (ret == -1) {
            perror("run_command failed");
            exit(EXIT_FAILURE);
        }
        return;
    }
    /* A negative return value indicates something went wrong. */
    else if(c_pid < 0) {
        perror("Fork failed.");
        exit(EXIT_FAILURE);
    }
    /* Parent receives the process id of its child. */
    else {
        /* These macro explanations are from waitpid function explanation:
         * - WUNTRACED macro returns when the child has stopped but is not being traced.
         * - WCONTINUED also makes it return if the child continues after being stopped.
         *
         * The status of the child process is kept in status int, depending on it further
         * action can be taken. It also waits until the specified pid exits.*/
        do {
            wait_pid = waitpid(c_pid, &status, WUNTRACED);
            if (wait_pid == -1) {
                perror("Waitpid failed.");
                exit(EXIT_FAILURE);
            }
            /* Following macros are used to track the status of the process. */
            if (WIFEXITED(status)) {
                PRINTF("Child process exited with status %d. \n", WEXITSTATUS(status));
                return;
            }
            else if (WIFSIGNALED(status)) {
                PRINTF("Child process killed by signal %d. \n", WTERMSIG(status));
                return;
            }
            else if (WIFSTOPPED(status)) {
                PRINTF("Child process stopped by signal %d. \n", WSTOPSIG(status));
                return;
            }
        } while(WIFEXITED(status) || WIFSIGNALED(status));
        /* Continues until child process exits or signaled. */
    }

    return;
}

int main (void)
{
    char *command;
    char **argument_list;

    PRINTF("This is ilkorshell.\n");

    while (1) {
        /* The command prompt. */
        printf("ilkorsh>> ");
        command = read_input();
        argument_list = parse_arguments(command);
        PRINTF("Here is the command: %s \n", command);

        for (int i = 0; argument_list[i] != NULL; i++){
            PRINTF("Argument %d: %s \n", i, argument_list[i]);
        }
        run_command(argument_list);

        free(command);
        free(argument_list);
    }
}