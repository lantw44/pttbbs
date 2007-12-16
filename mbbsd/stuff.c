/* $Id$ */
#include "bbs.h"

/* ----------------------------------------------------- */
/* set file path for boards/user home                    */
/* ----------------------------------------------------- */
static const char * const str_home_file = "home/%c/%s/%s";
static const char * const str_board_file = "boards/%c/%s/%s";
static const char * const str_board_n_file = "boards/%c/%s/%s.%d";


static const char * const str_dotdir = FN_DIR;

/* XXX set*() all assume buffer size = PATHLEN */
void
sethomepath(char *buf, const char *userid)
{
    assert(is_validuserid(userid));
    snprintf(buf, PATHLEN, "home/%c/%s", userid[0], userid);
}

void
sethomedir(char *buf, const char *userid)
{
    assert(is_validuserid(userid));
    snprintf(buf, PATHLEN, str_home_file, userid[0], userid, str_dotdir);
}

void
sethomeman(char *buf, const char *userid)
{
    assert(is_validuserid(userid));
    snprintf(buf, PATHLEN, str_home_file, userid[0], userid, "man");
}


void
sethomefile(char *buf, const char *userid, const char *fname)
{
    assert(is_validuserid(userid));
    assert(fname[0]);
    snprintf(buf, PATHLEN, str_home_file, userid[0], userid, fname);
}

void
setuserfile(char *buf, const char *fname)
{
    assert(is_validuserid(cuser.userid));
    assert(fname[0]);
    snprintf(buf, PATHLEN, str_home_file, cuser.userid[0], cuser.userid, fname);
}

void
setapath(char *buf, const char *boardname)
{
    //assert(boardname[0]);
    snprintf(buf, PATHLEN, "man/boards/%c/%s", boardname[0], boardname);
}

void
setadir(char *buf, const char *path)
{
    //assert(path[0]);
    snprintf(buf, PATHLEN, "%s/%s", path, str_dotdir);
}

void
setbpath(char *buf, const char *boardname)
{
    //assert(boardname[0]);
    snprintf(buf, PATHLEN, "boards/%c/%s", boardname[0], boardname);
}

void
setbdir(char *buf, const char *boardname)
{
    //assert(boardname[0]);
    snprintf(buf, PATHLEN, str_board_file, boardname[0], boardname,
	    (currmode & MODE_DIGEST ? fn_mandex : str_dotdir));
}

void
setbfile(char *buf, const char *boardname, const char *fname)
{
    //assert(boardname[0]);
    assert(fname[0]);
    snprintf(buf, PATHLEN, str_board_file, boardname[0], boardname, fname);
}

void
setbnfile(char *buf, const char *boardname, const char *fname, int n)
{
    //assert(boardname[0]);
    assert(fname[0]);
    snprintf(buf, PATHLEN, str_board_n_file, boardname[0], boardname, fname, n);
}

/*
 * input	direct
 * output	buf: copy direct
 * 		fname: direct ���ɦW����
 */
void
setdirpath(char *buf, const char *direct, const char *fname)
{
    char *p;
    strcpy(buf, direct);
    p = strrchr(buf, '/');
    assert(p);
    strlcpy(p + 1, fname, PATHLEN-(p+1-buf));
}

/**
 * ���w�峹���D title�A�Ǧ^����D�D�����������СC
 * @param title
 */
char           *
subject(char *title)
{
    if (!strncasecmp(title, str_reply, 3)) {
	title += 3;
	if (*title == ' ')
	    title++;
    }
    return title;
}

int is_validuserid(const char *id)
{
    int len, i;
    if(id==NULL)
	return 0;
    len = strlen(id);

    if (len < 2 || len>IDLEN)
	return 0;

    if (!isalpha(id[0]))
	return 0;
    for (i = 1; i < len; i++)
	if (!isalnum(id[i]))
	    return 0;
    return 1;
}

int
is_uBM(const char *list, const char *id)
{
    register int    len;

    if (list[0] == '[')
	list++;
    if (list[0] > ' ') {
	len = strlen(id);
	do {
	    if (!strncasecmp(list, id, len)) {
		list += len;
		if ((*list == 0) || (*list == '/') ||
		    (*list == ']') || (*list == ' '))
		    return 1;
	    }
	    if ((list = strchr(list, '/')) != NULL)
		list++;
	    else
		break;
	} while (1);
    }
    return 0;
}

int
is_BM(const char *list)
{
    if (is_uBM(list, cuser.userid)) {
	cuser.userlevel |= PERM_BM;	/* Ptt �۰ʥ[�WBM���v�Q */
	return 1;
    }
    return 0;
}

