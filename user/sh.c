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
// for dop
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

char* my_strstr(const char *haystack, const char *needle) {
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

int
strncmp(const char *s1, const char *s2, int n)
{
  for (int i = 0; i < n; i++) {
    if (s1[i] != s2[i] || s1[i] == 0 || s2[i] == 0)
      return (unsigned char)s1[i] - (unsigned char)s2[i];
  }
  return 0;
}


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


// Конвертирует int в строку, возвращает длину
// Конвертация числа в строку
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

void run_func(struct func *f, int argc, char **argv) {
    char expanded[MAXBODY];
    strcpy(expanded, f->body);

    // Подстановка $1..$9
    for (int i = 1; i < argc && i <= 9; i++) {
        char var[3] = { '$', '0'+i, 0 };
        char *pos = my_strstr(expanded, var);
        while (pos) {
            char tmp[MAXBODY];
            int prefix_len = pos - expanded;
            int var_len = 2;
            int arg_len = strlen(argv[i]);
            int suffix_len = strlen(pos + var_len);
            if (prefix_len + arg_len + suffix_len >= MAXBODY) break;
            memmove(tmp, expanded, prefix_len);
            memmove(tmp + prefix_len, argv[i], arg_len);
            memmove(tmp + prefix_len + arg_len, pos + var_len, suffix_len + 1);
            strcpy(expanded, tmp);
            pos = my_strstr(expanded, var);
        }
    }

    // Простая арифметика: число OP число
    int a = 0, b = 0;
    char op = 0;
    char *ptr = expanded;

    // Убираем пробелы
    char clean[MAXBODY];
    int j = 0;
    for (int i = 0; expanded[i]; i++) {
        if (expanded[i] != ' ')
            clean[j++] = expanded[i];
    }
    clean[j] = 0;
    ptr = clean;

    // Найдем оператор
    char *op_ptr = 0;
    for (int i = 0; ptr[i]; i++) {
        if (ptr[i] == '+' || ptr[i] == '-' || ptr[i] == '*' || ptr[i] == '/') {
            op = ptr[i];
            op_ptr = &ptr[i];
            break;
        }
    }

    if (op_ptr) {
        *op_ptr = 0;
        a = atoi(ptr);
        b = atoi(op_ptr + 1);
        int result = 0;
        switch(op) {
            case '+': result = a + b; break;
            case '-': result = a - b; break;
            case '*': result = a * b; break;
            case '/': result = (b != 0 ? a / b : 0); break;
        }
        char buf[32];
        int n = itoa(result, buf);
        write(1, buf, n);
        write(1, "\n", 1);
        return;
    }

    // Если арифметики нет — выводим как строку
    int len = strlen(expanded);
    if (len > 0) {
        write(1, expanded, len);
        write(1, "\n", 1);
        return;
    }

    // Иначе выполняем как команду
    struct cmd *c = parsecmd(expanded);
    runcmd(c);
}







int
main(void)
{
  static char buf[100];
  int fd;

  // Ensure that three file descriptors are open.
  while((fd = open("console", O_RDWR)) >= 0){
    if(fd >= 3){
      close(fd);
      break;
    }
  }

  while(getcmd(buf, sizeof(buf)) >= 0){
    if(buf[0] == 0)
      continue;

    if (strncmp(buf, "function ", 9) == 0) {
      char *p = buf + 9;
      char name[MAXNAME];
      int i = 0;

      while (*p && *p != ' ' && *p != '\t' && *p != '\n' && i < MAXNAME-1)
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

      char *start = strchr(p, '{');
      char *end   = strrchr(p, '}');
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

    char *argv[16];
    int argc = 0;
    char *token = strtok(buf, " \t\n");
    while (token && argc < 16) {
      argv[argc++] = token;
      token = strtok(0, " \t\n");
    }
    argv[argc] = 0;

    struct func *f = 0;
    if (argc > 0)
      f = find_func(argv[0]);

    if (f) {
      run_func(f, argc, argv);
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


