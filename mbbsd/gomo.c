/* $Id$ */
#include "bbs.h"

#define QCAST   int (*)(const void *, const void *)

static int      tick, lastcount, mylasttick, hislasttick;

typedef struct {
    char            x;
    char            y;
}               Horder_t;

static Horder_t *v;

/* pattern and advance map */

static int
intrevcmp(const void *a, const void *b)
{
    return (*(int *)b - *(int *)a);
}

// 以 (x,y) 為起點, 方向 (dx,dy), 傳回以 bit 表示相鄰哪幾格有子
// 如 10111 表示該方向相鄰 1,2,3 有子, 4 空地
// 最高位 1 表示對方的子, 或是牆
/* x,y: 0..BRDSIZ-1 ; color: CBLACK,CWHITE ; dx,dy: -1,0,+1 */
static int
gomo_getindex(int x, int y, int color, int dx, int dy)
{
    int             i, k, n;
    for (n = -1, i = 0, k = 1; i < 5; i++, k*=2) {
	x += dx;
	y += dy;

	if ((x < 0) || (x >= BRDSIZ) || (y < 0) || (y >= BRDSIZ)) {
	    n += k;
	    break;
	} else if (ku[x][y] != BBLANK) {
	    n += k;
	    if (ku[x][y] != color)
		break;
	}
    }

    if (i >= 5)
	n += k;

    return n;
}

int
chkwin(int style, int limit)
{
    if (style == 0x0c)
	return 1 /* style */ ;
    else if (limit == 0) {
	if (style == 0x0b)
	    return 1 /* style */ ;
	return 0;
    }
    if ((style < 0x0c) && (style > 0x07))
	return -1 /* -style */ ;
    return 0;
}

/* x,y: 0..BRDSIZ-1 ; color: CBLACK,CWHITE ; limit:1,0 ; dx,dy: 0,1 */
static int
dirchk(int x, int y, int color, int limit, int dx, int dy)
{
    int             le, ri, loc, style = 0;

    le = gomo_getindex(x, y, color, -dx, -dy);
    ri = gomo_getindex(x, y, color, dx, dy);

    loc = (le > ri) ? (((le * (le + 1)) >> 1) + ri) :
	(((ri * (ri + 1)) >> 1) + le);

    style = pat[loc];

    if (limit == 0)
	return (style & 0x0f);

    style >>= 4;

    if ((style == 3) || (style == 2)) {
	int             i, n = 0, tmp, nx, ny;

	n = adv[loc / 2];

	if(loc%2==0)
	    n/=16;
	else
	    n%=16;

	ku[x][y] = color;

	for (i = 0; i < 2; i++) {
	    if ((tmp = (i == 0) ? (-(n >> 2)) : (n & 3)) != 0) {
		nx = x + (le > ri ? 1 : -1) * tmp * dx;
		ny = y + (le > ri ? 1 : -1) * tmp * dy;

		if ((dirchk(nx, ny, color, 0, dx, dy) == 0x06) &&
		    (chkwin(getstyle(nx, ny, color, limit), limit) >= 0))
		    break;
	    }
	}
	if (i >= 2)
	    style = 0;
	ku[x][y] = BBLANK;
    }
    return style;
}

/* 例外=F 錯誤=E 有子=D 連五=C 連六=B 雙四=A 四四=9 三三=8 */
/* 四三=7 活四=6 斷四=5 死四=4 活三=3 斷三=2 保留=1 無效=0 */