int
userid_is_BM(const char *userid, const char *list)
{
    register int    ch, len;

    // TODO merge with is_uBM
    ch = list[0];
    if ((ch > ' ') && (ch < 128)) {
	len = strlen(userid);
	do {
	    if (!strncasecmp(list, userid, len)) {
		ch = list[len];
		if ((ch == 0) || (ch == '/') || (ch == ']'))
		    return 1;
	    }
	    while ((ch = *list++)) {
		if (ch == '/')
		    break;
	    }
	} while (ch);
    }
    return 0;
}

int
belong(const char *filelist, const char *key)
{
    return file_exist_record(filelist, key);
}


#ifndef _BBS_UTIL_C_ /* getdata_buf */
time4_t
gettime(int line, time4_t dt, const char*head)
{
    char            yn[7];
    int i;
    struct tm      *ptime = localtime4(&dt), endtime;

    memcpy(&endtime, ptime, sizeof(struct tm));
    snprintf(yn, sizeof(yn), "%4d", ptime->tm_year + 1900);
    move(line, 0); outs(head);
    i=strlen(head);
    do {
	getdata_buf(line, i, " �褸�~:", yn, 5, LCECHO);
    } while ((endtime.tm_year = atoi(yn) - 1900) < 0 || endtime.tm_year > 200);
    snprintf(yn, sizeof(yn), "%d", ptime->tm_mon + 1);
    do {
	getdata_buf(line, i+15, "��:", yn, 3, LCECHO);
    } while ((endtime.tm_mon = atoi(yn) - 1) < 0 || endtime.tm_mon > 11);
    snprintf(yn, sizeof(yn), "%d", ptime->tm_mday);
    do {
	getdata_buf(line, i+24, "��:", yn, 3, LCECHO);
    } while ((endtime.tm_mday = atoi(yn)) < 1 || endtime.tm_mday > 31);
    snprintf(yn, sizeof(yn), "%d", ptime->tm_hour);
    do {
	getdata_buf(line, i+33, "��(0-23):", yn, 3, LCECHO);
    } while ((endtime.tm_hour = atoi(yn)) < 0 || endtime.tm_hour > 23);
    return mktime(&endtime);
}

// synchronize 'now'
void syncnow(void)
{
#ifdef OUTTA_TIMER
        now = SHM->GV2.e.now;
#else
	now = time(0);
#endif
}

#endif


#ifndef _BBS_UTIL_C_
/* �o�@�ϳ��O������e���B�z��, �G _BBS_UTIL_C_ �����n */
static void
capture_screen(void)
{
    char            fname[200];
    FILE           *fp;
    int             i;

    getdata(b_lines - 2, 0, "��o�ӵe�����J��Ȧs�ɡH[y/N] ",
	    fname, 4, LCECHO);
    if (fname[0] != 'y')
	return;

    setuserfile(fname, ask_tmpbuf(b_lines - 1));
    if ((fp = fopen(fname, "w"))) {
	for (i = 0; i < scr_lns; i++)
	    fprintf(fp, "%.*s\n", big_picture[i].len, big_picture[i].data);
	fclose(fp);
    }
}

#ifdef PLAY_ANGEL
void
pressanykey_or_callangel(){
    int             ch;

    outmsg(
	    ANSI_COLOR(1;34;44) " �e�e�e�e " 
	    ANSI_COLOR(32) "H " ANSI_COLOR(36) "�I�s�p�Ѩ�" ANSI_COLOR(34) 
	    " �e�e�e�e" ANSI_COLOR(37;44) " �Ы� " ANSI_COLOR(36) "���N�� " 
	    ANSI_COLOR(37) "�~�� " ANSI_COLOR(1;34) 
	    "�e�e�e�e�e" ANSI_COLOR(36) "^T �����Ȧs��" ANSI_COLOR(34) "�e�e�e " ANSI_RESET);
    do {
	ch = igetch();

	if (ch == Ctrl('T')) {
	    capture_screen();
	    break;
	}else if (ch == 'h' || ch == 'H'){
	    CallAngel();
	    break;
	}
    } while ((ch != ' ') && (ch != KEY_LEFT) && (ch != '\r') && (ch != '\n'));
    move(b_lines, 0);
    clrtoeol();
    refresh();
}
#endif

/**
 * �� printf format ���ѼơA�L��̩��U�@��C
 * �Ǧ^�ϥΪ̪����(char)�C
 */
char
getans(const char *fmt,...)
{
    char   msg[256];
    char   ans[3];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg , 128, fmt, ap);
    va_end(ap);

    getdata(b_lines, 0, msg, ans, sizeof(ans), LCECHO);
    return ans[0];
}

int
getkey(const char *fmt,...)
{
    char   msg[256], i;
    va_list ap;
    va_start(ap, fmt);
    i = vsnprintf(msg , 128, fmt, ap);
    va_end(ap);
    return vmsg(msg);
}

