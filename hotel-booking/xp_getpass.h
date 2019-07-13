/**
 * Francesco Urbani
 * Thu Jul 11 08:26:14 CEST 2019
 *
 *
 * `getpass()` is deprecated on Linux, hence this header file.
 *          << This function is obsolete.  Do not use it.  If you want to read input
 *             without terminal echoing enabled, see the description of the ECHO
 *             flag in termios(3). >>
 */



#ifndef XP_GETPASS  // XP for cross-platform, btw
#define XP_GETPASS


/* Use this variable to remember original terminal attributes. */
struct termios saved_attributes;

void
reset_input_mode(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
}


void
set_input_mode(const char* prompt)
{
    struct termios tattr;
    // char* name;

    /* Make sure stdin is a terminal. */
    if (!isatty(STDIN_FILENO)){
        fprintf(stderr, "Not a terminal.\n");
        exit(EXIT_FAILURE);
    }

    /* Save the terminal attributes so we can restore them later. */
    tcgetattr(STDIN_FILENO, &saved_attributes);
    
    #if 1 // __APPLE__ doesn't care but Ubuntu does. 
    atexit(reset_input_mode);
    #endif

    
    /* Set the funny terminal modes. */
    tcgetattr(STDIN_FILENO, &tattr);
    printf("%s", prompt);
    fflush(stdout);
    tattr.c_lflag &= ~(ICANON | ECHO);    /* Clear ICANON and ECHO. */
    tattr.c_cc[VMIN] = 1;
    tattr.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);
}


char*
term_getpass(const char* prompt)
{
    static char pass[100];
    char c;

    char asterisk = '*';
    // char leerzeichen = ' ';

    set_input_mode(prompt);

    int i = 0;
    while (read (STDIN_FILENO, &c, 1) && (isalnum (c) || ispunct (c)) && i < sizeof (pass) - 2){
        pass[i++] = c;
        write (STDOUT_FILENO, &asterisk, 1);
    }

    pass[i] = '\0';

    printf("\n");
    return pass;

}


static inline char* 
xp_getpass(const char* prompt)
{
    #ifndef __APPLE__
        return getpass(prompt);
    #else

        // seems like getpass works fine on Linux too
        #if 0
            return term_getpass(prompt);
        #else
            return getpass(prompt);
        #endif

    #endif
}


#endif