/* x,y: 0..BRDSIZ-1 ; color: CBLACK,CWHITE ; limit: 1,0 */
int
getstyle(int x, int y, int color, int limit)
{
    int             i, j, dir[4], style;

    if ((x < 0) || (x >= BRDSIZ) || (y < 0) || (y >= BRDSIZ))
	return 0x0f;
    if (ku[x][y] != BBLANK)
	return 0x0d;

    // (-1,1), (0,1), (1,0), (1,1)
    for (i = 0; i < 4; i++)
	dir[i] = dirchk(x, y, color, limit, i ? (i >> 1) : -1, i ? (i & 1) : 1);

    qsort(dir, 4, sizeof(int), (QCAST)intrevcmp);

    if ((style = dir[0]) >= 2) {
	for (i = 1, j = 6 + (limit ? 1 : 0); i < 4; i++) {
	    if ((style > j) || (dir[i] < 2))
		break;
	    if (dir[i] > 3)
		style = 9;
	    else if ((style < 7) && (style > 3))
		style = 7;
	    else
		style = 8;
	}
    }
    return style;
}
static void
HO_init(Horder_t *pool)
{
    memset(pool, 0, sizeof(Horder_t)*BRDSIZ*BRDSIZ);
    v = pool;
    pat = pat_gomoku;
    adv = adv_gomoku;
    memset(ku, 0, sizeof(ku));
}

static void
HO_add(Horder_t * mv)
{
    *v++ = *mv;
}

static void
HO_undo(Horder_t * mv)
{
    char           *str = I18N[1099];
    int             n1, n2, loc;

    *mv = *(--v);
    ku[(int)mv->x][(int)mv->y] = BBLANK;
    BGOTO(mv->x, mv->y);
    n1 = (mv->x == 0) ? 0 : (mv->x == 14) ? 2 : 1;
    n2 = (mv->y == 14) ? 0 : (mv->y == 0) ? 2 : 1;
    loc = 2 * (n2 * 3 + n1);
    prints("%.2s", str + loc);
}

static void
HO_log(Horder_t *pool, char *user)
{
    int             i;
    FILE           *log;
    char            buf[80];
    char            buf1[80];
    char            title[80];
    Horder_t       *ptr = pool;
    fileheader_t    mymail;

    snprintf(buf, sizeof(buf), "home/%c/%s/F.%d",
	     cuser.userid[0], cuser.userid,  rand() & 65535);
    log = fopen(buf, "w");
    assert(log);

    for (i = 1; i < 17; i++)
	fprintf(log, "%.*s\n", big_picture[i].len, big_picture[i].data);

    i = 0;
    do {
	fprintf(log, "[%2d]%s ==> %c%d%c", i + 1, I18N[1097 + i % 2],
		'A' + ptr->x, ptr->y + 1, (i % 2) ? '\n' : '\t');
	i++;
    } while (++ptr < v);
    fclose(log);

    sethomepath(buf1, cuser.userid);
    stampfile(buf1, &mymail);

    mymail.filemode = FILE_READ ;
    strlcpy(mymail.owner, I18N[1100], sizeof(mymail.owner));
    snprintf(mymail.title, sizeof(mymail.title),
	     I18N[1101], cuser.userid, user);
    sethomedir(title, cuser.userid);
    Rename(buf, buf1);
    append_record(title, &mymail, sizeof(mymail));

    unlink(buf);
}

static int
countgomo(Horder_t *pool)
{
    return v-pool;
}

static int
chkmv(Horder_t * mv, int color, int limit)
{
    char           *xtype[] = {I18N[1102], I18N[1103],
	I18N[1104], I18N[1105],
	I18N[1106], I18N[1107],
	I18N[1108], I18N[1109],
	I18N[1110], I18N[1111],
    I18N[1112]};
    int             rule = getstyle(mv->x, mv->y, color, limit);
    if (rule > 1 && rule < 13) {
	move(15, 40);
	outs(xtype[rule - 2]);
	bell();
    }
    return chkwin(rule, limit);
}

