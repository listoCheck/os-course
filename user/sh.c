// Shell.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"
// Parsed command representation
#define EXEC  1
#define REDIR 2
#define PIPE  3
#define LIST  4
#define BACK  5

#define MAXARGS 10

struct cmd {
  int type;
};

struct execcmd {
  int type;
  char *argv[MAXARGS];
  char *eargv[MAXARGS];
};

struct redircmd {
  int type;
  struct cmd *cmd;
  char *file;
  char *efile;
  int mode;
  int fd;
};

#define MAXFUNCS 16
#define MAXNAME 32
#define MAXBODY 256

struct func {
    char name[MAXNAME];
    char args[MAXARGS][MAXNAME];
    int argc;
    char body[MAXBODY];
};
//
struct func funcs[MAXFUNCS];
int func_count = 0;

struct pipecmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct listcmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct backcmd {
  int type;
  struct cmd *cmd;
};

int fork1(void);  // Fork but panics on failure.
void panic(char*);
struct cmd *parsecmd(char*);
void runcmd(struct cmd*) __attribute__((noreturn));

// Execute cmd.  Never returns.
void
runcmd(struct cmd *cmd)
{
  int p[2];
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    exit(1);

  switch(cmd->type){
  default:
    panic("runcmd");

  case EXEC:
    ecmd = (struct execcmd*)cmd;
    if(ecmd->argv[0] == 0)
      exit(1);
    exec(ecmd->argv[0], ecmd->argv);
    fprintf(2, "exec %s failed\n", ecmd->argv[0]);
    break;

  case REDIR:
    rcmd = (struct redircmd*)cmd;
    close(rcmd->fd);
    if(open(rcmd->file, rcmd->mode) < 0){
      fprintf(2, "open %s failed\n", rcmd->file);
      exit(1);
    }
    runcmd(rcmd->cmd);
    break;

  case LIST:
    lcmd = (struct listcmd*)cmd;
    if(fork1() == 0)
      runcmd(lcmd->left);
    wait(0);
    runcmd(lcmd->right);
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;
    if(pipe(p) < 0)
      panic("pipe");
    if(fork1() == 0){
      close(1);
      dup(p[1]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->left);
    }
    if(fork1() == 0){
      close(0);
      dup(p[0]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->right);
    }
    close(p[0]);
    close(p[1]);
    wait(0);
    wait(0);
    break;

  case BACK:
    bcmd = (struct backcmd*)cmd;
    if(fork1() == 0)
      runcmd(bcmd->cmd);
    break;
  }
  exit(0);
}

int
getcmd(char *buf, int nbuf)
{
  write(2, "$ ", 2);
  memset(buf, 0, nbuf);
  gets(buf, nbuf);
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}
struct func* find_func(char *name) {
    for (int i = 0; i < func_count; i++) {
        if (strcmp(funcs[i].name, name) == 0)
            return &funcs[i];
    }
    return 0;
}

//поиск подстроки needle в строке haystack
char*
my_strstr(const char *haystack, const char *needle) {
    if (!*needle) return (char*)haystack;
    for (; *haystack; haystack++) {
        if (*haystack == *needle) {
            const char *h = haystack, *n = needle;
            while (*h && *n && *h == *n) {
                h++; n++;
            }
            if (!*n) return (char*)haystack;
        }
    }
    return 0;
}

//Сравнивает не более чем n символов двух строк s1 и s2.
int
strncmp(const char *s1, const char *s2, int n)
{
  for (int i = 0; i < n; i++) {
    if (s1[i] != s2[i] || s1[i] == 0 || s2[i] == 0)
      return (unsigned char)s1[i] - (unsigned char)s2[i];
  }
  return 0;
}

//ищет последнее вхождение символа c в строке s
char*
strrchr(const char *s, int c)
{
  const char *last = 0;
  for (; *s; s++) {
    if (*s == (char)c)
      last = s;
  }
  return (char*)last;
}