static const char *msg_pressanykey_full =
    ANSI_COLOR(37;44) " �Ы�" ANSI_COLOR(36) " ���N�� " ANSI_COLOR(37) "�~�� " ANSI_COLOR(34);
#define msg_pressanykey_full_len (18)

static const char *msg_pressanykey_full_trail =
    ANSI_COLOR(36) 
    " [^T �����Ȧs��] "  ANSI_RESET;
#define msg_pressanykey_full_trail_len (18) /* 4 for head */

static const char* msg_pressanykey_trail =
    ANSI_COLOR(33;46) " " ANSI_COLOR(200) ANSI_COLOR(1431) ANSI_COLOR(506) 
    "[�����N���~��]" ANSI_COLOR(201) " " ANSI_RESET;
#define msg_pressanykey_trail_len (16+1+4) /* 4 for head */

int
vmsg(const char *msg)
{
    int len = msg ? strlen(msg) : 0;
    int i = 0;

    if(len == 0) msg = NULL;

    move(b_lines, 0);
    clrtoeol();

    if(!msg)
    {
	/* msg_pressanykey_full */ 
	int w = (t_columns - msg_pressanykey_full_len - 8) / 2;
	int pad = 0;

	outs(ANSI_COLOR(1;34;44) " ");
	pad += 1;
	for (i = 0; i < w; i += 2)
	    outs("�e"), pad+=2;
	outs(msg_pressanykey_full), pad+= msg_pressanykey_full_len;
	/* pad now points to position of current cursor. */
	pad = t_columns - msg_pressanykey_full_trail_len - pad;
	/* pad is now those left . */
	if (pad > 0)
	{
	    for (i = 0; i <= pad-2; i += 2)
		outs("�e");
	    if (i == pad-1)
		outc(' ');
	}
	outs(msg_pressanykey_full_trail);
    } else {
	/* msg_pressanykey_trail */ 
	outs(ANSI_COLOR(1;36;44) " �� ");
	if(len >= t_columns - msg_pressanykey_trail_len)
	    len = t_columns - msg_pressanykey_trail_len;
	while (i++ < len)
	    outc(*msg++);
	i--;
	while (i++ < t_columns - msg_pressanykey_trail_len)
	    outc(' ');
	outs(msg_pressanykey_trail);
    }

    do {
	if( (i = igetch()) == Ctrl('T') )
	    if(cuser.userid[0]) // if already login
		capture_screen();
    } while( i == 0 );

    move(b_lines, 0);
    clrtoeol();
    return i;
}

int
vmsgf(const char *fmt,...)
{
    char   msg[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg)-1, fmt, ap);
    va_end(ap);
    msg[sizeof(msg)-1] = 0;
    return vmsg(msg);
}

/**
 * �q�� y �C�}�l show �X filename �ɮפ����e lines ��C
 * mode �� output ���Ҧ��A�ѼƦP strip_ansi�C
 * @param filename
 * @param x
 * @param lines
 * @param mode
 * @return ���ѶǦ^ 0�A�_�h�� 1�C
 */
int
show_file(const char *filename, int y, int lines, int mode)
{
    FILE           *fp;
    char            buf[1024];

    if (y >= 0)
	move(y, 0);
    clrtoline(lines + y);
    if ((fp = fopen(filename, "r"))) {
	while (fgets(buf, sizeof(buf), fp) && lines--)
	    outs(Ptt_prints(buf, sizeof(buf), mode));
	fclose(fp);
    } else
	return 0;
    return 1;
}

void
bell(void)
{
    char            c;

    c = Ctrl('G');
    write(1, &c, 1);
}

int
search_num(int ch, int max)
{
    int             clen = 1;
    int             x, y;
    char            genbuf[10];

    outmsg(ANSI_COLOR(7) " ���ܲĴX���G" ANSI_RESET);
    outc(ch);
    genbuf[0] = ch;
    getyx(&y, &x);
    x--;
    while ((ch = igetch()) != '\r') {
	if (ch == 'q' || ch == 'e')
	    return -1;
	if (ch == '\n')
	    break;
	if (ch == '\177' || ch == Ctrl('H')) {
	    if (clen == 0) {
		bell();
		continue;
	    }
	    clen--;
	    move(y, x + clen);
	    outc(' ');
	    move(y, x + clen);
	    continue;
	}
	if (!isdigit(ch)) {
	    bell();
	    continue;
	}
	if (x + clen >= scr_cols || clen >= 6) {
	    bell();
	    continue;
	}
	genbuf[clen++] = ch;
	outc(ch);
    }
    genbuf[clen] = '\0';
    move(b_lines, 0);
    clrtoeol();
    if (genbuf[0] == '\0')
	return -1;
    clen = atoi(genbuf);
    if (clen == 0)
	return 0;
    if (clen > max)
	return max;
    return clen - 1;
}

/**
 * �b�ù����W�� show �X "�ititle�j"
 * @param title
 */