static int
gomo_key(int fd, int ch, Horder_t * mv)
{
    if (ch >= 'a' && ch <= 'o') {
	char            pbuf[4], vx, vy;

	pbuf[0] = ch;
	if (fd)
	    add_io(0, 0);
	oldgetdata(17, 0, I18N[1113], pbuf, sizeof(pbuf), DOECHO);
	if (fd)
	    add_io(fd, 0);
	vx = pbuf[0] - 'a';
	vy = atoi(pbuf + 1) - 1;
	if (vx >= 0 && vx < 15 && vy >= 0 && vy < 15 &&
	    ku[(int)vx][(int)vy] == BBLANK) {
	    mv->x = vx;
	    mv->y = vy;
	    return 1;
	}
    } else {
	switch (ch) {
	case KEY_RIGHT:
	    if(mv->x<BRDSIZ-1)
		mv->x++;
	    break;
	case KEY_LEFT:
	    if(mv->x>0)
		mv->x--;
	    break;
	case KEY_UP:
	    if(mv->y<BRDSIZ-1)
		mv->y++;
	    break;
	case KEY_DOWN:
	    if(mv->y>0)
		mv->y--;
	    break;
	case ' ':
	case '\r':
	    if (ku[(int)mv->x][(int)mv->y] == BBLANK)
		return 1;
	}
    }
    return 0;
}