//азделяет строку на токены
char*
strtok(char *str, const char *delim)
{
  static char *last;
  if (str)
    last = str;
  if (!last)
    return 0;

  while (*last && strchr(delim, *last))
    last++;
  if (!*last)
    return 0;

  char *token = last;
  while (*last && !strchr(delim, *last))
    last++;

  if (*last) {
    *last = '\0';
    last++;
  } else {
    last = 0;
  }

  return token;
}


int itoa(int val, char *buf) {
    char tmp[16];
    int i = 0, j, neg = 0;
    if (val < 0) {
        neg = 1;
        val = -val;
    }
    do {
        tmp[i++] = '0' + (val % 10);
        val /= 10;
    } while (val > 0);

    int len = 0;
    if (neg) buf[len++] = '-';
    for (j = i-1; j >= 0; j--)
        buf[len++] = tmp[j];
    buf[len] = 0;
    return len;
}
static char *trim(char *s) {
    if (!s) return s;
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') s++;
    char *end = s + strlen(s) - 1;
    while (end >= s && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
        *end = 0;
        end--;
    }
    return s;
}
static int startswith(const char *s, const char *pref) {
    while (*pref) {
        if (*s++ != *pref++) return 0;
    }
    return 1;
}

static int is_arith_string(const char *s) {
    if (!s) return 0;
    int found_digit = 0;
    while (*s) {
        char c = *s++;
        if (c == ' ' || c == '\t') continue;
        if ((c >= '0' && c <= '9') || c=='+' || c=='-' || c=='*' || c=='/') {
            if (c >= '0' && c <= '9') found_digit = 1;
            continue;
        }
        return 0;
    }
    return found_digit;
}

static int eval_arith(const char *s, int *out) {
    if (!s) return 0;
    char buf[MAXBODY];
    int bi = 0;
    for (const char *p = s; *p && bi < (int)sizeof(buf)-1; p++) {
        if (*p != ' ' && *p != '\t') buf[bi++] = *p;
    }
    buf[bi] = 0;
    if (bi == 0) return 0;

    int isdigit_local(char c) { return c >= '0' && c <= '9'; }

    const char *p = buf;
    int first = 1;
    int result = 0;
    char op = 0;
    int parse_error = 0;

    while (*p && !parse_error) {
        int neg = 0;
        if (first) {
            if (*p == '+') p++;
            else if (*p == '-') { neg = 1; p++; }
        }

        if (!isdigit_local(*p)) { parse_error = 1; break; }

        int val = 0;
        while (*p && isdigit_local(*p)) {
            val = val * 10 + (*p - '0');
            p++;
        }
        if (neg) val = -val;

        if (first) {
            result = val;
            first = 0;
        } else {
            switch (op) {
                case '+': result = result + val; break;
                case '-': result = result - val; break;
                case '*': result = result * val; break;
                case '/':
                    if (val == 0) { parse_error = 1; break; }
                    result = result / val;
                    break;
                default: parse_error = 1; break;
            }
            if (parse_error) break;
        }

        if (!*p) break;

        if (*p == '+' || *p == '-' || *p == '*' || *p == '/') {
            op = *p;
            p++;
            if (!*p) { parse_error = 1; break; }
        } else {
            parse_error = 1;
            break;
        }
    }

    if (!parse_error && !first && *p == 0) {
        *out = result;
        return 1;
    }
    return 0;
}

char* strncpy(char *dst, const char *src, int n) {
    int i;
    for (i = 0; i < n && src[i]; i++) dst[i] = src[i];
    for (; i < n; i++) dst[i] = 0;
    return dst;
}
char* my_strncat(char *dst, const char *src, int n) {
    char *p = dst;
    while (*p) p++;
    int i;
    for (i = 0; i < n && src[i]; i++) *p++ = src[i];
    *p = 0;
    return dst;
}

