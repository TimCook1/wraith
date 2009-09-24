
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

static char lower_resps[1024] = "";

char *strtoupper(char *string)
{
  static char ret[51] = "";
  int i = 0;

  while (*string) {
    ret[i] = toupper(*string);
    i++;
    string++;
    ret[i] = 0;
  }
  return ret;
}

char *replace(char *string, char *oldie, char *newbie)
{
  static char newstring[1024] = "";
  int str_index, newstr_index, oldie_index, end, new_len, old_len, cpy_len;
  char *c = NULL;

  if (string == NULL) return "";
  if ((c = (char *) strstr(string, oldie)) == NULL) return string;
  new_len = strlen(newbie);
  old_len = strlen(oldie);
  end = strlen(string) - old_len;
  oldie_index = c - string;
  newstr_index = 0;
  str_index = 0;
  while(str_index <= end && c != NULL) {
    cpy_len = oldie_index-str_index;
    strncpy(newstring + newstr_index, string + str_index, cpy_len);
    newstr_index += cpy_len;
    str_index += cpy_len;
    strcpy(newstring + newstr_index, newbie);
    newstr_index += new_len;
    str_index += old_len;
    if((c = (char *) strstr(string + str_index, oldie)) != NULL)
     oldie_index = c - string;
  }
  strcpy(newstring + newstr_index, string + str_index);
  return (newstring);
}


char *step_thru_file(FILE *fd)
{
  char tempBuf[1024] = "";
  char *retStr = NULL;

  if (fd == NULL) {
    return NULL;
  }
  retStr = NULL;
  while (!feof(fd)) {
    if (fgets(tempBuf, sizeof(tempBuf), fd) && !feof(fd)) {
      if (retStr == NULL) {
        retStr = (char *) calloc(1, strlen(tempBuf) + 2);
        strcpy(retStr, tempBuf);
      } else {
        retStr = (char *) realloc(retStr, strlen(retStr) + strlen(tempBuf));
        strcat(retStr, tempBuf);
      }
      if (retStr[strlen(retStr)-1] == '\n') {
        retStr[strlen(retStr)-1] = 0;
        break;
      }
    }
  }
  return retStr;
}

char *newsplit(char **rest)
{
  register char *o, *r;

  if (!rest)
    return *rest = "";
  o = *rest;
  while (*o == ' ')
    o++;
  r = o;
  while (*o && (*o != ' '))
    o++;
  if (*o)
    *o++ = 0;
  *rest = o;
  return r;
}

int skipline (char *line, int *skip) {
  static int multi = 0;
  if ((!strncmp(line, "//", 2))) {
    (*skip)++;
  } else if ( (strstr(line, "/*")) && (strstr(line, "*/")) ) {
    multi = 0;
    (*skip)++;
  } else if ( (strstr(line, "/*")) ) {
    (*skip)++;
    multi = 1;
  } else if ( (strstr(line, "*/")) ) {
    multi = 0;
  } else {
    if (!multi) (*skip) = 0;
  }
  return (*skip);
}

int parse_res(char *in, char *out, char *outs) {
  FILE *inf = NULL, *outf = NULL, *outsf = NULL;
  char *buffer = NULL, *cmd = NULL;
  int skip = 0, line = 0, total_responses = 0;

  if (!(inf = fopen(in, "r"))) {
    printf("Error: Cannot open '%s' for reading\n", in);
    return 1;
  }
  if (!(outf = fopen(out, "w"))) {
    printf("Error: Cannot open '%s' for writing\n", out);
    return 1;
  }
  if (!(outsf = fopen(outs, "w"))) {
    printf("Error: Cannot open '%s' for writing\n", outs);
    return 1;
  }
  printf("Parsing res file '%s'", in);

  fprintf(outf, "/* DO NOT EDIT THIS FILE. */\n\
#ifndef _RESPONSE_H\n\
#define _RESPONSE_H\n\
\n\
typedef unsigned int response_t;\n\n\
enum {\n");

  fprintf(outsf, "/* DO NOT EDIT THIS FILE. */\n\
#ifndef _RESPONSES_H\n\
#define _RESPONSES_H\n\
\n\
typedef const char * res_t;\n\n");

  while ((!feof(inf)) && ((buffer = step_thru_file(inf)) != NULL) ) {
    line++;
    if ((*buffer)) {
      if (strchr(buffer, '\n')) *(char*)strchr(buffer, '\n') = 0;
      if ((skipline(buffer, &skip))) continue;
      if (buffer[0] == ':') { /* New cmd */
        char *p = NULL;

        if (cmd && cmd[0]) {		/* CLOSE LAST RES */
          fprintf(outf, ",\n");	/* for enum */
          fprintf(outsf, "\tNULL\n};\n\n");
          free(cmd);
        }
        p = strchr(buffer, ':');
        p++;
        if (strcmp(p, "end")) {		/* NEXT RES */
          cmd = (char *) calloc(1, strlen(p) + 1);

          total_responses++;
          strcpy(cmd, p);
          printf(".");

          fprintf(outf, "\tRES_%s", strtoupper(cmd));
          if (total_responses == 1)
            fprintf(outf, " = 1");
          fprintf(outsf, "static res_t res_%s[] = {\n", cmd);
          sprintf(lower_resps, "%s,\n\tres_%s", lower_resps, cmd);
        } else {			/* END */
          fprintf(outf, "\tRES_END\n};\n\n#define RES_TYPES %d\n", total_responses);
          fprintf(outf, "const char *response(response_t);\nvoid init_responses();\nconst char *r_banned(struct chanset_t* chan);\n\n#endif /* !_RESPONSE_H */\n");
          fprintf(outsf, "static res_t *res[] = {\n\tNULL%s\n};\n#endif /* !_RESPONSES_H */\n", lower_resps);
        }
      } else {				/* NEXT RES TEXT */
        buffer++;
        fprintf(outsf, "\t\"%s\",\n", buffer);
/*        fprintf(outf, "%s\\n", replace(buffer, "\"", "\\\"")); */
      }
    }
    buffer = NULL;
  }
  printf(" Success\n");
  if (inf) fclose(inf);
  if (outf) fclose(outf);
  if (outsf) fclose(outsf);
  return 0;
}

int main(int argc, char **argv) {
  char *in = NULL, out[1024] = "", outs[1024] = "";
  int ret = 0;

  if (argc < 3) return 1;
  in = (char *) calloc(1, strlen(argv[1]) + 1);
  strcpy(in, argv[1]);
  sprintf(out, "%s/response.h%s", argv[2], argc == 4 ? "~" : "");
  sprintf(outs, "%s/responses.h%s", argv[2], argc == 4 ? "~" : "");
  ret = parse_res(in, out, outs);
  free(in);
  return ret;
}