void
stand_title(const char *title)
{
    clear();
    prints(ANSI_COLOR(1;37;46) "�i %s �j" ANSI_RESET "\n", title);
}

void
cursor_show(int row, int column)
{
    move(row, column);
    outs(STR_CURSOR);
    move(row, column + 1);
}

void
cursor_clear(int row, int column)
{
    move(row, column);
    outs(STR_UNCUR);
}

int
cursor_key(int row, int column)
{
    int             ch;

    cursor_show(row, column);
    ch = igetch();
    move(row, column);
    outs(STR_UNCUR);
    return ch;
}

void
printdash(const char *mesg, int msglen)
{
    int             head = 0, tail;

    if(msglen <= 0)
	msglen = strlen(mesg);

    if (mesg)
	head = (msglen + 1) >> 1;

    tail = head;

    while (head++ < t_columns/2-2)
	outc('-');

    if (tail) {
	outc(' ');
	if(mesg) outs(mesg);
	outc(' ');
    }
    while (tail++ < t_columns/2-2)
	outc('-');

    outc('\n');
}

int
log_user(const char *fmt, ...)
{
    char msg[256], filename[256];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(msg , 128, fmt, ap);
    va_end(ap);

    sethomefile(filename, cuser.userid, "USERLOG");
    return log_filef(filename, LOG_CREAT, "%s: %s %s", cuser.userid, msg,  Cdate(&now));
}

void
show_help(const char * const helptext[])
{
    const char     *str;
    int             i;

    clear();
    for (i = 0; (str = helptext[i]); i++) {
	if (*str == '\0')
	    prints(ANSI_COLOR(1) "�i %s �j" ANSI_COLOR(0) "\n", str + 1);
	else if (*str == '\01')
	    prints("\n" ANSI_COLOR(36) "�i %s �j" ANSI_RESET "\n", str + 1);
	else
	    prints("        %s\n", str);
    }
#ifdef PLAY_ANGEL
    if (HasUserPerm(PERM_LOGINOK))
	pressanykey_or_callangel();
    else
#endif
	pressanykey();
}

void
show_helpfile(const char *helpfile)
{
    clear();
    show_file((char *)helpfile, 0, b_lines, NO_RELOAD);
#ifdef PLAY_ANGEL
    if (HasUserPerm(PERM_LOGINOK))
	pressanykey_or_callangel();
    else
#endif
    pressanykey();
}

#endif // _BBS_UTIL_C_

/* ----------------------------------------------------- */
/* use mmap() to malloc large memory in CRITICAL_MEMORY  */
/* ----------------------------------------------------- */
#ifdef CRITICAL_MEMORY
void *MALLOC(int size)
{
    int     *p;
    p = (int *)mmap(NULL, (size + 4), PROT_READ | PROT_WRITE,
	    MAP_ANON | MAP_PRIVATE, -1, 0);
    p[0] = size;
#if defined(DEBUG) && !defined(_BBS_UTIL_C_)
    vmsgf("critical malloc %d bytes", size);
#endif
    return (void *)&p[1];
}

void FREE(void *ptr)
{
    int     size = ((int *)ptr)[-1];
    munmap((void *)(&(((int *)ptr)[-1])), size);
#if defined(DEBUG) && !defined(_BBS_UTIL_C_)
    vmsgf("critical free %d bytes", size);
#endif
}
#endif


unsigned
DBCS_StringHash(const char *s)
{
    return fnv1a_32_dbcs_strcase(s, FNV1_32_INIT);
}

inline int *intbsearch(int key, const int *base0, int nmemb)
{
    /* ��� /usr/src/lib/libc/stdlib/bsearch.c ,
       �M���j int array �Ϊ�, ���z�L compar function �G���֨� */
    const   char *base = (const char *)base0;
    size_t  lim;
    int     *p;

    for (lim = nmemb; lim != 0; lim >>= 1) {
	p = (int *)(base + (lim >> 1) * 4);
	if( key == *p )
	    return p;
	if( key > *p ){/* key > p: move right */
	    base = (char *)p + 4;
	    lim--;
	}               /* else move left */
    }
    return (NULL);
}

inline unsigned int *
uintbsearch(const unsigned int key, const unsigned int *base0, const int nmemb)
{
    /* ��� /usr/src/lib/libc/stdlib/bsearch.c ,
       �M���j int array �Ϊ�, ���z�L compar function �G���֨� */
    const   char *base = (const char *)base0;
    size_t  lim;
    unsigned int     *p;

    for (lim = nmemb; lim != 0; lim >>= 1) {
        p = (unsigned int *)(base + (lim >> 1) * 4);
        if( key == *p )
            return p;
        if( key > *p ){/* key > p: move right */
            base = (char *)p + 4;
            lim--;
        }               /* else move left */
    }
    return (NULL);
}