void run_func(struct func *f, int argc, char **argv, int capture) {
    char expanded[MAXBODY];
    strcpy(expanded, f->body);

    for (int i = 1; i < argc && i <= 9; i++) {
        char var[3] = { '$', '0'+i, 0 };
        char *pos = my_strstr(expanded, var);
        while (pos) {
            char tmp[MAXBODY];
            int prefix_len = pos - expanded;
            int var_len = 2;
            char *arg = (i < argc) ? argv[i] : "";
            int arg_len = strlen(arg);
            int suffix_len = strlen(pos + var_len);
            if (prefix_len + arg_len + suffix_len >= MAXBODY) break;
            memmove(tmp, expanded, prefix_len);
            memmove(tmp + prefix_len, arg, arg_len);
            memmove(tmp + prefix_len + arg_len, pos + var_len, suffix_len + 1);
            strcpy(expanded, tmp);
            pos = my_strstr(expanded, var);
        }
    }

    int body_has_echo = (my_strstr(expanded, "echo") != 0);

    int pipefd[2];
    if (capture) {
        if (pipe(pipefd) < 0) {
            fprintf(2, "pipe failed\n");
            return;
        }
    }

    int pid = fork();
    if (pid < 0) {
        fprintf(2, "fork failed in function\n");
        if (capture) { close(pipefd[0]); close(pipefd[1]); }
        return;
    }

    if (pid == 0) {
        if (capture) {
            close(pipefd[0]);
            close(1);
            dup(pipefd[1]);
            close(pipefd[1]);
        }
        char *line = expanded;
        while (*line) {
            char *nl = strchr(line, '\n');
            char save = 0;
            if (nl) { save = *nl; *nl = 0; }

            char *trimmed = trim(line);
            if (trimmed && *trimmed) {
                if (startswith(trimmed, "echo") && (trimmed[4] == ' ' || trimmed[4] == '\t' || trimmed[4] == 0)) {
                    char *args = trimmed + 4;
                    args = trim(args);
                    if (!args) args = "";
                    int val;
                    if (is_arith_string(args) && eval_arith(args, &val)) {
                        char buf[64];
                        int n = itoa(val, buf);
                        write(1, buf, n);
                        write(1, "\n", 1);
                    } else {
                        int len = strlen(args);
                        if (len > 0) write(1, args, len);
                        write(1, "\n", 1);
                    }
                } else if (is_arith_string(trimmed)) {
                    if (capture || body_has_echo) {
                      int val;
                      if (eval_arith(trimmed, &val)) {
                        char buf[64];
                        int n = itoa(val, buf);
                        write(1, buf, n);
                        write(1, "\n", 1);
                      } else {
                        int len = strlen(trimmed);
                        write(1, trimmed, len);
                        write(1, "\n", 1);
                      }
                  }
                } else {
                    if (!capture && !body_has_echo) {
                        int fd = open(".__func_tmp_out", O_WRONLY|O_CREATE|O_TRUNC);
                        if (fd >= 0) {
                            close(1);
                            dup(fd);
                            close(2);
                            dup(fd);
                        }
                        int pid2 = fork();
                        if (pid2 < 0) {
                            fprintf(2, "fork failed in func\n");
                        } else if (pid2 == 0) {
                            struct cmd *c = parsecmd(trimmed);
                            runcmd(c);
                        } else {
                            wait(0);
                        }
                    } else {
                        int pid2 = fork();
                        if (pid2 < 0) {
                            fprintf(2, "fork failed in func\n");
                        } else if (pid2 == 0) {
                            struct cmd *c = parsecmd(trimmed);
                            runcmd(c);
                        } else {
                            wait(0);
                        }
                    }
                }
            }

            if (nl) {
                *nl = save;
                line = nl + 1;
            } else break;
        }
        exit(0);
    } else {
        // parent
        if (capture) {
            close(pipefd[1]);
            char buf[256];
            int n;
            while ((n = read(pipefd[0], buf, sizeof(buf))) > 0) {
                write(1, buf, n);
            }
            close(pipefd[0]);
        }
        wait(0);
    }
}