int
gomoku(int fd)
{
    Horder_t        mv;
    int             me, he, ch;
    int             hewantpass, iwantpass;
    userinfo_t     *my = currutmp;
    Horder_t        pool[BRDSIZ*BRDSIZ];

    HO_init(pool);
    me = !(my->turn) + 1;
    he = my->turn + 1;
    tick = now + MAX_TIME;
    lastcount = MAX_TIME;
    setutmpmode(M_FIVE);
    clear();

    prints(I18N[1114],
	   cuser.userid, my->mateid);
    show_file("etc/@five", 1, -1, ONLY_COLOR);
    move(11, 40);
    prints(I18N[1115], me == BBLACK ? I18N[1116] : I18N[1117]);
    move(16, 40);
    prints("\033[1;33m%s", cuser.userid);
    move(17, 40);
    prints("\033[1;33m%s", my->mateid);

    move(16, 60);
    prints(I18N[1118], cuser.five_win, cuser.five_lose, cuser.five_tie);

    getuser(my->mateid);
    move(17, 60);
    prints(I18N[1119], xuser.five_win, xuser.five_lose, xuser.five_tie);

    cuser.five_lose++;
    /* 一進來先加一場敗場, 贏了後再扣回去, 避免快輸了惡意斷線 */
    passwd_update(usernum, &cuser);

    add_io(fd, 0);

    hewantpass = iwantpass = 0;
    mv.x = mv.y = 7;
    move(18, 40);
    prints(I18N[1120], my->turn ? I18N[1121] : I18N[1122],
	   MAX_TIME / 60, MAX_TIME % 60);
    for (;;) {
	move(13, 40);
	outs(my->turn ? I18N[1123] : I18N[1124]);
	if (lastcount != tick - now) {
	    lastcount = tick - now;
	    move(18, 40);
	    prints(I18N[1125], my->turn ? I18N[1126] : I18N[1127],
		   lastcount / 60, lastcount % 60);
	    if (lastcount <= 0 && my->turn) {
		move(19, 40);
		outs(I18N[1128]);
		my->five_lose++;
		send(fd, '\0', 1, 0);
		break;
	    }
	    if (lastcount <= -5 && !my->turn) {
		move(19, 40);
		outs(I18N[1129]);
		cuser.five_lose--;
		cuser.five_win++;
		my->five_win++;
                passwd_update(usernum, &cuser);
		mv.x = mv.y = -2;
		send(fd, &mv, sizeof(Horder_t), 0);
		mv = *(v - 1);
		break;
	    }
	}
	move(14, 40);
	if (hewantpass) {
	    outs(I18N[1130]);
	    bell();
	} else
	    clrtoeol();
	BGOTOCUR(mv.x, mv.y);
	ch = igetch();
	if (ch != I_OTHERDATA)
	    iwantpass = 0;
	if (ch == 'q') {
	    if (countgomo(pool) < 10) {
		cuser.five_lose--;
                passwd_update(usernum, &cuser);
	    }
	    send(fd, '\0', 1, 0);
	    break;
	} else if (ch == 'u' && !my->turn && v > pool) {
	    mv.x = mv.y = -1;
	    ch = send(fd, &mv, sizeof(Horder_t), 0);
	    if (ch == sizeof(Horder_t)) {
		HO_undo(&mv);
		tick = mylasttick;
		my->turn = 1;
		continue;
	    } else
		break;
	}
	if (ch == 'p') {
	    if (my->turn) {
		if (iwantpass == 0) {
		    iwantpass = 1;
		    mv.x = mv.y = -2;
		    send(fd, &mv, sizeof(Horder_t), 0);
		    mv = *(v - 1);
		}
		continue;
	    } else if (hewantpass) {
		cuser.five_lose--;
		cuser.five_tie++;
		my->five_tie++;
		passwd_update(usernum, &cuser);
		mv.x = mv.y = -2;
		send(fd, &mv, sizeof(Horder_t), 0);
		mv = *(v - 1);
		break;
	    }
	}
	if (ch == I_OTHERDATA) {
	    ch = recv(fd, &mv, sizeof(Horder_t), 0);
	    if (ch != sizeof(Horder_t)) {
		lastcount = tick - now;
		if (lastcount >= 0) {
		    cuser.five_lose--;
		    if (countgomo(pool) >= 10) {
			cuser.five_win++;
			my->five_win++;
		    }
		    passwd_update(usernum, &cuser);
		    outmsg(I18N[1131]);
		    break;
		} else {
		    outmsg(I18N[1132]);
		    my->five_lose++;
		    break;
		}
	    } else if (mv.x == -2 && mv.y == -2) {
		if (iwantpass == 1) {
		    cuser.five_lose--;
		    cuser.five_tie++;
		    my->five_tie++;
		    passwd_update(usernum, &cuser);
		    break;
		} else {
		    hewantpass = 1;
		    mv = *(v - 1);
		    continue;
		}
	    }
	    if (my->turn && mv.x == -1 && mv.y == -1) {
		outmsg(I18N[1133]);
		tick = hislasttick;
		HO_undo(&mv);
		my->turn = 0;
		continue;
	    }
	    if (!my->turn) {
		int win;
		win = chkmv(&mv, he, he == BBLACK);
		HO_add(&mv);
		hislasttick = tick;
		tick = now + MAX_TIME;
		ku[(int)mv.x][(int)mv.y] = he;
		bell();
		BGOTO(mv.x, mv.y);
		outs(I18N[1097 + he - 1]);

		if (win) {
		    outmsg(win == 1 ? I18N[1134] : I18N[1135]);
		    if (win != 1) {
			cuser.five_lose--;
			cuser.five_win++;
			my->five_win++;
			passwd_update(usernum, &cuser);
		    } else
			my->five_lose++;
		    break;
		}
		my->turn = 1;
	    }
	    continue;
	}
	if (my->turn) {
	    if (gomo_key(fd, ch, &mv))
		my->turn = 0;
	    else
		continue;

	    if (!my->turn) {
		int win;
		HO_add(&mv);
		BGOTO(mv.x, mv.y);
		outs(I18N[1097 + me - 1]);
		win = chkmv(&mv, me, me == BBLACK);
		ku[(int)mv.x][(int)mv.y] = me;
		mylasttick = tick;
		tick = now + MAX_TIME;	/* 倒數 */
		lastcount = MAX_TIME;
		if (send(fd, &mv, sizeof(Horder_t), 0) != sizeof(Horder_t))
		    break;
		if (win) {
		    outmsg(win == 1 ? I18N[1136] : I18N[1137]);
		    if (win == 1) {
			cuser.five_lose--;
			cuser.five_win++;
			my->five_win++;
			passwd_update(usernum, &cuser);
		    } else
			my->five_lose++;
		    break;
		}
		move(15, 40);
		clrtoeol();
	    }
	}
    }
    add_io(0, 0);
    close(fd);

    igetch();
    if (v > pool) {
	char            ans[4];

	getdata(19, 0, I18N[1138], ans, sizeof(ans), LCECHO);
	if (*ans == 'y')
	    HO_log(pool, my->mateid);
    }
    return 0;
}