int
main(void)
{
  static char buf[2048];
  int fd;

  while((fd = open("console", O_RDWR)) >= 0){
    if(fd >= 3){
      close(fd);
      break;
    }
  }

  while(getcmd(buf, sizeof(buf)) >= 0){
    if(buf[0] == 0)
      continue;

    char *bptr = buf;
    while (*bptr == ' ' || *bptr == '\t') bptr++;

    if (strncmp(bptr, "function ", 9) == 0) {
      char localbuf[MAXBODY];
      memset(localbuf, 0, sizeof(localbuf));

      strncpy(localbuf, bptr + 9, sizeof(localbuf)-1);

      char *start = strchr(localbuf, '{');
      char *end = strrchr(localbuf, '}');

      if (!start) {
        printf("syntax error in function definition: missing '{'\n");
        continue;
      }
      while (!end) {
        char more[512];
        if (getcmd(more, sizeof(more)) < 0) break;
        int cur = strlen(localbuf);
        int add = strlen(more);
        if (cur + add + 2 >= (int)sizeof(localbuf)) break;
        if (add > 0 && more[add-1] == '\n') {
            more[add-1] = '\0';
            add--;
        }
        localbuf[cur] = '\n';
        localbuf[cur+1] = '\0';
        my_strncat(localbuf, more, add);
        start = strchr(localbuf, '{');
        end = strrchr(localbuf, '}');
      }

      char *p = localbuf;
      char name[MAXNAME];
      int i = 0;

      while (*p && *p == ' ') p++;
      while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '(' && i < MAXNAME-1)
        name[i++] = *p++;
      name[i] = '\0';

      while (*p == ' ') p++;
      int argc = 0;
      char args[MAXARGS][MAXNAME];
      if (*p == '(') {
        p++;
        while (*p && *p != ')') {
          while (*p == ' ' || *p == ',') p++;
          if (*p == ')') break;
          i = 0;
          while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != ',' && *p != ')' && i < MAXNAME-1)
            args[argc][i++] = *p++;
          args[argc][i] = '\0';
          argc++;
          while (*p == ' ' || *p == ',') p++;
        }
        if (*p == ')') p++;
      }
      start = strchr(localbuf, '{');
      end   = strrchr(localbuf, '}');
      if (!start || !end || end <= start) {
        printf("syntax error in function definition\n");
        continue;
      }

      char body[MAXBODY];
      int len = end - start - 1;
      if (len >= MAXBODY) len = MAXBODY - 1;
      for (i = 0; i < len; i++)
        body[i] = start[i+1];
      body[len] = '\0';

      if (func_count < MAXFUNCS) {
        struct func *f = &funcs[func_count++];
        strcpy(f->name, name);
        strcpy(f->body, body);
        f->argc = argc;
        for (int j = 0; j < argc; j++)
          strcpy(f->args[j], args[j]);
        printf("Function '%s' defined with %d args.\n", name, argc);
      } else {
        printf("Too many functions defined.\n");
      }

      continue;
    }

    // Tokenize input into argv as usual
    char *argv[16];
    int argc = 0;
    char *token = strtok(buf, " \t\n");
    while (token && argc < 16) {
      argv[argc++] = token;
      token = strtok(0, " \t\n");
    }
    argv[argc] = 0;
    if (argc > 0 && strcmp(argv[0], "echo") == 0) {
      if (argc >= 2) {
        struct func *ff = find_func(argv[1]);
        if (ff) {
          run_func(ff, argc - 1, &argv[1], 1);
          continue;
        }
      }

      if (argc == 2 && is_arith_string(argv[1])) {
        int val;
        if (eval_arith(argv[1], &val)) {
          char bufv[64];
          int n = itoa(val, bufv);
          write(1, bufv, n);
          write(1, "\n", 1);
          continue;
        }
      }
      for (int i = 1; i < argc; i++) {
        if (i > 1) write(1, " ", 1);
        write(1, argv[i], strlen(argv[i]));
      }
      write(1, "\n", 1);
      continue;
    }


    struct func *f = 0;
    if (argc > 0)
      f = find_func(argv[0]);

    if (f) {
      run_func(f, argc, argv, 0);
      continue;
    }
    
    if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
      // Chdir must be called by the parent, not the child.
      buf[strlen(buf)-1] = 0;  // chop \n
      if(chdir(buf+3) < 0)
        fprintf(2, "cannot cd %s\n", buf+3);
      continue;
    }

    if(fork1() == 0)
      runcmd(parsecmd(buf));
    wait(0);
  }
  exit(0);
}


void
panic(char *s)
{
  fprintf(2, "%s\n", s);
  exit(1);
}

int
fork1(void)
{
  int pid;

  pid = fork();
  if(pid == -1)
    panic("fork");
  return pid;
}

//PAGEBREAK!
// Constructors

struct cmd*
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = EXEC;
  return (struct cmd*)cmd;
}

struct cmd*
redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = REDIR;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->efile = efile;
  cmd->mode = mode;
  cmd->fd = fd;
  return (struct cmd*)cmd;
}

struct cmd*
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = PIPE;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
listcmd(struct cmd *left, struct cmd *right)
{
  struct listcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = LIST;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
backcmd(struct cmd *subcmd)
{
  struct backcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = BACK;
  cmd->cmd = subcmd;
  return (struct cmd*)cmd;
}
//PAGEBREAK!
// Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>&;()";

int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;

  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  if(q)
    *q = s;
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
  case '(':
  case ')':
  case ';':
  case '&':
  case '<':
    s++;
    break;
  case '>':
    s++;
    if(*s == '>'){
      ret = '+';
      s++;
    }
    break;
  default:
    ret = 'a';
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if(eq)
    *eq = s;

  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

int
peek(char **ps, char *es, char *toks)
{
  char *s;

  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);
struct cmd *nulterminate(struct cmd*);

struct cmd*
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if(s != es){
    fprintf(2, "leftovers: %s\n", s);
    panic("syntax");
  }
  nulterminate(cmd);
  return cmd;
}

struct cmd*
parseline(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parsepipe(ps, es);
  while(peek(ps, es, "&")){
    gettoken(ps, es, 0, 0);
    cmd = backcmd(cmd);
  }
  if(peek(ps, es, ";")){
    gettoken(ps, es, 0, 0);
    cmd = listcmd(cmd, parseline(ps, es));
  }
  return cmd;
}

struct cmd*
parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if(peek(ps, es, "|")){
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd*
parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while(peek(ps, es, "<>")){
    tok = gettoken(ps, es, 0, 0);
    if(gettoken(ps, es, &q, &eq) != 'a')
      panic("missing file for redirection");
    switch(tok){
    case '<':
      cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
      break;
    case '>':
      cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE|O_TRUNC, 1);
      break;
    case '+':  // >>
      cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
      break;
    }
  }
  return cmd;
}

struct cmd*
parseblock(char **ps, char *es)
{
  struct cmd *cmd;

  if(!peek(ps, es, "("))
    panic("parseblock");
  gettoken(ps, es, 0, 0);
  cmd = parseline(ps, es);
  if(!peek(ps, es, ")"))
    panic("syntax - missing )");
  gettoken(ps, es, 0, 0);
  cmd = parseredirs(cmd, ps, es);
  return cmd;
}

struct cmd*
parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;

  if(peek(ps, es, "("))
    return parseblock(ps, es);

  ret = execcmd();
  cmd = (struct execcmd*)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while(!peek(ps, es, "|)&;")){
    if((tok=gettoken(ps, es, &q, &eq)) == 0)
      break;
    if(tok != 'a')
      panic("syntax");
    cmd->argv[argc] = q;
    cmd->eargv[argc] = eq;
    argc++;
    if(argc >= MAXARGS)
      panic("too many args");
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  cmd->eargv[argc] = 0;
  return ret;
}

// NUL-terminate all the counted strings.
struct cmd*
nulterminate(struct cmd *cmd)
{
  int i;
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    return 0;

  switch(cmd->type){
  case EXEC:
    ecmd = (struct execcmd*)cmd;
    for(i=0; ecmd->argv[i]; i++)
      *ecmd->eargv[i] = 0;
    break;

  case REDIR:
    rcmd = (struct redircmd*)cmd;
    nulterminate(rcmd->cmd);
    *rcmd->efile = 0;
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;
    nulterminate(pcmd->left);
    nulterminate(pcmd->right);
    break;

  case LIST:
    lcmd = (struct listcmd*)cmd;
    nulterminate(lcmd->left);
    nulterminate(lcmd->right);
    break;

  case BACK:
    bcmd = (struct backcmd*)cmd;
    nulterminate(bcmd->cmd);
    break;
  }
  return cmd;
}

